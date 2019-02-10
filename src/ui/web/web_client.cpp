#include "core/log.hpp"
#include "ui/constants/web.hpp"
#include "ui/web/web_client.hpp"

using boost::asio::ip::tcp;

WebClient::WebClient(boost::asio::io_context &context)
    : mContext(context)
    , mState(WebClientState::NOT_CONNECTED)
{
    qRegisterMetaType<WebToken>("WebToken");
    qRegisterMetaType<UserRegistrationResponse>("UserRegistrationResponse");
    qRegisterMetaType<WebTokenRequestResponse>("WebTokenRequestResponse");
    qRegisterMetaType<WebTokenValidationResponse>("WebTokenValidationResponse");
    qRegisterMetaType<WebNameCheckResponse>("WebNameCheckResponse");
    qRegisterMetaType<WebNameRegistrationResponse>("WebNameRegistrationResponse");

    qRegisterMetaType<WebClientState>("WebClientState");
}

void WebClient::validateToken(const QString &token) {

}

void WebClient::createConnection(connectionHandler handler) {
    mContext.dispatch([this, handler]() {
        assert(mState == WebClientState::NOT_CONNECTED);
        mState = WebClientState::CONNECTING;
        emit stateChanged(mState);

        tcp::resolver resolver(mContext);
        tcp::resolver::results_type endpoints;

        try {
            endpoints = resolver.resolve(Constants::WEB_HOST, std::to_string(Constants::WEB_PORT));
        }
        catch(const std::exception &e) {
            log_error().field("message", e.what()).msg("Failed resolving web host. Failing");
            handler(boost::system::errc::make_error_code(boost::system::errc::invalid_argument));
            return;
        }

        mSocket = tcp::socket(mContext);

        // TODO: Somehow kill when taking too long
        boost::asio::async_connect(*mSocket, endpoints, [this, handler](boost::system::error_code ec, tcp::endpoint) {
            if (ec) {
                log_error().field("message", ec.message()).msg("Encountered error when connecting to web host. Failing");
                killConnection();
                handler(ec);
                return;
            }

            mConnection = NetworkConnection(std::move(*mSocket));
            mSocket.reset();
            mConnection->asyncJoin([this, handler](boost::system::error_code ec) {
                if (ec) {
                    log_error().field("message", ec.message()).msg("Encountered error handshaking with web host. Killing connection");
                    killConnection();
                    handler(ec);
                    return;
                }

                handler(ec);
            });
        });
    });
}

void WebClient::loginUser(const QString &email, const QString &password) {
    createConnection([this, email, password](boost::system::error_code ec) {
        if (ec) {
            killConnection();
            emit loginFailed(WebTokenRequestResponse::SERVER_ERROR);
            return;
        }

        auto loginMessage = std::make_shared<NetworkMessage>();
        loginMessage->encodeRequestWebToken(email.toStdString(), password.toStdString());
        mConnection->asyncWrite(*loginMessage, [this, loginMessage](boost::system::error_code ec) {
            if (ec) {
                log_error().field("message", ec.message()).msg("Encountered error writing request token message. Killing connection");
                killConnection();
                emit loginFailed(WebTokenRequestResponse::SERVER_ERROR);
                return;
            }

            auto responseMessage = std::make_shared<NetworkMessage>();
            mConnection->asyncRead(*responseMessage, [this, responseMessage](boost::system::error_code ec) {
                if (ec) {
                    log_error().field("message", ec.message()).msg("Encountered error reading request token response. Failing");
                    killConnection();
                    emit loginFailed(WebTokenRequestResponse::SERVER_ERROR);
                    return;
                }

                if (responseMessage->getType() != NetworkMessage::Type::REQUEST_WEB_TOKEN_RESPONSE) {
                    log_error().msg("Received response message of wrong type. Failing");
                    killConnection();
                    emit loginFailed(WebTokenRequestResponse::SERVER_ERROR);
                    return;
                }

                WebTokenRequestResponse response;
                std::optional<WebToken> token;
                responseMessage->decodeRequestWebTokenResponse(response, token);

                if (response != WebTokenRequestResponse::SUCCESSFUL) {
                    killConnection();
                    emit loginFailed(response);
                    return;
                }

                mState = WebClientState::CONNECTED;
                emit loginSucceeded(token.value());
                emit stateChanged(mState);
            });
        });
    });
}

void WebClient::registerUser(const QString &email, const QString &password) {
    // TODO: Implement registration
}

void WebClient::disconnect() {
    mContext.dispatch([this]() {
        assert(mState == WebClientState::CONFIGURED);
        mState = WebClientState::DISCONNECTING;
        emit stateChanged(mState);
        // TODO: Implement disconnect
    });
}

void WebClient::registerWebName(TournamentId id, const QString &webName) {
    log_debug().field("id", id).field("webName", webName.toStdString()).msg("Registering web name");
    mContext.dispatch([this, id, webName]() {
        assert(mState == WebClientState::CONNECTED);
        mState = WebClientState::CONFIGURING;
        emit stateChanged(mState);

        auto registerMessage = std::make_shared<NetworkMessage>();
        registerMessage->encodeRegisterWebName(id, webName.toStdString());
        mConnection->asyncWrite(*registerMessage, [this, registerMessage, webName](boost::system::error_code ec) {
            if (ec) {
                log_error().field("message", ec.message()).msg("Encountered error writing register message. Killing connection");
                killConnection();
                emit registrationFailed(WebNameRegistrationResponse::SERVER_ERROR);
                return;
            }

            auto responseMessage = std::make_shared<NetworkMessage>();
            mConnection->asyncRead(*responseMessage, [this, responseMessage, webName](boost::system::error_code ec) {
                if (ec) {
                    log_error().field("message", ec.message()).msg("Encountered error reading registration response. Failing");
                    killConnection();
                    emit registrationFailed(WebNameRegistrationResponse::SERVER_ERROR);
                    return;
                }

                if (responseMessage->getType() != NetworkMessage::Type::REGISTER_WEB_NAME_RESPONSE) {
                    log_error().msg("Received response message of wrong type. Failing");
                    killConnection();
                    emit registrationFailed(WebNameRegistrationResponse::SERVER_ERROR);
                    return;
                }

                WebNameRegistrationResponse response;
                responseMessage->decodeRegisterWebNameResponse(response);

                if (response != WebNameRegistrationResponse::SUCCESSFUL) {
                    killConnection();
                    emit registrationFailed(response);
                    return;
                }

                mState = WebClientState::CONFIGURED;
                emit registrationSucceeded(webName);
                emit stateChanged(mState);
            });
        });
    });
}

void WebClient::checkWebName(TournamentId id, const QString &webName) {
    // TODO: Implement checking of web names
}

void WebClient::killConnection() {
    mConnection.reset();
    mSocket.reset();
    mState = WebClientState::NOT_CONNECTED;
    emit stateChanged(mState);
    // while (!mWriteQueue.empty())
    //     mWriteQueue.pop();
}

