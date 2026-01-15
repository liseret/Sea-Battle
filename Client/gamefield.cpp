#include "gamefield.h"

GameField::GameField(QWidget *parent) : QWidget(parent) {
    clear();
    setFixedSize(301, 301);
}

void GameField::clear() {
    for(int i=0; i<10; ++i){
        for(int j=0; j<10; ++j){
            grid[i][j]=Empty;
        }
    }
    selectedCells.clear();
    update();
}

void GameField::setCell(int x, int y, CellState state) {
    if (x >= 0 && x < 10 && y >= 0 && y < 10) {
        grid[x][y] = state;
        update();
    }
}

void GameField::paintEvent(QPaintEvent *) {
    QPainter p(this);
    int step = 30;

    for(int i=0; i<=10; ++i) {
        p.drawLine(i*step, 0, i*step, 300);
        p.drawLine(0, i*step, 300, i*step);
    }

    for(int x=0; x<10; ++x) {
        for(int y=0; y<10; ++y) {
            QRect rect(x*step+1, y*step+1, step-1, step-1);
            if (grid[x][y] == Ship){
                p.fillRect(rect, Qt::gray);
            }
            else if (grid[x][y] == Hit){
                p.fillRect(rect, Qt::red);
            }
            else if (grid[x][y] == Miss) {
                p.setBrush(Qt::blue);
                p.drawEllipse(rect.center(), 3, 3);
            }
        }
    }
}

void GameField::mousePressEvent(QMouseEvent *event) {
    if (!interactive) {
        return;
    }
    int x = event->x() / 30;
    int y = event->y() / 30;
    if (x < 10 && y < 10) {
        if (grid[x][y] == Empty) {
            grid[x][y] = Ship;
            selectedCells.insert({x, y});
        } else if (grid[x][y] == Ship) {
            grid[x][y] = Empty;
            selectedCells.remove({x, y});
        }
        emit cellClicked(x, y);
        update();
    }
}
