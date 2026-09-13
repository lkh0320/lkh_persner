#include "gamewidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GameWidget w;
    w.setWindowTitle(QStringLiteral("Time Echo — 과거의 나와 협동하기"));
    w.show();

    return app.exec();
}
