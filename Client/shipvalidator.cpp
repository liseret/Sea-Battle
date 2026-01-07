#include "shipvalidator.h"
#include <algorithm>

ShipValidator::Result ShipValidator::validateFullMap(const QSet<QPair<int, int>>& cells) {
    if (cells.size() != 20)
        return {false, QString("Need exactly 20 cells. Current: %1").arg(cells.size())};

    QVector<int> shipLengths;
    QSet<QPair<int, int>> visited;

    for (auto const& cell : cells) {
        if (visited.contains(cell)) continue;

        QVector<QPair<int, int>> currentShip;
        QVector<QPair<int, int>> queue = {cell};
        visited.insert(cell);

        int head = 0;
        while(head < queue.size()){
            QPair<int, int> curr = queue[head++];
            currentShip.append(curr);
            int dx[] = {0, 0, 1, -1, 1, 1, -1, -1};
            int dy[] = {1, -1, 0, 0, 1, -1, 1, -1};

            for(int i = 0; i < 8; ++i){
                QPair<int, int> neighbor = {curr.first + dx[i], curr.second + dy[i]};
                if(cells.contains(neighbor) && !visited.contains(neighbor)){
                    if (i >= 4) return {false, "Ships cannot touch diagonally!"};

                    visited.insert(neighbor);
                    queue.append(neighbor);
                }
            }
        }

        if (!isLinear(currentShip))
            return {false, "Ships must be straight lines!"};

        shipLengths.append(currentShip.size());
    }

    if (!checkShipCounts(shipLengths))
        return {false, "Invalid fleet! Required: 1x4, 2x3, 3x2, 4x1"};

    return {true, "OK"};
}

bool ShipValidator::isLinear(const QVector<QPair<int, int>>& ship) {
    if (ship.isEmpty()) return false;
    int minX = 10, maxX = 0, minY = 10, maxY = 0;
    for(auto const& p : ship) {
        minX = std::min(minX, p.first); maxX = std::max(maxX, p.first);
        minY = std::min(minY, p.second); maxY = std::max(maxY, p.second);
    }
    return (minX == maxX) || (minY == maxY);
}

bool ShipValidator::checkShipCounts(const QVector<int>& lengths) {
    if (lengths.size() != 10) return false;
    QVector<int> sorted = lengths;
    std::sort(sorted.begin(), sorted.end());
    QVector<int> required = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4};
    return sorted == required;
}
