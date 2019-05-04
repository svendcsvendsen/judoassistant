#pragma once

#include "core/actions/action.hpp"

class TournamentStore;
class Ruleset;
class DrawCategoriesAction;

class ChangeCategoriesRulesetAction : public Action {
public:
    ChangeCategoriesRulesetAction() = default;
    ChangeCategoriesRulesetAction(std::vector<CategoryId> categoryIds, size_t ruleset);
    ChangeCategoriesRulesetAction(std::vector<CategoryId> categoryIds, size_t ruleset, unsigned int seed);
    void redoImpl(TournamentStore & tournament) override;
    void undoImpl(TournamentStore & tournament) override;

    std::unique_ptr<Action> freshClone() const override;
    std::string getDescription() const override;

    template<typename Archive>
    void serialize(Archive& ar, uint32_t const version) {
        ar(mCategoryIds, mRuleset, mSeed);
    }

private:
    std::vector<CategoryId> mCategoryIds;
    size_t mRuleset;
    unsigned int mSeed;

    // undo members
    std::vector<std::unique_ptr<Ruleset>> mOldRulesets;
    std::unique_ptr<DrawCategoriesAction> mDrawAction;
};

CEREAL_REGISTER_TYPE(ChangeCategoriesRulesetAction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Action, ChangeCategoriesRulesetAction)