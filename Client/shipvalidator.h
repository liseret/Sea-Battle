#ifndef SHIPVALIDATOR_H
#define SHIPVALIDATOR_H

#include <QSet>
#include <QPair>
#include <QVector>

class ShipValidator {
public:
    struct Result {
        bool success;
        QString message;
    };

    // Главный метод для проверки всей карты
    static Result validateFullMap(const QSet<QPair<int, int>>& cells);

private:
    // Вспомогательные методы (скрыты от других классов)
    static bool isLinear(const QVector<QPair<int, int>>& ship);
    static bool checkShipCounts(const QVector<int>& lengths);
};

#endif // SHIPVALIDATOR_H
