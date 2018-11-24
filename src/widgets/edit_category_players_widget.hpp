#pragma once

#include <QWidget>
#include <QTableView>

#include "core.hpp"
#include "id.hpp"

class StoreManager;
class PlayersProxyModel;

class EditCategoryPlayersWidget : public QWidget {
    Q_OBJECT
public:
    EditCategoryPlayersWidget (StoreManager & storeManager, QWidget *parent);
    void setCategory(std::optional<CategoryId> categoryId);

protected slots:
    void tournamentAboutToBeReset();

private:
    StoreManager &mStoreManager;
    QTableView *mTableView;
    PlayersProxyModel *mModel;
};
