#include <sstream>
#include <set>

#include "actions/category_actions.hpp"
#include "actions/match_actions.hpp"
#include "draw_systems/draw_systems.hpp"
#include "exception.hpp"
#include "rulesets/rulesets.hpp"
#include "stores/tournament_store.hpp"
#include "stores/category_store.hpp"
#include "stores/match_store.hpp"
#include "stores/tatami_store.hpp"

AddCategoryAction::AddCategoryAction(CategoryId id, const std::string &name, uint8_t ruleset, uint8_t drawSystem)
    : mId(id)
    , mName(name)
    , mRuleset(ruleset)
    , mDrawSystem(drawSystem)
{}

AddCategoryAction::AddCategoryAction(TournamentStore & tournament, const std::string &name, uint8_t ruleset, uint8_t drawSystem)
    : AddCategoryAction(CategoryId::generate(tournament), name, ruleset, drawSystem)
{}


std::unique_ptr<Action> AddCategoryAction::freshClone() const {
    return std::make_unique<AddCategoryAction>(mId, mName, mRuleset, mDrawSystem);
}

CategoryId AddCategoryAction::getId() const {
    return mId;
}

void AddCategoryAction::redoImpl(TournamentStore & tournament) {
    if (tournament.containsCategory(mId))
        throw ActionExecutionException("Failed to redo AddCategoryAction. Category already exists.");

    const auto &rulesets = Rulesets::getRulesets();
    if (mRuleset > rulesets.size())
        throw ActionExecutionException("Failed to redo AddCategoryAction. Invalid ruleset specified.");
    auto ruleset = rulesets[mRuleset]->clone();

    const auto &drawSystems = DrawSystems::getDrawSystems();
    if (mDrawSystem > drawSystems.size())
        throw ActionExecutionException("Failed to redo AddCategoryAction. Invalid drawSystem specified.");
    auto drawSystem = drawSystems[mDrawSystem]->clone();

    tournament.beginAddCategories({mId});
    tournament.addCategory(std::make_unique<CategoryStore>(mId, mName, std::move(ruleset), std::move(drawSystem)));
    tournament.endAddCategories();
}

void AddCategoryAction::undoImpl(TournamentStore & tournament) {
    tournament.beginEraseCategories({mId});
    tournament.eraseCategory(mId);
    tournament.endEraseCategories();
}

AddPlayersToCategoryAction::AddPlayersToCategoryAction(CategoryId categoryId, const std::vector<PlayerId> &playerIds)
    : mCategoryId(categoryId)
    , mPlayerIds(playerIds)
{}

std::unique_ptr<Action> AddPlayersToCategoryAction::freshClone() const {
    return std::make_unique<AddPlayersToCategoryAction>(mCategoryId, mPlayerIds);
}

void AddPlayersToCategoryAction::redoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId)) return;
    CategoryStore & category = tournament.getCategory(mCategoryId);

    for (auto playerId : mPlayerIds) {
        if (!tournament.containsPlayer(playerId)) continue;

        PlayerStore & player = tournament.getPlayer(playerId);
        if (player.containsCategory(mCategoryId))
            continue;

        mAddedPlayerIds.push_back(playerId);
        player.addCategory(mCategoryId);
        category.addPlayer(playerId);
    }

    tournament.addPlayersToCategory(mCategoryId, mAddedPlayerIds);

    if (!mAddedPlayerIds.empty()) {
        mDrawAction = std::make_unique<DrawCategoryAction>(mCategoryId);
        mDrawAction->redo(tournament);
    }
}

void AddPlayersToCategoryAction::undoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId)) return;

    tournament.erasePlayersFromCategory(mCategoryId, mAddedPlayerIds);

    CategoryStore & category = tournament.getCategory(mCategoryId);
    for (auto playerId : mAddedPlayerIds) {
        PlayerStore & player = tournament.getPlayer(playerId);
        player.eraseCategory(mCategoryId);
        category.erasePlayer(playerId);
    }

    if (mDrawAction != nullptr) {
        mDrawAction->undo(tournament);
        mDrawAction.reset();
    }

    mAddedPlayerIds.clear();
}

ErasePlayersFromCategoryAction::ErasePlayersFromCategoryAction(CategoryId categoryId, const std::vector<PlayerId> &playerIds)
    : mCategoryId(categoryId)
    , mPlayerIds(playerIds)
{}

std::unique_ptr<Action> ErasePlayersFromCategoryAction::freshClone() const {
    return std::make_unique<ErasePlayersFromCategoryAction>(mCategoryId, mPlayerIds);
}

void ErasePlayersFromCategoryAction::redoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId)) return;

    CategoryStore & category = tournament.getCategory(mCategoryId);

    for (auto playerId : mPlayerIds) {
        if (!tournament.containsPlayer(playerId)) continue;

        PlayerStore & player = tournament.getPlayer(playerId);
        if (!player.containsCategory(mCategoryId))
            continue;

        mErasedPlayerIds.push_back(playerId);
        player.eraseCategory(mCategoryId);
        category.erasePlayer(playerId);
    }

    tournament.erasePlayersFromCategory(mCategoryId, mErasedPlayerIds);

    if (!mErasedPlayerIds.empty()) {
        mDrawAction = std::make_unique<DrawCategoryAction>(mCategoryId);
        mDrawAction->redo(tournament);
    }
}

void ErasePlayersFromCategoryAction::undoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId)) return;

    CategoryStore & category = tournament.getCategory(mCategoryId);

    tournament.addPlayersToCategory(mCategoryId, mErasedPlayerIds);

    for (auto playerId : mErasedPlayerIds) {
        PlayerStore & player = tournament.getPlayer(playerId);
        player.addCategory(mCategoryId);
        category.addPlayer(playerId);
    }

    if (mDrawAction != nullptr) {
        mDrawAction->undo(tournament);
        mDrawAction.reset();
    }

    mErasedPlayerIds.clear();
}

EraseCategoriesAction::EraseCategoriesAction(const std::vector<CategoryId> &categoryIds)
    : mCategoryIds(categoryIds)
{}

std::unique_ptr<Action> EraseCategoriesAction::freshClone() const {
    return std::make_unique<EraseCategoriesAction>(mCategoryIds);
}

void EraseCategoriesAction::redoImpl(TournamentStore & tournament) {
    for (auto categoryId : mCategoryIds) {
        if (!tournament.containsCategory(categoryId))
            continue;

        mErasedCategoryIds.push_back(categoryId);
    }

    std::unordered_set<TatamiLocation> locations;
    std::unordered_set<std::pair<CategoryId, MatchType>> blocks;

    tournament.beginEraseCategories(mErasedCategoryIds);

    for (auto categoryId : mErasedCategoryIds) {
        CategoryStore & category = tournament.getCategory(categoryId);

        {
            auto loc = category.getTatamiLocation(MatchType::KNOCKOUT);
            if (loc) {
                locations.insert(*loc);
                blocks.insert(std::make_pair(categoryId, MatchType::KNOCKOUT));
                tournament.getTatamis().moveBlock(tournament, categoryId, MatchType::KNOCKOUT, loc, std::nullopt, 0);
            }
        }

        {
            auto loc = category.getTatamiLocation(MatchType::FINAL);
            if (loc) {
                locations.insert(*loc);
                blocks.insert(std::make_pair(categoryId, MatchType::FINAL));
                tournament.getTatamis().moveBlock(tournament, categoryId, MatchType::FINAL, loc, std::nullopt, 0);
            }
        }

        for (auto playerId : category.getPlayers()) {
            PlayerStore & player = tournament.getPlayer(playerId);
            player.eraseCategory(categoryId);
        }

        for (const std::unique_ptr<MatchStore> &match : category.getMatches()) {
            auto whitePlayerId = match->getPlayer(MatchStore::PlayerIndex::WHITE);
            if (whitePlayerId)
                tournament.getPlayer(*whitePlayerId).eraseMatch(categoryId, match->getId());

            auto bluePlayerId = match->getPlayer(MatchStore::PlayerIndex::BLUE);
            if (bluePlayerId)
                tournament.getPlayer(*bluePlayerId).eraseMatch(categoryId, match->getId());
        }

        mCategories.push(tournament.eraseCategory(categoryId));
    }

    tournament.endEraseCategories();

    mLocations = std::vector(locations.begin(), locations.end());
    mBlocks = std::vector(blocks.begin(), blocks.end());

    if (!mBlocks.empty())
        tournament.changeTatamis(mLocations, mBlocks);
}

void EraseCategoriesAction::undoImpl(TournamentStore & tournament) {
    tournament.beginAddCategories(mErasedCategoryIds);

    while (!mCategories.empty()) {
        std::unique_ptr<CategoryStore> category = std::move(mCategories.top());

        for (auto playerId : category->getPlayers()) {
            PlayerStore & player = tournament.getPlayer(playerId);
            player.addCategory(category->getId());
        }

        for (const std::unique_ptr<MatchStore> &match : category->getMatches()) {
            auto whitePlayerId = match->getPlayer(MatchStore::PlayerIndex::WHITE);
            if (whitePlayerId)
                tournament.getPlayer(*whitePlayerId).addMatch(category->getId(), match->getId());

            auto bluePlayerId = match->getPlayer(MatchStore::PlayerIndex::BLUE);
            if (bluePlayerId)
                tournament.getPlayer(*bluePlayerId).addMatch(category->getId(), match->getId());
        }

        // copy before category
        auto categoryId = category->getId();
        auto knockoutLoc = category->getTatamiLocation(MatchType::KNOCKOUT);
        auto finalLoc = category->getTatamiLocation(MatchType::FINAL);

        tournament.addCategory(std::move(category));

        if (knockoutLoc)
            tournament.getTatamis().moveBlock(tournament, categoryId, MatchType::KNOCKOUT, std::nullopt, knockoutLoc, 0);

        if (finalLoc)
            tournament.getTatamis().moveBlock(tournament, categoryId, MatchType::FINAL, std::nullopt, finalLoc, 0);

        mCategories.pop();
    }

    tournament.endAddCategories();
    if (!mBlocks.empty())
        tournament.changeTatamis(mLocations, mBlocks);

    mErasedCategoryIds.clear();
    mBlocks.clear();
    mLocations.clear();
}

DrawCategoryAction::DrawCategoryAction(CategoryId categoryId)
    : mCategoryId(categoryId)
{}

std::unique_ptr<Action> DrawCategoryAction::freshClone() const {
    return std::make_unique<DrawCategoryAction>(mCategoryId);
}

void DrawCategoryAction::redoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    // Delete all existing matches
    tournament.beginResetMatches(mCategoryId);
    CategoryStore & category = tournament.getCategory(mCategoryId);

    for (const std::unique_ptr<MatchStore> &match : category.getMatches()) {
        std::optional<PlayerId> whitePlayer = match->getPlayer(MatchStore::PlayerIndex::WHITE);
        if (whitePlayer && tournament.containsPlayer(*whitePlayer))
            tournament.getPlayer(*whitePlayer).eraseMatch(mCategoryId, match->getId());

        std::optional<PlayerId> bluePlayer = match->getPlayer(MatchStore::PlayerIndex::BLUE);
        if (bluePlayer && tournament.containsPlayer(*bluePlayer))
            tournament.getPlayer(*bluePlayer).eraseMatch(mCategoryId, match->getId());
    }

    mOldMatches = std::move(category.clearMatches());
    mOldDrawSystem = category.getDrawSystem().clone();

    std::vector<PlayerId> playerIds(category.getPlayers().begin(), category.getPlayers().end());
    std::vector<std::unique_ptr<Action>> actions = category.getDrawSystem().initCategory(playerIds, tournament, category);

    for (size_t i = 0; i < actions.size(); ++i) {
        actions[i]->redo(tournament);
        mActions.push(std::move(actions[i]));
    }

    tournament.endResetMatches(mCategoryId);

    std::vector<TatamiLocation> changedTatamiLocations;
    std::vector<std::pair<CategoryId, MatchType>> changedBlocks;

    for (MatchType type : {MatchType::FINAL, MatchType::KNOCKOUT}) {
        std::optional<TatamiLocation> location = category.getTatamiLocation(type);
        if (location) {
            tournament.getTatamis().recomputeBlock(tournament, *location);
            changedTatamiLocations.push_back(*location);
            changedBlocks.push_back({mCategoryId, type});
        }
    }

    if (!changedTatamiLocations.empty())
        tournament.changeTatamis(changedTatamiLocations, changedBlocks);
}

void DrawCategoryAction::undoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    tournament.beginResetMatches(mCategoryId);
    CategoryStore & category = tournament.getCategory(mCategoryId);

    while (!mActions.empty()) {
        mActions.top()->undo(tournament);
        mActions.pop();
    }

    category.setDrawSystem(std::move(mOldDrawSystem));

    for (std::unique_ptr<MatchStore> & match : mOldMatches) {
        std::optional<PlayerId> whitePlayer = match->getPlayer(MatchStore::PlayerIndex::WHITE);
        if (whitePlayer && tournament.containsPlayer(*whitePlayer))
            tournament.getPlayer(*whitePlayer).eraseMatch(mCategoryId, match->getId());

        std::optional<PlayerId> bluePlayer = match->getPlayer(MatchStore::PlayerIndex::BLUE);
        if (bluePlayer && tournament.containsPlayer(*bluePlayer))
            tournament.getPlayer(*bluePlayer).eraseMatch(mCategoryId, match->getId());

        category.pushMatch(std::move(match));
    }

    mOldMatches.clear();

    tournament.endResetMatches(mCategoryId);

    std::vector<TatamiLocation> changedTatamiLocations;
    std::vector<std::pair<CategoryId, MatchType>> changedBlocks;

    for (MatchType type : {MatchType::FINAL, MatchType::KNOCKOUT}) {
        std::optional<TatamiLocation> location = category.getTatamiLocation(type);
        if (location) {
            tournament.getTatamis().recomputeBlock(tournament, *location);
            changedTatamiLocations.push_back(*location);
            changedBlocks.push_back({mCategoryId, type});
        }
    }

    if (!changedTatamiLocations.empty())
        tournament.changeTatamis(changedTatamiLocations, changedBlocks);
}

ErasePlayersFromAllCategoriesAction::ErasePlayersFromAllCategoriesAction(const std::vector<PlayerId> &playerIds)
    : mPlayerIds(playerIds)
{}

std::unique_ptr<Action> ErasePlayersFromAllCategoriesAction::freshClone() const {
    return std::make_unique<ErasePlayersFromAllCategoriesAction>(mPlayerIds);
}

void ErasePlayersFromAllCategoriesAction::redoImpl(TournamentStore & tournament) {
    std::vector<PlayerId> playerIds;
    std::unordered_set<CategoryId> categoryIds;

    for (auto playerId : mPlayerIds) {
        if (!tournament.containsPlayer(playerId)) continue;

        playerIds.push_back(playerId);
        const PlayerStore &player = tournament.getPlayer(playerId);
        const auto &playerCategories = player.getCategories();
        categoryIds.insert(playerCategories.begin(), playerCategories.end());
    }

    for (auto categoryId : categoryIds) {
        auto action = std::make_unique<ErasePlayersFromCategoryAction>(categoryId, playerIds);
        action->redo(tournament);
        mActions.push(std::move(action));
    }
}

void ErasePlayersFromAllCategoriesAction::undoImpl(TournamentStore & tournament) {
    while (!mActions.empty()) {
        mActions.top()->undo(tournament);
        mActions.pop();
    }
}

struct Solution {
    bool possible;
    std::multiset<size_t> sizes;
    size_t next;

    bool operator<(const Solution &other) {
        if (possible != other.possible)
            return possible; // smaller iff possible

        auto i = sizes.begin();
        auto j = other.sizes.begin();

        while (i != sizes.end() && j != other.sizes.end()) {
            if (*i != *j)
                return *i > *j;

            ++j;
            ++i;
        }

        return false;
    }
};

AutoAddCategoriesAction::AutoAddCategoriesAction(TournamentStore &tournament, std::vector<PlayerId> playerIds, std::string baseName, float maxDifference, size_t maxSize)
    : mBaseName(baseName)
{
    log_debug().msg("Redoing");
    std::vector<std::pair<float, PlayerId>> weights;
    for (auto playerId : playerIds) {
        if (!tournament.containsPlayer(playerId))
            continue;
        const PlayerStore &player = tournament.getPlayer(playerId);
        if (!player.getWeight())
            continue;
        weights.push_back({player.getWeight()->toFloat(), playerId});
    }

    log_debug().field("weights", weights).msg("Created weights array");
    if (weights.empty())
        return;

    std::sort(weights.begin(), weights.end());

    std::vector<Solution> dp(weights.size()+1);

    auto & back = dp.back();
    back.possible = true;

    for (int i = weights.size() - 1; i >= 0; --i) {
        Solution best_so_far;
        best_so_far.possible = false;

        for (size_t j = static_cast<size_t>(i); j < weights.size(); ++j) {
            auto diff = 100.0 * (weights[j].first - weights[i].first)/weights[j].first;
            auto size = j-static_cast<size_t>(i)+1;

            if (size > maxSize)
                break;
            if (diff > maxDifference)
                break;

            const auto & next = dp[j+1];

            Solution current;
            current.possible = next.possible;
            if (current.possible) { // no reason to copy if not possible
                current.sizes = next.sizes;
                current.sizes.insert(size);
                if (current.sizes.size() > 50) // Limit to 50 to avoid n^3 complexity
                    current.sizes.erase(std::prev(current.sizes.end()));
            }
            current.next = j+1;

            if (current < best_so_far)
                best_so_far = current;
        }

        dp[i] = best_so_far;
    }

    if (!dp[0].possible) return;

    size_t next = 0;
    while (next != weights.size()) {
        auto sol = dp[next];
        std::vector<PlayerId> playerIds;
        for (size_t j = next; j < sol.next; ++j)
            playerIds.push_back(weights[j].second);

        mCategoryIds.push_back(CategoryId::generate(tournament));
        mPlayerIds.push_back(std::move(playerIds));

        next = sol.next;
    }
}

AutoAddCategoriesAction::AutoAddCategoriesAction(const std::vector<std::vector<PlayerId>> &playerIds, std::vector<CategoryId> categoryIds, std::string baseName)
    : mPlayerIds(playerIds)
    , mCategoryIds(categoryIds)
    , mBaseName(baseName)
{}

std::unique_ptr<Action> AutoAddCategoriesAction::freshClone() const {
    return std::make_unique<AutoAddCategoriesAction>(mPlayerIds, mCategoryIds, mBaseName);
}

void AutoAddCategoriesAction::redoImpl(TournamentStore & tournament) {
    for (auto categoryId : mCategoryIds) {
        if (tournament.containsCategory(categoryId))
            throw ActionExecutionException("Failed to redo AutoAddCategoriesAction. Category already exists.");
    }

    const auto &rulesets = Rulesets::getRulesets();
    const auto &drawSystems = DrawSystems::getDrawSystems();
    tournament.beginAddCategories(mCategoryIds);
    for (size_t i = 0; i < mCategoryIds.size(); ++i) {
        // TODO: Use different drawsystems for different size groups
        std::stringstream ss;
        ss << mBaseName << " " << i+1;
        auto ruleset = rulesets[0]->clone();
        auto drawSystem = drawSystems[0]->clone();
        tournament.addCategory(std::make_unique<CategoryStore>(mCategoryIds[i], ss.str(), std::move(ruleset), std::move(drawSystem)));
    }
    tournament.endAddCategories();

    for (size_t i = 0; i < mCategoryIds.size(); ++i) {
        AddPlayersToCategoryAction playersAction(mCategoryIds[i], mPlayerIds[i]);
        playersAction.redo(tournament);
    }
}

void AutoAddCategoriesAction::undoImpl(TournamentStore & tournament) {
    EraseCategoriesAction eraseAction(mCategoryIds);
    eraseAction.redo(tournament);
}

ChangeCategoryNameAction::ChangeCategoryNameAction(CategoryId categoryId, const std::string &value)
    : mCategoryId(categoryId)
    , mValue(value)
{}

std::unique_ptr<Action> ChangeCategoryNameAction::freshClone() const {
    return std::make_unique<ChangeCategoryNameAction>(mCategoryId, mValue);
}

void ChangeCategoryNameAction::redoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    CategoryStore & category = tournament.getCategory(mCategoryId);
    mOldValue = category.getName();
    category.setName(mValue);
    tournament.changeCategories({mCategoryId});
}

void ChangeCategoryNameAction::undoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    CategoryStore & category = tournament.getCategory(mCategoryId);
    category.setName(mOldValue);
    tournament.changeCategories({mCategoryId});
}

ChangeCategoryRulesetAction::ChangeCategoryRulesetAction(CategoryId categoryId, uint8_t ruleset)
    : mCategoryId(categoryId)
    , mRuleset(ruleset)
{}

std::unique_ptr<Action> ChangeCategoryRulesetAction::freshClone() const {
    return std::make_unique<ChangeCategoryRulesetAction>(mCategoryId, mRuleset);
}

void ChangeCategoryRulesetAction::redoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    CategoryStore & category = tournament.getCategory(mCategoryId);

    const auto &rulesets = Rulesets::getRulesets();
    if (mRuleset > rulesets.size())
        throw ActionExecutionException("Failed to redo ChangeCategoryRulesetAction. Invalid ruleset specified.");
    auto ruleset = rulesets[mRuleset]->clone();

    mOldRuleset = category.setRuleset(std::move(ruleset));
    mDrawAction = std::make_unique<DrawCategoryAction>(mCategoryId);
    mDrawAction->redo(tournament);

    tournament.changeCategories({mCategoryId});
}

void ChangeCategoryRulesetAction::undoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    CategoryStore & category = tournament.getCategory(mCategoryId);

    mOldRuleset = category.setRuleset(std::move(mOldRuleset));
    mDrawAction->undo(tournament);
    mDrawAction.reset();

    tournament.changeCategories({mCategoryId});
}

ChangeCategoryDrawSystemAction::ChangeCategoryDrawSystemAction (CategoryId categoryId, uint8_t drawSystem)
    : mCategoryId(categoryId)
    , mDrawSystem(drawSystem)
{}

std::unique_ptr<Action> ChangeCategoryDrawSystemAction::freshClone() const {
    return std::make_unique<ChangeCategoryDrawSystemAction>(mCategoryId, mDrawSystem);
}

void ChangeCategoryDrawSystemAction::redoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    CategoryStore & category = tournament.getCategory(mCategoryId);

    const auto &drawSystems = DrawSystems::getDrawSystems();
    if (mDrawSystem > drawSystems.size())
        throw ActionExecutionException("Failed to redo ChangeCategoryDrawSystemAction. Invalid drawSystem specified.");
    auto drawSystem = drawSystems[mDrawSystem]->clone();

    mOldDrawSystem = category.setDrawSystem(std::move(drawSystem));

    mDrawAction = std::make_unique<DrawCategoryAction>(mCategoryId);
    mDrawAction->redo(tournament);

    tournament.changeCategories({mCategoryId});
}

void ChangeCategoryDrawSystemAction::undoImpl(TournamentStore & tournament) {
    if (!tournament.containsCategory(mCategoryId))
        return;

    CategoryStore & category = tournament.getCategory(mCategoryId);

    mOldDrawSystem = category.setDrawSystem(std::move(mOldDrawSystem));

    mDrawAction->undo(tournament);
    mDrawAction.reset();

    tournament.changeCategories({mCategoryId});
}

