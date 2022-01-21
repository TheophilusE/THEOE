//
// Copyright (c) 2020-2022 Theophilus Eriata.
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4244) // Conversion from 'double' to 'float'
    #pragma warning(disable : 4702) // unreachable code
#endif

#include "../Math/Random.h"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <type_traits>

namespace Urho3D
{

#undef M_PI
static constexpr float M_TAU =
    6.2831853071795864769252867665590057683943387987502116419498891846156328125724179972560696506842341359642961730265646132941876892191011644634507188162569622349005682054038770422111192892458979098607639f; // 2 Times PI
static constexpr float M_PI =
    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930382f; // PI
static constexpr float M_HALF_PI =
    1.570796326794896619231321691639751442098584699687552910487472296153908203143104499314017412671058533991074043256641153323546922304775291115862679704064240558725142051350969260552779822311474477465191f; // Half PI (0.5 * PI)
static constexpr float M_GOLDEN_RATIO =
    1.6180339887498948482045868343656381177203091798057628621354486227052604628189024497072072041893911374847540880753868917521266338622235369317931800607667263544333890865959395829056383226613199282902679f; // Golden Ratio (Golden Number) (1 + Sqrt(5)) / 2
static constexpr float M_SQRT_3 =
    1.732050807568877293527446341505872366942805253810380628055806979451933016908800037081146186757248575675626141415406703029969945094998952478811655512094373648528093231902305582067974820101084674923265f; // Theodorus' Constant (Sqrt(3))
static constexpr float M_SQRT_2 =
    1.4142135623730950488016887242096980785696718753769480731766797379907324784621070388503875343276415727350138462309122970249248360558507372126441214970999358314132226659275055927557999505011527820605715f; // Sqrt(2)
static constexpr float M_1_SQRT_3 =
    0.57735026918962576450914878050195745564760175127012687601860232648397767230293334569371539558574952522520871380513556767665664836499965082627055183736479121617603107730076852735599160670036155830775501f; // 1 / Sqrt(3)
static constexpr float M_1_SQRT_2 =
    0.70710678118654752440084436210484903928483593768847403658833986899536623923105351942519376716382078636750692311545614851246241802792536860632206074854996791570661133296375279637789997525057639103028574f; // 1 / Sqrt(2)
static constexpr int M_MIN_INT = 0x80000000;
static constexpr int M_MAX_INT = 0x7fffffff;
static constexpr unsigned M_MIN_UNSIGNED = 0x00000000;
static constexpr unsigned M_MAX_UNSIGNED = 0xffffffff;
static constexpr float M_MAX_FLOAT = std::numeric_limits<float>::max();

static constexpr float M_EPSILON = 0.000001f;
static constexpr float M_LARGE_EPSILON = 0.00005f;
static constexpr float M_MIN_NEARCLIP = 0.01f;
static constexpr float M_MAX_FOV = 160.0f;
static constexpr float M_LARGE_VALUE = 100000000.0f;
static constexpr float M_INFINITY = (float)HUGE_VAL;
static constexpr float M_DEGTORAD = M_PI / 180.0f;
static constexpr float M_DEGTORAD_2 = M_PI / 360.0f; // M_DEGTORAD / 2.f
static constexpr float M_RADTODEG = 1.0f / M_DEGTORAD;

/// Intersection test result.
enum Intersection
{
    OUTSIDE,
    INTERSECTS,
    INSIDE
};

/// Check whether two floating point values are equal within accuracy.
/// @specialization{float}
template <class T> inline bool Equals(T lhs, T rhs, T eps = M_EPSILON) { return lhs + eps >= rhs && lhs - eps <= rhs; }

/// Linear interpolation between two values.
/// @specialization{float,float}
template <class T, class U> inline T Lerp(T lhs, T rhs, U t) { return static_cast<T>(lhs * (1.0 - t) + rhs * t); }

/// Inverse linear interpolation between two values.
/// @specialization{float}
template <class T> inline T InverseLerp(T lhs, T rhs, T x) { return (x - lhs) / (rhs - lhs); }

/// Return the smaller of two values.
/// @specialization{float,float} @specialization{int,int}
template <class T, class U> inline T Min(T lhs, U rhs) { return lhs < rhs ? lhs : rhs; }

/// Return the larger of two values.
/// @specialization{float,float} @specialization{int,int}
template <class T, class U> inline T Max(T lhs, U rhs) { return lhs > rhs ? lhs : rhs; }

/// Return absolute value of a value.
/// @specialization{float}
template <class T> inline T Abs(T value) { return value >= 0.0 ? value : -value; }

/// Return the sign of a float (-1, 0 or 1).
/// @specialization{float}
template <class T> inline T Sign(T value) { return value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0); }

/// Convert degrees to radians.
template <class T> inline T ToRadians(const T degrees) { return M_DEGTORAD * degrees; }

/// Convert radians to degrees.
template <class T> inline T ToDegrees(const T radians) { return M_RADTODEG * radians; }

/// Return a representation of the specified floating-point value as a single format bit layout.
inline unsigned FloatToRawIntBits(float value)
{
    unsigned u = *((unsigned*)&value);
    return u;
}

/// Check whether a floating point value is NaN.
/// @specialization{float} @specialization{double}
template <class T> inline bool IsNaN(T value) { return std::isnan(value); }

/// Check whether a floating point value is positive or negative infinity.
template <class T> inline bool IsInf(T value) { return std::isinf(value); }

/// Clamp a number to a range.
template <class T> inline T Clamp(T value, T min, T max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

// Clamps value between 0 and 1 and returns value.
inline float Clamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;
    else if (value > 1.0f)
        return 1.0f;
    else
        return value;
}

/// Per-component clamp of vector.
template <class T> inline T VectorClamp(const T& value, const T& min, const T& max)
{
    return VectorMax(min, VectorMin(value, max));
}

/// Smoothly damp between values.
/// @specialization{float}
template <class T> inline T SmoothStep(T lhs, T rhs, T t)
{
    t = Clamp((t - lhs) / (rhs - lhs), T(0.0), T(1.0)); // Saturate t
    return t * t * (3.0 - 2.0 * t);
}

/// Return sine of an angle in degrees.
/// @specialization{float}
template <class T> inline T Sin(T angle) { return sin(angle * M_DEGTORAD); }

/// Return cosine of an angle in degrees.
/// @specialization{float}
template <class T> inline T Cos(T angle) { return cos(angle * M_DEGTORAD); }

/// Return tangent of an angle in degrees.
/// @specialization{float}
template <class T> inline T Tan(T angle) { return tan(angle * M_DEGTORAD); }

/// Return arc sine in degrees.
/// @specialization{float}
template <class T> inline T Asin(T x) { return M_RADTODEG * asin(Clamp(x, T(-1.0), T(1.0))); }

/// Return arc cosine in degrees.
/// @specialization{float}
template <class T> inline T Acos(T x) { return M_RADTODEG * acos(Clamp(x, T(-1.0), T(1.0))); }

/// Return arc tangent in degrees.
/// @specialization{float}
template <class T> inline T Atan(T x) { return M_RADTODEG * atan(x); }

/// Return arc tangent of y/x in degrees.
/// @specialization{float}
template <class T> inline T Atan2(T y, T x) { return M_RADTODEG * atan2(y, x); }

/// Return X in power Y.
/// @specialization{float}
template <class T> inline T Pow(T x, T y) { return pow(x, y); }

/// Return natural logarithm of X.
/// @specialization{float}
template <class T> inline T Ln(T x) { return log(x); }

/// Return square root of X.
/// @specialization{float}
template <class T> inline T Sqrt(T x) { return sqrt(x); }

/// Return remainder of X/Y for float values.
template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr> inline T Mod(T x, T y)
{
    return fmod(x, y);
}

/// Return remainder of X/Y for integer values.
template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr> inline T Mod(T x, T y)
{
    return x % y;
}

/// Return always positive remainder of X/Y.
template <class T> inline T AbsMod(T x, T y)
{
    const T result = Mod(x, y);
    return result < 0 ? result + y : result;
}

/// Return fractional part of passed value in range [0, 1).
/// @specialization{float}
template <class T> inline T Fract(T value) { return value - floor(value); }

/// Round value down.
/// @specialization{float}
template <class T> inline T Floor(T x) { return floor(x); }

/// Round value down to nearest number that can be represented as i*y, where i is integer.
template <class T> inline T SnapFloor(T x, T y) { return floor(x / y) * y; }

/// Round value down. Returns integer value.
/// @specialization{float}
template <class T> inline int FloorToInt(T x) { return static_cast<int>(floor(x)); }

/// Round value to nearest integer.
/// @specialization{float}
template <class T> inline T Round(T x) { return round(x); }

#ifndef SWIG
/// Compute average value of the range.
template <class Iterator>
inline auto Average(Iterator begin, Iterator end) -> typename std::decay<decltype(*begin)>::type
{
    using T = typename std::decay<decltype(*begin)>::type;

    T average{};
    unsigned size{};
    for (Iterator it = begin; it != end; ++it)
    {
        average += *it;
        ++size;
    }

    return size != 0 ? average / size : average;
}
#endif

/// Round value to nearest number that can be represented as i*y, where i is integer.
template <class T> inline T SnapRound(T x, T y) { return round(x / y) * y; }

/// Round value to nearest integer.
/// @specialization{float}
template <class T> inline int RoundToInt(T x) { return static_cast<int>(round(x)); }

/// Round value to nearest multiple.
template <class T> inline T RoundToNearestMultiple(T x, T multiple)
{
    T mag = Abs(x);
    multiple = Abs(multiple);
    T remainder = Mod(mag, multiple);
    if (remainder >= multiple / 2)
        return (FloorToInt<T>(mag / multiple) * multiple + multiple) * Sign(x);
    else
        return (FloorToInt<T>(mag / multiple) * multiple) * Sign(x);
}

/// Round value up.
/// @specialization{float}
template <class T> inline T Ceil(T x) { return ceil(x); }

/// Round value up to nearest number that can be represented as i*y, where i is integer.
template <class T> inline T SnapCeil(T x, T y) { return ceil(x / y) * y; }

/// Round value up.
/// @specialization{float}
template <class T> inline int CeilToInt(T x) { return static_cast<int>(ceil(x)); }

/// Check whether an unsigned integer is a power of two.
inline bool IsPowerOfTwo(unsigned value) { return !(value & (value - 1)) && value; }

/// Round up to next power of two.
inline unsigned NextPowerOfTwo(unsigned value)
{
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --value;
    value |= value >> 1u;
    value |= value >> 2u;
    value |= value >> 4u;
    value |= value >> 8u;
    value |= value >> 16u;
    return ++value;
}

/// Round up or down to the closest power of two.
inline unsigned ClosestPowerOfTwo(unsigned value)
{
    const unsigned next = NextPowerOfTwo(value);
    const unsigned prev = next >> 1u;
    return (value - prev) > (next - value) ? next : prev;
}

/// Return log base two or the MSB position of the given value.
inline unsigned LogBaseTwo(unsigned value)
{
    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    unsigned ret = 0;
    while (value >>= 1) // Unroll for more speed...
        ++ret;
    return ret;
}

/// Count the number of set bits in a mask.
inline unsigned CountSetBits(unsigned value)
{
    // Brian Kernighan's method
    unsigned count = 0;
    for (count = 0; value; count++)
        value &= value - 1;
    return count;
}

/// Update a hash with the given 8-bit value using the SDBM algorithm.
inline constexpr unsigned SDBMHash(unsigned hash, unsigned char c) { return c + (hash << 6u) + (hash << 16u) - hash; }

/// Return a random float between 0.0 (inclusive) and 1.0 (exclusive).
inline float Random() { return Rand() / 32768.0f; }

/// Return a random float between 0.0 and range, inclusive from both ends.
inline float Random(float range) { return Rand() * range / 32767.0f; }

/// Return a random float between min and max, inclusive from both ends.
inline float Random(float min, float max) { return Rand() * (max - min) / 32767.0f + min; }

/// Return a random integer between 0 and range - 1.
/// @alias{RandomInt}
inline int Random(int range) { return (int)(Random() * range); }

/// Return a random integer between min and max - 1.
/// @alias{RandomInt}
inline int Random(int min, int max)
{
    auto range = (float)(max - min);
    return (int)(Random() * range) + min;
}

/// Return a random normal distributed number with the given mean value and variance.
inline float RandomNormal(float meanValue, float variance)
{
    return RandStandardNormal() * sqrtf(variance) + meanValue;
}

/// Convert float to half float. From https://gist.github.com/martinkallman/5049614
inline unsigned short FloatToHalf(float value)
{
    unsigned inu = FloatToRawIntBits(value);
    unsigned t1 = inu & 0x7fffffffu; // Non-sign bits
    unsigned t2 = inu & 0x80000000u; // Sign bit
    unsigned t3 = inu & 0x7f800000u; // Exponent

    t1 >>= 13; // Align mantissa on MSB
    t2 >>= 16; // Shift sign bit into position

    t1 -= 0x1c000; // Adjust bias

    t1 = (t3 < 0x38800000) ? 0 : t1; // Flush-to-zero
    t1 = (t3 > 0x47000000) ? 0x7bff : t1; // Clamp-to-max
    t1 = (t3 == 0 ? 0 : t1); // Denormals-as-zero

    t1 |= t2; // Re-insert sign bit

    return (unsigned short)t1;
}

/// Convert half float to float. From https://gist.github.com/martinkallman/5049614
inline float HalfToFloat(unsigned short value)
{
    unsigned t1 = value & 0x7fffu; // Non-sign bits
    unsigned t2 = value & 0x8000u; // Sign bit
    unsigned t3 = value & 0x7c00u; // Exponent

    t1 <<= 13; // Align mantissa on MSB
    t2 <<= 16; // Shift sign bit into position

    t1 += 0x38000000; // Adjust bias

    t1 = (t3 == 0 ? 0 : t1); // Denormals-as-zero

    t1 |= t2; // Re-insert sign bit

    float out;
    *((unsigned*)&out) = t1;
    return out;
}

/// Wrap a value fitting it in the range defined by [min, max)
template <typename T> inline T Wrap(T value, T min, T max)
{
    T range = max - min;
    return min + Mod(value, range);
}

/// Calculate both sine and cosine, with angle in degrees.
URHO3D_API void SinCos(float angle, float& sin, float& cos);

/// Return X in power 2.
template <class T> inline T Square(T x) { return pow(x, 2); }

/// Return the inverse square root of X.
template <class T> inline T InvSqrt(T x) { return 1.f / sqrt(x); }

/// Return the inverse square root of X.
template <class T> inline T FastInvSqrt(T x)
{
    const float threehalfs = 1.5F;

    float x2 = x * 0.5F;
    float y = x;

    // evil floating point bit level hacking
    long i = *(long*)&y;

    // value is pre-assumed
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;

    // 1st iteration
    y = y * (threehalfs - (x2 * y * y));

    // 2nd iteration, this can be removed
    // y = y * ( threehalfs - ( x2 * y * y ) );

    return y;
}

/// Compares two floating point values if they are similar.
inline bool Approximately(float a, float b)
{
    // If a or b is zero, compare that the other is less or equal to epsilon.
    // If neither a or b are 0, then find an epsilon that is good for
    // comparing numbers at the maximum magnitude of a and b.
    // Floating points have about 7 significant digits, so
    // 1.000001f can be represented while 1.0000001f is rounded to zero,
    // thus we could use an epsilon of 0.000001f for comparing values close to 1.
    // We multiply this epsilon by the biggest magnitude of a and b.
    return Abs(b - a) < Max(0.000001f * Max(Abs(a), Abs(b)), M_EPSILON * 8);
}

/// Gradually changes a value towards a desired goal over time.
inline float SmoothDamp(
    float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
{
    // Based on Game Programming Gems 4 Chapter 1.10
    smoothTime = Max(0.0001F, smoothTime);
    float omega = 2.f / smoothTime;

    float x = omega * deltaTime;
    float exp = 1.f / (1.f + x + 0.48F * x * x + 0.235F * x * x * x);
    float change = current - target;
    float originalTo = target;

    // Clamp maximum speed
    float maxChange = maxSpeed * smoothTime;
    change = Clamp(change, -maxChange, maxChange);
    target = current - change;

    float temp = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temp) * exp;
    float output = target + (change + temp) * exp;

    // Prevent overshooting
    if (originalTo - current > 0.0F == output > originalTo)
    {
        output = originalTo;
        currentVelocity = (output - originalTo) / deltaTime;
    }

    return output;
}

/// Loops the value t, so that it is never larger than length and never smaller than 0.
inline float Repeat(float t, float length) { return Clamp(t - Floor(t / length) * length, 0.0f, length); }

/// PingPongs the value t, so that it is never larger than length and never smaller than 0.
inline float PingPong(float t, float length)
{
    t = Repeat(t, length * 2.f);
    return length - Abs(t - length);
}

/// Moves a value /current/ towards /target/.
inline float MoveTowards(float current, float target, float maxDelta)
{
    if (Abs(target - current) <= maxDelta)
        return target;

    return current + Sign(target - current) * maxDelta;
}

/// Calculates the shortest difference between two given angles.
inline float DeltaAngle(float current, float target)
{
    float delta = Repeat((target - current), 360.0F);

    if (delta > 180.0F)
        delta -= 360.0F;

    return delta;
}

// Same as MoveTowards but makes sure the values interpolate correctly when they wrap around 360 degrees.
inline float MoveTowardsAngle(float current, float target, float maxDelta)
{
    float deltaAngle = DeltaAngle(current, target);

    if (-maxDelta < deltaAngle && deltaAngle < maxDelta)
        return target;

    target = current + deltaAngle;

    return MoveTowards(current, target, maxDelta);
}

inline float Gamma(float value, float absmax, float gamma)
{
    bool negative = value < 0.f;
    float absval = Abs(value);

    if (absval > absmax)
        return negative ? -absval : absval;

    float result = Pow(absval / absmax, gamma) * absmax;

    return negative ? -result : result;
}

// Same as Lerp but makes sure the values interpolate correctly when they wrap around 360 degrees.
inline float LerpAngle(float a, float b, float t)
{
    float delta = Repeat((b - a), 360);

    if (delta > 180)
        delta -= 360;

    return a + delta * Clamp01(t);
}

/// Return the hyponense given a X and Y term.
inline float Hypotenuse(float x, float y) { return Sqrt(Square(x) + Square(y)); }

/// Calculates a value between 0 and 1.
inline float Normalize(double value, double min, double max) { return 1 - ((value - min) / (max - min)); }
} // namespace Urho3D

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
