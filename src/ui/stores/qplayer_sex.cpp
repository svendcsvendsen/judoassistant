#include "ui/stores/qplayer_sex.hpp"

std::vector<QString> QPlayerSex::strings() const {
    switch (mValue) {
        case MALE: return {tr("male"), tr("man"), tr("m"), tr("boy")};
        case FEMALE: return {tr("female"), tr("woman"), tr("w"), tr("f"), tr("girl")};
        default: return {};
    }
}

QPlayerSex QPlayerSex::fromHumanString(const QString &str) {
    for (int i = 0; i < static_cast<int>(SIZE); ++i) {
        QPlayerSex sex(i);

        const auto lower = str.toLower();
        for (const QString &repr : sex.strings()) {
            if (repr.toLower() == lower)
                return sex;
        }
    }

    throw std::invalid_argument(str.toStdString());
}

QString QPlayerSex::toHumanString() const {
    switch (mValue) {
        case MALE: return tr("male");
        case FEMALE: return tr("female");
        default: return "";
    }
}

