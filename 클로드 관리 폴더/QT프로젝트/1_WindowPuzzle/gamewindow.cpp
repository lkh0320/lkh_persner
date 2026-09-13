#include "gamewindow.h"
#include "world.h"

#include <QPainter>
#include <QCloseEvent>

GameWindow::GameWindow(World *world, int index, QWidget *parent)
    : QWidget(parent), m_world(world), m_index(index)
{
    setWindowTitle(QStringLiteral("창 %1  —  끌어서 옮기고, 모서리를 잡아 늘이세요").arg(index + 1));
    setMinimumSize(80, 60);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_DeleteOnClose);
    m_world->addWindow(this);
}

void GameWindow::moveEvent(QMoveEvent *)   { update(); }   // 창 이동 = 플레이어 입력
void GameWindow::resizeEvent(QResizeEvent *) { update(); } // 창 크기조절 = 레벨 변형

void GameWindow::closeEvent(QCloseEvent *e)
{
    m_world->removeWindow(this);   // 창을 닫으면 그 공간이 세계에서 사라진다
    e->accept();
}

void GameWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPoint origin = mapToGlobal(QPoint(0, 0));   // 전역 → 로컬 변환용
    p.translate(-origin);                               // 이제 전역 좌표로 그린다

    const QRect view = worldViewRect();

    // 배경
    p.fillRect(view, QColor(24, 26, 34));

    // 격자 (세계 좌표 기준이라 창을 옮기면 격자가 "흐르지 않는다" = 같은 세계임이 드러남)
    p.setPen(QPen(QColor(44, 48, 60), 1));
    const int G = 32;
    for (int x = (view.left() / G) * G; x <= view.right(); x += G)
        p.drawLine(x, view.top(), x, view.bottom());
    for (int y = (view.top() / G) * G; y <= view.bottom(); y += G)
        p.drawLine(view.left(), y, view.right(), y);

    // 바닥선 (이 창의 아래 테두리 = 발판)
    p.setPen(QPen(QColor(90, 200, 160), 3));
    p.drawLine(view.bottomLeft() + QPoint(0, -1), view.bottomRight() + QPoint(0, -1));

    // 목표 지점
    const QRect goal = m_world->goalRect();
    if (view.intersects(goal)) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 200, 60));
        p.drawEllipse(goal);
        p.setPen(QPen(QColor(255, 240, 190), 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(goal.adjusted(-5, -5, 5, 5));
    }

    // 플레이어
    const QRect pr = m_world->playerRect();
    if (view.intersects(pr)) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_world->inVoid() ? QColor(230, 90, 90) : QColor(235, 238, 245));
        p.drawRoundedRect(pr, 4, 4);
        p.setBrush(QColor(24, 26, 34));
        p.drawRect(pr.left() + 5,  pr.top() + 8, 3, 4);
        p.drawRect(pr.right() - 7, pr.top() + 8, 3, 4);
    }

    // 안내문 (1번 창에만)
    p.resetTransform();
    if (m_index == 0) {
        p.setPen(QColor(150, 156, 175));
        p.drawText(rect().adjusted(10, 8, -10, 0), Qt::AlignTop | Qt::AlignLeft,
                   QStringLiteral("←/→ 이동   Space 점프   R 리스폰   N 새 창\n"
                                  "창을 끌어다 붙이면 다리가 됩니다"));
    }
    if (m_world->cleared()) {
        p.setPen(QColor(255, 210, 90));
        QFont f = p.font(); f.setPointSize(14); f.setBold(true);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("CLEAR!"));
    }
}
