#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <renderer.h>
#include <QPainter>

namespace mandelbrot {

struct Frame {
    QPixmap frame;
    QPointF dragOffset;
    double scale = 1;

    QPointF savedDragOffset;
    double savedScale = 1;

    // call during requestFrame
    void save() {
        savedDragOffset = dragOffset;
        savedScale = scale;
    }

    // call after updateFrame
    void restore() {
        dragOffset -= savedDragOffset;
        dragOffset *= savedScale;
        scale /= savedScale;
        savedDragOffset = QPointF();
        savedScale = 1;
    }

    void setPixmap(QPixmap frame) {
        this->frame = frame;
    }

    void drag(QPointF vec) {
        dragOffset += vec / scale;
    }

    void zoom(double factor) {
        scale /= factor;
    }

    bool isNull() const {
        return frame.isNull();
    }

    bool changed() const {
        return !dragOffset.isNull() || (scale != 1) || isNull();
    }

    void draw(QPainter& p) {
        auto diff = QSizeF(frame.size() - p.window().size()) / 2.;
        auto vec = dragOffset - QPointF(diff.width(), diff.height());

        p.save();
        p.scale(scale, scale);
        p.drawPixmap(vec, frame);
        p.restore();
    }

    void reset() {
        frame = QPixmap();
        dragOffset = QPointF();
        scale = 1;
        savedDragOffset = QPointF();
        savedScale = 1;
    }
};

}

class Viewport : public QWidget {

  Q_OBJECT
public:
    Viewport(QWidget *parent = 0);
    bool ready() const;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    bool getOffline() const;
    bool getCursorDependentZoom() const;
    bool getLowResolution() const;
    mandelbrot::RendererSettings getRendererSettings() const;

    void setOffline(bool);
    void setCursorDependentZoom(bool);
    void setLowResolution(bool);
    void setRendererSettings(mandelbrot::RendererSettings);

    void move(QPointF, bool = true, bool = true);
    void zoom(QPointF, double);
    bool screenshot();
    void reset();

signals:
    void widgetInfoDelivery(mandelbrot::ViewportInfo);

private slots:
     void updateFrame(QImage, bool, size_t);

private:
     void requestFrame();
     void broadcastWidgetInfo();

    // online-render options
    double scale = mandelbrot::INITIAL_SCALE;
    mandelbrot::Pos centerOffset = mandelbrot::INTIAL_CENTER_OFFSET;
    double scaleLog = 1;

    // offline-render options
    mandelbrot::Frame downscaledFrame;
    mandelbrot::Frame detailedFrame;
    QPixmap delayedFrame;
    QPointF prevDragPos;

    // viewport options
    bool cursorDependentZoom = true;
    bool lowResolution = false;

    // render features
    mandelbrot::RendererState rendererState;
    Renderer renderer;
    size_t frameSeqId = 0;
};

#endif // VIEWPORT_H
