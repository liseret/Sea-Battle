#include "mainwindow.h"
#include <QJsonArray>
#include <QJsonObject>
#include "shipvalidator.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    this->setWindowTitle("Sea Battle");
    QWidget *central = new QWidget;
    setCentralWidget(central);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);

    QVBoxLayout *uiLayout = new QVBoxLayout();
    nameEdit = new QLineEdit("Player");
    ipEdit = new QLineEdit("127.0.0.1");
    btnConnect = new QPushButton("Connect");
    btnReady = new QPushButton("Ready");
    btnReady->setEnabled(false);
    log = new QTextEdit();
    log->setReadOnly(true);
    statusLabel = new QLabel("Status: waiting");

    uiLayout->addWidget(new QLabel("Name:")); uiLayout->addWidget(nameEdit);
    uiLayout->addWidget(new QLabel("IP:")); uiLayout->addWidget(ipEdit);
    uiLayout->addWidget(btnConnect);
    uiLayout->addWidget(btnReady);
    uiLayout->addWidget(statusLabel);
    uiLayout->addWidget(log);

    myField = new GameField();
    enemyField = new GameField();
    myField->setInteractive(false);
    enemyField->setInteractive(false);

    mainLayout->addLayout(uiLayout);
    mainLayout->addWidget(myField);
    mainLayout->addWidget(enemyField);

    client = new ClientManager(this);
    shotsMade.clear();
    gameFinished = false;
    restartRequested = false;

    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(btnReady, &QPushButton::clicked, this, &MainWindow::onReady);
    connect(enemyField, &GameField::cellClicked, this, &MainWindow::onShot);
    connect(client, &ClientManager::statusChanged, this, &MainWindow::onStatusChanged);
    connect(client, &ClientManager::messageReceived, log, &QTextEdit::append);
    connect(client, &ClientManager::gameStarted, this, &MainWindow::onGameStarted);
    connect(client, &ClientManager::turnChanged, this, &MainWindow::onTurnChanged);
    connect(client, &ClientManager::shotResult, this, &MainWindow::onShotResult);
    connect(client, &ClientManager::enemyShot, this, &MainWindow::onEnemyShot);
    connect(client, &ClientManager::gameOver, this, &MainWindow::onGameOver);
    connect(client, &ClientManager::errorOccurred, this, &MainWindow::onError);
    connect(client, &ClientManager::errorOccurred, this, &MainWindow::onError);
}

void MainWindow::onStatusChanged(const QString &status) {
    statusLabel->setText("Status: " + status);
    qDebug() << "Status changed to:" << status;

    if (status == "WAITING") {
        myField->setInteractive(false);
        enemyField->setInteractive(false);
        btnReady->setEnabled(false);
        shotsMade.clear();
        myTurn = false;
        log->append("Waiting for another player...");
    }
    else if (status == "PLACING_SHIPS") {
        myField->setInteractive(true);
        enemyField->setInteractive(false);
        btnReady->setEnabled(true);
        shotsMade.clear();
        myTurn = false;
        gameFinished = false;
        log->append("Place your ships: 1x4, 2x3, 3x2, 4x1");
    }
    else if (status == "PLAYER1_TURN" || status == "PLAYER2_TURN") {
    }
    else if (status == "GAME_OVER") {
        myField->setInteractive(false);
        enemyField->setInteractive(false);
        btnReady->setEnabled(false);
        myTurn = false;
    }
    else {
        myField->setInteractive(false);
        enemyField->setInteractive(false);
    }
}

void MainWindow::onTurnChanged(const QString &currentPlayer) {
    qDebug() << "Turn changed to:" << currentPlayer << "My name:" << nameEdit->text();

    bool isMyTurn = (currentPlayer == nameEdit->text());
    myTurn = isMyTurn;

    if (isMyTurn) {
        log->append("<b>=== YOUR TURN ===</b>");
        enemyField->setInteractive(true);
    }
    else {
        log->append("=== OPPONENT TURN ===");
        enemyField->setInteractive(false);
    }
}

void MainWindow::onShotResult(int x, int y, bool hit, bool sunk) {
    qDebug() << "Shot result:" << x << y << "hit:" << hit << "sunk:" << sunk;
    shotsMade.insert(qMakePair(x, y));

    if (hit) {
        enemyField->setCell(x, y, GameField::Hit);
        if (sunk) {
            log->append(QString("You sunk a ship at (%1, %2)!").arg(x).arg(y));
        } else {
            log->append(QString("Hit at (%1, %2)!").arg(x).arg(y));
        }
        enemyField->setInteractive(true);
    }
    else {
        enemyField->setCell(x, y, GameField::Miss);
        log->append(QString("Miss at (%1, %2).").arg(x).arg(y));
        enemyField->setInteractive(false);
    }
}

void MainWindow::onEnemyShot(int x, int y, bool hit, bool sunk) {
    qDebug() << "Enemy shot:" << x << y << "hit:" << hit;

    if (hit) {
        myField->setCell(x, y, GameField::Hit);
        if (sunk) {
            log->append(QString("Your ship was sunk at (%1, %2)!").arg(x).arg(y));
        }
        else {
            log->append(QString("Your ship was hit at (%1, %2)!").arg(x).arg(y));
        }
    }
    else {
        myField->setCell(x, y, GameField::Miss);
        log->append(QString("Enemy missed at (%1, %2).").arg(x).arg(y));
    }
}

void MainWindow::onGameStarted(const QString &info) {
    log->append("Game started! " + info);
    myField->setInteractive(false);
    enemyField->setInteractive(false);
    shotsMade.clear();
    gameFinished = false;
}

void MainWindow::onGameOver(const QString &winner, const QString &stats) {
    if (gameFinished) return;
    gameFinished = true;

    myTurn = false;
    enemyField->setInteractive(false);
    myField->setInteractive(false);
    shotsMade.clear();

    statusLabel->setText("===GAME OVER===");
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Sea Battle");
    msgBox.setIcon(QMessageBox::Information);

    QString winnerText;
    if (winner == nameEdit->text()) {
        winnerText = "<h2 style='color: green;'> VICTORY! You won! </h2>";
    } else {
        winnerText = QString("<h2 style='color: red;'> DEFEAT! Winner: %1 </h2>").arg(winner);
    }

    msgBox.setText(winnerText);

    QString formattedStats = "<div style='font-family: monospace; margin: 10px;'>";
    QString statsCopy = stats;
    formattedStats += statsCopy.replace("\n", "<br>");
    formattedStats += "</div>";

    msgBox.setInformativeText(formattedStats);

    QPushButton *playAgainBtn = msgBox.addButton("Play Again", QMessageBox::AcceptRole);
    QPushButton *quitBtn = msgBox.addButton("Quit", QMessageBox::RejectRole);
    playAgainBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    quitBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px; }");

    msgBox.exec();

    if (msgBox.clickedButton() == playAgainBtn) {
        log->append("Requesting game restart...");

        client->sendCommand("RESTART");
        QTimer::singleShot(1000, this, &MainWindow::restartGame);

    } else if (msgBox.clickedButton() == quitBtn) {
        log->append("Quitting game...");
        QTimer::singleShot(500, this, [this]() {
            this->close();
        });
    }

    btnConnect->setEnabled(true);
    log->append("<b>=== Game Over ===");
}

void MainWindow::onError(const QString &error) {
    log->append("<font color='red'>Error: " + error + "</font>");
    QMessageBox::warning(this, "Error", error);
}

void MainWindow::onConnect() {
    QString name = nameEdit->text();
    if (name.isEmpty()) {
        name = "Player";
        nameEdit->setText(name);
    }

    QString ip = ipEdit->text();
    if (ip.isEmpty()) {
        ip = "127.0.0.1";
        ipEdit->setText(ip);
    }

    myField->clear();
    enemyField->clear();
    shotsMade.clear();
    myTurn = false;
    gameFinished = false;
    btnReady->setEnabled(false);
    myField->setInteractive(false);
    enemyField->setInteractive(false);
    statusLabel->setText("Status: connecting...");
    log->clear();

    client->connectToServer(ip, 777, name);
    btnConnect->setEnabled(false);
    log->append("Connecting to server at " + ip + "...");
}

void MainWindow::onReady() {
    auto cells = myField->getSelectedCells();
    ShipValidator::Result res = ShipValidator::validateFullMap(cells);

    if (!res.success) {
        log->append("<font color='red'>Error: " + res.message + "</font>");
        QMessageBox::warning(this, "Placement Error", res.message);
        return;
    }

    QJsonArray arr;
    for(auto p : cells) {
        QJsonObject cell;
        cell["x"] = p.first;
        cell["y"] = p.second;
        arr.append(cell);
    }

    client->sendCommand("SHIPS_PLACED", arr);
    myField->setInteractive(false);
    btnReady->setEnabled(false);
    log->append("<font color='green'>Fleet deployed! Waiting for opponent...</font>");
}

void MainWindow::onShot(int x, int y) {
    if (!myTurn) {
        log->append("<font color='orange'>Not your turn! Wait for your turn.</font>");
        return;
    }
    if (shotsMade.contains(qMakePair(x, y))) {
        qDebug() << "Already shot at:" << x << y << "- ignoring";
        return;
    }


    qDebug() << "Sending shot at:" << x << y;
    enemyField->setInteractive(false);
    shotsMade.insert(qMakePair(x, y));
    QJsonObject shot;
    shot["x"] = x;
    shot["y"] = y;
    client->sendCommand("SHOT", shot);

    log->append(QString("Shooting at (%1, %2)...").arg(x).arg(y));
}
MainWindow::~MainWindow() {
    if (client) {
        client->disconnectFromServer();
    }
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (client) {
        client->disconnectFromServer();
    }
    event->accept();
}
void MainWindow::onGameReset() {
    qDebug() << "Game reset handler called";
    myField->clear();
    enemyField->clear();
    shotsMade.clear();
    myTurn = false;
    gameFinished = false;
    myField->setInteractive(true);
    enemyField->setInteractive(false);
    btnReady->setEnabled(true);
    btnConnect->setEnabled(false);

    statusLabel->setText("Status: PLACING_SHIPS");
    log->append("=== GAME RESET ===");
    log->append("Place your ships again!");
}
void MainWindow::restartGame() {
    qDebug() << "Restarting game...";
    myField->clear();
    enemyField->clear();
    shotsMade.clear();
    myTurn = false;
    gameFinished = false;
    log->append("=== Starting new game ===");
    statusLabel->setText("Status: waiting for game restart...");
}
