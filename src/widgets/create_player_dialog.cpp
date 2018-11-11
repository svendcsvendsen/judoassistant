#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QFormLayout>

#include "widgets/create_player_dialog.hpp"
#include "actions/player_actions.hpp"
#include "widgets/validators/optional_validator.hpp"

CreatePlayerDialog::CreatePlayerDialog(StoreManager & storeManager, QWidget *parent)
    : QDialog(parent)
    , mStoreManager(storeManager)
{
    mFirstNameContent = new QLineEdit;
    mLastNameContent = new QLineEdit;

    mAgeContent = new QLineEdit;
    mAgeContent->setValidator(new OptionalValidator(new QIntValidator(PlayerAge::min(), PlayerAge::max()), this));

    mRankContent = new QComboBox;
    mRankContent->addItem("");
    for (PlayerRank rank : PlayerRank::values())
        mRankContent->addItem(QString::fromStdString(rank.toString()));

    mClubContent = new QLineEdit;

    mWeightContent = new QLineEdit;
    mWeightContent->setValidator(new OptionalValidator(new QDoubleValidator(PlayerWeight::min(), PlayerWeight::max(), 2), this));

    mCountryContent = new QComboBox;
    mCountryContent->addItem("");
    for (PlayerCountry country : PlayerCountry::values())
        mCountryContent->addItem(QString::fromStdString(country.toString()));

    mSexContent = new QComboBox;
    mSexContent->addItem("");
    for (PlayerSex country : PlayerSex::values())
        mSexContent->addItem(QString::fromStdString(country.toString()));

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("First name"), mFirstNameContent);
    formLayout->addRow(tr("Last name"), mLastNameContent);
    formLayout->addRow(tr("Age"), mAgeContent);
    formLayout->addRow(tr("Rank"), mRankContent);
    formLayout->addRow(tr("Club"), mClubContent);
    formLayout->addRow(tr("Weight"), mWeightContent);
    formLayout->addRow(tr("Country"), mCountryContent);
    formLayout->addRow(tr("Sex"), mSexContent);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("Create new player"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &CreatePlayerDialog::acceptClick);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CreatePlayerDialog::cancelClick);
}

void CreatePlayerDialog::acceptClick() {
    std::string firstName = mFirstNameContent->text().toStdString();
    std::string lastName = mLastNameContent->text().toStdString();

    std::optional<PlayerAge> age;
    if (!mAgeContent->text().isEmpty())
        age = PlayerAge(mAgeContent->text().toInt());

    std::optional<PlayerWeight> weight;
    if (!mWeightContent->text().isEmpty())
        weight = PlayerWeight(mWeightContent->text().toFloat());

    std::optional<PlayerRank> rank;
    if (mRankContent->currentIndex() > 0) // account for the first index being nullopt
        rank = PlayerRank(mRankContent->currentIndex() - 1);

    std::string club = mClubContent->text().toStdString();

    std::optional<PlayerCountry> country;
    if (mCountryContent->currentIndex() > 0) // account for the first index being nullopt
        country = PlayerCountry(mCountryContent->currentIndex() - 1);

    std::optional<PlayerSex> sex;
    if (mSexContent->currentIndex() > 0) // account for the first index being nullopt
        sex = PlayerSex(mSexContent->currentIndex() - 1);

    mStoreManager.dispatch(std::make_unique<AddPlayerAction>(mStoreManager.getTournament(), firstName, lastName, age, rank, club, weight, country, sex));
    accept();
}

void CreatePlayerDialog::cancelClick() {
    reject();
}
