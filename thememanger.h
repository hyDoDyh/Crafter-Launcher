#ifndef THEMEMANGER_H
#define THEMEMANGER_H
#include <QPropertyAnimation>
class ThemeManger
{
public:
    ThemeManger();
    static QEasingCurve fromString(QString str);
};

#endif // THEMEMANGER_H
