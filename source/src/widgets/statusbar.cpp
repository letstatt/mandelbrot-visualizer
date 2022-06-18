#include "statusbar.h"
#include <QString>
#include <QTimer>
#include <QLocale>

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent), prevRendererState(mandelbrot::RendererState::INITIAL_RENDERING)
{
    qtimer = new QTimer(this);
    connect(qtimer, SIGNAL(timeout()), this, SLOT(updateTimer()));
}

void StatusBar::updateInfo(mandelbrot::ViewportInfo info) {
    using namespace mandelbrot;

    // unfortunately, QWidget constructor knows nothing about children
    // that's why we try to find them here

    if (status == nullptr) {
        status = findChild<QLabel*>("status");
        coords = findChild<QLabel*>("coords");
        scale = findChild<QLabel*>("scale");
        timer = findChild<QLabel*>("timer");
    }

    const QString x = QString::number(info.offset.x, 'f', 10);
    const QString y = QString::number(info.offset.y, 'f', 10);

    coords->setText(QString("x: %1 y: %2").arg(x, y));
    scale->setText(QString(" scale: %1x").arg((size_t) std::floor(info.scaleLog)));

    if (info.scaleLog == MAX_SCALE_LOG) {
        scale->setStyleSheet("color: red");
    } else if (info.scaleLog >= WARN_SCALE_LOG) {
        scale->setStyleSheet("color: yellow");
    } else {
        scale->setStyleSheet("color: white");
    }

    // DFA analysis. no more comments

    switch (info.state) {
    case RendererState::READY:
        status->setText("Ready");
        if (prevRendererState == RendererState::RENDERING) {
            qtimer->stop();
            updateTimer();
        }
        if (time > 0) {
            time -= 0.01;
            updateTimer();
        }
        break;

    case RendererState::INITIAL_RENDERING:
        time = 0;
        qtimer->stop();
        coords->setText("");
        scale->setText("");
        status->setText("");
        timer->setText("");
        break;

    case RendererState::RENDERING:
        status->setText("Rendering...");
        if (prevFrameSeqId != info.frameSeqId) {
            qtimer->stop();
            time = 0;
            qtimer->start(10); // 10ms
        }
        break;

    case RendererState::OFFLINE:
        status->setText("Offline");
        qtimer->stop();
        timer->setText("");

        switch (prevRendererState) {
        case RendererState::RENDERING:
        case RendererState::INITIAL_RENDERING:
            time = 0;
        default:
            break;
        }
    }

    prevFrameSeqId = info.frameSeqId;
    prevRendererState = info.state;
}

void StatusBar::updateTimer() {
    using namespace mandelbrot;

    time += 0.01;
    timer->setText(QString("%1 sec ").arg(QString::number(time, 'f', 2)));

    if (time >= WARN_RENDER_LATENCY) {
        timer->setStyleSheet("color: yellow");
    } else {
        timer->setStyleSheet("color: white");
    }
}

StatusBar::~StatusBar() {
    qtimer->stop();
    delete qtimer;
}
