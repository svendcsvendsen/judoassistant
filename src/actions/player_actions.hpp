#pragma once

#include <string>

#include "serialize.hpp"
#include "actions/action.hpp"
#include "actions/match_actions.hpp"
#include "actions/category_actions.hpp"
#include "stores/tournament_store.hpp"
#include "stores/player_store.hpp"

class CreatePlayerAction : public Action {
public:
    CreatePlayerAction(TournamentStore & tournament, const std::string & firstName, const std::string & lastName, std::optional<uint8_t> age, std::optional<PlayerRank> rank, const std::string &club, std::optional<float> weight, std::optional<PlayerCountry> country);
    void redoImpl(TournamentStore & tournament) override;
    void undoImpl(TournamentStore & tournament) override;

private:
    Id mId;
    std::string mFirstName;
    std::string mLastName;
    std::optional<uint8_t> mAge;
    std::optional<PlayerRank> mRank;
    std::string mClub;
    std::optional<float> mWeight;
    std::optional<PlayerCountry> mCountry;
};

class RemovePlayerFromCategoryAction;

class ErasePlayerAction : public Action {
public:
    ErasePlayerAction(TournamentStore & tournament, Id player);
    void redoImpl(TournamentStore & tournament) override;
    void undoImpl(TournamentStore & tournament) override;
private:
    Id mId;
    std::stack<RemovePlayerFromCategoryAction> mActions;
    std::unique_ptr<PlayerStore> mPlayer;
};
