#include <fstream>

#include <boost/system/system_error.hpp>

#include "log.hpp"
#include "network/network_server.hpp"
#include "serializables.hpp"
#include "store_managers/master_store_manager.hpp"
#include "stores/qtournament_store.hpp"

MasterStoreManager::MasterStoreManager()
    : mDirty(false)
{
    auto &tatamis = getTournament().getTatamis();
    auto location = tatamis.generateLocation(0);
    tatamis[location.handle];
}

MasterStoreManager::~MasterStoreManager() {
    stopServer();
}

bool MasterStoreManager::read(const QString &path) {
    log_debug().field("path", path.toStdString()).msg("Reading tournament from file");
    std::ifstream file(path.toStdString(), std::ios::in | std::ios::binary);

    if (!file.is_open())
        return false;

    auto tournament = std::make_unique<QTournamentStore>();
    cereal::PortableBinaryInputArchive archive(file);
    try {
        archive(*tournament);
    }
    catch(const std::exception &e) {
        return false;
    }


    sync(std::move(tournament));

    mDirty = false;
    return true;
}

void MasterStoreManager::resetTournament() {
    log_debug().msg("Resetting tournament");
    auto tournament = std::make_unique<QTournamentStore>();
    auto &tatamis = getTournament().getTatamis();
    auto location = tatamis.generateLocation(0);
    tatamis[location.handle];
    sync(std::move(tournament));
    mDirty = false;
}

bool MasterStoreManager::write(const QString &path) {
    log_debug().field("path", path.toStdString()).msg("Writing tournament to file");
    std::ofstream file(path.toStdString(), std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open())
        return false;

    cereal::PortableBinaryOutputArchive archive(file);
    try {
        archive(getTournament());
    }
    catch(const std::exception &e) {
        return false;
    }
    mDirty = false;
    return true;
}

void MasterStoreManager::startServer(int port) {
    try {
        startInterface(std::make_unique<NetworkServer>(port));
    }
    catch (const boost::system::system_error &e) {
        if (e.code() == boost::system::errc::address_in_use)
            throw AddressInUseException(port);
        else
            throw e;
    }
    sync();
}

void MasterStoreManager::stopServer() {
    stopInterface();
}

void MasterStoreManager::dispatch(std::unique_ptr<Action> action) {
    mDirty = true;
    StoreManager::dispatch(std::move(action));
}

void MasterStoreManager::undo() {
    mDirty = true;
    StoreManager::undo();
}

void MasterStoreManager::redo() {
    mDirty = true;
    StoreManager::redo();
}

bool MasterStoreManager::isDirty() const {
    return mDirty;
}

