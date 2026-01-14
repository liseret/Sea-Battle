#include "clientmanager.h"
#include <QDebug>

ClientManager::ClientManager(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &ClientManager::onReadyRead);

    connect(socket, &QTcpSocket::connected, [this]() {
        qDebug() << "Connected to server";
    });

    connect(socket, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError error) {
        qDebug() << "Socket error:" << error << socket->errorString();
        emit errorOccurred(socket->errorString());
    });
}

void ClientManager::connectToServer(const QString &host, quint16 port, const QString &name) {
    socket->connectToHost(host, port);
    connect(socket, &QTcpSocket::connected, [this, name]() {
        sendCommand("USERNAME", name);
    });
}

void ClientManager::sendCommand(const QString &command, const QJsonValue &data) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Cannot send command: socket not connected";
        return;
    }

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

    qint64 bytesWritten = socket->write(block);
    if (bytesWritten == -1) {
        qDebug() << "Failed to write to socket";
    }
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
    qDebug() << "Received message:" << message;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) {
        qDebug() << "Invalid JSON received";
        return;
    }

    QJsonObject obj = doc.object();
    QString cmd = obj["command"].toString();
    QString data = obj["data"].toString();

    qDebug() << "Command:" << cmd << "Data:" << data;

    if (cmd == "CONNECTED") emit messageReceived("Connected to server");
    else if (cmd == "STATUS") emit statusChanged(data);
    else if (cmd == "MESSAGE") emit messageReceived(data);
    else if (cmd == "GAME_START") emit gameStarted(data);
    else if (cmd == "TURN_CHANGE") emit turnChanged(data);
    else if (cmd == "SHIP_SUNK") emit shipSunk(data);
    else if (cmd == "SHIPS_ACCEPTED") emit messageReceived("Ships placement accepted!");
    else if (cmd == "PLAYER_READY") emit messageReceived(data);
    else if (cmd == "PLAYER_JOINED") emit messageReceived("Player joined: " + data);
    else if (cmd == "PLAYER_LEFT") emit messageReceived("Player left: " + data);
    else if (cmd == "SHOT_RESULT") {
        QJsonObject res = QJsonDocument::fromJson(data.toUtf8()).object();
        emit shotResult(res["x"].toInt(), res["y"].toInt(), res["hit"].toBool(), res["sunk"].toBool());
    }
    else if (cmd == "ENEMY_SHOT") {
        QJsonObject res = QJsonDocument::fromJson(data.toUtf8()).object();
        emit enemyShot(res["x"].toInt(), res["y"].toInt(), res["hit"].toBool(), res["sunk"].toBool());
    }
    else if (cmd == "GAME_OVER") {
        QString winner;
        QString stats;

        if (data.contains("|")) {
            QStringList parts = data.split("|");
            for (const QString& part : parts) {
                if (part.startsWith("WINNER:")) {
                    winner = part.mid(7);
                } else if (part.startsWith("STATS:")) {
                    stats = part.mid(6);
                }
            }
        } else {
            winner = data;
        }

        emit gameOver(winner, stats);
        emit messageReceived("GAME OVER! Winner: " + winner);
        emit messageReceived(stats);
    }
    else if (cmd == "ERROR") emit errorOccurred(data);
    else {
        qDebug() << "Unknown command:" << cmd;
    }
}
