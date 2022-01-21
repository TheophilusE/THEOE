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

#pragma once

#include "../Math/MathDefs.h"
#include "../Math/Vector2.h"

namespace Urho3D
{

/// Three-dimensional vector with integer values.
class URHO3D_API IntVector3
{
public:
    /// Construct a zero vector.
    IntVector3() noexcept
        : x_(0)
        , y_(0)
        , z_(0)
    {
    }

    /// Construct from coordinates.
    IntVector3(int x, int y, int z) noexcept
        : x_(x)
        , y_(y)
        , z_(z)
    {
    }

    /// Construct from an int array.
    explicit IntVector3(const int data[]) noexcept
        : x_(data[0])
        , y_(data[1])
        , z_(data[2])
    {
    }

    /// Copy-construct from another vector.
    IntVector3(const IntVector3& rhs) noexcept = default;

    /// Assign from another vector.
    IntVector3& operator=(const IntVector3& rhs) noexcept = default;

    /// Test for equality with another vector.
    bool operator==(const IntVector3& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_; }

    /// Test for inequality with another vector.
    bool operator!=(const IntVector3& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_ || z_ != rhs.z_; }

    /// Add a vector.
    IntVector3 operator+(const IntVector3& rhs) const { return IntVector3(x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_); }

    /// Return negation.
    IntVector3 operator-() const { return IntVector3(-x_, -y_, -z_); }

    /// Subtract a vector.
    IntVector3 operator-(const IntVector3& rhs) const { return IntVector3(x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_); }

    /// Multiply with a scalar.
    IntVector3 operator*(int rhs) const { return IntVector3(x_ * rhs, y_ * rhs, z_ * rhs); }

    /// Multiply with a vector.
    IntVector3 operator*(const IntVector3& rhs) const { return IntVector3(x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_); }

    /// Divide by a scalar.
    IntVector3 operator/(int rhs) const { return IntVector3(x_ / rhs, y_ / rhs, z_ / rhs); }

    /// Divide by a vector.
    IntVector3 operator/(const IntVector3& rhs) const { return IntVector3(x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_); }

    /// Add-assign a vector.
    IntVector3& operator+=(const IntVector3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    IntVector3& operator-=(const IntVector3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    IntVector3& operator*=(int rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    IntVector3& operator*=(const IntVector3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    IntVector3& operator/=(int rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        z_ /= rhs;
        return *this;
    }

    /// Divide-assign a vector.
    IntVector3& operator/=(const IntVector3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Return integer data.
    const int* Data() const { return &x_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return 2D vector (z component is ignored).
    IntVector2 ToVector2() const { return {x_, y_}; }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return (unsigned)x_ * 31 * 31 + (unsigned)y_ * 31 + (unsigned)z_; }

    /// Return length.
    float Length() const { return sqrtf((float)(x_ * x_ + y_ * y_ + z_ * z_)); }

    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;
    /// Z coordinate.
    int z_;

    /// Zero vector.
    static const IntVector3 ZERO;
    /// (-1,0,0) vector.
    static const IntVector3 LEFT;
    /// (1,0,0) vector.
    static const IntVector3 RIGHT;
    /// (0,1,0) vector.
    static const IntVector3 UP;
    /// (0,-1,0) vector.
    static const IntVector3 DOWN;
    /// (0,0,1) vector.
    static const IntVector3 FORWARD;
    /// (0,0,-1) vector.
    static const IntVector3 BACK;
    /// (1,1,1) vector.
    static const IntVector3 ONE;
};

/// Three-dimensional vector.
class URHO3D_API Vector3
{
public:
    /// Construct a zero vector.
    Vector3() noexcept
        : x_(0.0f)
        , y_(0.0f)
        , z_(0.0f)
    {
    }

    /// Copy-construct from another vector.
    Vector3(const Vector3& vector) noexcept = default;

    /// Construct from a two-dimensional vector and the Z coordinate.
    Vector3(const Vector2& vector, float z) noexcept
        : x_(vector.x_)
        , y_(vector.y_)
        , z_(z)
    {
    }

    /// Construct from a two-dimensional vector (for Urho2D).
    explicit Vector3(const Vector2& vector) noexcept
        : x_(vector.x_)
        , y_(vector.y_)
        , z_(0.0f)
    {
    }

    /// Construct from an IntVector3.
    explicit Vector3(const IntVector3& vector) noexcept
        : x_((float)vector.x_)
        , y_((float)vector.y_)
        , z_((float)vector.z_)
    {
    }

    /// Construct from coordinates.
    Vector3(float x, float y, float z) noexcept
        : x_(x)
        , y_(y)
        , z_(z)
    {
    }

    /// Construct from two-dimensional coordinates (for Urho2D).
    Vector3(float x, float y) noexcept
        : x_(x)
        , y_(y)
        , z_(0.0f)
    {
    }

    /// Construct from a float array.
    explicit Vector3(const float data[]) noexcept
        : x_(data[0])
        , y_(data[1])
        , z_(data[2])
    {
    }

    /// Construct from 2D vector in X0Z plane.
    static Vector3 FromXZ(const Vector2& vector, float y = 0.0f) { return {vector.x_, y, vector.y_}; }

    /// Assign from another vector.
    Vector3& operator=(const Vector3& rhs) noexcept = default;

    /// Test for equality with another vector without epsilon.
    bool operator==(const Vector3& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_; }

    /// Test for inequality with another vector without epsilon.
    bool operator!=(const Vector3& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_ || z_ != rhs.z_; }

    /// Add a vector.
    Vector3 operator+(const Vector3& rhs) const { return Vector3(x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_); }

    /// Return negation.
    Vector3 operator-() const { return Vector3(-x_, -y_, -z_); }

    /// Subtract a vector.
    Vector3 operator-(const Vector3& rhs) const { return Vector3(x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_); }

    /// Multiply with a scalar.
    Vector3 operator*(float rhs) const { return Vector3(x_ * rhs, y_ * rhs, z_ * rhs); }

    /// Multiply with a vector.
    Vector3 operator*(const Vector3& rhs) const { return Vector3(x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_); }

    /// Divide by a scalar.
    Vector3 operator/(float rhs) const { return Vector3(x_ / rhs, y_ / rhs, z_ / rhs); }

    /// Divide by a vector.
    Vector3 operator/(const Vector3& rhs) const { return Vector3(x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_); }

    /// Add-assign a vector.
    Vector3& operator+=(const Vector3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    Vector3& operator-=(const Vector3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    Vector3& operator*=(float rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    Vector3& operator*=(const Vector3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    Vector3& operator/=(float rhs)
    {
        float invRhs = 1.0f / rhs;
        x_ *= invRhs;
        y_ *= invRhs;
        z_ *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    Vector3& operator/=(const Vector3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Normalize to unit length.
    void Normalize()
    {
        float lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            float invLen = 1.0f / sqrtf(lenSquared);
            x_ *= invLen;
            y_ *= invLen;
            z_ *= invLen;
        }
    }

    /// Return the length (magnitude) of this vector.
    /// @property
    float Length() const { return sqrtf(x_ * x_ + y_ * y_ + z_ * z_); }

    /// Return the squared length of this vector.
    /// @property
    float LengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

    /// Return the length of the 2D components of this vector.
    /// @property
    float Length2D() const { return Sqrt(x_ * x_ + y_ * y_); }

    /// Return the squared length of the 2D components of this vector.
    /// @property
    float LengthSquared2D() const { return x_ * x_ + y_ * y_; }

    /// Calculate dot product.
    float DotProduct(const Vector3& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_ + z_ * rhs.z_; }

    /// Calculate absolute dot product.
    float AbsDotProduct(const Vector3& rhs) const
    {
        return Urho3D::Abs(x_ * rhs.x_) + Urho3D::Abs(y_ * rhs.y_) + Urho3D::Abs(z_ * rhs.z_);
    }

    /// Project direction vector onto axis.
    float ProjectOntoAxis(const Vector3& axis) const { return DotProduct(axis.Normalized()); }

    /// Project position vector onto plane with given origin and normal.
    Vector3 ProjectOntoPlane(const Vector3& origin, const Vector3& normal) const
    {
        const Vector3 delta = *this - origin;
        return *this - normal.Normalized() * delta.ProjectOntoAxis(normal);
    }

    /// Project position vector onto line segment.
    Vector3 ProjectOntoLine(const Vector3& from, const Vector3& to, bool clamped = false) const
    {
        const Vector3 direction = to - from;
        const float lengthSquared = direction.LengthSquared();
        float factor = (*this - from).DotProduct(direction) / lengthSquared;

        if (clamped)
            factor = Clamp(factor, 0.0f, 1.0f);

        return from + direction * factor;
    }

    /// Calculate distance to another position vector.
    float DistanceToPoint(const Vector3& point) const { return (*this - point).Length(); }

    /// Calculate distance to the plane with given origin and normal.
    float DistanceToPlane(const Vector3& origin, const Vector3& normal) const
    {
        return (*this - origin).ProjectOntoAxis(normal);
    }

    /// Make vector orthogonal to the axis.
    Vector3 Orthogonalize(const Vector3& axis) const
    {
        return axis.CrossProduct(*this).CrossProduct(axis).Normalized();
    }

    /// Calculate cross product.
    Vector3 CrossProduct(const Vector3& rhs) const
    {
        return Vector3(y_ * rhs.z_ - z_ * rhs.y_, z_ * rhs.x_ - x_ * rhs.z_, x_ * rhs.y_ - y_ * rhs.x_);
    }

    /// Return absolute vector.
    Vector3 Abs() const { return Vector3(Urho3D::Abs(x_), Urho3D::Abs(y_), Urho3D::Abs(z_)); }

    /// Linear interpolation with another vector.
    Vector3 Lerp(const Vector3& rhs, float t) const { return *this * (1.0f - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const Vector3& rhs, float eps = M_EPSILON) const
    {
        return Urho3D::Equals(x_, rhs.x_, eps) && Urho3D::Equals(y_, rhs.y_, eps) && Urho3D::Equals(z_, rhs.z_, eps);
    }

    /// Returns the angle between this vector and another vector in degrees.
    float Angle(const Vector3& rhs) const { return Urho3D::Acos(DotProduct(rhs) / (Length() * rhs.Length())); }

    /// Return whether any component is NaN.
    bool IsNaN() const { return Urho3D::IsNaN(x_) || Urho3D::IsNaN(y_) || Urho3D::IsNaN(z_); }

    /// Return whether any component is Inf.
    bool IsInf() const { return Urho3D::IsInf(x_) || Urho3D::IsInf(y_) || Urho3D::IsInf(z_); }

    /// Return normalized to unit length.
    Vector3 Normalized() const
    {
        const float lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            float invLen = 1.0f / sqrtf(lenSquared);
            return *this * invLen;
        }
        else
            return *this;
    }

    /// Return normalized to unit length or zero if length is too small.
    Vector3 NormalizedOrDefault(const Vector3& defaultValue = Vector3::ZERO, float eps = M_LARGE_EPSILON) const
    {
        const float lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;
        return *this / sqrtf(lenSquared);
    }

    /// Return normalized vector with length in given range.
    Vector3 ReNormalized(float minLength, float maxLength, const Vector3& defaultValue = Vector3::ZERO,
        float eps = M_LARGE_EPSILON) const
    {
        const float lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;

        const float len = sqrtf(lenSquared);
        const float newLen = Clamp(len, minLength, maxLength);
        return *this * (newLen / len);
    }

    /// Checks whether vector is near to zero within a specified tolerance.
    bool IsNearlyZero(float Tolerance) const
    {
        return Urho3D::Abs(x_) <= Tolerance && Urho3D::Abs(y_) <= Tolerance && Urho3D::Abs(z_) <= Tolerance;
    }

    /// Checks whether all components of the vector are exactly zero.
    bool IsZero() const { return x_ == 0.f && y_ == 0.f && z_ == 0.f; }

    ///  Check if the vector is of unit length, with specified tolerance.
    bool IsUnit(float LengthSquaredTolerance = M_EPSILON) const
    {
        return Urho3D::Abs(1.0f - LengthSquared()) < LengthSquaredTolerance;
    }

    /// Checks whether all components of this vector are the same, within a tolerance.
    bool AllComponentsEqual(float Tolerance) const
    {
        return Urho3D::Abs(x_ - y_) <= Tolerance && Urho3D::Abs(x_ - z_) <= Tolerance
            && Urho3D::Abs(y_ - z_) <= Tolerance;
    }

    /// Gets the reciprocal of this vector, avoiding division by zero. Zero components are set to LARGE_VALUE.
    Vector3 Reciprocal() const
    {
        Vector3 RecVector;
        if (x_ != 0.f)
        {
            RecVector.x_ = 1.f / x_;
        }
        else
        {
            RecVector.x_ = M_LARGE_VALUE;
        }
        if (y_ != 0.f)
        {
            RecVector.y_ = 1.f / y_;
        }
        else
        {
            RecVector.y_ = M_LARGE_VALUE;
        }
        if (z_ != 0.f)
        {
            RecVector.z_ = 1.f / z_;
        }
        else
        {
            RecVector.z_ = M_LARGE_VALUE;
        }

        return RecVector;
    }

    /// Check whether X, Y and Z are nearly equal.
    bool IsUniform(float Tolerance = M_LARGE_EPSILON) const { return AllComponentsEqual(Tolerance); }

    /// Get a copy of this vector, clamped inside of a cube.
    Vector3 BoundToCube(float Radius) const
    {
        return Vector3(Clamp(x_, -Radius, Radius), Clamp(y_, -Radius, Radius), Clamp(z_, -Radius, Radius));
    }

    /// Get a copy of this vector, clamped inside of a cube.
    Vector3 BoundToBox(const Vector3& Min, const Vector3 Max) const
    {
        return Vector3(Clamp(x_, Min.x_, Max.x_), Clamp(y_, Min.y_, Max.y_), Clamp(z_, Min.z_, Max.z_));
    }

    /// Create a copy of this vector, with its magnitude clamped between Min and Max.
    Vector3 GetClampedToSize(float Min, float Max) const
    {
        float VecSize = Length();
        Vector3 VecDir = (VecSize > M_EPSILON) ? (*this / VecSize) : ZERO;

        VecSize = Clamp(VecSize, Min, Max);

        return VecDir * VecSize;
    }

    /// Create a copy of this vector, with the 2D magnitude clamped between Min and Max. Z is unchanged.
    Vector3 GetClampedToSize2D(float Min, float Max) const
    {
        float VecSize2D = Length();
        const Vector3 VecDir = (VecSize2D > M_EPSILON) ? (*this / VecSize2D) : ZERO;

        VecSize2D = Clamp(VecSize2D, Min, Max);

        return Vector3(VecSize2D * VecDir.x_, VecSize2D * VecDir.y_, z_);
    }

    /// Create a copy of this vector, with its maximum magnitude clamped to MaxSize.
    Vector3 GetClampedToMaxSize(float MaxSize) const
    {
        if (MaxSize < M_LARGE_EPSILON)
        {
            return Vector3::ZERO;
        }

        const float VSq = LengthSquared();
        if (VSq > Square(MaxSize))
        {
            const float Scale = MaxSize * InvSqrt(VSq);
            return Vector3(x_ * Scale, y_ * Scale, z_ * Scale);
        }
        else
        {
            return *this;
        }
    }

    /// Create a copy of this vector, with the maximum 2D magnitude clamped to MaxSize. Z is unchanged.
    Vector3 GetClampedToMaxSize2D(float MaxSize) const
    {
        if (MaxSize < M_LARGE_EPSILON)
        {
            return Vector3(0.f, 0.f, z_);
        }

        const float VSq2D = LengthSquared2D();
        if (VSq2D > Square(MaxSize))
        {
            const float Scale = MaxSize * InvSqrt(VSq2D);
            return Vector3(x_ * Scale, y_ * Scale, z_);
        }
        else
        {
            return *this;
        }
    }

    /// Add a vector to this and clamp the result in a cube.
    void AddBounded(const Vector3& V, float Radius = UINT16_MAX) { *this = (*this + V).BoundToCube(Radius); }

    /// Util to calculate distance from a point to a bounding box.
    /// 
    /// @param Mins 3D Point defining the lower values of the axis of the bound box
    /// @param Max 3D Point defining the lower values of the axis of the bound box
    /// @param Point 3D position of interest
    /// @return the distance from the Point to the bounding box.
    float ComputeSquaredDistanceFromBoxToPoint(const Vector3& Mins, const Vector3& Maxs, const Vector3& Point)
    {
        // Accumulates the distance as we iterate axis
        float DistSquared = 0.f;

        // Check each axis for min/max and add the distance accordingly
        // NOTE: Loop manually unrolled for > 2x speed up
        if (Point.x_ < Mins.x_)
        {
            DistSquared += Square(Point.x_ - Mins.x_);
        }
        else if (Point.x_ > Maxs.x_)
        {
            DistSquared += Square(Point.x_ - Maxs.x_);
        }

        if (Point.y_ < Mins.y_)
        {
            DistSquared += Square(Point.y_ - Mins.y_);
        }
        else if (Point.y_ > Maxs.y_)
        {
            DistSquared += Square(Point.y_ - Maxs.y_);
        }

        if (Point.z_ < Mins.z_)
        {
            DistSquared += Square(Point.z_ - Mins.z_);
        }
        else if (Point.z_ > Maxs.z_)
        {
            DistSquared += Square(Point.z_ - Maxs.z_);
        }

        return DistSquared;
    }

    /// Return float data.
    const float* Data() const { return &x_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const
    {
        unsigned hash = 37;
        hash = 37 * hash + FloatToRawIntBits(x_);
        hash = 37 * hash + FloatToRawIntBits(y_);
        hash = 37 * hash + FloatToRawIntBits(z_);

        return hash;
    }

    /// Return 2D vector (z component is ignored).
    Vector2 ToVector2() const { return {x_, y_}; }

    /// Return x and z components as 2D vector (y component is ignored).
    Vector2 ToXZ() const { return {x_, z_}; }

    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
    /// Z coordinate.
    float z_;

    /// Zero vector.
    static const Vector3 ZERO;
    /// (-1,0,0) vector.
    static const Vector3 LEFT;
    /// (1,0,0) vector.
    static const Vector3 RIGHT;
    /// (0,1,0) vector.
    static const Vector3 UP;
    /// (0,-1,0) vector.
    static const Vector3 DOWN;
    /// (0,0,1) vector.
    static const Vector3 FORWARD;
    /// (0,0,-1) vector.
    static const Vector3 BACK;
    /// (1,1,1) vector.
    static const Vector3 ONE;
};

/// Multiply Vector3 with a scalar.
inline Vector3 operator*(float lhs, const Vector3& rhs) { return rhs * lhs; }

/// Multiply IntVector3 with a scalar.
inline IntVector3 operator*(int lhs, const IntVector3& rhs) { return rhs * lhs; }

/// Per-component linear interpolation between two 3-vectors.
inline Vector3 VectorLerp(const Vector3& lhs, const Vector3& rhs, const Vector3& t) { return lhs + (rhs - lhs) * t; }

/// Per-component min of two 3-vectors.
inline Vector3 VectorMin(const Vector3& lhs, const Vector3& rhs)
{
    return Vector3(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_), Min(lhs.z_, rhs.z_));
}

/// Per-component max of two 3-vectors.
inline Vector3 VectorMax(const Vector3& lhs, const Vector3& rhs)
{
    return Vector3(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_), Max(lhs.z_, rhs.z_));
}

/// Per-component floor of 3-vector.
inline Vector3 VectorFloor(const Vector3& vec) { return Vector3(Floor(vec.x_), Floor(vec.y_), Floor(vec.z_)); }

/// Per-component round of 3-vector.
inline Vector3 VectorRound(const Vector3& vec) { return Vector3(Round(vec.x_), Round(vec.y_), Round(vec.z_)); }

/// Per-component ceil of 3-vector.
inline Vector3 VectorCeil(const Vector3& vec) { return Vector3(Ceil(vec.x_), Ceil(vec.y_), Ceil(vec.z_)); }

/// Per-component absolute value of 3-vector.
inline Vector3 VectorAbs(const Vector3& vec) { return Vector3(Abs(vec.x_), Abs(vec.y_), Abs(vec.z_)); }

/// Per-component square root of 3-vector.
inline Vector3 VectorSqrt(const Vector3& vec) { return Vector3(Sqrt(vec.x_), Sqrt(vec.y_), Sqrt(vec.z_)); }

/// Per-component floor of 3-vector. Returns IntVector3.
inline IntVector3 VectorFloorToInt(const Vector3& vec)
{
    return IntVector3(FloorToInt(vec.x_), FloorToInt(vec.y_), FloorToInt(vec.z_));
}

/// Per-component round of 3-vector. Returns IntVector3.
inline IntVector3 VectorRoundToInt(const Vector3& vec)
{
    return IntVector3(RoundToInt(vec.x_), RoundToInt(vec.y_), RoundToInt(vec.z_));
}

/// Per-component ceil of 3-vector. Returns IntVector3.
inline IntVector3 VectorCeilToInt(const Vector3& vec)
{
    return IntVector3(CeilToInt(vec.x_), CeilToInt(vec.y_), CeilToInt(vec.z_));
}

/// Per-component min of two 3-vectors.
inline IntVector3 VectorMin(const IntVector3& lhs, const IntVector3& rhs)
{
    return IntVector3(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_), Min(lhs.z_, rhs.z_));
}

/// Per-component max of two 3-vectors.
inline IntVector3 VectorMax(const IntVector3& lhs, const IntVector3& rhs)
{
    return IntVector3(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_), Max(lhs.z_, rhs.z_));
}

/// Per-component absolute value of integer 3-vector.
inline IntVector3 VectorAbs(const IntVector3& vec) { return IntVector3(Abs(vec.x_), Abs(vec.y_), Abs(vec.z_)); }

/// Return a random value from [0, 1) from 3-vector seed.
inline float StableRandom(const Vector3& seed)
{
    return StableRandom(Vector2(StableRandom(Vector2(seed.x_, seed.y_)), seed.z_));
}

} // namespace Urho3D
