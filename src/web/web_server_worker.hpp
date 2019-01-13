#pragma once

#include <boost/asio.hpp> // TODO: Do not include boost convenience headers

// Responsible for handling connections to a set of tournaments
class WebServerWorker {
public:
    WebServerWorker();

    void run();
    void quit();

private:
    boost::asio::io_context mContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mWorkGuard;
};
