#ifndef GAMEFIELD_H
#define GAMEFIELD_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QSet>
#include <QPair>

class GameField : public QWidget {
    Q_OBJECT
public:
    enum CellState { Empty, Ship, Hit, Miss };
    explicit GameField(QWidget *parent = nullptr);

    void setCell(int x, int y, CellState state);
    void clear();
    void setInteractive(bool i) { interactive = i; }
    QSet<QPair<int, int>> getSelectedCells() const { return selectedCells; }

signals:
    void cellClicked(int x, int y);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    CellState grid[10][10];
    bool interactive = false;
    QSet<QPair<int, int>> selectedCells;
};

#endif // GAMEFIELD_H
