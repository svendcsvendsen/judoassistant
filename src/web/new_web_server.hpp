#pragma once

#include <boost/asio/io_context_strand.hpp>
#include <thread>

#include "web/config/config.hpp"
#include "web/gateways/meta_service_gateway.hpp"
#include "web/gateways/storage_gateway.hpp"
#include "web/handlers/tcp_participant_handler.hpp"
#include "web/handlers/web_participant_handler.hpp"

class NewWebServer {
public:
    NewWebServer(const Config &config);
    void run();
    void quit();
private:
    const Config &mConfig;
    boost::asio::io_context mContext;
    boost::asio::io_context::strand mStrand;
    Logger mLogger;
    std::vector<std::thread> mThreads;
    TCPParticipantHandler mTCPParticipantHandler;
    WebParticipantHandler mWebParticipantHandler;
    MetaServiceGateway mMetaServiceGateway;
    StorageGateway mStorageGateway;
};
