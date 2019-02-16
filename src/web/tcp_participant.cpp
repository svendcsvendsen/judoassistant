#include <boost/asio/bind_executor.hpp>

#include "core/actions/action.hpp"
#include "core/stores/tournament_store.hpp"
#include "web/tcp_participant.hpp"
#include "web/web_server.hpp"

// TODO: Ensure strands are used correctly

TCPParticipant::TCPParticipant(boost::asio::io_context &context, std::shared_ptr<NetworkConnection> connection, WebServer &server, Database &database)
    : mStrand(context)
    , mConnection(std::move(connection))
    , mReadMessage(std::make_unique<NetworkMessage>())
    , mServer(server)
    , mDatabase(database)
    , mState(State::NOT_AUTHENTICATED)
{
}

void TCPParticipant::asyncAuth() {
    mReadMessage = std::make_unique<NetworkMessage>();
    mConnection->asyncRead(*mReadMessage, [this](boost::system::error_code ec) {
        assert(mState == State::NOT_AUTHENTICATED);
        if (ec) {
            forceQuit();
            return;
        }

        if (mReadMessage->getType() == NetworkMessage::Type::QUIT) {
            mServer.leave(shared_from_this());
            return;
        }

        if (mReadMessage->getType() == NetworkMessage::Type::REQUEST_WEB_TOKEN) {
            std::string email;
            std::string password;
            if (!mReadMessage->decodeRequestWebToken(email, password)) {
                log_warning().field("message", ec.message()).msg("Encountered error when writing message. Kicking client");
                forceQuit();
                return;
            }

            mDatabase.asyncRequestWebToken(email, password, [this] (WebTokenRequestResponse response, const std::optional<WebToken> token, std::optional<int> userId) {
                assert(mState == State::NOT_AUTHENTICATED);
                auto message = std::make_unique<NetworkMessage>();
                message->encodeRequestWebTokenResponse(response, token);
                deliver(std::move(message));

                if (response == WebTokenRequestResponse::SUCCESSFUL) {
                    mState = State::AUTHENTICATED;
                    mUserId = userId;
                    asyncTournamentRegister();
                }
                else {
                    asyncAuth();
                }
            });
        }
        else if (mReadMessage->getType() == NetworkMessage::Type::VALIDATE_WEB_TOKEN) {
            std::string email;
            WebToken token;
            if (!mReadMessage->decodeValidateWebToken(email, token)) {
                log_warning().field("message", ec.message()).msg("Encountered error when writing message. Kicking client");
                forceQuit();
                return;
            }

            mDatabase.asyncValidateWebToken(email, token, [this] (WebTokenValidationResponse response, std::optional<int> userId) {
                assert(mState == State::NOT_AUTHENTICATED);
                auto message = std::make_unique<NetworkMessage>();
                message->encodeValidateWebTokenResponse(response);
                deliver(std::move(message));

                if (response == WebTokenValidationResponse::SUCCESSFUL) {
                    mState = State::AUTHENTICATED;
                    mUserId = userId;
                    asyncTournamentRegister();
                }
                else {
                    asyncAuth();
                }
            });
        }
        else {
            log_warning().field("type", mReadMessage->getType()).msg("Received unexpected message type when authenticating");
        }
    });
}

// TODO: See if shared_from_this pattern can be avoided
void TCPParticipant::write() {
    mConnection->asyncWrite(*(mMessageQueue.front()), boost::asio::bind_executor(mStrand, [this](boost::system::error_code ec) {
        if (ec) {
            log_warning().field("message", ec.message()).msg("Encountered error when writing message. Kicking client");
            forceQuit();
            return;
        }

        mMessageQueue.pop();
        if (!mMessageQueue.empty())
            write();
    }));
}

void TCPParticipant::deliver(std::shared_ptr<NetworkMessage> message) {
    boost::asio::post(mStrand, [this, message](){
        bool writeInProgress = !mMessageQueue.empty();
        mMessageQueue.push(std::move(message));

        if (!writeInProgress)
            write();
    });
}

void TCPParticipant::asyncTournamentRegister() {
    assert(mState == State::AUTHENTICATED);

    mReadMessage = std::make_unique<NetworkMessage>();
    mConnection->asyncRead(*mReadMessage, [this](boost::system::error_code ec) {
        assert(mState == State::AUTHENTICATED);
        if (ec) {
            forceQuit();
            return;
        }

        if (mReadMessage->getType() == NetworkMessage::Type::QUIT) {
            mServer.leave(shared_from_this());
            return;
        }

        if (mReadMessage->getType() == NetworkMessage::Type::REGISTER_WEB_NAME) {
            TournamentId id;
            std::string webName;
            if (!mReadMessage->decodeRegisterWebName(id, webName)) {
                forceQuit();
                return;
            }

            mDatabase.asyncRegisterWebName(*mUserId, id, webName, [this, webName] (WebNameRegistrationResponse response) {
                assert(mState == State::AUTHENTICATED);
                auto message = std::make_unique<NetworkMessage>();
                message->encodeRegisterWebNameResponse(response);
                deliver(std::move(message));

                if (response == WebNameRegistrationResponse::SUCCESSFUL) {
                    mState = State::TOURNAMENT_SELECTED;
                    mWebName = std::move(webName);
                    asyncTournamentSync();
                }
                else {
                    asyncTournamentRegister();
                }
            });
        }
        else if (mReadMessage->getType() == NetworkMessage::Type::CHECK_WEB_NAME) {
            TournamentId id;
            std::string webName;
            if (!mReadMessage->decodeCheckWebName(id, webName)) {
                forceQuit();
                return;
            }

            mDatabase.asyncCheckWebName(*mUserId, id, webName, [this] (WebNameCheckResponse response) {
                assert(mState == State::AUTHENTICATED);
                auto message = std::make_unique<NetworkMessage>();
                message->encodeCheckWebNameResponse(response);
                deliver(std::move(message));

                asyncTournamentRegister();
            });
        }
        else {
            log_warning().field("type", mReadMessage->getType()).msg("Received unexpected message type when registering web name");
        }
    });
}

void TCPParticipant::quit() {
    // TODO: Implement
    log_debug().msg("Quit called");
}

struct MoveWrapper {
    std::unique_ptr<TournamentStore> tournament;
    SharedActionList actionList;
};

void TCPParticipant::asyncTournamentSync() {
    mReadMessage = std::make_unique<NetworkMessage>();
    mConnection->asyncRead(*mReadMessage, [this](boost::system::error_code ec) {
        assert(mState == State::TOURNAMENT_SELECTED);

        if (ec) {
            forceQuit();
            return;
        }

        if (mReadMessage->getType() == NetworkMessage::Type::QUIT) {
            mServer.leave(shared_from_this());
            return;
        }

        if (mReadMessage->getType() == NetworkMessage::Type::SYNC) {
            auto wrapper = std::make_shared<MoveWrapper>();
            wrapper->tournament = std::make_unique<TournamentStore>();

            if (!mReadMessage->decodeSync(*(wrapper->tournament), wrapper->actionList)) {
                // TODO: Unown tournament
                forceQuit();
                return;
            }

            mServer.acquireTournament(mWebName, mStrand.wrap([this, wrapper](std::shared_ptr<LoadedTournament> loadedTournament) {
                loadedTournament->sync(std::move(wrapper->tournament), std::move(wrapper->actionList));
                mTournament = loadedTournament;

                mDatabase.asyncSetSynced(mWebName, [this](bool success) {
                    if (!success)
                        log_warning().field("webName", mWebName).msg("Failed marking tournament as synced");
                });

                asyncTournamentListen();
            }));

            return;
        }

        log_warning().field("type", mReadMessage->getType()).msg("Received unexpected message type when registering web name");
        asyncTournamentSync();
    });
}

void TCPParticipant::asyncTournamentListen() {
    mReadMessage = std::make_unique<NetworkMessage>();
    mConnection->asyncRead(*mReadMessage, [this](boost::system::error_code ec) {
        assert(mState == State::TOURNAMENT_SELECTED);

        if (ec) {
            forceQuit();
            return;
        }

        auto type = mReadMessage->getType();
        if (type == NetworkMessage::Type::QUIT) {
            mServer.leave(shared_from_this());
            return;
        }
        else if (type == NetworkMessage::Type::UNDO) {
            ClientActionId actionId;

            if (!mReadMessage->decodeUndo(actionId)) {
                log_warning().msg("Failed decoding undo. Kicking client.");
                forceQuit();
                return;
            }

            mTournament->undo(actionId);
        }
        else if (type == NetworkMessage::Type::ACTION) {
            ClientActionId actionId;
            std::shared_ptr<Action> action;

            if (!mReadMessage->decodeAction(actionId, action)) {
                log_warning().msg("Failed decoding action. Kicking client.");
                forceQuit();
                return;
            }

            mTournament->dispatch(actionId, std::move(action));
        }
        else if (type == NetworkMessage::Type::SYNC) {
            auto tournament = std::make_unique<TournamentStore>();
            SharedActionList actionList;

            if (!mReadMessage->decodeSync(*tournament, actionList)) {
                // TODO: Unown tournament
                forceQuit();
                return;
            }

           mTournament->sync(std::move(tournament), std::move(actionList));
        }
        else {
            log_warning().field("type", type).msg("Received message of unexpected type when listening");
        }

        // TODO: Implement method
        asyncTournamentListen();
    });
}

void TCPParticipant::forceQuit() {
    mServer.leave(shared_from_this());
}

