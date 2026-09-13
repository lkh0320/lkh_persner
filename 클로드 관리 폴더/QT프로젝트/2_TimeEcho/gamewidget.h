#pragma once
#include <QWidget>
#include <QElapsedTimer>
#include "sim.h"

class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);

    static constexpr int HUD_H = 46;

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;

private slots:
    void onFrame();

private:
    void restartRecording();   // 지금 회차를 유령으로 남기고 다시 시작
    void clearGhosts();

    te::Sim m_sim;
    std::vector<te::Tape> m_ghosts;

    uint8_t m_input = 0;
    QElapsedTimer m_clock;
    qint64 m_accumNs = 0;
    int    m_run = 1;
};
