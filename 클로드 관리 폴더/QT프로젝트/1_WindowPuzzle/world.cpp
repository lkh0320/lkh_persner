#include "world.h"
#include "gamewindow.h"
#include "rectutil.h"

#include <QCursor>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace {
constexpr double GRAVITY  = 0.85;
constexpr double JUMP_V   = 12.0;
constexpr double MAX_FALL = 18.0;
constexpr int    SPEED    = 4;
constexpr int    KILL_Y   = 4000;   // 이보다 아래로 떨어지면 리스폰
}

World::World(QObject *parent) : QObject(parent)
{
    // 목표 지점은 세계의 오른쪽 끝. 처음엔 창 3번 안에 들어 있다.
    m_goal  = QPoint(1010, 410);
    m_spawn = QPoint(240, 240);
    m_x = m_spawn.x();
    m_y = m_spawn.y();
}

void World::addWindow(GameWindow *w)
{
    if (!m_windows.contains(w))
        m_windows.append(w);
}

void World::removeWindow(GameWindow *w)
{
    m_windows.removeAll(w);
}

QRect World::playerRect() const
{
    return QRect(m_x, m_y, PW, PH);
}

QVector<QRect> World::freeRects() const
{
    QVector<QRect> out;
    for (GameWindow *w : m_windows)
        if (w->isVisible())
            out.append(w->worldViewRect());
    return out;
}

bool World::canStandAt(const QRect &box) const
{
    return fullyCovered(box, freeRects());
}

void World::setKey(int key, bool down)
{
    switch (key) {
    case Qt::Key_Left:  case Qt::Key_A: m_left  = down; break;
    case Qt::Key_Right: case Qt::Key_D: m_right = down; break;
    case Qt::Key_Up:    case Qt::Key_W:
    case Qt::Key_Space:                 m_jump  = down; break;
    default: break;
    }
}

void World::respawn()
{
    // 남아 있는 창 중 첫 번째 창 안쪽에 다시 세운다.
    const QVector<QRect> rects = freeRects();
    if (!rects.isEmpty()) {
        const QRect r = rects.first();
        m_x = r.left() + 24;
        m_y = r.top()  + 12;
    } else {
        m_x = m_spawn.x();
        m_y = m_spawn.y();
    }
    m_vy = 0.0;
    m_cleared = false;
    emit changed();
}

void World::spawnWindowAtCursor()
{
    auto *w = new GameWindow(this, m_windows.size());
    const QPoint c = QCursor::pos();
    w->setGeometry(c.x() - 100, c.y() - 70, 200, 140);
    w->show();
}

void World::tick()
{
    const QVector<QRect> rects = freeRects();
    if (rects.isEmpty()) { emit changed(); return; }

    QRect box = playerRect();

    // --- 발밑의 창이 사라졌다면: 허공 상태. 다른 창 안으로 들어갈 때까지 낙하 ---
    m_inVoid = !fullyCovered(box, rects);
    if (m_inVoid) {
        m_vy = std::min(m_vy + GRAVITY, MAX_FALL);
        const int steps = std::max(1, int(std::round(m_vy)));
        for (int i = 0; i < steps; ++i) {
            ++m_y;
            if (fullyCovered(playerRect(), rects)) { m_inVoid = false; break; }
        }
        if (m_y > KILL_Y) respawn();
        emit changed();
        return;
    }

    // --- 좌우 이동 (1px씩 밀어보고 막히면 멈춘다) ---
    const int dir = (m_right ? 1 : 0) - (m_left ? 1 : 0);
    if (dir != 0) {
        for (int i = 0; i < SPEED; ++i) {
            QRect n = playerRect().translated(dir, 0);
            if (!fullyCovered(n, rects)) break;
            m_x += dir;
        }
    }

    // --- 점프 ---
    if (m_jump && m_onGround) {
        m_vy = -JUMP_V;
        m_onGround = false;
    }

    // --- 중력 + 상하 이동 ---
    m_vy = std::min(m_vy + GRAVITY, MAX_FALL);
    m_onGround = false;

    const int vsteps = int(std::round(std::fabs(m_vy)));
    const int vdir   = (m_vy > 0) ? 1 : -1;
    for (int i = 0; i < vsteps; ++i) {
        QRect n = playerRect().translated(0, vdir);
        if (!fullyCovered(n, rects)) {
            if (vdir > 0) m_onGround = true;   // 창 아래 테두리 = 바닥
            m_vy = 0.0;
            break;
        }
        m_y += vdir;
    }

    if (playerRect().intersects(goalRect()))
        m_cleared = true;

    emit changed();
}
