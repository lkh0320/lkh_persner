#include "world.h"
#include "gamewindow.h"

#include <QApplication>
#include <QTimer>
#include <QKeyEvent>

// 어느 창이 포커스를 갖고 있든 키 입력을 한 곳에서 받는다.
class KeyFilter : public QObject
{
public:
    explicit KeyFilter(World *w) : m_world(w) {}
protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
            auto *ke = static_cast<QKeyEvent *>(e);
            if (!ke->isAutoRepeat()) {
                const bool down = (e->type() == QEvent::KeyPress);
                if (down && ke->key() == Qt::Key_R) { m_world->respawn(); return true; }
                if (down && ke->key() == Qt::Key_N) { m_world->spawnWindowAtCursor(); return true; }
                m_world->setKey(ke->key(), down);
            }
        }
        return QObject::eventFilter(o, e);
    }
private:
    World *m_world;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    World world;
    app.installEventFilter(new KeyFilter(&world));

    // 창 3개. 1번과 2번 사이에는 건널 수 없는 틈이 있다 → 2번 창을 끌어다 붙여야 한다.
    struct { int x, y, w, h; } layout[] = {
        { 200, 200, 320, 220 },
        { 560, 260, 280, 200 },
        { 900, 200, 240, 260 },
    };
    for (int i = 0; i < 3; ++i) {
        auto *win = new GameWindow(&world, i);
        win->setGeometry(layout[i].x, layout[i].y, layout[i].w, layout[i].h);
        win->show();
    }
    world.respawn();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &world, [&] {
        world.tick();
        for (QWidget *w : QApplication::topLevelWidgets())
            w->update();
    });
    timer.start(16);   // 약 60fps

    return app.exec();
}
