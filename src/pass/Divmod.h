#ifndef HALIDE_DIVMOD_H
#define HALIDE_DIVMOD_H

#include <cmath>

namespace Halide {
namespace Internal {
    
/** Implementations of division and mod that are specific to Halide.
 * Use these implementations; do not use native C division or mod to
 * simplify Halide expressions. Halide division and modulo satisify
 * the Euclidean definition of division for integers a and b:
 *
 /code
 (a/b)*b + a%b = a
 0 <= a%b < |b|
 /endcode
 *
 */
// @{
template<typename T>
inline T mod_imp(T a, T b) {
    Type t = type_of<T>();
    if (t.is_int()) {
        T r = a % b;
        r = r + (r < 0 ? (T)std::abs((int64_t)b) : 0);
        return r;
    } else {
        return a % b;
    }
}

template<typename T>
inline T div_imp(T a, T b) {
    Type t = type_of<T>();
    if (t.is_int()) {
        int64_t q = a / b;
        int64_t r = a - q * b;
        int64_t bs = b >> (t.bits() - 1);
        int64_t rs = r >> (t.bits() - 1);
        return q - (rs & bs) + (rs & ~bs);
    } else {
        return a / b;
    }
}
// @}

// Special cases for float, double.
template<> inline float mod_imp<float>(float a, float b) {
    float f = a - b * (floorf(a / b));
    // The remainder has the same sign as b.
    return f;
}
template<> inline double mod_imp<double>(double a, double b) {
    double f = a - b * (std::floor(a / b));
    return f;
}

template<> inline float div_imp<float>(float a, float b) {
    return a/b;
}
template<> inline double div_imp<double>(double a, double b) {
    return a/b;
}

}
}

#endif
