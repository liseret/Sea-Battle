#ifndef PLAYER_H
#define PLAYER_H
#include <QTcpSocket>
#include <QSet>
#include <QPair>
class player
{
private:
    QString username;
    QTcpSocket* socket;
    QSet<QPair<int, int>> ships;
    QSet<QPair<int, int>> hits;
    QSet<QPair<int, int>> misses;
    QSet<QPair<int, int>> receivedHits;
    int shipsRemaining;
    int shotsFired;
    int shotsHit;
    bool ready;
public:
    player();
    player(QTcpSocket* socket);

    QString getUsername() const;
    QTcpSocket* getSocket() const;
    QSet<QPair<int, int>> getShips() const;
    QSet<QPair<int, int>> getHits() const;
    QSet<QPair<int, int>> getMisses() const;
    QSet<QPair<int, int>> getReceivedHits() const;
    int getShipsRemaining() const;
    int getShotsFired() const;
    int getShotsHit() const;
    bool isReady() const;
    float getAccuracy() const;

    void setUsername(const QString& username);
    void setSocket(QTcpSocket* socket);
    void setReady(bool ready);

    void addShip(int x, int y);
    void clearShips();
    bool hasShipAt(int x, int y) const;

    void addHit(int x, int y);
    void addMiss(int x, int y);
    void addReceivedHit(int x, int y);
    void incrementShotsFired();
    void incrementShotsHit();

    bool hasShotAt(int x, int y) const;
    bool hasReceivedHitAt(int x, int y) const;

    void decrementShipsRemaining();
    void resetGameStats();

    QJsonArray shipsToJson() const;
    bool setShipsFromJson(const QJsonArray& shipsArray);
};

#endif // PLAYER_H
