#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <mandelbrot.h>

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);

    ~StatusBar();

public slots:
    void updateInfo(mandelbrot::ViewportInfo);
    void updateTimer();

private:
    QLabel* status = nullptr;
    QLabel* timer = nullptr;
    QLabel* coords = nullptr;
    QLabel* scale = nullptr;

    mandelbrot::RendererState prevRendererState;
    size_t prevFrameSeqId = 0;
    QTimer* qtimer;
    double time;
};

#endif // STATUSBAR_H
