#include "stores/player_store.hpp"

PlayerStore::PlayerStore(PlayerId id, const std::string & firstName, const std::string & lastName, std::optional<uint8_t> age, std::optional<PlayerRank> rank, const std::string &club, std::optional<float> weight, std::optional<PlayerCountry> country)
    : mId(id)
    , mFirstName(firstName)
    , mLastName(lastName)
    , mAge(age)
    , mRank(rank)
    , mClub(club)
    , mWeight(weight)
    , mCountry(country)
{}

const std::string & PlayerStore::getFirstName() const {
    return mFirstName;
}

const std::string & PlayerStore::getLastName() const {
    return mLastName;
}

const std::optional<uint8_t> & PlayerStore::getAge() const {
    return mAge;
}

const PlayerId & PlayerStore::getId() const {
    return mId;
}

int PlayerRank::toInt() const {
    return static_cast<int>(mValue);
}

std::string PlayerRank::toString() const {
    // TODO: handle translation
    switch (mValue) {
        case KYU_6: return "6th kyu";
        case KYU_5: return "5th kyu";
        case KYU_4: return "4th kyu";
        case KYU_3: return "3th kyu";
        case KYU_2: return "2nd kyu";
        case KYU_1: return "1st kyu";
        case DAN_1: return "1st dan";
        case DAN_2: return "2nd dan";
        case DAN_3: return "3th dan";
        case DAN_4: return "4th dan";
        case DAN_5: return "5th dan";
        case DAN_6: return "6th dan";
        case DAN_7: return "7th dan";
        case DAN_8: return "8th dan";
        case DAN_9: return "9th dan";
        case DAN_10: return "10th dan";
        default: return "";
    }
}

std::vector<PlayerRank> PlayerRank::values() {
    std::vector<PlayerRank> res;
    for (size_t i = 0; i < SIZE; ++i) {
        res.push_back(PlayerRank(static_cast<Enum>(i)));
    }

    return res;
}

std::string PlayerCountry::toString() const {
    // TODO: handle translation
    switch (mValue) {
        case DENMARK: return "Denmark";
        case UNITED_KINGDOM: return "United Kingdom";
        case FRANCE: return "France";
        default: return "";
    }
}

std::vector<PlayerCountry> PlayerCountry::values() {
    std::vector<PlayerCountry> res;
    for (size_t i = 0; i < SIZE; ++i) {
        res.push_back(PlayerCountry(static_cast<Enum>(i)));
    }

    return res;
}

const std::optional<float> & PlayerStore::getWeight() const {
    return mWeight;
}

const std::optional<PlayerRank> & PlayerStore::getRank() const {
    return mRank;
}

const std::optional<PlayerCountry> & PlayerStore::getCountry() const {
    return mCountry;
}

const std::string & PlayerStore::getClub() const {
    return mClub;
}

void PlayerStore::eraseMatch(CategoryId categoryId, MatchId matchId) {
    mMatches.erase(std::make_pair(categoryId, matchId));
}

void PlayerStore::addMatch(CategoryId categoryId, MatchId matchId) {
    mMatches.insert(std::make_pair(categoryId, matchId));
}

const std::unordered_set<std::pair<CategoryId,MatchId>, CategoryIdMatchIdHasher> & PlayerStore::getMatches() const {
    return mMatches;
}

bool PlayerStore::containsMatch(CategoryId categoryId, MatchId matchId) const {
    return mMatches.find(std::make_pair(categoryId, matchId)) != mMatches.end();
}

void PlayerStore::eraseCategory(CategoryId id) {
    mCategories.erase(id);
}

void PlayerStore::addCategory(CategoryId id) {
    mCategories.insert(id);
}

const std::unordered_set<CategoryId, CategoryId::Hasher> & PlayerStore::getCategories() const {
    return mCategories;
}

int PlayerCountry::toInt() const {
    return static_cast<int>(mValue);
}

bool PlayerStore::containsCategory(CategoryId id) const {
    return mCategories.find(id) != mCategories.end();
}

void PlayerStore::setFirstName(const std::string & firstName) {
    mFirstName = firstName;
}

void PlayerStore::setLastName(const std::string & lastName) {
    mLastName = lastName;
}

void PlayerStore::setAge(std::optional<uint8_t> age) {
    mAge = age;
}

void PlayerStore::setWeight(std::optional<float> weight) {
    mWeight = weight;
}

void PlayerStore::setRank(std::optional<PlayerRank> rank) {
    mRank = rank;
}

void PlayerStore::setCountry(std::optional<PlayerCountry> country) {
    mCountry = country;
}

void PlayerStore::setClub(const std::string & club) {
    mClub = club;
}

std::ostream & operator<<(std::ostream &out, const PlayerCountry &country) {
    return out << country.toString();
}

std::ostream & operator<<(std::ostream &out, const PlayerRank &rank) {
    return out << rank.toString();
}
