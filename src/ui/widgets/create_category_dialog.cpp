#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QFormLayout>

#include "core/actions/add_category_with_players_action.hpp"
#include "core/actions/add_players_to_category_action.hpp"
#include "core/actions/add_category_action.hpp"
#include "core/actions/draw_categories_action.hpp"
#include "core/draw_systems/draw_system.hpp"
#include "core/rulesets/ruleset.hpp"
#include "core/stores/category_store.hpp"
#include "ui/store_managers/store_manager.hpp"
#include "ui/stores/qtournament_store.hpp"
#include "ui/widgets/create_category_dialog.hpp"

CreateCategoryDialog::CreateCategoryDialog(StoreManager & storeManager, const std::vector<PlayerId> &playerIds, QWidget *parent)
    : QDialog(parent)
    , mStoreManager(storeManager)
    , mPlayerIds(playerIds)
{
    mNameContent = new QLineEdit;

    mRulesetContent = new QComboBox;
    for (const auto & ruleset : Ruleset::getRulesets())
        mRulesetContent->addItem(QString::fromStdString(ruleset->getName()));

    mDrawSystemContent = new QComboBox;
    for (const auto & system : DrawSystem::getDrawSystems())
        mDrawSystemContent->addItem(QString::fromStdString(system->getName()));

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Name"), mNameContent);
    formLayout->addRow(tr("Ruleset"), mRulesetContent);
    formLayout->addRow(tr("Draw System"), mDrawSystemContent);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("Create new category"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &CreateCategoryDialog::acceptClick);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CreateCategoryDialog::cancelClick);
}

void CreateCategoryDialog::acceptClick() {
    auto action = std::make_unique<AddCategoryWithPlayersAction>(mStoreManager.getTournament(), mNameContent->text().toStdString(), mRulesetContent->currentIndex(), mDrawSystemContent->currentIndex(), mPlayerIds);
    mStoreManager.dispatch(std::move(action));
    accept();
}

void CreateCategoryDialog::cancelClick() {
    reject();
}

