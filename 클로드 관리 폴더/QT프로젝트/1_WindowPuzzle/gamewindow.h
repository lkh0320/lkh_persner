#pragma once
#include <QWidget>

class World;

// 창 하나 = 세계를 들여다보는 구멍 하나.
// 이 창이 전역 좌표계의 어느 부분을 비추는지는 geometry()가 알려준다.
class GameWindow : public QWidget
{
    Q_OBJECT
public:
    GameWindow(World *world, int index, QWidget *parent = nullptr);

    // 이 창이 담당하는 전역 데스크톱 영역(클라이언트 영역)
    QRect worldViewRect() const { return QRect(mapToGlobal(QPoint(0, 0)), size()); }

protected:
    void paintEvent(QPaintEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void closeEvent(QCloseEvent *) override;

private:
    World *m_world;
    int    m_index;
};
