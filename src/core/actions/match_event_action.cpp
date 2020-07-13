#include "core/actions/match_event_action.hpp"
#include "core/draw_systems/draw_system.hpp"
#include "core/rulesets/ruleset.hpp"
#include "core/stores/category_store.hpp"
#include "core/stores/tournament_store.hpp"
#include "core/stores/player_store.hpp"

MatchEventAction::MatchEventAction(CategoryId categoryId, MatchId matchId, std::chrono::milliseconds masterTime)
    : mCategoryId(categoryId)
    , mMatchId(matchId)
    , mMasterTime(masterTime)
{}

void MatchEventAction::save(const MatchStore &match, unsigned int eventsToSave ) {
    mDidSave = true;
    mPrevState = match.getState();
    mPrevEventSize = match.getEvents().size();
}

void MatchEventAction::recover(TournamentStore &tournament) {
    assert(mDidSave);

    while (!mDrawActions.empty()) {
        mDrawActions.top()->undo(tournament);
        mDrawActions.pop();
    }

    auto &category = tournament.getCategory(mCategoryId);
    auto &match = category.getMatch(mMatchId);
    auto updatedStatus = match.getStatus();
    auto prevStatus = mPrevState.status;

    match.setState(std::move(mPrevState));

    assert(match.getEvents().size() >= mPrevEventSize);
    while (match.getEvents().size() > mPrevEventSize)
        match.popEvent();

    // Recover lastFinishTime for players
    if (updatedStatus == MatchStatus::FINISHED) {
        if (match.getWhitePlayer()) {
            auto &whitePlayer = tournament.getPlayer(*match.getWhitePlayer());
            whitePlayer.setLastFinishTime(mPrevWhiteLastFinishTime);
        }

        if (match.getBluePlayer()) {
            auto &bluePlayer = tournament.getPlayer(*match.getBluePlayer());
            bluePlayer.setLastFinishTime(mPrevBlueLastFinishTime);
        }
    }

    // Update category and tatamis if matches went to/from finished or not_started
    if (updatedStatus != prevStatus && (prevStatus == MatchStatus::NOT_STARTED || prevStatus == MatchStatus::FINISHED || updatedStatus == MatchStatus::NOT_STARTED || updatedStatus == MatchStatus::FINISHED)) {
        auto &categoryStatus = category.getStatus(match.getType());

        if (updatedStatus == MatchStatus::NOT_STARTED) {
            assert(categoryStatus.notStartedMatches > 0);
            --(categoryStatus.notStartedMatches);
        }
        else if (updatedStatus == MatchStatus::PAUSED || updatedStatus == MatchStatus::UNPAUSED) {
            assert(categoryStatus.startedMatches > 0);
            --(categoryStatus.startedMatches);
        }
        else if (updatedStatus == MatchStatus::FINISHED) {
            assert(categoryStatus.finishedMatches > 0);
            --(categoryStatus.finishedMatches);
        }

        if (prevStatus == MatchStatus::NOT_STARTED)
            ++(categoryStatus.notStartedMatches);
        else if (prevStatus == MatchStatus::PAUSED || prevStatus == MatchStatus::UNPAUSED)
            ++(categoryStatus.startedMatches);
        else if (prevStatus == MatchStatus::FINISHED)
            ++(categoryStatus.finishedMatches);

        auto blockLocation = category.getLocation(match.getType());
        if (blockLocation) {
            auto &concurrentGroup = tournament.getTatamis().at(blockLocation->sequentialGroup.concurrentGroup);
            concurrentGroup.updateStatus(match);

            std::pair<CategoryId, MatchType> block{category.getId(), match.getType()};

            tournament.changeTatamis({*blockLocation}, {block});
        }
    }


    // Notify of match changed
    tournament.changeMatches(match.getCategory(), {match.getId()});

    // Notify results
    if (updatedStatus == MatchStatus::FINISHED || prevStatus == MatchStatus::FINISHED)
        tournament.resetCategoryResults({match.getCategory()});
}

bool MatchEventAction::shouldRecover() {
    return mDidSave;
}

void MatchEventAction::notify(TournamentStore &tournament, MatchStore &match) {
    auto &category = tournament.getCategory(match.getCategory());
    auto updatedStatus = match.getStatus();
    auto prevStatus = mPrevState.status;

    // Update lastFinishTime for players
    if (updatedStatus == MatchStatus::FINISHED) {
        if (match.getWhitePlayer()) {
            auto &whitePlayer = tournament.getPlayer(*match.getWhitePlayer());
            mPrevWhiteLastFinishTime = whitePlayer.getLastFinishTime();
            whitePlayer.setLastFinishTime(mMasterTime);
        }

        if (match.getBluePlayer()) {
            auto &bluePlayer = tournament.getPlayer(*match.getBluePlayer());
            mPrevBlueLastFinishTime = bluePlayer.getLastFinishTime();
            bluePlayer.setLastFinishTime(mMasterTime);
        }
    }

    // update category status
    auto & categoryStatus = category.getStatus(match.getType());
    if (updatedStatus != prevStatus && (prevStatus == MatchStatus::NOT_STARTED || prevStatus == MatchStatus::FINISHED || updatedStatus == MatchStatus::NOT_STARTED || updatedStatus == MatchStatus::FINISHED)) {
        if (prevStatus == MatchStatus::NOT_STARTED) {
            assert(categoryStatus.notStartedMatches > 0);
            --(categoryStatus.notStartedMatches);
        }
        else if (prevStatus == MatchStatus::PAUSED || prevStatus == MatchStatus::UNPAUSED) {
            assert(categoryStatus.startedMatches > 0);
            --(categoryStatus.startedMatches);
        }
        else if (prevStatus == MatchStatus::FINISHED) {
            assert(categoryStatus.finishedMatches > 0);
            --(categoryStatus.finishedMatches);
        }

        if (updatedStatus == MatchStatus::NOT_STARTED)
            ++(categoryStatus.notStartedMatches);
        else if (updatedStatus == MatchStatus::PAUSED || updatedStatus == MatchStatus::UNPAUSED)
            ++(categoryStatus.startedMatches);
        else if (updatedStatus == MatchStatus::FINISHED)
            ++(categoryStatus.finishedMatches);

        // Updates tatami groups
        auto blockLocation = category.getLocation(match.getType());
        if (blockLocation) {
            auto &concurrentGroup = tournament.getTatamis().at(blockLocation->sequentialGroup.concurrentGroup);
            concurrentGroup.updateStatus(match);

            std::pair<CategoryId, MatchType> block{category.getId(), match.getType()};
            tournament.changeTatamis({*blockLocation}, {block});
        }
    }

    // Notify draw system
    // Changes to draws can only occur if the match was finished or is finished
    if (prevStatus == MatchStatus::FINISHED || match.getStatus() == MatchStatus::FINISHED) {
        const auto &drawSystem = category.getDrawSystem();
        auto drawActions = drawSystem.updateCategory(tournament, category);
        for (std::unique_ptr<Action> &action : drawActions) {
            action->redo(tournament);
            mDrawActions.push(std::move(action));
        }

        tournament.resetCategoryResults({match.getCategory()});
    }

    // Notify of match changed
    tournament.changeMatches(match.getCategory(), {match.getId()});
}

