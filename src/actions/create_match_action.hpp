#pragma once

#include <string>

#include "actions/action.hpp"
#include "stores/tournament_store.hpp"
#include "stores/category_store.hpp"
#include "stores/player_store.hpp"

class CreateMatchAction : public Action {
public:
    CreateMatchAction(TournamentStore * tournament, CategoryStore * category, std::optional<Id> whitePlayer, std::optional<Id> bluePlayer);
    virtual bool operator()(TournamentStore * store) const;
    virtual void getInverse(Action *ptr) const;

    Id getId() const;
private:
    Id mId;
    Id mCategory;
    std::optional<Id> mWhitePlayer;
    std::optional<Id> mBluePlayer;
};
