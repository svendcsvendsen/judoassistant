#include "id.hpp"
#include "log.hpp"
#include "web/web_server_database_worker.hpp"
#include "config/web_server.hpp"
#include "botan-2/botan/rng.h"
#include "botan-2/botan/system_rng.h"
#include "botan-2/botan/bcrypt.h"

WebServerDatabaseWorker::WebServerDatabaseWorker(boost::asio::io_context &masterContext)
    : mContext()
    , mMasterContext(masterContext)
    , mWorkGuard(boost::asio::make_work_guard(mContext))
    , mConnection(Config::POSTGRES_CONFIG)
{

}

void WebServerDatabaseWorker::run() {
    mContext.run();
}

void WebServerDatabaseWorker::quit() {
    mWorkGuard.reset();
}

void WebServerDatabaseWorker::asyncRequestToken(const std::string &email, const std::string &password, TokenRequestCallback callback) {
    boost::asio::post(mContext, std::bind(&WebServerDatabaseWorker::requestToken, this, email, password, callback));
}

void WebServerDatabaseWorker::asyncValidateToken(const std::string &email, const Token &token, TokenValidationCallback callback) {
    boost::asio::post(mContext, std::bind(&WebServerDatabaseWorker::validateToken, this, email, token, callback));
}

void WebServerDatabaseWorker::asyncRegisterUser(const std::string &email, const std::string &password, UserRegistrationCallback callback) {
    boost::asio::post(mContext, std::bind(&WebServerDatabaseWorker::registerUser, this, email, password, callback));
}

void WebServerDatabaseWorker::asyncCheckWebName(int user, const TournamentId &id, const std::string &webName, WebNameCheckCallback callback) {
    boost::asio::post(mContext, std::bind(&WebServerDatabaseWorker::checkWebName, this, user, id, webName, callback));
}

void WebServerDatabaseWorker::asyncRegisterWebName(int user, const TournamentId &id, const std::string &webName, WebNameRegistrationCallback callback) {
    boost::asio::post(mContext, std::bind(&WebServerDatabaseWorker::registerWebName, this, user, id, webName, callback));
}

bool WebServerDatabaseWorker::hasUser(const std::string &email) {
    pqxx::work work(mConnection);
    pqxx::result r = work.exec("select 1 FROM users where email = " + work.quote(email));
    work.commit();

    return (r.size() > 0);
}

bool WebServerDatabaseWorker::checkPassword(const std::string &email, const std::string &password) {
    pqxx::work work(mConnection);
    pqxx::result r = work.exec("select password_hash FROM users where email = " + work.quote(email));
    work.commit();

    if (r.empty())
        return false;

    auto hash = r.front()[0].as<std::string>();
    log_debug().field("hash",hash).msg("Got password hash");
    return Botan::check_bcrypt(password, hash);
}

void WebServerDatabaseWorker::validateToken(const std::string &email, const Token &token, TokenValidationCallback callback) {
    std::string maxTokenExpiration = "2019-02-16 04:05:06";

    try {
        pqxx::work work(mConnection);
        pqxx::result r = work.exec("select token_expiration FROM users where email = "
                                + work.quote(email)
                                + " and token = " + work.quote_raw(token.data(), token.size()));
        work.commit();


        if (r.size() == 0)
            callback(TokenValidationResponse::INVALID_TOKEN);
        else
            callback(TokenValidationResponse::SUCCESSFUL);
    }
    catch (const std::exception &e) {
        log_error().field("what", e.what()).msg("PQXX exception caught");
        callback(TokenValidationResponse::SERVER_ERROR);
    }
}

void WebServerDatabaseWorker::registerUser(const std::string &email, const std::string &password, UserRegistrationCallback callback) {
    try {
        if (hasUser(email)) {
            callback(UserRegistrationResponse::EMAIL_EXISTS, Token());
            return;
        }

        auto token = generateToken();
        auto tokenExpiration = generateTokenExpiration();

        auto &rng = Botan::system_rng();
        auto passwordHash = Botan::generate_bcrypt(password, rng);
        log_debug().field("passwordHash", passwordHash).msg("Generated password");

        pqxx::work work(mConnection);
        pqxx::result r = work.exec("insert into users (email, password_hash, token, token_expiration) values ("
                                + work.quote(email)
                                + ", " + work.quote(passwordHash)
                                + ", " + work.quote_raw(token.data(), token.size())
                                + ", " + work.quote(tokenExpiration)
                                + ")");
        work.commit();

        callback(UserRegistrationResponse::EMAIL_EXISTS, token);
    }
    catch (const std::exception &e) {
        log_error().field("what", e.what()).msg("PQXX exception caught");
        callback(UserRegistrationResponse::SERVER_ERROR, Token());
    }
}

void WebServerDatabaseWorker::requestToken(const std::string &email, const std::string &password, TokenRequestCallback callback) {
    try {
        if (!checkPassword(email, password)) {
            callback(TokenRequestResponse::INCORRECT_CREDENTIALS, Token());
            return;
        }

        auto token = generateToken();
        auto tokenExpiration = generateTokenExpiration();

        pqxx::work work(mConnection);
        pqxx::result r = work.exec("update users set token=" + work.quote_raw(token.data(), token.size())
                                + ", token_expiration=" + work.quote(tokenExpiration)
                                + " where email=" + work.quote(email));
        work.commit();

        callback(TokenRequestResponse::SUCCESSFUL, token);
    }
    catch (const std::exception &e) {
        log_error().field("what", e.what()).msg("PQXX exception caught");
        callback(TokenRequestResponse::SERVER_ERROR, Token());
    }
}


Token WebServerDatabaseWorker::generateToken() {
    Token res;
    Botan::system_rng().randomize(res.data(), res.size());

    return res;
}

std::string WebServerDatabaseWorker::generateTokenExpiration() {
    return "2019-02-17"; // TODO: Implement
}

void WebServerDatabaseWorker::checkWebName(int user, const TournamentId &id, const std::string &webName, WebNameCheckCallback callback) {
    try {
        pqxx::work work(mConnection);
        pqxx::result r = work.exec("select owner, tournament_id FROM tournaments where web_name = "
                                + work.quote(webName));
        work.commit();


        if (r.empty())
            callback(WebNameCheckResponse::FREE);
        else if (r.front()[0].as<int>() != user)
            callback(WebNameCheckResponse::OCCUPIED_OTHER_USER);
        else if (r.front()[1].as<TournamentId::InternalType>() != id.getValue())
            callback(WebNameCheckResponse::OCCUPIED_OTHER_TOURNAMENT);
        else
            callback(WebNameCheckResponse::OCCUPIED_SAME_TOURNAMENT);
    }
    catch (const std::exception &e) {
        log_error().field("what", e.what()).msg("PQXX exception caught");
        callback(WebNameCheckResponse::SERVER_ERROR);
    }
}

void WebServerDatabaseWorker::registerWebName(int user, const TournamentId &id, const std::string &webName, WebNameRegistrationCallback callback) {
    try {
        pqxx::work work(mConnection);
        pqxx::result r = work.exec("select owner, tournament_id FROM tournaments where web_name = "
                                + work.quote(webName));
        work.commit();


        if (!r.empty() && r.front()[0].as<int>() != user) {
            callback(WebNameRegistrationResponse::OCCUPIED_OTHER_USER);
            return;
        }

        if (!r.empty() && r.front()[1].as<TournamentId::InternalType>() == id.getValue()) {
            callback(WebNameRegistrationResponse::SUCCESSFUL);
            return;
        }

        if (r.empty()) {
            pqxx::work work(mConnection);
            pqxx::result r = work.exec("insert into tournaments (owner, tournament_id, web_name) values (" + work.quote(user)
                                        + ", " + work.quote(id.getValue())
                                        + ", " + work.quote(webName)
                                        + ")");
            work.commit();
        }
        else {
            pqxx::work work(mConnection);
            pqxx::result r = work.exec("update tournaments set owner=" + work.quote(user)
                                        + ", tournament_id=" + work.quote(id.getValue())
                                        + " WHERE web_name=" + work.quote(webName));
            work.commit();
        }

        callback(WebNameRegistrationResponse::SUCCESSFUL);
    }
    catch (const std::exception &e) {
        log_error().field("what", e.what()).msg("PQXX exception caught");
        callback(WebNameRegistrationResponse::SERVER_ERROR);
    }
}

