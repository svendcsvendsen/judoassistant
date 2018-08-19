#pragma once

#include "src/draw_strategies/draw_strategy.hpp"

class PoolDrawStrategy : public DrawStrategy {
public:
    PoolDrawStrategy();

    virtual void initCategory(const std::vector<Id> &players, std::unique_ptr<TournamentStore> & tournament, std::unique_ptr<CategoryStore> & category) = 0;
    virtual void updateCategory(std::unique_ptr<TournamentStore> & tournament, std::unique_ptr<CategoryStore> & category) = 0;
    virtual bool isFinished(std::unique_ptr<TournamentStore> & tournament, std::unique_ptr<CategoryStore> & category) = 0;
    virtual Id get_rank(size_t rank) = 0;
private:
    std::vector<Id> mMatchIds;
};
