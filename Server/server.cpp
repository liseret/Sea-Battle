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
    player* Player = players.value(client, nullptr);
    if (!Player) {
        return;
    }

    if (command == "USERNAME") {
        Player->setUsername(data);
        qDebug() << "Player set username:" << data;

        broadcast("PLAYER_JOINED", data);

        if (connectedPlayers == 2) {
            QStringList playerList;
            for (auto p : players.values()) {
                if (!p->getUsername().isEmpty()) {
                    playerList << p->getUsername();
                }
            }
            broadcast("PLAYERS_LIST", playerList.join(","));
        }

    }
    else if (command == "SHIPS_PLACED") {
        if (gameStatus != "PLACING_SHIPS") {
            sendToClient(client, "ERROR", "Cannot place ships now");
            return;
        }

        if (Player->isReady()) {
            sendToClient(client, "ERROR", "Ships already placed");
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonArray shipsArray = doc.array();

        if (!Player->setShipsFromJson(shipsArray)) {
            sendToClient(client, "ERROR", "Invalid ships data");
            return;
        }


        if (!isValidShipPlacement(Player->getShips())) {
            sendToClient(client, "ERROR", "Invalid ships placement");
            Player->clearShips();
            return;
        }

        Player->setReady(true);
        sendToClient(client, "SHIPS_ACCEPTED");
        broadcast("PLAYER_READY", Player->getUsername() + " placed ships");
        if (areAllShipsPlaced()) {
            startGame();
        }

    }
    else if (command == "SHOT") {
        if (gameStatus != "PLAYER1_TURN" && gameStatus != "PLAYER2_TURN") {
            sendToClient(client, "ERROR", "Not your turn or game not started");
            return;
        }

        // Проверяем, чей сейчас ход
        player* currentPlayer = getPlayerByUsername(currentPlayerUsername);
        if (!currentPlayer) {
            sendToClient(client, "ERROR", "Game error: current player not found");
            return;
        }

        // Если стреляет не тот игрок, чей ход
        if (Player != currentPlayer) {
            sendToClient(client, "ERROR", "Not your turn");
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonObject shotObj = doc.object();
        int x = shotObj["x"].toInt();
        int y = shotObj["y"].toInt();

        if (x < 0 || x >= 10 || y < 0 || y >= 10) {
            sendToClient(client, "ERROR", "Invalid coordinates");
            return;
        }

        if (Player->hasShotAt(x, y)) {
            sendToClient(client, "ERROR", "Already shot at this position");
            return;
        }

        Player->incrementShotsFired();

        player* Opponent = getOpponent(Player);
        if (!Opponent) {
            sendToClient(client, "ERROR", "Opponent not found");
            return;
        }

        if (Opponent->hasShipAt(x, y) && !Opponent->hasReceivedHitAt(x, y)) {
            Player->incrementShotsHit();
            Player->addHit(x, y);
            Opponent->addReceivedHit(x, y);

            bool shipSunk = true;
            // Проверяем все клетки этого корабля
            QSet<QPair<int, int>> shipCells;
            QList<QPair<int, int>> queue;
            queue.append(qMakePair(x, y));

            while (!queue.isEmpty()) {
                QPair<int, int> cell = queue.takeFirst();
                shipCells.insert(cell);

                // Проверяем соседние клетки (только по вертикали/горизонтали)
                QList<QPair<int, int>> directions = {
                    qMakePair(0, 1), qMakePair(0, -1),
                    qMakePair(1, 0), qMakePair(-1, 0)
                };

                for (const auto& dir : directions) {
                    int nx = cell.first + dir.first;
                    int ny = cell.second + dir.second;
                    QPair<int, int> neighbor = qMakePair(nx, ny);

                    if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
                        if (Opponent->hasShipAt(nx, ny) && !shipCells.contains(neighbor)) {
                            queue.append(neighbor);
                        }
                    }
                }
            }

            // Проверяем, все ли клетки корабля поражены
            for (const auto& cell : shipCells) {
                if (!Opponent->hasReceivedHitAt(cell.first, cell.second)) {
                    shipSunk = false;
                    break;
                }
            }

            QJsonObject result;
            result["x"] = x;
            result["y"] = y;
            result["hit"] = true;
            result["sunk"] = shipSunk;

            QJsonDocument resultDoc(result);
            sendToClient(client, "SHOT_RESULT", resultDoc.toJson());
            sendToClient(Opponent->getSocket(), "ENEMY_SHOT", resultDoc.toJson());

            if (shipSunk) {
                Opponent->decrementShipsRemaining();
                broadcast("SHIP_SUNK", QString("%1 sunk %2's ship!").arg(Player->getUsername()).arg(Opponent->getUsername()));

                if (Opponent->getShipsRemaining() == 0) {
                    endGame(Player);
                    return;
                }
            }

            broadcast("MESSAGE", QString("%1 hit at (%2, %3)").arg(Player->getUsername()).arg(x).arg(y));

            // При попадании ход остается у того же игрока
            // НЕ вызываем switchTurn()
        }
        else {
            Player->addMiss(x, y);

            QJsonObject result;
            result["x"] = x;
            result["y"] = y;
            result["hit"] = false;

            QJsonDocument resultDoc(result);
            sendToClient(client, "SHOT_RESULT", resultDoc.toJson());
            sendToClient(Opponent->getSocket(), "ENEMY_SHOT", resultDoc.toJson());

            // При промахе переключаем ход
            switchTurn();

            broadcast("MESSAGE", QString("%1 missed at (%2, %3)").arg(Player->getUsername()).arg(x).arg(y));
        }

    } else if (command == "READY") {
        if (gameStatus == "PLACING_SHIPS") {
            Player->setReady(true);
            broadcast("PLAYER_READY", Player->getUsername() + " is ready");

            if (areAllShipsPlaced()) {
                startGame();
            }
        }

    } else if (command == "RESTART") {
        if (gameStatus == "GAME_OVER") {
            for (auto p : players.values()) {
                p->resetGameStats();
                p->setReady(false);
            }

            gameStatus = "PLACING_SHIPS";
            currentPlayerUsername.clear();

            broadcast("GAME_RESTART");
            broadcast("STATUS", "PLACING_SHIPS");
            broadcast("MESSAGE", "Game restarted. Place your ships.");
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

    // Определяем, кто ходит первым (первый подключившийся)
    QList<player*> playerList = players.values();
    player* firstPlayer = playerList.first();
    currentPlayerUsername = firstPlayer->getUsername();
    gameStatus = "PLAYER1_TURN";

    broadcast("STATUS", "PLAYER1_TURN");
    broadcast("GAME_START", currentPlayerUsername + " goes first");
    broadcast("TURN_CHANGE", currentPlayerUsername);  // Важно: отправляем смену хода

    qDebug() << "Game started. First player:" << currentPlayerUsername;
}

void server::endGame(player* winner) {
    gameStatus = "GAME_OVER";
    QString stats;
    for (auto player : players.values()) {
        stats += getPlayerStats(player) + "\n\n";
    }

    broadcast("GAME_OVER", winner->getUsername() + " wins!");
    broadcast("STATS", stats);

    qDebug() << "Game over. Winner:" << winner->getUsername();
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
    // Находим следующего игрока
    QList<player*> playerList = players.values();
    if (playerList.size() < 2) return;

    player* currentPlayer = getPlayerByUsername(currentPlayerUsername);
    if (!currentPlayer) return;

    player* nextPlayer = (playerList[0] == currentPlayer) ? playerList[1] : playerList[0];

    currentPlayerUsername = nextPlayer->getUsername();

    // Обновляем статус игры
    if (gameStatus == "PLAYER1_TURN") {
        gameStatus = "PLAYER2_TURN";
    } else {
        gameStatus = "PLAYER1_TURN";
    }

    // Отправляем обновления всем клиентам
    broadcast("STATUS", gameStatus);
    broadcast("TURN_CHANGE", currentPlayerUsername);
    broadcast("MESSAGE", QString("%1's turn").arg(currentPlayerUsername));

    qDebug() << "Turn switched to:" << currentPlayerUsername;
}

QString server::getPlayerStats(player* player) {
    QString stats = QString("=== %1 ===\n").arg(player->getUsername());
    stats += QString("Shots fired: %1\n").arg(player->getShotsFired());
    stats += QString("Hits: %1\n").arg(player->getShotsHit());
    stats += QString("Misses: %1\n").arg(player->getShotsFired() - player->getShotsHit());
    stats += QString("Accuracy: %1%\n").arg(QString::number(player->getAccuracy(), 'f', 1));
    stats += QString("Ships remaining: %1/%2\n").arg(player->getShipsRemaining()).arg(player->getShips().size());
    return stats;
}
server::~server() {
    for (auto player : players.values()) {
        delete player;
    }
    players.clear();
}
