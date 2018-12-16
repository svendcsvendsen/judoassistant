#pragma once

#include <chrono>

#include <QMetaType>
#include <QString>

#include "stores/player_store.hpp"
#include "stores/match_store.hpp"
#include "stores/match_event.hpp"

struct MatchCardPlayerFields {
    QString firstName;
    QString lastName;
    QString club;
    std::optional<PlayerCountry> country;

    MatchStore::Score score;
};

class MatchCard {
public:
    MatchCard() = default;
    MatchCard(size_t tatami, const TournamentStore & tournament, const CategoryStore &category, const MatchStore &match);

    void paint(QPainter *painter, const QRect &rect, const QPalette &palette) const;
    QSize sizeHint();

private:
    static const int WIDTH_HINT = 250;
    static const int HEIGHT_HINT = 120;

    void paintPlayer(MatchCardPlayerFields playerFields, QPainter *painter, QFont &font, int insideWidth, int insideHeight, int columnTwoOffset, int columnThreeOffset, int padding) const;

    std::optional<MatchCardPlayerFields> mWhitePlayer;
    std::optional<MatchCardPlayerFields> mBluePlayer;
    size_t mTatami;
    QString mCategory;
    MatchStatus mStatus;
    bool mIsStopped;
    bool mBye;
    bool mGoldenScore;
    int time;
};

Q_DECLARE_METATYPE(MatchCard)
