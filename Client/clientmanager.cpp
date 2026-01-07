#include "clientmanager.h"

ClientManager::ClientManager(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &ClientManager::onReadyRead);
}

void ClientManager::connectToServer(const QString &host, quint16 port, const QString &name) {
    socket->connectToHost(host, port);
    connect(socket, &QTcpSocket::connected, [this, name]() {
        sendCommand("USERNAME", name);
    });
}

void ClientManager::sendCommand(const QString &command, const QJsonValue &data) {
    if (socket->state() != QAbstractSocket::ConnectedState) return;

    QJsonObject obj;
    obj["command"] = command;

    if (data.isArray()) obj["data"] = QString(QJsonDocument(data.toArray()).toJson(QJsonDocument::Compact));
    else if (data.isObject()) obj["data"] = QString(QJsonDocument(data.toObject()).toJson(QJsonDocument::Compact));
    else obj["data"] = data.toString();

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    out << quint16(0) << QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    socket->write(block);
}

void ClientManager::onReadyRead() {
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_15);

    while (true) {
        if (nextBlockSize == 0) {
            if (socket->bytesAvailable() < sizeof(quint16)) break;
            in >> nextBlockSize;
        }
        if (socket->bytesAvailable() < nextBlockSize) break;

        QString str;
        in >> str;
        nextBlockSize = 0;
        processMessage(str);
    }
}

void ClientManager::processMessage(const QString &message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();
    QString cmd = obj["command"].toString();
    QString data = obj["data"].toString();

    if (cmd == "STATUS") emit statusChanged(data);
    else if (cmd == "MESSAGE") emit messageReceived(data);
    else if (cmd == "GAME_START") emit gameStarted(data);
    else if (cmd == "TURN_CHANGE") emit turnChanged(data);
    else if (cmd == "SHOT_RESULT") {
        QJsonObject res = QJsonDocument::fromJson(data.toUtf8()).object();
        emit shotResult(res["x"].toInt(), res["y"].toInt(), res["hit"].toBool(), res["sunk"].toBool());
    }
    else if (cmd == "ENEMY_SHOT") {
        QJsonObject res = QJsonDocument::fromJson(data.toUtf8()).object();
        emit enemyShot(res["x"].toInt(), res["y"].toInt(), res["hit"].toBool(), res["sunk"].toBool());
    }
    else if (cmd == "SHIP_SUNK") emit shipSunk(data);
    else if (cmd == "GAME_OVER") emit messageReceived("Победа: " + data);
    else if (cmd == "STATS") emit gameOver("", data);
    else if (cmd == "ERROR") emit errorOccurred(data);
}
