#include "mandelbrot.h"
#include <QDebug>

namespace mandelbrot {

size_t approxSteps_power2(Pos c, const double EPS, size_t iterationsCount) {
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
    // let Re(z[n]) = z[n]_r, Im(z[n]) = z[n]_i

    // z[n+1] = z[n]^2 + c = (z[n]_r + z[n]_i*i)^2 + (c_r + c_i*i) =
    // = (z[n]_r^2 - z[n]_i^2 + c_r) + (2 * z[n]_r * z[n]_i + c_i)*i

    // let z[n]_r_sqr = z[n]_r^2
    // let z[n]_i_sqr = z[n]_i^2

    // so we've got:

    // z[n+1]_r = z[n]_r_sqr - z[n]_i_sqr + c_r
    // z[n+1]_i = 2 * z[n]_r * z[n]_i + c_i
    // z[n+1]_r_sqr = z[n+1]_r^2
    // z[n+1]_i_sqr = z[n+1]_i^2
    // |z[n+1]|^2 = z[n+1]_r_sqr + z[n+1]_i_sqr

    // let's also check if the calculating point is near to its attractor

    double z_r_old = 0;
    double z_i_old = 0;
    size_t period = 0;

    double z_r_sqr = 0;
    double z_i_sqr = 0;
    double z_r = 0;
    double z_i = 0;

    for (size_t i = 0; i < iterationsCount; ++i) {
        if (z_r_sqr + z_i_sqr >= 4.) {
            return i; // outside
        }

        double z_r_tmp = z_r_sqr - z_i_sqr + c.x;
        z_i = (z_r + z_r) *  z_i + c.y;
        z_r = z_r_tmp;
        z_r_sqr = z_r * z_r;
        z_i_sqr = z_i * z_i;

        if (abs(z_i - z_i_old) < EPS && abs(z_r - z_r_old) < EPS) {
            return 0; // if not outside, but near attractor, then inside
        }

        ++period;
        if (period > PERIODICITY_CHECK_THRESHOLD) {
            period = 0;
            z_i_old = z_i;
            z_r_old = z_r;
        }
    }
    return 0; // inside
}


size_t approxSteps_power2(double z_i, double z_r, size_t initialSteps, Pos c, const double EPS, size_t iterationsCount) {
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
    // let Re(z[n]) = z[n]_r, Im(z[n]) = z[n]_i

    // z[n+1] = z[n]^2 + c = (z[n]_r + z[n]_i*i)^2 + (c_r + c_i*i) =
    // = (z[n]_r^2 - z[n]_i^2 + c_r) + (2 * z[n]_r * z[n]_i + c_i)*i

    // let z[n]_r_sqr = z[n]_r^2
    // let z[n]_i_sqr = z[n]_i^2

    // so we've got:

    // z[n+1]_r = z[n]_r_sqr - z[n]_i_sqr + c_r
    // z[n+1]_i = 2 * z[n]_r * z[n]_i + c_i
    // z[n+1]_r_sqr = z[n+1]_r^2
    // z[n+1]_i_sqr = z[n+1]_i^2
    // |z[n+1]|^2 = z[n+1]_r_sqr + z[n+1]_i_sqr

    // let's also check if the calculating point is near to its attractor

    double z_r_old = 0;
    double z_i_old = 0;
    size_t period = 0;

    double z_r_sqr = z_r * z_r;
    double z_i_sqr = z_i * z_i;

    for (size_t i = initialSteps; i < iterationsCount; ++i) {
        if (z_r_sqr + z_i_sqr >= 4.) {
            return i; // outside
        }

        double z_r_tmp = z_r_sqr - z_i_sqr + c.x;
        z_i = (2 * z_r) *  z_i + c.y;
        z_r = z_r_tmp;
        z_r_sqr = z_r * z_r;
        z_i_sqr = z_i * z_i;

        if (abs(z_i - z_i_old) < EPS && abs(z_r - z_r_old) < EPS) {
            return 0; // if not outside, but near attractor, then inside
        }

        ++period;
        if (period > PERIODICITY_CHECK_THRESHOLD) {
            period = 0;
            z_i_old = z_i;
            z_r_old = z_r;
        }
    }
    return 0; // inside
}

}
