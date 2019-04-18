#pragma once

#include "core/draw_systems/draw_system.hpp"
#include "core/serialize.hpp"

class PoolDrawSystem : public DrawSystem {
public:
    PoolDrawSystem() {}
    PoolDrawSystem(const PoolDrawSystem &) = default;
    virtual ~PoolDrawSystem() {};

    std::unique_ptr<DrawSystem> clone() const override;
    std::string getName() const override;
    bool hasFinalBlock() const override;

    std::vector<std::unique_ptr<Action>> initCategory(const TournamentStore &tournament, const CategoryStore &category, const std::vector<PlayerId> &playerIds, unsigned int seed) override;
    std::vector<std::unique_ptr<Action>> updateCategory(const TournamentStore &tournament, const CategoryStore &category) const override;
    std::vector<std::pair<std::optional<unsigned int>, PlayerId>> getResults(const TournamentStore &tournament, const CategoryStore &category) const override;

    bool isFinished(const TournamentStore &tournament, const CategoryStore &category, const std::vector<PlayerId> &playerIds, unsigned int seed) const;

    template<typename Archive>
    void serialize(Archive& ar, uint32_t const version) {
        ar(mMatches, mPlayers);
    }

private:
    std::vector<MatchId> mMatches;
    std::vector<PlayerId> mPlayers;
};

CEREAL_REGISTER_TYPE(PoolDrawSystem)
CEREAL_REGISTER_POLYMORPHIC_RELATION(DrawSystem, PoolDrawSystem)
