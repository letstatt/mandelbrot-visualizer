#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <QPoint>
#include <QSize>
#include <QRect>
#include <QThread>

#ifdef AVX
#include <immintrin.h>
#endif

namespace mandelbrot {

struct Pos {
    double x, y;

    Pos() : x(0), y(0) {}
    Pos(QPointF const& p) : x(p.x()), y(p.y()) {}
    Pos(QSizeF const& p) : x(p.width()), y(p.height()) {}

    template <typename T1, typename T2>
    Pos(T1 x, T2 y) : x(x), y(y) {}

    Pos operator+(Pos const& other) const {
        return {x + other.x, y + other.y};
    }

    Pos& operator+=(Pos const& other) {
        return *this = *this + other;
    }

    Pos operator-(Pos const& other) const {
        return {x - other.x, y - other.y};
    }

    Pos& operator-=(Pos const& other) {
        return *this = *this - other;
    }

    Pos operator*(double c) const {
        return {x * c, y * c};
    }

    Pos& operator*=(double c) {
        return *this = *this * c;
    }

    Pos operator/(double c) const {
        return {x / c, y / c};
    }

    Pos& operator/=(double c) {
        return *this = *this / c;
    }

    bool operator==(Pos const& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(Pos const& other) const {
        return !(operator==(other));
    }

    Pos fit(QRectF const& other) const {
        return {
            qBound(other.left(), x, other.right()),
            qBound(other.bottom(), y, other.top())
        };
    }
};

// WIDGETS CONSTANTS
const inline double SCALE_STEP = 0.5;
const inline int WARN_SCALE_LOG = 30; // statusbar stops showing coords properly
const inline int MAX_SCALE_LOG = 45; // double data type starts distort (?)
const inline double WARN_RENDER_LATENCY = 2;

const inline double INITIAL_SCALE = 0.005;
const inline Pos INTIAL_CENTER_OFFSET = {-0.5, 0};

// on win it suprisingly works like f(x0,y0,dx,dy) = {{x0, y0}, {x0 + dx, y0 + dy}}
// but "height" should be > 0 and it should works like {{x0, y0}, {x0 + dx, y0 - dy}}
const inline QRectF ALLOWED_COORDS_RECT = {-3, 2, 6, -4};

// RENDERER CONSTANTS
const inline size_t MAX_THREADS_COUNT = QThread::idealThreadCount();
const inline size_t DOWNSCALE_LEVEL = 4;
const inline size_t MIN_ITERATIONS_BY_PIXEL = 64;
const inline size_t MAX_ITERATIONS_BY_PIXEL = 2048;
const inline size_t DROPPED_FRAME_CHECK_THRESHOLD = 256;
const inline size_t DOWNSCALED_IMAGE_SIZE_MULTIPLIER = 4;

// OPTIMIZATION CONSTANTS
const inline size_t PERIODICITY_CHECK_THRESHOLD = 19;
#ifdef AVX
const inline size_t AVX_APPROXIMATION_STEPS = 1024;
#endif

enum RendererState {
    INITIAL_RENDERING, READY, RENDERING, OFFLINE
};

struct ViewportInfo {
    Pos offset;
    double scaleLog;
    RendererState state;
    size_t frameSeqId;
};

}

#endif // MANDELBROT_H
