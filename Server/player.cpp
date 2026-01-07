#include "player.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
player::player()
    : socket(nullptr),
    shipsRemaining(0),
    shotsFired(0),
    shotsHit(0),
    ready(false)
{
}

player::player(QTcpSocket* socket)
    : socket(socket),
    shipsRemaining(0),
    shotsFired(0),
    shotsHit(0),
    ready(false)
{
}

QString player::getUsername() const {
    return username;
}

QTcpSocket* player::getSocket() const {
    return socket;
}

QSet<QPair<int, int>> player::getShips() const {
    return ships;
}

QSet<QPair<int, int>> player::getHits() const {
    return hits;
}

QSet<QPair<int, int>> player::getMisses() const {
    return misses;
}

QSet<QPair<int, int>> player::getReceivedHits() const {
    return receivedHits;
}

int player::getShipsRemaining() const {
    return shipsRemaining;
}

int player::getShotsFired() const {
    return shotsFired;
}

int player::getShotsHit() const {
    return shotsHit;
}

bool player::isReady() const {
    return ready;
}

float player::getAccuracy() const {
    if (shotsFired == 0) return 0.0f;
    return (shotsHit * 100.0f) / shotsFired;
}

void player::setUsername(const QString& username) {
    this->username = username;
}

void player::setSocket(QTcpSocket* socket) {
    this->socket = socket;
}

void player::setReady(bool ready) {
    this->ready = ready;
}

void player::addShip(int x, int y) {
    ships.insert(qMakePair(x, y));
}

void player::clearShips() {
    ships.clear();
    shipsRemaining = 0;
}

bool player::hasShipAt(int x, int y) const {
    return ships.contains(qMakePair(x, y));
}

void player::addHit(int x, int y) {
    hits.insert(qMakePair(x, y));
}

void player::addMiss(int x, int y) {
    misses.insert(qMakePair(x, y));
}

void player::addReceivedHit(int x, int y) {
    receivedHits.insert(qMakePair(x, y));
}

void player::incrementShotsFired() {
    shotsFired++;
}

void player::incrementShotsHit() {
    shotsHit++;
}

bool player::hasShotAt(int x, int y) const {
    QPair<int, int> coord = qMakePair(x, y);
    return hits.contains(coord) || misses.contains(coord);
}

bool player::hasReceivedHitAt(int x, int y) const {
    return receivedHits.contains(qMakePair(x, y));
}

void player::decrementShipsRemaining() {
    if (shipsRemaining > 0) {
        shipsRemaining--;
    }
}
void player::resetGameStats() {
    hits.clear();
    misses.clear();
    receivedHits.clear();
    shotsFired = 0;
    shotsHit = 0;
    shipsRemaining = ships.size();
    ready = false;
}

QJsonArray player::shipsToJson() const {
    QJsonArray shipsArray;
    for (const auto& ship : ships) {
        QJsonObject shipObj;
        shipObj["x"] = ship.first;
        shipObj["y"] = ship.second;
        shipsArray.append(shipObj);
    }
    return shipsArray;
}

bool player::setShipsFromJson(const QJsonArray& shipsArray) {
    clearShips();

    for (const auto& ship : shipsArray) {
        QJsonObject shipObj = ship.toObject();
        int x = shipObj["x"].toInt();
        int y = shipObj["y"].toInt();
        if (x < 0 || x >= 10 || y < 0 || y >= 10) {
            clearShips();
            return false;
        }

        addShip(x, y);
    }

    shipsRemaining = ships.size();
    return true;
}
