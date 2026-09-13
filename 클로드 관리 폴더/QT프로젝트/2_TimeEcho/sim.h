#pragma once
#include <vector>
#include <cstdint>

// ─────────────────────────────────────────────────────────────
//  결정론적 시뮬레이션
//  - Qt에 전혀 의존하지 않는다 (그래야 재현성을 신뢰할 수 있다)
//  - 부동소수점을 한 번도 쓰지 않는다 → 정수 연산만
//  - 고정 타임스텝 1/60초로만 전진한다
//  - 입력은 틱마다 1바이트. 같은 입력 배열 → 언제나 같은 결과.
// ─────────────────────────────────────────────────────────────
namespace te {

constexpr int TILE = 32;
constexpr int COLS = 30;
constexpr int ROWS = 16;
constexpr int SUB  = 256;          // 1픽셀 = 256 서브픽셀

constexpr int PW = 24;             // 플레이어 폭
constexpr int PH = 40;             // 플레이어 높이

constexpr int SPEED    = 819;      // 3.20 px/tick
constexpr int GRAVITY  = 154;      // 0.60 px/tick^2
constexpr int JUMP_V   = 2765;     // 10.80 px/tick  (점프 높이 ≒ 97px = 3타일)
constexpr int MAX_FALL = 16 * SUB;

enum Btn : uint8_t { BTN_LEFT = 1, BTN_RIGHT = 2, BTN_JUMP = 4 };

struct Rect { int x, y, w, h; };
bool overlaps(const Rect &a, const Rect &b);

struct Entity {
    int  px = 0, py = 0;           // 픽셀 좌표(좌상단)
    int  ax = 0, ay = 0;           // 서브픽셀 누적분
    int  vy = 0;                   // 서브픽셀/틱
    bool onGround = false;
    bool jumpHeld = false;
    bool alive    = true;

    Rect rect() const { return { px, py, PW, PH }; }
};

using Tape = std::vector<uint8_t>;   // 틱별 입력 기록

class Sim {
public:
    Sim();

    // ghosts: 이전 회차들의 입력 기록. 순서 = 회차 순서.
    void reset(const std::vector<Tape> &ghosts);

    // 이번 틱의 플레이어 입력을 받아 1/60초만큼 전진하고, 입력을 기록한다.
    void step(uint8_t input);

    const std::vector<Entity> &entities() const { return m_ents; }
    int   playerIndex() const { return int(m_ents.size()) - 1; }
    const Tape &tape() const  { return m_tape; }

    int  tick()     const { return m_tick; }
    bool doorOpen() const { return m_doorOpen; }
    bool cleared()  const { return m_cleared; }

    static bool isWallTile(int col, int row);
    static bool isDoorTile(int col, int row);
    static Rect buttonRect();
    static Rect goalRect();
    static char tileAt(int col, int row);

private:
    bool blocked(const Rect &r, int selfIdx) const;   // 벽 또는 다른 개체
    void moveEntity(int idx, uint8_t input);

    std::vector<Entity> m_ents;
    std::vector<Tape>   m_ghostTapes;
    Tape m_tape;
    int  m_tick = 0;
    bool m_doorOpen = false;
    bool m_cleared  = false;
};

} // namespace te
