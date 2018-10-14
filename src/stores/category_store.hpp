#pragma once

#include <string>
#include <unordered_set>
#include <map>

#include "core.hpp"
#include "stores/match_store.hpp"
#include "stores/tatami_store.hpp"
#include "draw_systems/draw_system.hpp"
#include "rulesets/ruleset.hpp"
#include "id.hpp"

// TODO: Use rvalue references for functions where appropriate
// TODO: Make members const where appropriate

class CategoryStore {
public:
    typedef std::vector<std::unique_ptr<MatchStore>> MatchList;

    CategoryStore() {}
    CategoryStore(CategoryId id, const std::string &name, std::unique_ptr<Ruleset> ruleset, std::unique_ptr<DrawSystem> drawSystem);

    const std::string & getName() const;
    void setName(const std::string &name);

    const CategoryId & getId() const;

    const MatchList & getMatches() const;
    MatchList & getMatches();
    MatchStore & getMatch(MatchId id);
    const MatchStore & getMatch(MatchId id) const;
    void pushMatch(std::unique_ptr<MatchStore> &&match);
    std::unique_ptr<MatchStore> popMatch();
    bool containsMatch(MatchId id) const;
    MatchList clearMatches();
    size_t getMatchCount(MatchType type) const;

    const std::unordered_set<PlayerId> & getPlayers() const;
    void erasePlayer(PlayerId id);
    void addPlayer(PlayerId id);
    bool containsPlayer(PlayerId id) const;

    void setRuleset(std::unique_ptr<Ruleset> && ptr);
    Ruleset & getRuleset();
    const Ruleset & getRuleset() const;

    void setDrawSystem(std::unique_ptr<DrawSystem> && ptr);
    DrawSystem & getDrawSystem();
    const DrawSystem & getDrawSystem() const;

    std::optional<TatamiLocation> getTatamiLocation(MatchType type) const;
    void setTatamiLocation(MatchType type, std::optional<TatamiLocation> location);

    template<typename Archive>
    void serialize(Archive& ar, uint32_t const version) {
        ar(cereal::make_nvp("id", mId));
        ar(cereal::make_nvp("name", mName));
        ar(cereal::make_nvp("players", mPlayers));
        ar(cereal::make_nvp("matches", mMatches));
        ar(cereal::make_nvp("ruleset", mRuleset));
        ar(cereal::make_nvp("drawSystem", mDrawSystem));
    }
private:
    CategoryId mId;
    std::string mName;
    std::unordered_set<PlayerId> mPlayers;
    MatchList mMatches; // order matters in this case
    std::unordered_map<MatchId, size_t> mMatchMap;
    size_t mMatchCount[2];
    std::optional<TatamiLocation> mTatamiLocation[2];
    std::unique_ptr<Ruleset> mRuleset;
    std::unique_ptr<DrawSystem> mDrawSystem;
};

