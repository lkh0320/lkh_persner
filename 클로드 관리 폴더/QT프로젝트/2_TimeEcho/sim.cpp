#include "sim.h"
#include <algorithm>

namespace te {

// '#' 벽,  'D' 문,  'b' 버튼,  'G' 목표,  'P' 시작점
static const char *LEVEL[ROWS] = {
    "##############################",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#......................G.....#",
    "#.....................#######7",
    "#...........D.........#######7",
    "#...........D.........#######7",
    "#b..P.......D.........#######7",
    "##############################",
};
// (11~14행 끝의 '7'은 오른쪽 벽 자리. 아래 tileAt에서 '#'과 동일하게 처리.)

char Sim::tileAt(int col, int row)
{
    if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return '#';
    const char c = LEVEL[row][col];
    return (c == '7') ? '#' : c;
}

bool Sim::isWallTile(int col, int row) { return tileAt(col, row) == '#'; }
bool Sim::isDoorTile(int col, int row) { return tileAt(col, row) == 'D'; }

Rect Sim::buttonRect()
{
    // 버튼은 바닥(15행) 위, 14행의 'b' 자리 (시작점 왼쪽 = 오가는 길과 겹치지 않게)
    return { 1 * TILE + 4, 15 * TILE - 8, TILE - 8, 8 };
}

Rect Sim::goalRect()
{
    return { 23 * TILE + 6, 10 * TILE + 6, TILE - 12, TILE - 12 };
}

bool overlaps(const Rect &a, const Rect &b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}

Sim::Sim() { reset({}); }

void Sim::reset(const std::vector<Tape> &ghosts)
{
    m_ghostTapes = ghosts;
    m_tick = 0;
    m_doorOpen = false;
    m_cleared  = false;
    m_tape.clear();

    m_ents.clear();
    const int spawnX = 4 * TILE;
    const int spawnY = 15 * TILE - PH;
    // 과거의 나들이 먼저, 지금의 내가 마지막. 순서가 고정되어야 결정론이 유지된다.
    for (size_t i = 0; i <= m_ghostTapes.size(); ++i) {
        Entity e;
        e.px = spawnX;
        e.py = spawnY;
        m_ents.push_back(e);
    }
}

bool Sim::blocked(const Rect &r, int selfIdx) const
{
    // 1) 타일
    const int c0 = r.x / TILE,           c1 = (r.x + r.w - 1) / TILE;
    const int r0 = r.y / TILE,           r1 = (r.y + r.h - 1) / TILE;
    for (int row = r0; row <= r1; ++row)
        for (int col = c0; col <= c1; ++col) {
            if (isWallTile(col, row)) return true;
            if (isDoorTile(col, row) && !m_doorOpen) return true;
        }

    // 2) 다른 개체 (= 과거의 나는 딛고 올라설 수 있는 발판이다)
    //    단, 지금 이미 겹쳐 있는 상대에게는 막히지 않는다.
    //    (시작 지점에서 모두가 같은 자리에 겹쳐 스폰되므로 이 예외가 없으면 전원이 굳어버린다)
    const Rect self = m_ents[size_t(selfIdx)].rect();
    for (size_t i = 0; i < m_ents.size(); ++i) {
        if (int(i) == selfIdx) continue;
        const Rect o = m_ents[i].rect();
        if (overlaps(self, o)) continue;
        if (overlaps(r, o)) return true;
    }
    return false;
}

void Sim::moveEntity(int idx, uint8_t input)
{
    Entity &e = m_ents[size_t(idx)];

    // ── 좌우 ──
    const int dir = ((input & BTN_RIGHT) ? 1 : 0) - ((input & BTN_LEFT) ? 1 : 0);
    e.ax += dir * SPEED;
    int stepX = e.ax / SUB;
    e.ax -= stepX * SUB;
    while (stepX != 0) {
        const int s = (stepX > 0) ? 1 : -1;
        Rect n = e.rect(); n.x += s;
        if (blocked(n, idx)) { e.ax = 0; break; }
        e.px += s;
        stepX -= s;
    }

    // ── 점프 (누른 순간에만) ──
    const bool jump = (input & BTN_JUMP) != 0;
    if (jump && !e.jumpHeld && e.onGround) {
        e.vy = -JUMP_V;
        e.onGround = false;
    }
    e.jumpHeld = jump;

    // ── 중력 + 상하 ──
    e.vy = std::min(e.vy + GRAVITY, MAX_FALL);
    e.ay += e.vy;
    int stepY = e.ay / SUB;
    e.ay -= stepY * SUB;
    e.onGround = false;
    while (stepY != 0) {
        const int s = (stepY > 0) ? 1 : -1;
        Rect n = e.rect(); n.y += s;
        if (blocked(n, idx)) {
            if (s > 0) e.onGround = true;
            e.vy = 0; e.ay = 0;
            break;
        }
        e.py += s;
        stepY -= s;
    }
    // 착지 판정 보정: 바로 아래가 막혀 있으면 땅 위
    if (!e.onGround) {
        Rect n = e.rect(); n.y += 1;
        if (blocked(n, idx)) e.onGround = true;
    }
}

void Sim::step(uint8_t input)
{
    if (m_cleared) return;

    // 1) 이번 틱의 문 상태를 먼저 확정한다 (직전 위치 기준 → 재현 가능)
    m_doorOpen = false;
    const Rect btn = buttonRect();
    for (const Entity &e : m_ents)
        if (overlaps(e.rect(), btn)) { m_doorOpen = true; break; }

    // 2) 고정된 순서로 모든 개체를 같은 코드로 굴린다
    for (size_t i = 0; i < m_ents.size(); ++i) {
        uint8_t in = 0;
        if (i < m_ghostTapes.size()) {                 // 과거의 나
            const Tape &t = m_ghostTapes[i];
            in = (m_tick < int(t.size())) ? t[size_t(m_tick)] : 0;
        } else {                                       // 지금의 나
            in = input;
        }
        moveEntity(int(i), in);
    }

    // 3) 기록
    m_tape.push_back(input);
    ++m_tick;

    // 4) 클리어 판정 (지금의 나만)
    if (!m_ents.empty() && overlaps(m_ents.back().rect(), goalRect()))
        m_cleared = true;
}

} // namespace te
