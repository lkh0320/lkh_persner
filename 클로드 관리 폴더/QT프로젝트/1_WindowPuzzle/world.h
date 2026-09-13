#pragma once
#include <QObject>
#include <QRect>
#include <QVector>

class GameWindow;

// 게임 세계는 "전역 데스크톱 좌표계" 하나뿐이다.
// 갈 수 있는 공간 = 열려 있는 모든 창의 클라이언트 영역의 합집합.
// 창을 옮기고 늘이는 것이 곧 레벨을 바꾸는 것.
class World : public QObject
{
    Q_OBJECT
public:
    static constexpr int PW = 20;     // 플레이어 폭
    static constexpr int PH = 28;     // 플레이어 높이
    static constexpr int GOAL_SIZE = 24;

    explicit World(QObject *parent = nullptr);

    void addWindow(GameWindow *w);
    void removeWindow(GameWindow *w);

    void tick();

    QRect  playerRect() const;
    QRect  goalRect()   const { return QRect(m_goal, QSize(GOAL_SIZE, GOAL_SIZE)); }
    bool   cleared()    const { return m_cleared; }
    bool   inVoid()     const { return m_inVoid; }

    void setKey(int key, bool down);
    void respawn();
    void spawnWindowAtCursor();

signals:
    void changed();

private:
    QVector<QRect> freeRects() const;          // 각 창의 클라이언트 영역(전역 좌표)
    bool canStandAt(const QRect &box) const;

    QVector<GameWindow *> m_windows;

    int  m_x = 0, m_y = 0;          // 플레이어 좌상단(전역 좌표, px)
    double m_vy = 0.0;
    bool m_onGround = false;
    bool m_inVoid   = false;
    bool m_cleared  = false;

    bool m_left = false, m_right = false, m_jump = false;
    QPoint m_goal;
    QPoint m_spawn;
};
