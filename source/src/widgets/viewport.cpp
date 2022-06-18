#include "viewport.h"
#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <cmath>
#include <QFileDialog>
#include <QMessageBox>

Viewport::Viewport(QWidget* parent)
    : QWidget(parent) {
    connect(&renderer,
            SIGNAL(frameDelivery(QImage,bool,size_t)),
            this,
            SLOT(updateFrame(QImage,bool,size_t)),
            Qt::BlockingQueuedConnection);
}

bool Viewport::ready() const {
    return (!detailedFrame.isNull() || !downscaledFrame.isNull())
            && rendererState != mandelbrot::RendererState::INITIAL_RENDERING;
}

void Viewport::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(0, 0, width(), height(), Qt::black);

    if (!ready()) {
        const static QString rendering = "Rendering...";
        const static QString offline = "Frame reset happened. Switch to online mode to continue";

        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, getOffline() ? offline : rendering);
    } else {
        downscaledFrame.draw(p);

        // do not draw detailed frame if lowResolution or zoomed out
        if (!lowResolution && detailedFrame.scale >= 1) {
            detailedFrame.draw(p);
        }
    }
}

void Viewport::resizeEvent(QResizeEvent*) {
    // we can't drag frames correctly using qresizeevent, so drop all frames
    // note, that update() will be called after resizeEvent by Qt itself
    downscaledFrame.reset();
    detailedFrame.reset();
    requestFrame();
}

/*
 * Every drag movement moves plane, not viewport!
 * Note, that until we have got newly rendered frame, we use temporary
 * frame offset (frameDragOffset/frameScaleOffset), not global centerOffset.
 *
 * centerOffset - difference between coordinate center and center of viewport.
 */

void Viewport::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        prevDragPos = event->pos();
    }
}

void Viewport::mouseMoveEvent(QMouseEvent* event) {
    // if not entered into drag mode correctly, then do not handle
    if (prevDragPos.isNull()) {
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        move(event->pos() - prevDragPos, true, false);
        prevDragPos = event->pos();
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent* event) {
    // if not entered into drag mode correctly, then do not handle
    if (prevDragPos.isNull()) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        move(event->pos() - prevDragPos);
        prevDragPos = QPointF();
    }
}

void Viewport::wheelEvent(QWheelEvent* event) {
    // official docs constant
    double steps = event->angleDelta().y() / 120.;
    zoom(event->position(), steps);
}

bool Viewport::getOffline() const {
    return rendererState == mandelbrot::RendererState::OFFLINE;
}

bool Viewport::getCursorDependentZoom() const {
    return cursorDependentZoom;
}

bool Viewport::getLowResolution() const {
    return lowResolution;
}

mandelbrot::RendererSettings Viewport::getRendererSettings() const {
    return renderer.getSettings();
}

void Viewport::setOffline(bool offline) {
    if (offline) {
        rendererState = mandelbrot::RendererState::OFFLINE;
    } else {
        rendererState = mandelbrot::RendererState::READY;
    }
    requestFrame();
    update();
}

void Viewport::setCursorDependentZoom(bool val) {
    cursorDependentZoom = val;
}

void Viewport::setLowResolution(bool val) {
    lowResolution = val;
    if (!val) {
        requestFrame();
    }
}

void Viewport::setRendererSettings(mandelbrot::RendererSettings settings) {
    renderer.setSettings(settings);
}

void Viewport::move(QPointF pixels, bool update, bool requestFrame) {
    if (!ready()) {
        return;
    }

    using namespace mandelbrot;
    Pos diff = pixels * scale;
    Pos allowed = (centerOffset - diff).fit(ALLOWED_COORDS_RECT) - centerOffset;

    // offline
    downscaledFrame.drag(-QPointF(allowed.x, allowed.y) / scale);
    detailedFrame.drag(-QPointF(allowed.x, allowed.y) / scale);

    // online
    centerOffset += allowed;

    if (update) {
        this->update();
        broadcastWidgetInfo();
    }

    if (requestFrame) {
        this->requestFrame();
    }
}

void Viewport::zoom(QPointF mousePos, double steps) {
    if (!ready()) {
        return;
    }

    using namespace mandelbrot;
    double allowedScaleLog = qBound(1., scaleLog + steps, (double) MAX_SCALE_LOG);
    double dy = allowedScaleLog - scaleLog;

    // i wanted to implement delaying on zooming/moving due to being close to bounds
    // and waiting for rendering, but it is not necessary maybe

    if (dy != 0) {
        double factor = std::pow(SCALE_STEP, dy);

        QPointF viewportCenter = QPointF(width(), height()) / 2.0;
        QPointF newCenterOffset = viewportCenter * factor;
        QPointF oldCenterOffset = viewportCenter;

        // offline
        downscaledFrame.drag(newCenterOffset - oldCenterOffset);
        detailedFrame.drag(newCenterOffset - oldCenterOffset);

        if (cursorDependentZoom) {
            QPointF centerDiff = (mousePos - viewportCenter) * (factor - 1);
            move(centerDiff, false, false);
        }

        // offline
        downscaledFrame.zoom(factor);
        detailedFrame.zoom(factor);

        // online
        scale *= factor;
        scaleLog = allowedScaleLog;

        update();

        // do not request new frame while pressing mouse
        if (!prevDragPos.isNull()) {
            broadcastWidgetInfo();
        } else {
            requestFrame();
        }
    }
}

bool Viewport::screenshot() {
    if (!ready()) {
        return false;
    }

    const static QString caption = "Export screenshot";
    const static QString filter = "Images (*.png)";

    bool prev = getOffline();
    setOffline(true);

    // nobody can repaint the widget now, so we render it safely.

    QString fileName = QFileDialog::getSaveFileName(this, caption, QString(), filter);

    if (!fileName.isEmpty()) {
        QPixmap screenshot(size());
        render(&screenshot, QPoint(), QRegion(), RenderFlags());
        screenshot.save(fileName, "PNG");
    }

    setOffline(prev);
    return true;
}

void Viewport::reset() {
    using namespace mandelbrot;

    // make sure reset is not useless
    bool check = (scale != INITIAL_SCALE) || (centerOffset != INTIAL_CENTER_OFFSET);
    check |= downscaledFrame.changed();

    if (check) {
        scale = INITIAL_SCALE;
        centerOffset = INTIAL_CENTER_OFFSET;
        scaleLog = 1.;

        prevDragPos = QPoint(); // stops drag mode
        downscaledFrame.reset();
        detailedFrame.reset();

        update();
        requestFrame();
    }
}

void Viewport::updateFrame(QImage frame, bool downscaled, size_t frameSeqId) {
    // discard previous frames. yes, it can happen.
    if (frameSeqId != this->frameSeqId) {
        return;
    }

    if (!getOffline()) {
        if (downscaled) {
            if (!lowResolution) {
                delayedFrame = QPixmap::fromImage(frame);
            } else {
                downscaledFrame.setPixmap(QPixmap::fromImage(frame));
                downscaledFrame.restore();
            }

        } else {
            detailedFrame.setPixmap(QPixmap::fromImage(frame));
            detailedFrame.restore();

            if (!delayedFrame.isNull()) {
                downscaledFrame.setPixmap(delayedFrame);
                downscaledFrame.restore();
                delayedFrame = QPixmap();
            }
        }

        if (lowResolution || !downscaled) {
            rendererState = mandelbrot::RendererState::READY;
        }
        update();
    }

    broadcastWidgetInfo();
}

void Viewport::requestFrame() {
    if (getOffline()) {
        broadcastWidgetInfo();
        return;
    }

    if (downscaledFrame.isNull() && detailedFrame.isNull()) {
        rendererState = mandelbrot::RendererState::INITIAL_RENDERING;
    } else if (downscaledFrame.changed() || (detailedFrame.changed() && !lowResolution)) {
        rendererState = mandelbrot::RendererState::RENDERING;
    } else {
        rendererState = mandelbrot::RendererState::READY;
        broadcastWidgetInfo();
        return;
    }

    ++frameSeqId;
    broadcastWidgetInfo();

    downscaledFrame.save();
    detailedFrame.save();

    renderer.request(frameSeqId, centerOffset, size(), scale, scaleLog, lowResolution);
}

void Viewport::broadcastWidgetInfo() {
    emit widgetInfoDelivery({centerOffset, scaleLog, rendererState, frameSeqId});
}
