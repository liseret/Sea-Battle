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
}

void MainWindow::onStatusChanged(const QString &status) {
    statusLabel->setText("Status: " + status);
    qDebug() << "Status changed to:" << status;

    if (status == "PLACING_SHIPS") {
        myField->setInteractive(true);
        enemyField->setInteractive(false);
        btnReady->setEnabled(true);
        log->append("Place your ships: 1x4, 2x3, 3x2, 4x1");
    }
    else if (status == "PLAYER1_TURN" || status == "PLAYER2_TURN") {
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
    } else {
        log->append("=== OPPONENT TURN ===");
        enemyField->setInteractive(false);
    }
}

void MainWindow::onShotResult(int x, int y, bool hit, bool sunk) {
    qDebug() << "Shot result:" << x << y << "hit:" << hit << "sunk:" << sunk;

    if (hit) {
        enemyField->setCell(x, y, GameField::Hit);
        if (sunk) {
            log->append(QString("You sunk a ship at (%1, %2)!").arg(x).arg(y));
        } else {
            log->append(QString("Hit at (%1, %2)!").arg(x).arg(y));
        }
        enemyField->setInteractive(true);
    } else {
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
        } else {
            log->append(QString("Your ship was hit at (%1, %2)!").arg(x).arg(y));
        }
    } else {
        myField->setCell(x, y, GameField::Miss);
        log->append(QString("Enemy missed at (%1, %2).").arg(x).arg(y));
    }
}

void MainWindow::onGameStarted(const QString &info) {
    log->append("Game started! " + info);
    myField->setInteractive(false);
    enemyField->setInteractive(false);
}

void MainWindow::onGameOver(const QString &winner, const QString &stats) {
    myTurn = false;
    enemyField->setInteractive(false);
    myField->setInteractive(false);

    statusLabel->setText("Игра окончена!");
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Конец игры");
    msgBox.setIcon(QMessageBox::Information);

    if (winner == nameEdit->text()) {
        msgBox.setText("<h2>Поздравляем! Вы победили!</h2>");
    } else {
        msgBox.setText(QString("<h2>Победил игрок %1</h2>").arg(winner));
    }

    msgBox.setInformativeText(stats);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    log->append("<b>Игра окончена. Победитель: " + winner + "</b>");
    btnConnect->setEnabled(true);
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

    qDebug() << "Sending shot at:" << x << y;
    enemyField->setInteractive(false);

    QJsonObject shot;
    shot["x"] = x;
    shot["y"] = y;
    client->sendCommand("SHOT", shot);

    log->append(QString("Shooting at (%1, %2)...").arg(x).arg(y));
}
