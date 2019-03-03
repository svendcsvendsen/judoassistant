#pragma once

#include <string>
#include <cstdint>
#include <unordered_set>

#include "core/core.hpp"
#include "core/serialize.hpp"
#include "core/id.hpp"
#include "core/hash.hpp"

class PlayerCountry {
public:
    // TODO: Find list of countries
    // TODO: Make a more sophisticated from string method
    static const size_t SIZE = 3;
    enum Enum {
        DENMARK,
        UNITED_KINGDOM,
        FRANCE,
    };

    PlayerCountry() {}
    PlayerCountry(const PlayerCountry &other) = default;
    PlayerCountry(const std::string &str);
    PlayerCountry(int value);

    std::string toString() const;
    int toInt() const;
    static std::vector<PlayerCountry> values();

    bool operator==(const PlayerCountry &other) const {
        return mValue == other.mValue;
    }

    bool operator<(const PlayerCountry &other) const {
        return mValue < other.mValue;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(mValue);
    }

private:
    PlayerCountry(Enum value) : mValue(value) {}

    Enum mValue;
};

std::ostream & operator<<(std::ostream &out, const PlayerCountry &country);

class PlayerRank {
public:
    static const size_t SIZE = 16;
    enum Enum {
        KYU_6,
        KYU_5,
        KYU_4,
        KYU_3,
        KYU_2,
        KYU_1,
        DAN_1,
        DAN_2,
        DAN_3,
        DAN_4,
        DAN_5,
        DAN_6,
        DAN_7,
        DAN_8,
        DAN_9,
        DAN_10,
    };

    PlayerRank() {}
    PlayerRank(const PlayerRank &other) = default;
    PlayerRank(const std::string &str);
    PlayerRank(int value);

    std::string toString() const;
    int toInt() const;
    static std::vector<PlayerRank> values();

    bool operator==(const PlayerRank &other) const {
        return mValue == other.mValue;
    }

    bool operator<(const PlayerRank &other) const {
        return mValue < other.mValue;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(mValue);
    }
private:
    PlayerRank(Enum value) : mValue(value) {}

    Enum mValue;
};

enum class Sex {
    MALE, FEMALE
};

std::ostream & operator<<(std::ostream &out, const PlayerRank &rank);

class PlayerSex {
public:
    static const size_t SIZE = 2;
    enum Enum {
        MALE,
        FEMALE,
    };

    PlayerSex() {}
    PlayerSex(const PlayerSex &other) = default;
    PlayerSex(int value);
    PlayerSex(const std::string &str);

    std::string toString() const;
    int toInt() const;
    static std::vector<PlayerSex> values();

    bool operator==(const PlayerSex &other) const {
        return mValue == other.mValue;
    }

    bool operator<(const PlayerSex &other) const {
        return mValue < other.mValue;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(mValue);
    }
private:
    PlayerSex(Enum value) : mValue(value) {}

    Enum mValue;
};

std::ostream & operator<<(std::ostream &out, const PlayerSex &rank);

class PlayerWeight {
public:
    PlayerWeight() {}
    PlayerWeight(const PlayerWeight &other) = default;
    PlayerWeight(float weight);
    PlayerWeight(const std::string &str);

    std::string toString() const;
    float toFloat() const;

    bool operator==(const PlayerWeight &other) const {
        return mValue == other.mValue;
    }

    bool operator<(const PlayerWeight &other) const {
        return mValue < other.mValue;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(mValue);
    }

    static float max();
    static float min();
private:
    float mValue;
};

std::ostream & operator<<(std::ostream &out, const PlayerWeight &weight);

class PlayerAge {
public:
    PlayerAge() {}
    PlayerAge(const PlayerAge &other) = default;
    PlayerAge(int age);
    PlayerAge(const std::string &str);

    std::string toString() const;
    int toInt() const;

    bool operator==(const PlayerAge &other) const {
        return mValue == other.mValue;
    }

    bool operator<(const PlayerAge &other) const {
        return mValue < other.mValue;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(mValue);
    }

    static int max();
    static int min();
private:
    int mValue;
};

std::ostream & operator<<(std::ostream &out, const PlayerAge &age);

struct PlayerFields {
    std::string firstName;
    std::string lastName;
    std::optional<PlayerAge> age;
    std::optional<PlayerRank> rank;
    std::string club;
    std::optional<PlayerWeight> weight;
    std::optional<PlayerCountry> country;
    std::optional<PlayerSex> sex;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(cereal::make_nvp("firstName", firstName));
        ar(cereal::make_nvp("lastName", lastName));
        ar(cereal::make_nvp("age", age));
        ar(cereal::make_nvp("rank", rank));
        ar(cereal::make_nvp("club", club));
        ar(cereal::make_nvp("weight", weight));
        ar(cereal::make_nvp("country", country));
        ar(cereal::make_nvp("sex", sex));
    }
};

class PlayerStore {
public:
    PlayerStore() {}
    PlayerStore(const PlayerStore &other) = default;
    PlayerStore(PlayerId id, const PlayerFields &fields);

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar(cereal::make_nvp("id", mId));
        ar(cereal::make_nvp("categories", mCategories));
        ar(cereal::make_nvp("matches", mMatches));
        ar(cereal::make_nvp("fields", mFields));
    }

    const std::string & getFirstName() const;
    const std::string & getLastName() const;
    const std::optional<PlayerAge> & getAge() const;
    const std::optional<PlayerWeight> & getWeight() const;
    const std::optional<PlayerRank> & getRank() const;
    const std::optional<PlayerCountry> & getCountry() const;
    const std::string & getClub() const;
    const PlayerId & getId() const;
    const std::optional<PlayerSex> getSex() const;

    void setFirstName(const std::string & firstName);
    void setLastName(const std::string & lastName);
    void setAge(std::optional<PlayerAge> age);
    void setWeight(std::optional<PlayerWeight> weight);
    void setRank(std::optional<PlayerRank> rank);
    void setCountry(std::optional<PlayerCountry> country);
    void setClub(const std::string & club);
    void setSex(const std::optional<PlayerSex> sex);

    const std::unordered_set<CategoryId> & getCategories() const;
    void addCategory(CategoryId id);
    void eraseCategory(CategoryId id);
    bool containsCategory(CategoryId id) const;

    const std::unordered_set<std::pair<CategoryId,MatchId>> & getMatches() const;
    void addMatch(CategoryId categoryId, MatchId matchId);
    void eraseMatch(CategoryId categoryId, MatchId matchId);
    bool containsMatch(CategoryId categoryId, MatchId matchId) const;
private:
    PlayerId mId;
    PlayerFields mFields;

    std::unordered_set<CategoryId> mCategories;
    std::unordered_set<std::pair<CategoryId,MatchId>> mMatches;
};
