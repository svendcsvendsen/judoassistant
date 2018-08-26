#pragma once

#include "core.hpp"

class TournamentStore;

class Action {
public:
    virtual ~Action() {}

    virtual bool operator()(std::unique_ptr<TournamentStore> & tournament) const = 0;
    virtual std::unique_ptr<Action> getInverse() const = 0;
};

