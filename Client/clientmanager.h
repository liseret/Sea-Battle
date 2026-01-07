#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H
#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>

class ClientManager : public QObject {
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = nullptr);
    void connectToServer(const QString &host, quint16 port, const QString &name);
    void sendCommand(const QString &command, const QJsonValue &data = QJsonValue());

signals:
    void statusChanged(const QString &status);
    void messageReceived(const QString &msg);
    void gameStarted(const QString &info);
    void turnChanged(const QString &currentPlayer);
    void shotResult(int x, int y, bool hit, bool sunk);
    void enemyShot(int x, int y, bool hit, bool sunk);
    void shipSunk(const QString &msg);
    void gameOver(const QString &winner, const QString &stats);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();

private:
    QTcpSocket *socket;
    quint16 nextBlockSize = 0;
    void processMessage(const QString &message);
};
#endif // CLIENTMANAGER_H
