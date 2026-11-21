#include "thememanger.h"
#include <QMap>
ThemeManger::ThemeManger() {}

QEasingCurve  ThemeManger::fromString(QString str){
    QMap<QString, QEasingCurve::Type> easingMap = {
        {"Linear", QEasingCurve::Linear},
        {"InQuad", QEasingCurve::InQuad},
        {"OutQuad", QEasingCurve::OutQuad},
        {"InOutQuad", QEasingCurve::InOutQuad},
        {"OutCubic", QEasingCurve::OutCubic},
        {"OutBack", QEasingCurve::OutBack},
        {"OutBounce", QEasingCurve::OutBounce},
        {"InElastic", QEasingCurve::InElastic},
        {"OutElastic", QEasingCurve::OutElastic}
    };
    return easingMap.value(str, QEasingCurve::Linear);
}

