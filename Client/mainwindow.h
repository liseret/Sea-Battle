#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "gamefield.h"
#include "clientmanager.h"

              class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onConnect();
    void onReady();
    void onShot(int x, int y);

    // Слоты для обработки сигналов от ClientManager
    void onStatusChanged(const QString &status);
    void onTurnChanged(const QString &currentPlayer);
    void onShotResult(int x, int y, bool hit, bool sunk);
    void onEnemyShot(int x, int y, bool hit, bool sunk);
    void onGameStarted(const QString &info);
    void onGameOver(const QString &winner, const QString &stats);
    void onError(const QString &error);

private:
    ClientManager *client;
    GameField *myField, *enemyField;
    QTextEdit *log;
    QLineEdit *nameEdit, *ipEdit;
    QPushButton *btnConnect, *btnReady;
    QLabel *statusLabel;
    bool myTurn = false;
};

#endif // MAINWINDOW_H
