#include "core/actions/match_actions.hpp"
#include "core/draw_systems/knockout_draw_system.hpp"
#include "core/log.hpp"
#include "core/rulesets/ruleset.hpp"
#include "core/stores/category_store.hpp"

std::unique_ptr<DrawSystem> KnockoutDrawSystem::clone() const {
    return std::make_unique<KnockoutDrawSystem>(*this);
}

std::string KnockoutDrawSystem::getName() const {
    return "Knockout";
}

std::string matchTitle(size_t round, size_t depth) {
    if (round + 1 == depth)
        return "Final";
    if (round + 2 == depth)
        return "Semi-Final";
    if (round + 3 == depth)
        return "Quarter-Final";

    if (round == 0)
        return "1st Round";
    if (round == 1)
        return "2nd Round";
    if (round == 2)
        return "3rd Round";
    return std::to_string(round+1) + "th Round";
}

std::vector<std::unique_ptr<Action>> KnockoutDrawSystem::initCategory(const TournamentStore &tournament, const CategoryStore &category, const std::vector<PlayerId> &playerIds, unsigned int seed) {
    mMatches.clear();
    mPlayers = playerIds;

    MatchId::Generator generator(seed);
    std::vector<std::unique_ptr<Action>> actions;

    if (mPlayers.size() <= 1)
        return actions;

    std::default_random_engine randomEngine(seed);
    std::shuffle(mPlayers.begin(), mPlayers.end(), randomEngine);

    // Size of round 1
    size_t nodeCount = 1;
    size_t rounds = 1;
    while (nodeCount < (mPlayers.size() + 1)/2) {
        nodeCount *= 2;
        ++rounds;
    }

    mInitialRoundSize = nodeCount;

    // Create the list of players for the first round and add
    // (2*nodeCount-playerCount) byes
    std::vector<std::optional<PlayerId>> nodePlayers;
    nodePlayers.reserve(2*nodeCount);
    size_t j = 0;
    size_t byesNeeded = 2 * nodeCount - mPlayers.size();
    for (size_t i = 0; i < 2*nodeCount; ++i) {
        if (byesNeeded > 0 && i % 2 == 1) {
            nodePlayers.push_back(std::nullopt);
            --byesNeeded;
        }
        else {
            nodePlayers.push_back(mPlayers[j++]);
        }
    }

    assert(j == mPlayers.size());
    assert(byesNeeded == 0);

    // Create matches for all rounds
    for (size_t round = 0;; ++round) {
        for (size_t i = 0; i < nodeCount; ++i) {
            auto whiteId = nodePlayers[2*i];
            auto blueId = nodePlayers[2*i+1];
            bool isBye = (round == 0 && !(whiteId && blueId));
            auto matchType = (round + 2 < rounds ? MatchType::KNOCKOUT : MatchType::FINAL);

            auto action = std::make_unique<AddMatchAction>(MatchId::generate(category, generator), category.getId(), matchType, matchTitle(round, rounds), isBye, whiteId, blueId);
            mMatches.push_back(action->getMatchId());
            actions.push_back(std::move(action));
        }

        if (nodeCount == 1) // The final was just added
            break;

        // Update nodePlayers arrays
        std::vector<std::optional<PlayerId>> updatedNodePlayers;
        updatedNodePlayers.reserve(nodeCount/2);
        assert(updatedNodePlayers.empty());
        for (size_t i = 0; i < nodeCount; ++i) {
            auto whiteId = nodePlayers[2*i];
            auto blueId = nodePlayers[2*i+1];
            if (whiteId && !blueId)
                updatedNodePlayers.push_back(whiteId);
            else if (!whiteId && blueId)
                updatedNodePlayers.push_back(blueId);
            else
                updatedNodePlayers.push_back(std::nullopt);
        }

        nodePlayers = std::move(updatedNodePlayers);
        nodeCount /= 2;
    }

    return actions;
}

// TODO: Matches must be updated in case of winner change
std::vector<std::unique_ptr<Action>> KnockoutDrawSystem::updateCategory(const TournamentStore &tournament, const CategoryStore &category) const {
    const auto &ruleset = category.getRuleset();
    std::vector<std::unique_ptr<Action>> actions;

    // Iterate over all matches from 2nd round and onwards
    size_t prevLayer = 0; // Index to first node of previous layer
    size_t currentLayer = mInitialRoundSize; // Index to first node of current layer
    size_t roundSize = mInitialRoundSize / 2;

    while (currentLayer < mMatches.size()) {
        bool shouldBreak = true; // The loop can break early if no matches has finished in this round
        for (size_t i = currentLayer; i < currentLayer + roundSize; ++i) {
            auto matchId = mMatches[i];
            const auto &match = category.getMatch(matchId);

            if (match.isBye()) {
                shouldBreak = false;
                continue;
            }

            if (match.getStatus() == MatchStatus::FINISHED) {
                shouldBreak = false;
                continue;
            }

            if (!match.getWhitePlayer()) {
                MatchId whiteChildId = mMatches[(i-currentLayer)*2+prevLayer];
                const auto &whiteChild = category.getMatch(whiteChildId);

                if (whiteChild.getStatus() == MatchStatus::FINISHED) {
                    auto winner = ruleset.getWinner(whiteChild);
                    if (winner)
                        actions.push_back(std::make_unique<SetMatchPlayerAction>(category.getId(), matchId, MatchStore::PlayerIndex::WHITE, whiteChild.getPlayer(*winner).value()));
                    else
                        log_warning().field("matchId", whiteChildId).msg("Match is finished but has no winner");
                }
            }

            if (!match.getBluePlayer()) {
                MatchId blueChildId = mMatches[(i-currentLayer)*2+prevLayer + 1];
                const auto &blueChild = category.getMatch(blueChildId);

                if (blueChild.getStatus() == MatchStatus::FINISHED) {
                    auto winner = ruleset.getWinner(blueChild);
                    if (winner)
                        actions.push_back(std::make_unique<SetMatchPlayerAction>(category.getId(), matchId, MatchStore::PlayerIndex::BLUE, blueChild.getPlayer(*winner).value()));
                    else
                        log_warning().field("matchId", blueChildId).msg("Match is finished but has no winner");
                }
            }
        }

        prevLayer = currentLayer;
        currentLayer += roundSize;
        roundSize /= 2;

        if (shouldBreak)
            break;
    }

    return actions;
}

bool KnockoutDrawSystem::isFinished(const TournamentStore &tournament, const CategoryStore &category) const {
    const auto &knockoutStatus = category.getStatus(MatchType::KNOCKOUT);
    if (knockoutStatus.notStartedMatches > 0 || knockoutStatus.startedMatches > 0)
        return false;

    const auto &finalStatus = category.getStatus(MatchType::FINAL);
    if (finalStatus.notStartedMatches > 0 || finalStatus.startedMatches > 0)
        return false;
    return true;
}

std::vector<std::pair<std::optional<unsigned int>, PlayerId>> KnockoutDrawSystem::getResults(const TournamentStore &tournament, const CategoryStore &category) const {
    std::vector<std::pair<std::optional<unsigned int>, PlayerId>> results;
    const auto &ruleset = category.getRuleset();

    // Iterate over each layer(starting from the final) and add the losers.
    // Special case the root where we also add the winner
    unsigned int pos = 1;
    size_t layer_size = 1;
    size_t next_layer = 1;

    if (!isFinished(tournament, category) || category.getMatches().size() == 0) {
        for (auto playerId : mPlayers)
            results.emplace_back(std::nullopt, playerId);
        return results;
    }

    for (size_t i = 0; i < mMatches.size(); ++i) {
        if (i == next_layer) {
            pos += layer_size;
            if (i == 1)
                pos += 1;
            layer_size *= 2;
            next_layer = i + layer_size;
        }

        size_t j = mMatches.size() - i - 1;
        const auto &match = category.getMatch(mMatches[j]);

        if (match.isBye())
            continue;

        assert(match.getStatus() == MatchStatus::FINISHED);

        MatchStore::PlayerIndex winner = ruleset.getWinner(match).value();
        MatchStore::PlayerIndex loser = (winner == MatchStore::PlayerIndex::WHITE ? MatchStore::PlayerIndex::BLUE : MatchStore::PlayerIndex::WHITE);
        if (i != 0) {  // Special case root(add winner if we're at the root)
            results.push_back({std::make_optional(pos), match.getPlayer(loser).value()});
        }
        else {
            results.push_back({std::make_optional(pos), match.getPlayer(winner).value()});
            results.push_back({std::make_optional(pos+1), match.getPlayer(loser).value()});
        }
    }

    return results;
}

bool KnockoutDrawSystem::hasFinalBlock() const {
    return true;
}
