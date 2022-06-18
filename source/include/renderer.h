#ifndef RENDERER_H
#define RENDERER_H

#include <QFrame>
#include <mutex>
#include <mandelbrot.h>

// forward declaration
class Renderer;

namespace mandelbrot {

struct RendererSettings {
    size_t threadsCount = MAX_THREADS_COUNT;
    size_t iterationsCount;
    bool threadsCountAuto = true;
    bool iterationsCountAuto = true;
};

struct WorkerSettings : RendererSettings {
    WorkerSettings() = default;
    WorkerSettings(RendererSettings const& rs) : RendererSettings(rs) {}

    size_t frameSeqId;
    QSize originalSize;
    QSize size;
    Pos offset;
    Pos c; // see Mandelbrot's formula
    double scale;
    double scaleLog;
    bool lowResolutionOnly;
    double EPS;
};

}

class Renderer : public QThread
{
    Q_OBJECT
public:
    Renderer();

    void request(size_t, mandelbrot::Pos, QSize, double, double, bool);
    void stop();

    mandelbrot::RendererSettings getSettings() const;
    void setSettings(mandelbrot::RendererSettings);
    size_t iterationsCountAuto(size_t) const;
    size_t threadsCountAuto() const;

    ~Renderer();

signals:
    void frameDelivery(QImage, bool, size_t);

protected:
    void run() override;

private:
    // helpers
    void runWorkers(void(Renderer::*)(size_t, size_t), size_t, bool);
    QRgb color(size_t);

    // workers
    void workerImprecise(size_t, size_t);
    void workerPrecise(size_t, size_t);

    // well, I don't know why, but static functions work faster.
    static size_t approxStepsPower2(mandelbrot::Pos, size_t, mandelbrot::Pos, size_t, double);
#ifdef AVX
    static size_t approxStepsPower2AVX(__m256d&, __m256d&, size_t);
#endif

    std::atomic<mandelbrot::RendererSettings> settings;
    std::atomic<mandelbrot::WorkerSettings> requested;
    mandelbrot::WorkerSettings current;

    // external control
    std::atomic_bool dropFrame = false;
    std::atomic_bool shutdown = false;

    // something necessary
    std::mutex mutex;
    std::condition_variable cv;

    QImage buffer;
    std::vector<std::thread> threads{mandelbrot::MAX_THREADS_COUNT};
};

#endif // RENDERER_H
