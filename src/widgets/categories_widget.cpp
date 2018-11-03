#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHeaderView>

#include "widgets/categories_widget.hpp"
#include "widgets/create_category_dialog.hpp"

#include "actions/category_actions.hpp"

CategoriesWidget::CategoriesWidget(QStoreHandler & storeHandler)
    : mStoreHandler(storeHandler)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    {
        QToolBar *toolBar = new QToolBar(tr("Categories toolbar"), this);

        QAction *categoryCreateAction = new QAction(QIcon("player-add.svg"), tr("New category"));
        categoryCreateAction->setStatusTip(tr("Create a new category"));
        toolBar->addAction(categoryCreateAction);

        connect(categoryCreateAction, &QAction::triggered, this, &CategoriesWidget::showCategoryCreateDialog);

        mEraseAction = new QAction(QIcon("player-erase.svg"), tr("Erase the selected categories"));
        mEraseAction->setStatusTip(tr("Erase the selected categories"));
        mEraseAction->setEnabled(false);
        toolBar->addAction(mEraseAction);
        connect(mEraseAction, &QAction::triggered, this, &CategoriesWidget::eraseSelectedCategories);

        layout->addWidget(toolBar);
    }

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    {
        mTableView = new QTableView(this);
        mModel = new CategoriesProxyModel(storeHandler, layout);

        mTableView->setModel(mModel);
        mTableView->horizontalHeader()->setStretchLastSection(true);
        mTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        mTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableView->setSortingEnabled(true);
        mTableView->sortByColumn(0, Qt::AscendingOrder);

        connect(mTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CategoriesWidget::selectionChanged);

        splitter->addWidget(mTableView);
    }

    {
        QTabWidget *tabWidget = new QTabWidget(this);

        mEditCategoryWidget = new EditCategoryWidget(storeHandler, splitter);
        tabWidget->addTab(mEditCategoryWidget, tr("General"));

        mEditCategoryPlayersWidget = new EditCategoryPlayersWidget(mStoreHandler, this);
        tabWidget->addTab(mEditCategoryPlayersWidget, tr("Players"));

        QWidget *c = new QWidget(this);
        tabWidget->addTab(c, tr("Matches"));

        splitter->addWidget(tabWidget);
    }

    layout->addWidget(splitter);
    setLayout(layout);
}

void CategoriesWidget::showCategoryCreateDialog() {
    CreateCategoryDialog dialog(mStoreHandler, this);

    dialog.exec();
}

void CategoriesWidget::eraseSelectedCategories() {
    auto categoryIds = mModel->getCategories(mTableView->selectionModel()->selection());
    mStoreHandler.dispatch(std::make_unique<EraseCategoriesAction>(mStoreHandler.getTournament(), std::move(categoryIds)));
}

void CategoriesWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    auto categoryIds = mModel->getCategories(selected);
    std::optional<CategoryId> categoryId = (categoryIds.size() == 1 ? std::make_optional<CategoryId>(categoryIds.front()) : std::nullopt);

    mEraseAction->setEnabled(!categoryIds.empty());

    mEditCategoryWidget->setCategory(categoryId);
    mEditCategoryPlayersWidget->setCategory(categoryId);
}

