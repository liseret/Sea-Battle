#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include<QTcpSocket>
#include<QVector>
#include<QTime>
#include <QMap>
#include <QSet>
#include "player.h"

class server: public QTcpServer
{
    Q_OBJECT
private:
    QMap<QTcpSocket*, player*> players;
    QByteArray data;
    qint16 DataSize;

    QString gameStatus;
    QString currentPlayerUsername;
    int connectedPlayers;

    void sendToClient(QTcpSocket* socket, const QString& command, const QString& data = "");
    void broadcast(const QString& command, const QString& data = "");
    void processMessage(QTcpSocket* client, const QString& message);
    void handleGameCommand(QTcpSocket* client, const QString& command, const QString& data);

    void checkGameOver();
    QString getPlayerStats(player* player);
    void switchTurn();
    bool isValidShipPlacement(const QSet<QPair<int, int>>& ships);
    bool areAllShipsPlaced();
    void startGame();
    void endGame(player* winner);
    player* getPlayerByUsername(const QString& username);
    player* getOpponent(player* player);

public:
    server();
    ~server();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

public slots:
    void SlotReadyRead();
    void slotDisconnected();

};

#endif // SERVER_H
