#include "mainwindow.h"
#include <QJsonArray>
#include <QJsonObject>
#include "shipvalidator.h"

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
    statusLabel = new QLabel("Status: wating");

    uiLayout->addWidget(new QLabel("Name:")); uiLayout->addWidget(nameEdit);
    uiLayout->addWidget(new QLabel("IP:")); uiLayout->addWidget(ipEdit);
    uiLayout->addWidget(btnConnect);
    uiLayout->addWidget(btnReady);
    uiLayout->addWidget(statusLabel);
    uiLayout->addWidget(log);

    myField = new GameField();
    enemyField = new GameField();

    mainLayout->addLayout(uiLayout);
    mainLayout->addWidget(myField);
    mainLayout->addWidget(enemyField);

    client = new ClientManager(this);

    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(btnReady, &QPushButton::clicked, this, &MainWindow::onReady);
    connect(enemyField, &GameField::cellClicked, this, &MainWindow::onShot);

    connect(client, &ClientManager::statusChanged, [this](QString s){
        statusLabel->setText("Status: " + s);
        if(s == "PLACING_SHIPS") {
            myField->setInteractive(true);
            btnReady->setEnabled(true);
            log->append("Place your ships: 1x4, 2x3, 3x2, 4x1");
        }
    });
    connect(client, &ClientManager::messageReceived, log, &QTextEdit::append);
    connect(client, &ClientManager::turnChanged, [this](QString p){
        myTurn = (p == nameEdit->text());
        log->append(myTurn ? "=== YOUR TURN ===" : "=== OPPONENT TURN ===");
        enemyField->setInteractive(myTurn);
    });
    connect(client, &ClientManager::shotResult, [this](int x, int y, bool hit, bool sunk){
        enemyField->setCell(x, y, hit ? GameField::Hit : GameField::Miss);
    });
    connect(client, &ClientManager::enemyShot, [this](int x, int y, bool hit, bool sunk){
        myField->setCell(x, y, hit ? GameField::Hit : GameField::Miss);
    });
}

void MainWindow::onConnect() {
    client->connectToServer(ipEdit->text(), 777, nameEdit->text());
    btnConnect->setEnabled(false);
}

void MainWindow::onReady() {
    auto cells = myField->getSelectedCells();
    ShipValidator::Result res = ShipValidator::validateFullMap(cells);

    if (!res.success) {
        log->append("<font color='red'>Error: " + res.message + "</font>");
        return;
    }

    QJsonArray arr;
    for(auto p : cells) {
        QJsonObject cell;
        cell["x"] = p.first; cell["y"] = p.second;
        arr.append(cell);
    }
    client->sendCommand("SHIPS_PLACED", arr);
    myField->setInteractive(false);
    btnReady->setEnabled(false);
    log->append("<font color='green'>Fleet deployed! Waiting for opponent...</font>");
}

void MainWindow::onShot(int x, int y) {
    if(!myTurn) return;
    QJsonObject shot;
    shot["x"] = x; shot["y"] = y;
    client->sendCommand("SHOT", shot);
}
