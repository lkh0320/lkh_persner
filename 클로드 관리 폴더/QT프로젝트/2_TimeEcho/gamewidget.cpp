#include "gamewidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QTimer>

using namespace te;

namespace {
constexpr qint64 STEP_NS = 1000000000LL / 60;   // 고정 타임스텝: 정확히 1/60초
QRect toQRect(const Rect &r) { return QRect(r.x, r.y, r.w, r.h); }
}

GameWidget::GameWidget(QWidget *parent) : QWidget(parent)
{
    setFixedSize(COLS * TILE, ROWS * TILE + HUD_H);
    setFocusPolicy(Qt::StrongFocus);
    m_sim.reset({});
    m_clock.start();

    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &GameWidget::onFrame);
    t->start(4);   // 화면 갱신은 자주, 물리는 아래에서 1/60초 단위로만
}

void GameWidget::onFrame()
{
    // 렌더링 프레임과 시뮬레이션 스텝을 분리한다.
    // 물리는 언제나 정확히 1/60초씩만 전진 → 기록/재생이 어긋나지 않는다.
    m_accumNs += m_clock.nsecsElapsed();
    m_clock.restart();
    if (m_accumNs > STEP_NS * 8) m_accumNs = STEP_NS * 8;   // 창 끌기 등으로 밀렸을 때

    while (m_accumNs >= STEP_NS) {
        m_accumNs -= STEP_NS;
        m_sim.step(m_input);
    }
    update();
}

void GameWidget::restartRecording()
{
    m_ghosts.push_back(m_sim.tape());
    ++m_run;
    m_sim.reset(m_ghosts);
    m_accumNs = 0;
    m_clock.restart();
}

void GameWidget::clearGhosts()
{
    m_ghosts.clear();
    m_run = 1;
    m_sim.reset(m_ghosts);
    m_accumNs = 0;
    m_clock.restart();
}

void GameWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;
    switch (e->key()) {
    case Qt::Key_Left:  case Qt::Key_A: m_input |= BTN_LEFT;  break;
    case Qt::Key_Right: case Qt::Key_D: m_input |= BTN_RIGHT; break;
    case Qt::Key_Up:    case Qt::Key_W:
    case Qt::Key_Space:                 m_input |= BTN_JUMP;  break;
    case Qt::Key_R:         restartRecording(); break;
    case Qt::Key_Backspace: clearGhosts();      break;
    default: QWidget::keyPressEvent(e);
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;
    switch (e->key()) {
    case Qt::Key_Left:  case Qt::Key_A: m_input &= ~BTN_LEFT;  break;
    case Qt::Key_Right: case Qt::Key_D: m_input &= ~BTN_RIGHT; break;
    case Qt::Key_Up:    case Qt::Key_W:
    case Qt::Key_Space:                 m_input &= ~BTN_JUMP;  break;
    default: QWidget::keyReleaseEvent(e);
    }
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(20, 22, 30));

    // ── 타일 ──
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            const QRect cell(col * TILE, row * TILE, TILE, TILE);
            const char t = Sim::tileAt(col, row);
            if (t == '#') {
                p.fillRect(cell, QColor(52, 58, 74));
                p.fillRect(cell.adjusted(0, 0, 0, -TILE + 3), QColor(72, 80, 100));
            } else if (t == 'D') {
                if (m_sim.doorOpen()) {
                    p.setPen(QPen(QColor(80, 150, 110), 1, Qt::DashLine));
                    p.setBrush(Qt::NoBrush);
                    p.drawRect(cell.adjusted(6, 0, -6, 0));
                } else {
                    p.fillRect(cell.adjusted(4, 0, -4, 0), QColor(190, 110, 70));
                    p.fillRect(cell.adjusted(4, 0, -4, -TILE + 4), QColor(220, 140, 95));
                }
            }
        }
    }

    // ── 버튼 ──
    {
        const QRect b = toQRect(Sim::buttonRect());
        p.setPen(Qt::NoPen);
        p.setBrush(m_sim.doorOpen() ? QColor(110, 220, 150) : QColor(200, 90, 90));
        p.drawRoundedRect(m_sim.doorOpen() ? b.adjusted(0, 4, 0, 0) : b, 3, 3);
    }

    // ── 목표 ──
    {
        const QRect g = toQRect(Sim::goalRect());
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 205, 70));
        p.drawEllipse(g);
    }

    // ── 개체 ──
    const auto &ents = m_sim.entities();
    for (size_t i = 0; i < ents.size(); ++i) {
        const QRect r = toQRect(ents[i].rect());
        const bool isPlayer = (int(i) == m_sim.playerIndex());
        if (isPlayer) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(240, 243, 250));
            p.drawRoundedRect(r, 5, 5);
            p.setBrush(QColor(20, 22, 30));
            p.drawRect(r.left() + 6,  r.top() + 11, 3, 5);
            p.drawRect(r.right() - 8, r.top() + 11, 3, 5);
        } else {
            p.setPen(QPen(QColor(120, 190, 255, 190), 2));
            p.setBrush(QColor(90, 150, 220, 75));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 5, 5);
            p.setPen(QColor(190, 220, 255, 200));
            p.drawText(r, Qt::AlignCenter, QString::number(i + 1));
        }
    }

    // ── HUD ──
    const QRect hud(0, ROWS * TILE, width(), HUD_H);
    p.fillRect(hud, QColor(14, 15, 21));
    p.setPen(QColor(210, 215, 230));
    p.drawText(hud.adjusted(12, 0, -12, 0), Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("%1회차   과거의 나: %2명   문: %3")
                   .arg(m_run).arg(m_ghosts.size())
                   .arg(m_sim.doorOpen() ? QStringLiteral("열림") : QStringLiteral("닫힘")));
    p.setPen(QColor(130, 137, 158));
    p.drawText(hud.adjusted(12, 0, -12, 0), Qt::AlignVCenter | Qt::AlignRight,
               QStringLiteral("←/→ 이동   Space 점프   R 다시(유령 남김)   Backspace 전부 초기화"));

    if (m_sim.cleared()) {
        p.fillRect(QRect(0, ROWS * TILE / 2 - 40, width(), 80), QColor(0, 0, 0, 190));
        p.setPen(QColor(255, 215, 90));
        QFont f = p.font(); f.setPointSize(20); f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(0, ROWS * TILE / 2 - 40, width(), 80), Qt::AlignCenter,
                   QStringLiteral("CLEAR!   (%1회차에 성공)").arg(m_run));
    }
}
