#include "server.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

server::server()
    : DataSize(0),
    gameStatus("WAITING"),
    connectedPlayers(0)
{
    if (this->listen(QHostAddress::Any, 777)) {
        qDebug() << "start";
    }
}


void server::incomingConnection(qintptr SoketDescription){
    if (connectedPlayers >= 2) {
        qDebug() << "Maximum players reached. Rejecting connection.";
        QTcpSocket* tempSocket = new QTcpSocket(this);
        tempSocket->setSocketDescriptor(SoketDescription);
        sendToClient(tempSocket, "ERROR", "Server is full");
        tempSocket->disconnectFromHost();
        tempSocket->deleteLater();
        return;
    }

    QTcpSocket* newClient = new QTcpSocket(this);
    newClient->setSocketDescriptor(SoketDescription);

    player* Player = new player(newClient);
    players[newClient] = Player;
    connectedPlayers++;

    connect(newClient, &QTcpSocket::readyRead, this, &server::SlotReadyRead);
    connect(newClient, &QTcpSocket::disconnected, this, &server::slotDisconnected);

    sendToClient(newClient, "CONNECTED");
    sendToClient(newClient, "STATUS", "WAITING");

    qDebug() << "Client connected. Total players:" << connectedPlayers;

    if (connectedPlayers == 2) {
        gameStatus = "PLACING_SHIPS";
        broadcast("STATUS", "PLACING_SHIPS");
        broadcast("MESSAGE", "Both players connected! Place your ships.");
    }
}

void server::SlotReadyRead(){
    QTcpSocket* CurrClient = qobject_cast<QTcpSocket*>(sender());

    if (!CurrClient) {
        return;
    }
    QDataStream in(CurrClient);
    in.setVersion(QDataStream::Qt_5_15);

    if (in.status() == QDataStream::Ok){
        while (true){
            if (DataSize == 0){
                if(CurrClient->bytesAvailable() < 2){
                    break;
                }
                in >> DataSize;
            }
            if (CurrClient->bytesAvailable() < DataSize){
                break;
            }

            QString str;
            in >>str;
            DataSize = 0;
            if (!str.isEmpty()) {
                processMessage(CurrClient,str);
            }
        }
    }

}

void server::processMessage(QTcpSocket* client, const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) return;

    QJsonObject obj = doc.object();
    QString command = obj["command"].toString();
    QString data = obj["data"].toString();

    handleGameCommand(client, command, data);
}

void server::handleGameCommand(QTcpSocket* client, const QString& command, const QString& data) {
    // player* Player = players.value(client, nullptr);
    // if (!Player) {
    //     return;
    // }

    // if (command == "USERNAME") {
    //     Player->setUsername(data);
    //     qDebug() << "Player set username:" << data;

    //     broadcast("PLAYER_JOINED", data);

    //     if (connectedPlayers == 2) {
    //         QStringList playerList;
    //         for (auto p : players.values()) {
    //             if (!p->getUsername().isEmpty()) {
    //                 playerList << p->getUsername();
    //             }
    //         }
    //         broadcast("PLAYERS_LIST", playerList.join(","));
    //     }

    // }
    // else if (command == "SHIPS_PLACED") {
    //     if (gameStatus != "PLACING_SHIPS") {
    //         sendToClient(client, "ERROR", "Cannot place ships now");
    //         return;
    //     }

    //     if (Player->isReady()) {
    //         sendToClient(client, "ERROR", "Ships already placed");
    //         return;
    //     }

    //     QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    //     QJsonArray shipsArray = doc.array();

    //     if (!Player->setShipsFromJson(shipsArray)) {
    //         sendToClient(client, "ERROR", "Invalid ships data");
    //         return;
    //     }


    //     if (!isValidShipPlacement(Player->getShips())) {
    //         sendToClient(client, "ERROR", "Invalid ships placement");
    //         Player->clearShips();
    //         return;
    //     }

    //     Player->setReady(true);
    //     sendToClient(client, "SHIPS_ACCEPTED");
    //     broadcast("PLAYER_READY", Player->getUsername() + " placed ships");
    //     if (areAllShipsPlaced()) {
    //         startGame();
    //     }

    // }
    // else if (command == "SHOT") {
    //     if (gameStatus != "PLAYER1_TURN" && gameStatus != "PLAYER2_TURN") {
    //         sendToClient(client, "ERROR", "Not your turn or game not started");
    //         return;
    //     }
    //     player* currentPlayer = getPlayerByUsername(currentPlayerUsername);
    //     if (!currentPlayer) {
    //         sendToClient(client, "ERROR", "Game error: current player not found");
    //         return;
    //     }
    //     if (Player != currentPlayer) {
    //         sendToClient(client, "ERROR", "Not your turn");
    //         return;
    //     }

    //     QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    //     QJsonObject shotObj = doc.object();
    //     int x = shotObj["x"].toInt();
    //     int y = shotObj["y"].toInt();

    //     if (x < 0 || x >= 10 || y < 0 || y >= 10) {
    //         sendToClient(client, "ERROR", "Invalid coordinates");
    //         return;
    //     }

    //     if (Player->hasShotAt(x, y)) {
    //         sendToClient(client, "ERROR", "Already shot at this position");
    //         return;
    //     }

    //     Player->incrementShotsFired();

    //     player* Opponent = getOpponent(Player);
    //     if (!Opponent) {
    //         sendToClient(client, "ERROR", "Opponent not found");
    //         return;
    //     }

    //     if (Opponent->hasShipAt(x, y) && !Opponent->hasReceivedHitAt(x, y)) {
    //         Player->incrementShotsHit();
    //         Player->addHit(x, y);
    //         Opponent->addReceivedHit(x, y);

    //         bool shipSunk = true;
    //         QSet<QPair<int, int>> shipCells;
    //         QList<QPair<int, int>> queue;
    //         queue.append(qMakePair(x, y));

    //         while (!queue.isEmpty()) {
    //             QPair<int, int> cell = queue.takeFirst();
    //             shipCells.insert(cell);
    //             QList<QPair<int, int>> directions = {
    //                 qMakePair(0, 1), qMakePair(0, -1),
    //                 qMakePair(1, 0), qMakePair(-1, 0)
    //             };

    //             for (const auto& dir : directions) {
    //                 int nx = cell.first + dir.first;
    //                 int ny = cell.second + dir.second;
    //                 QPair<int, int> neighbor = qMakePair(nx, ny);

    //                 if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
    //                     if (Opponent->hasShipAt(nx, ny) && !shipCells.contains(neighbor)) {
    //                         queue.append(neighbor);
    //                     }
    //                 }
    //             }
    //         }
    //         for (const auto& cell : shipCells) {
    //             if (!Opponent->hasReceivedHitAt(cell.first, cell.second)) {
    //                 shipSunk = false;
    //                 break;
    //             }
    //         }

    //         QJsonObject result;
    //         result["x"] = x;
    //         result["y"] = y;
    //         result["hit"] = true;
    //         result["sunk"] = shipSunk;

    //         QJsonDocument resultDoc(result);
    //         sendToClient(client, "SHOT_RESULT", resultDoc.toJson());
    //         sendToClient(Opponent->getSocket(), "ENEMY_SHOT", resultDoc.toJson());

    //         if (shipSunk) {
    //             Opponent->decrementShipsRemaining();
    //             broadcast("SHIP_SUNK", QString("%1 потопил корабль игрока %2!").arg(Player->getUsername()).arg(Opponent->getUsername()));
    //             if (Opponent->getShipsRemaining() <= 0) {
    //                 endGame(Player);
    //                 return;
    //             }
    //         }

    //         broadcast("MESSAGE", QString("%1 hit at (%2, %3)").arg(Player->getUsername()).arg(x).arg(y));
    //     }
    //     else {
    //         Player->addMiss(x, y);

    //         QJsonObject result;
    //         result["x"] = x;
    //         result["y"] = y;
    //         result["hit"] = false;

    //         QJsonDocument resultDoc(result);
    //         sendToClient(client, "SHOT_RESULT", resultDoc.toJson());
    //         sendToClient(Opponent->getSocket(), "ENEMY_SHOT", resultDoc.toJson());
    //         switchTurn();

    //         broadcast("MESSAGE", QString("%1 missed at (%2, %3)").arg(Player->getUsername()).arg(x).arg(y));
    //     }

    // } else if (command == "READY") {
    //     if (gameStatus == "PLACING_SHIPS") {
    //         Player->setReady(true);
    //         broadcast("PLAYER_READY", Player->getUsername() + " is ready");

    //         if (areAllShipsPlaced()) {
    //             startGame();
    //         }
    //     }

    // } else if (command == "RESTART") {
    //     if (gameStatus == "GAME_OVER") {
    //         for (auto p : players.values()) {
    //             p->resetGameStats();
    //             p->setReady(false);
    //         }

    //         gameStatus = "PLACING_SHIPS";
    //         currentPlayerUsername.clear();

    //         broadcast("GAME_RESTART");
    //         broadcast("STATUS", "PLACING_SHIPS");
    //         broadcast("MESSAGE", "Game restarted. Place your ships.");
    //     }
    // }
    player* Player = players.value(client, nullptr);
    if (!Player) return;
    if (command == "USERNAME") {
        Player->setUsername(data);
        qDebug() << "Player set username:" << data;
        broadcast("PLAYER_JOINED", data);

        if (connectedPlayers == 2) {
            QStringList playerList;
            for (auto p : players.values()) {
                if (!p->getUsername().isEmpty()) playerList << p->getUsername();
            }
            broadcast("PLAYERS_LIST", playerList.join(","));
        }
    }
    else if (command == "SHIPS_PLACED") {
        if (gameStatus != "PLACING_SHIPS") {
            sendToClient(client, "ERROR", "Сейчас нельзя расставлять корабли");
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonArray shipsArray = doc.array();

        if (!Player->setShipsFromJson(shipsArray)) {
            sendToClient(client, "ERROR", "Неверные данные кораблей");
            return;
        }

        if (!isValidShipPlacement(Player->getShips())) {
            sendToClient(client, "ERROR", "Неправильная расстановка");
            Player->clearShips();
            return;
        }

        Player->setReady(true);
        sendToClient(client, "SHIPS_ACCEPTED");
        broadcast("PLAYER_READY", Player->getUsername() + " готов к бою!");

        if (areAllShipsPlaced()) {
            startGame();
        }
    }
    else if (command == "SHOT") {
        if (gameStatus != "PLAYER1_TURN" && gameStatus != "PLAYER2_TURN") {
            sendToClient(client, "ERROR", "Игра еще не началась или уже закончена");
            return;
        }

        if (Player->getUsername() != currentPlayerUsername) {
            sendToClient(client, "ERROR", "Сейчас не ваш ход!");
            return;
        }

        QJsonObject shotObj = QJsonDocument::fromJson(data.toUtf8()).object();
        int x = shotObj["x"].toInt();
        int y = shotObj["y"].toInt();

        if (Player->hasShotAt(x, y)) {
            sendToClient(client, "ERROR", "Вы уже стреляли в эту клетку");
            return;
        }

        Player->incrementShotsFired();
        player* Opponent = getOpponent(Player);
        if (Opponent->hasShipAt(x, y)) {
            Player->incrementShotsHit();
            Player->addHit(x, y);
            Opponent->addReceivedHit(x, y);
            bool shipSunk = true;
            QSet<QPair<int, int>> shipCells;
            QList<QPair<int, int>> queue = {qMakePair(x, y)};
            QSet<QPair<int, int>> visited;
            visited.insert(qMakePair(x, y));

            while (!queue.isEmpty()) {
                QPair<int, int> curr = queue.takeFirst();
                shipCells.insert(curr);
                int dx[] = {0, 0, 1, -1};
                int dy[] = {1, -1, 0, 0};
                for (int i = 0; i < 4; ++i) {
                    int nx = curr.first + dx[i], ny = curr.second + dy[i];
                    QPair<int, int> neighbor = qMakePair(nx, ny);
                    if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10 &&
                        Opponent->hasShipAt(nx, ny) && !visited.contains(neighbor)) {
                        visited.insert(neighbor);
                        queue.append(neighbor);
                    }
                }
            }
            for (const auto& cell : shipCells) {
                if (!Opponent->hasReceivedHitAt(cell.first, cell.second)) {
                    shipSunk = false;
                    break;
                }
            }

            QJsonObject result;
            result["x"] = x; result["y"] = y;
            result["hit"] = true; result["sunk"] = shipSunk;

            sendToClient(client, "SHOT_RESULT", QJsonDocument(result).toJson());
            sendToClient(Opponent->getSocket(), "ENEMY_SHOT", QJsonDocument(result).toJson());

            if (shipSunk) {
                Opponent->decrementShipsRemaining();
                broadcast("SHIP_SUNK", QString("Корабль игрока %1 потоплен!").arg(Opponent->getUsername()));
            }
            if (Opponent->getReceivedHits().size() >= 20) {
                endGame(Player);
                return;
            }
        }
        else {
            Player->addMiss(x, y);
            QJsonObject result;
            result["x"] = x; result["y"] = y; result["hit"] = false;

            sendToClient(client, "SHOT_RESULT", QJsonDocument(result).toJson());
            sendToClient(Opponent->getSocket(), "ENEMY_SHOT", QJsonDocument(result).toJson());

            switchTurn();
        }
    }
    else if (command == "RESTART") {
        if (gameStatus == "GAME_OVER") {
            for (auto p : players.values()) {
                p->resetGameStats();
                p->setReady(false);
            }
            gameStatus = "PLACING_SHIPS";
            currentPlayerUsername.clear();
            broadcast("GAME_RESTART");
            broadcast("STATUS", "PLACING_SHIPS");
        }
    }
}

void server::sendToClient(QTcpSocket* socket, const QString& command, const QString& data) {
    if (!socket || socket->state() != QTcpSocket::ConnectedState) return;

    QJsonObject message;
    message["command"] = command;
    if (!data.isEmpty()) {
        message["data"] = data;
    }

    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << quint16(0) << jsonString;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    socket->write(block);
}

void server::broadcast(const QString& command, const QString& data) {
    for (QTcpSocket* socket : players.keys()) {
        sendToClient(socket, command, data);
    }
}

void server::slotDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    if (players.contains(client)) {
        player* Player = players.value(client);
        QString username = Player->getUsername();

        players.remove(client);
        delete Player;
        connectedPlayers--;

        qDebug() << "Player disconnected:" << username;
        broadcast("PLAYER_LEFT", username);

        gameStatus = "WAITING";
        currentPlayerUsername.clear();

        if (connectedPlayers == 1) {
            broadcast("STATUS", "WAITING");
            broadcast("MESSAGE", "Waiting for another player...");
        } else if (connectedPlayers == 0) {
            qDebug() << "All players disconnected";
        }
    }

    client->deleteLater();
}

bool server::isValidShipPlacement(const QSet<QPair<int, int>>& ships) {
    if (ships.size() != 20) {
        qDebug() << "Invalid number of ships:" << ships.size();
        return false;
    }
    for (const auto& coord : ships) {
        if (coord.first < 0 || coord.first >= 10 || coord.second < 0 || coord.second >= 10) {
            return false;
        }
    }
    return true;
}

bool server::areAllShipsPlaced() {
    for (auto player : players.values()) {
        if (!player->isReady() || player->getShips().isEmpty()) {
            return false;
        }
    }
    return true;
}

void server::startGame() {
    if (players.size() < 2) return;
    QList<player*> playerList = players.values();
    player* firstPlayer = playerList.first();
    currentPlayerUsername = firstPlayer->getUsername();
    gameStatus = "PLAYER1_TURN";

    broadcast("STATUS", "PLAYER1_TURN");
    broadcast("GAME_START", currentPlayerUsername + " goes first");
    broadcast("TURN_CHANGE", currentPlayerUsername);
    qDebug() << "Game started. First player:" << currentPlayerUsername;
}

void server::endGame(player* winner) {
    // gameStatus = "GAME_OVER";
    // player* p1 = players.values().at(0);
    // player* p2 = (players.size() > 1) ? players.values().at(1) : nullptr;

    // QString finalStats = QString("WINNER:%1|STATS:").arg(winner->getUsername());
    // finalStats += getPlayerStats(p1);
    // if (p2) {
    //     finalStats += "\n" + getPlayerStats(p2);
    // }
    // broadcast("STATUS", "GAME_OVER");
    // broadcast("GAME_OVER", finalStats);

    gameStatus = "GAME_OVER";
    QString statsData;
    for (player* p : players.values()) {
        int total = p->getShotsFired();
        int hits = p->getShotsHit();
        double acc = (total > 0) ? (static_cast<double>(hits)/total * 100.0) : 0.0;

        statsData += QString("<b>Player %1:</b><br>").arg(p->getUsername());
        statsData += QString("Shots were fired: %1<br>").arg(total);
        statsData += QString("Hits: %1<br>").arg(hits);
        statsData += QString("Misses: %1<br>").arg(total - hits);
        statsData += QString("Accuracy: %1%<br><br>").arg(QString::number(acc, 'f', 1));
    }

    QString finalMessage = QString("WINNER:%1|STATS:%2").arg(winner->getUsername()).arg(statsData);

    broadcast("STATUS", "GAME_OVER");
    broadcast("GAME_OVER", finalMessage);
}

player* server::getPlayerByUsername(const QString& username) {
    for (auto player : players.values()) {
        if (player->getUsername() == username) {
            return player;
        }
    }
    return nullptr;
}

player* server::getOpponent(player* player) {
    for (auto p : players.values()) {
        if (p != player) {
            return p;
        }
    }
    return nullptr;
}

void server::switchTurn() {
    QList<player*> playerList = players.values();
    if (playerList.size() < 2) return;

    player* currentPlayer = getPlayerByUsername(currentPlayerUsername);
    if (!currentPlayer) return;

    player* nextPlayer = (playerList[0] == currentPlayer) ? playerList[1] : playerList[0];

    currentPlayerUsername = nextPlayer->getUsername();
    if (gameStatus == "PLAYER1_TURN") {
        gameStatus = "PLAYER2_TURN";
    } else {
        gameStatus = "PLAYER1_TURN";
    }
    broadcast("STATUS", gameStatus);
    broadcast("TURN_CHANGE", currentPlayerUsername);
    broadcast("MESSAGE", QString("%1's turn").arg(currentPlayerUsername));

    qDebug() << "Turn switched to:" << currentPlayerUsername;
}

QString server::getPlayerStats(player* p) {
    // player* opponent = getOpponent(p);

    // int shots = p->getShotsFired();
    // int hits = p->getShotsHit();
    // int misses = shots - hits;
    // float accuracy = (shots > 0) ? (static_cast<float>(hits) / shots * 100.0f) : 0.0f;

    // QString stats = QString("Игрок: %1\n").arg(p->getUsername());
    // stats += QString("Выстрелов: %1\n").arg(shots);
    // stats += QString("Попаданий: %1\n").arg(hits);
    // stats += QString("Промахов: %1\n").arg(misses);
    // stats += QString("Точность: %1%\n").arg(QString::number(accuracy, 'f', 1));

    // // Считаем сколько кораблей оппонента имеют статус "все клетки подбиты"
    // int sunkCount = 0;

    // return stats;
    int shots = p->getShotsFired();
    int hits = p->getShotsHit();
    int misses = shots - hits;
    double accuracy = (shots > 0) ? (static_cast<double>(hits) / shots * 100.0) : 0.0;
    player* opponent = getOpponent(p);
    int sunkShips = 10 - opponent->getShipsRemaining();

    QString stats = QString("Игрок: %1\n").arg(p->getUsername());
    stats += QString("• Выстрелов: %1\n").arg(shots);
    stats += QString("• Попаданий: %1\n").arg(hits);
    stats += QString("• Промахов: %1\n").arg(misses);
    stats += QString("• Точность: %1%\n").arg(QString::number(accuracy, 'f', 1));
    stats += QString("• Потоплено кораблей: %1\n").arg(sunkShips);
    return stats;

}
server::~server() {
    for (auto player : players.values()) {
        delete player;
    }
    players.clear();
}
void server::checkGameOver() {
    for (player* p : players.values()) {
        if (p->getReceivedHits().size() == 20) {
            player* winner = getOpponent(p);
            endGame(winner);
            return;
        }
    }
}
