#include "widgets/models/categories_model.hpp"
#include "stores/qtournament_store.hpp"

CategoriesModel::CategoriesModel(QStoreHandler & storeHandler, QObject * parent)
    : QAbstractTableModel(parent)
    , mStoreHandler(storeHandler)
{
    tournamentReset();
}

int CategoriesModel::rowCount(const QModelIndex &parent) const {
    return mStoreHandler.getTournament().getCategories().size();
}

int CategoriesModel::columnCount(const QModelIndex &parent) const {
    return COLUMN_COUNT;
}

QVariant CategoriesModel::data(const QModelIndex &index, int role) const {
    auto categoryId = getCategory(index.row());
    const CategoryStore &category = mStoreHandler.getTournament().getCategory(categoryId);

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0:
                return QString(QString::fromStdString(category.getName()));
            case 1:
                return QString(QString::fromStdString(category.getRuleset().getName()));
            case 2:
                return QString(QString::fromStdString(category.getDrawSystem().getName()));
            case 3:
                return static_cast<int>(category.getPlayers().size());
        }
    }
    return QVariant();
}

QVariant CategoriesModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
                case 0:
                    return QString(tr("Name"));
                case 1:
                    return QString(tr("Ruleset"));
                case 2:
                    return QString(tr("Draw System"));
                case 3:
                    return QString(tr("Players Count"));
            }
        }
    }
    return QVariant();
}

std::vector<CategoryId> CategoriesModel::getCategories(const QItemSelection &selection) const {
    std::unordered_set<int> rows;
    for (auto index : selection.indexes())
        rows.insert(index.row());

    std::vector<CategoryId> categoryIds;
    for (auto row : rows)
        categoryIds.push_back(getCategory(row));

    return std::move(categoryIds);
}

CategoryId CategoriesModel::getCategory(int row) const {
    auto it = mIds.begin();
    std::advance(it, row);
    return *it;
}

int CategoriesModel::getRow(CategoryId id) const {
    return std::distance(mIds.begin(), mIds.lower_bound(id));
}

void CategoriesModel::categoriesAdded(std::vector<CategoryId> ids) {
    for (auto id : ids) {
        int row = getRow(id);
        beginInsertRows(QModelIndex(), row, row);
        mIds.insert(id);
        endInsertRows();
    }
}

void CategoriesModel::categoriesChanged(std::vector<CategoryId> ids) {
    for (auto id : ids) {
        int row = getRow(id);
        emit dataChanged(createIndex(0,row), createIndex(COLUMN_COUNT-1,row));
    }
}

void CategoriesModel::categoriesAboutToBeErased(std::vector<CategoryId> ids) {
    for (auto id : ids) {
        int row = getRow(id);
        beginRemoveRows(QModelIndex(), row, row);
        mIds.erase(id);
        endRemoveRows();
    }
}

void CategoriesModel::categoriesAboutToBeReset() {
    beginResetModel();
}

void CategoriesModel::categoriesReset() {
    mIds.clear();
    for (const auto & p : mStoreHandler.getTournament().getCategories())
        mIds.insert(p.first);
    endResetModel();
}

void CategoriesModel::tournamentReset() {
    beginResetModel();
    QTournamentStore & tournament = mStoreHandler.getTournament();

    mIds.clear();
    for (const auto & p : tournament.getCategories())
        mIds.insert(p.first);

    connect(&tournament, &QTournamentStore::categoriesAdded, this, &CategoriesModel::categoriesAdded);
    connect(&tournament, &QTournamentStore::categoriesChanged, this, &CategoriesModel::categoriesChanged);
    connect(&tournament, &QTournamentStore::categoriesAboutToBeErased, this, &CategoriesModel::categoriesAboutToBeErased);
    connect(&tournament, &QTournamentStore::categoriesAboutToBeReset, this, &CategoriesModel::categoriesAboutToBeReset);
    connect(&tournament, &QTournamentStore::categoriesReset, this, &CategoriesModel::categoriesReset);
    connect(&mStoreHandler, &QStoreHandler::tournamentReset, this, &CategoriesModel::tournamentReset);

    endResetModel();
}

CategoriesProxyModel::CategoriesProxyModel(QStoreHandler &storeHandler, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    mModel = new CategoriesModel(storeHandler, this);

    setSourceModel(mModel);
    setSortRole(Qt::UserRole);
}

std::vector<CategoryId> CategoriesProxyModel::getCategories(const QItemSelection &selection) const {
    return mModel->getCategories(mapSelectionToSource(selection));
}
