#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QTimer>
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
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onConnect();
    void onReady();
    void onShot(int x, int y);
    void onStatusChanged(const QString &status);
    void onTurnChanged(const QString &currentPlayer);
    void onShotResult(int x, int y, bool hit, bool sunk);
    void onEnemyShot(int x, int y, bool hit, bool sunk);
    void onGameStarted(const QString &info);
    void onGameOver(const QString &winner, const QString &stats);
    void onError(const QString &error);
    void onGameReset();
    void restartGame();

private:
    ClientManager *client;
    GameField *myField, *enemyField;
    QTextEdit *log;
    QLineEdit *nameEdit, *ipEdit;
    QPushButton *btnConnect, *btnReady;
    QLabel *statusLabel;
    bool myTurn = false;
    bool gameFinished = false;
    bool restartRequested = false;
    QSet<QPair<int, int>> shotsMade;
};

#endif // MAINWINDOW_H
