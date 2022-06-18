#include "renderer.h"
#include <QDebug>

Renderer::Renderer() = default;

/*
 * Note, that QThread shouldn't have slots (following the docs)
 * Instead, use mutex to make Viewport -> renderer communication.
 * But renderer -> Viewport signal-slots mechanism works.
*/

void Renderer::request(size_t frameSeqId, mandelbrot::Pos offset, QSize size, double scale, double scaleLog, bool lowResOnly) {
    using namespace mandelbrot;

    WorkerSettings ws = settings.load(std::memory_order_acquire); // implicit conversion
    ws.offset = offset;
    ws.originalSize = size;
    ws.scale = scale;
    ws.scaleLog = scaleLog;
    ws.frameSeqId = frameSeqId;
    ws.lowResolutionOnly = lowResOnly;
    ws.EPS = std::min(ws.scale, 1e-3);
    if (ws.iterationsCountAuto) {
        ws.iterationsCount = iterationsCountAuto(ws.scaleLog);
    }
    requested.store(ws, std::memory_order_release);

    if (!isRunning()) {
        shutdown.store(false, std::memory_order_relaxed);
        dropFrame.store(false, std::memory_order_release);

        // because when workers eat all CPUs, GUI thread lose responsiveness
        start(QThread::LowPriority);
    } else {
        dropFrame.store(true, std::memory_order_release);
        cv.notify_one();
     }
}

void Renderer::stop() {
    shutdown.store(true, std::memory_order_relaxed);
    dropFrame.store(true, std::memory_order_release);
    cv.notify_one();
}

mandelbrot::RendererSettings Renderer::getSettings() const {
    auto tmp = settings.load(std::memory_order_acquire);
    if (tmp.iterationsCountAuto) {
        tmp.iterationsCount = requested.load(std::memory_order_acquire).iterationsCount;
    }
    return tmp;
}

void Renderer::setSettings(mandelbrot::RendererSettings rs) {
    using namespace mandelbrot;

    if (rs.threadsCountAuto) {
        rs.threadsCount = threadsCountAuto();
    }

    rs.threadsCount = qBound((size_t) 1, rs.threadsCount, MAX_THREADS_COUNT);
    rs.iterationsCount = qBound(MIN_ITERATIONS_BY_PIXEL, rs.iterationsCount, MAX_ITERATIONS_BY_PIXEL);
    settings.store(rs, std::memory_order_release);
}

size_t Renderer::iterationsCountAuto(size_t scaleLog) const {
    using namespace mandelbrot;
    return qBound(MIN_ITERATIONS_BY_PIXEL, (size_t) floor(30 * scaleLog), MAX_ITERATIONS_BY_PIXEL);
}

size_t Renderer::threadsCountAuto() const {
    using namespace mandelbrot;
    return std::min((size_t) QThread::idealThreadCount(), MAX_THREADS_COUNT);
}

void Renderer::run() {
    using namespace mandelbrot;

    while(!shutdown.load(std::memory_order_relaxed)) {
        current = requested.load(std::memory_order_acquire);

        runWorkers(&Renderer::workerImprecise, DOWNSCALED_IMAGE_SIZE_MULTIPLIER, true);
        if (!current.lowResolutionOnly && !dropFrame.load(std::memory_order_acquire)) {
            runWorkers(&Renderer::workerPrecise, 1, false);
        }

        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!dropFrame.load(std::memory_order_acquire)) {
                cv.wait(lock);
            }
            dropFrame.store(false);
        }
    }

    for (size_t i = 0; i < MAX_THREADS_COUNT; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}

void Renderer::runWorkers(void(Renderer::*worker)(size_t, size_t), size_t sizeMultiplier, bool downscaled) {
    // we don't actually use alpha channel. 32-bit is only for suitable alignment.
    current.size = current.originalSize * sizeMultiplier;
    buffer = QImage(current.size, QImage::Format_RGB32);

    current.c = {-current.size.width() / 2., -current.size.height() / 2.};
    // assume doing c += (x, y) in future
    current.c *= current.scale;
    current.c += current.offset;

    // ceiling and aligning
    size_t stripHeight = (current.size.height() + current.threadsCount - 1) / current.threadsCount;
    stripHeight += mandelbrot::DOWNSCALE_LEVEL - (stripHeight % mandelbrot::DOWNSCALE_LEVEL);

    for (size_t i = 0, from = 0; i < current.threadsCount; ++i, from += stripHeight) {
        size_t to = std::min(from + stripHeight, (size_t) current.size.height());
        threads[i] = std::thread(worker, this, from, to);
    }

    for (size_t i = 0; i < current.threadsCount; ++i) {
        threads[i].join();
    }

    if (!dropFrame.load(std::memory_order_acquire)) {
        // this emit is blocking.
        emit frameDelivery(buffer, downscaled, current.frameSeqId);
    }
}

QRgb Renderer::color(size_t steps) {
    return qRgb(static_cast<unsigned char>(steps * 255. / current.iterationsCount), 0, 0);
}

void Renderer::workerImprecise(size_t y0, size_t y1) {
    using namespace mandelbrot;

    QRgb* data = reinterpret_cast<QRgb*>(buffer.bits()) + y0 * buffer.width();
    size_t pixelsCnt = 0;

    const double downscaleOffset = DOWNSCALE_LEVEL * 0.5;
    size_t newLineTransition = (DOWNSCALE_LEVEL - 1) * buffer.width();

    for (size_t y = y0, y_next; y != y1; y = y_next) {

        y_next = std::min(y + DOWNSCALE_LEVEL, y1);

        size_t downscaledPixelHeight = buffer.width() * (y_next - y);

        for (size_t x = 0, x_next; x != (size_t) buffer.width(); x = x_next) {
            // do not use AVX for painting downscaled image because
            // my motivation was killed

            auto offset = Pos(x + downscaleOffset, y + downscaleOffset);
            auto probePoint = current.c + offset * current.scale;
            auto steps = approxStepsPower2(
                        {0, 0},
                        0,
                        probePoint,
                        current.iterationsCount,
                        current.EPS);
            auto pixel = color(steps);

            x_next = std::min(x + DOWNSCALE_LEVEL, (size_t) buffer.width());

#ifdef AVX
            // fill downscaleLevel^2 real pixels by calculated color
            // using some intrinsics (or not using them)

            alignas(16) const unsigned int pixels[] = {pixel, pixel, pixel, pixel};
            __m128i const* vec = reinterpret_cast<__m128i const*>(pixels);
#endif
            for (size_t i = y, j = x; i != y_next; ++i, j = x) {
#ifdef AVX
                for (; j + 4 <= x_next; j += 4, data += 4) {
                    _mm_store_si128(reinterpret_cast<__m128i*>(data), *vec);
                }
#endif
                for (; j < x_next; ++j) {
                    *data++ = pixel;
                }

                // paint downscaled pixel line by line
                data += buffer.width() - x_next + x;
            }

            // return pointer to the next downscaled pixel in a row
            data += (x_next - x) - downscaledPixelHeight;

            ++pixelsCnt;
            if (pixelsCnt >= DROPPED_FRAME_CHECK_THRESHOLD) {
                if (shutdown.load(std::memory_order_relaxed) || dropFrame.load(std::memory_order_relaxed)) {
                    return;
                }
                pixelsCnt = 0;
            }
        }

        // go to the next line
        data += newLineTransition;
    }
}

void Renderer::workerPrecise(size_t y0, size_t y1) {
    using namespace mandelbrot;

    QRgb* imgData = reinterpret_cast<QRgb*>(buffer.bits()) + y0 * buffer.width();
    size_t pixelsCnt = 0;

#ifdef AVX
    alignas(32) Pos probePoints[4];

    const size_t iterationsCountAVX =
            std::min(current.iterationsCount, AVX_APPROXIMATION_STEPS);

    for (size_t y = y0; y != y1; ++y) {
        for (size_t x = 0; x < (size_t) buffer.width(); x += 4) {
            __m256d c_r;
            __m256d c_i;

            for (size_t i = 0; i < 4; ++i) {
                auto offset = Pos(x + i + 0.5, y + 0.5);
                probePoints[i] = current.c + offset * current.scale;
                c_r[i] = probePoints[i].x;
                c_i[i] = probePoints[i].y;
            }

            size_t initialSteps = approxStepsPower2AVX(c_r, c_i, iterationsCountAVX);
            // c_r and c_i were updated

            for (int i = 0; i < 4 && x+i < (size_t) buffer.width(); ++i) {
                auto steps = approxStepsPower2(
                            {c_r[i], c_i[i]},
                            initialSteps,
                            probePoints[i],
                            current.iterationsCount,
                            current.EPS);
                *imgData++ = color(steps);
            }

            pixelsCnt += 4;
            if (pixelsCnt >= DROPPED_FRAME_CHECK_THRESHOLD) {
                if (shutdown.load(std::memory_order_relaxed) || dropFrame.load(std::memory_order_relaxed)) {
                    return;
                }
                pixelsCnt = 0;
            }
        }
    }
#else
    for (size_t y = y0; y != y1; ++y) {
        for (size_t x = 0; x != (size_t) buffer.width(); ++x) {
            auto offset = Pos(x + 0.5, y + 0.5);
            auto probePoint = current.c + offset * current.scale;
            auto steps = approxStepsPower2(
                        {0, 0},
                        0,
                        probePoint,
                        current.iterationsCount,
                        current.EPS);
            *imgData++ = color(steps);

            ++pixelsCnt;
            if (pixelsCnt >= DROPPED_FRAME_CHECK_THRESHOLD) {
                if (shutdown.load(std::memory_order_relaxed) || dropFrame.load(std::memory_order_relaxed)) {
                    return;
                }
                pixelsCnt = 0;
            }
        }
    }
#endif
}

#ifdef AVX
size_t Renderer::approxStepsPower2AVX(__m256d & c_r, __m256d & c_i, size_t iterationsCount) {
    const /*thread_local*/ static __m256d radius = {4., 4., 4., 4.};
    __m256d z_i = _mm256_setzero_pd();
    __m256d z_r = _mm256_setzero_pd();
    size_t i = 0;

    for (; i < iterationsCount; ++i) {
        __m256d z_i_sqr = _mm256_mul_pd(z_i, z_i);
        __m256d z_r_sqr = _mm256_mul_pd(z_r, z_r);
        __m256d check = _mm256_add_pd(z_r_sqr, z_i_sqr);

        __m256d res = _mm256_cmp_pd(check, radius, _CMP_NLT_UQ);
        if (_mm256_movemask_pd(res) != 0) {
            break; // if check[j] >= radius[j]: break
        }

        __m256d z_r_tmp = _mm256_add_pd(_mm256_sub_pd(z_r_sqr, z_i_sqr), c_r);
        z_i = _mm256_fmadd_pd(_mm256_add_pd(z_r, z_r), z_i, c_i);
        z_r = z_r_tmp;
    }

    c_r = z_r;
    c_i = z_i;
    return i;
}
#endif

size_t Renderer::approxStepsPower2(mandelbrot::Pos z, size_t initialSteps, mandelbrot::Pos c, size_t iterationsCount, double EPS) {
    using namespace mandelbrot;

    // welcome optimizations

    // cardioid check
    // this optimization helps to speed up the launch by 60 times.
    // it covers major part of mandelbrot, but absolutely useless
    // when moving away from the cardioid part of mandelbrot.
    // (replaced by periodicity checking)

    // one of reviewed optimization was hyperbolic components check
    // that included recursive calculating derivative
    // unfortunately, it has worked on thin edge only (< 10% pixels covered)
    // providing sometimes 2 times slower computing
    // it is also can't be used with iterations counting (no coloring)
    // i may be wrong.

    // basic check looks like this:

    /*std::complex<double> z = 0;
    for (size_t i = 0; i < iterationsCount; ++i) {
        if (z.imag() * z.imag() + z.real() * z.real() >= 4.) {
            return i; // outside
        }
        z = z * z + c;
    }
    return 0;*/

    // let's reduce muls count
    // let's also check if the calculating point is in a period set or converge

    Pos z_old = z;
    Pos z_sqr = {z.x * z.x, z.y * z.y};
    size_t period = 0;

    for (size_t i = initialSteps; i < iterationsCount; ++i) {
        if (z_sqr.x + z_sqr.y >= 4.) {
            return i; // outside
        }

        Pos z_new;
        z_new.x = z_sqr.x - z_sqr.y + c.x;
        z_new.y = (2 * z.x) * z.y + c.y;

        z = z_new;
        z_sqr = {z.x * z.x, z.y * z.y};

        if (abs(z.x - z_old.x) < EPS && abs(z.y - z_old.y) < EPS) {
            return iterationsCount; // if not outside, but converges, then inside
        }

        ++period;
        if (period > PERIODICITY_CHECK_THRESHOLD) {
            period = 0;
            z_old = z;
        }
    }
    return iterationsCount; // inside
}

Renderer::~Renderer() {
    stop();
    wait();
}
