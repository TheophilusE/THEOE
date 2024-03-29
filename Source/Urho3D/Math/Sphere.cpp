//
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

#include "../Precompiled.h"

#include "../Math/Frustum.h"
#include "../Math/Polyhedron.h"

#include "../DebugNew.h"

namespace Urho3D
{

Vector3 Circle::GetPoint(const Vector3& directionHint) const
{
    if (!IsValid())
        return center_;

    const Vector3 direction = directionHint.Orthogonalize(normal_);
    return center_ + direction * radius_;
}

void Sphere::Define(const Vector3* vertices, unsigned count)
{
    if (!count)
        return;

    Clear();
    Merge(vertices, count);
}

void Sphere::Define(const BoundingBox& box)
{
    const Vector3& min = box.min_;
    const Vector3& max = box.max_;

    Clear();
    Merge(min);
    Merge(Vector3(max.x_, min.y_, min.z_));
    Merge(Vector3(min.x_, max.y_, min.z_));
    Merge(Vector3(max.x_, max.y_, min.z_));
    Merge(Vector3(min.x_, min.y_, max.z_));
    Merge(Vector3(max.x_, min.y_, max.z_));
    Merge(Vector3(min.x_, max.y_, max.z_));
    Merge(max);
}

void Sphere::Define(const Frustum& frustum)
{
    Define(frustum.vertices_, NUM_FRUSTUM_VERTICES);
}

void Sphere::Define(const Polyhedron& poly)
{
    Clear();
    Merge(poly);
}

void Sphere::Merge(const Vector3* vertices, unsigned count)
{
    while (count--)
        Merge(*vertices++);
}

void Sphere::Merge(const BoundingBox& box)
{
    const Vector3& min = box.min_;
    const Vector3& max = box.max_;

    Merge(min);
    Merge(Vector3(max.x_, min.y_, min.z_));
    Merge(Vector3(min.x_, max.y_, min.z_));
    Merge(Vector3(max.x_, max.y_, min.z_));
    Merge(Vector3(min.x_, min.y_, max.z_));
    Merge(Vector3(max.x_, min.y_, max.z_));
    Merge(Vector3(min.x_, max.y_, max.z_));
    Merge(max);
}

void Sphere::Merge(const Frustum& frustum)
{
    const Vector3* vertices = frustum.vertices_;
    Merge(vertices, NUM_FRUSTUM_VERTICES);
}

void Sphere::Merge(const Polyhedron& poly)
{
    for (unsigned i = 0; i < poly.faces_.size(); ++i)
    {
        const ea::vector<Vector3>& face = poly.faces_[i];
        if (!face.empty())
            Merge(&face[0], face.size());
    }
}

void Sphere::Merge(const Sphere& sphere)
{
    if (radius_ < 0.0f)
    {
        center_ = sphere.center_;
        radius_ = sphere.radius_;
        return;
    }

    Vector3 offset = sphere.center_ - center_;
    float dist = offset.Length();

    // If sphere fits inside, do nothing
    if (dist + sphere.radius_ < radius_)
        return;

    // If we fit inside the other sphere, become it
    if (dist + radius_ < sphere.radius_)
    {
        center_ = sphere.center_;
        radius_ = sphere.radius_;
    }
    else
    {
        Vector3 NormalizedOffset = offset / dist;

        Vector3 min = center_ - radius_ * NormalizedOffset;
        Vector3 max = sphere.center_ + sphere.radius_ * NormalizedOffset;
        center_ = (min + max) * 0.5f;
        radius_ = (max - center_).Length();
    }
}

Intersection Sphere::IsInside(const BoundingBox& box) const
{
    float radiusSquared = radius_ * radius_;
    float distSquared = 0;
    float temp;
    Vector3 min = box.min_;
    Vector3 max = box.max_;

    if (center_.x_ < min.x_)
    {
        temp = center_.x_ - min.x_;
        distSquared += temp * temp;
    }
    else if (center_.x_ > max.x_)
    {
        temp = center_.x_ - max.x_;
        distSquared += temp * temp;
    }
    if (center_.y_ < min.y_)
    {
        temp = center_.y_ - min.y_;
        distSquared += temp * temp;
    }
    else if (center_.y_ > max.y_)
    {
        temp = center_.y_ - max.y_;
        distSquared += temp * temp;
    }
    if (center_.z_ < min.z_)
    {
        temp = center_.z_ - min.z_;
        distSquared += temp * temp;
    }
    else if (center_.z_ > max.z_)
    {
        temp = center_.z_ - max.z_;
        distSquared += temp * temp;
    }

    if (distSquared >= radiusSquared)
        return OUTSIDE;

    min -= center_;
    max -= center_;

    Vector3 tempVec = min; // - - -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x_ = max.x_; // + - -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y_ = max.y_; // + + -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x_ = min.x_; // - + -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.z_ = max.z_; // - + +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y_ = min.y_; // - - +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x_ = max.x_; // + - +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y_ = max.y_; // + + +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;

    return INSIDE;
}

Intersection Sphere::IsInsideFast(const BoundingBox& box) const
{
    float radiusSquared = radius_ * radius_;
    float distSquared = 0;
    float temp;
    Vector3 min = box.min_;
    Vector3 max = box.max_;

    if (center_.x_ < min.x_)
    {
        temp = center_.x_ - min.x_;
        distSquared += temp * temp;
    }
    else if (center_.x_ > max.x_)
    {
        temp = center_.x_ - max.x_;
        distSquared += temp * temp;
    }
    if (center_.y_ < min.y_)
    {
        temp = center_.y_ - min.y_;
        distSquared += temp * temp;
    }
    else if (center_.y_ > max.y_)
    {
        temp = center_.y_ - max.y_;
        distSquared += temp * temp;
    }
    if (center_.z_ < min.z_)
    {
        temp = center_.z_ - min.z_;
        distSquared += temp * temp;
    }
    else if (center_.z_ > max.z_)
    {
        temp = center_.z_ - max.z_;
        distSquared += temp * temp;
    }

    if (distSquared >= radiusSquared)
        return OUTSIDE;
    else
        return INSIDE;
}

Vector3 Sphere::GetLocalPoint(float theta, float phi) const
{
    return Vector3(
        radius_ * Sin(theta) * Sin(phi),
        radius_ * Cos(phi),
        radius_ * Cos(theta) * Sin(phi)
    );
}

Circle Sphere::Intersect(const Sphere& sphere, float* distanceFromCenter) const
{
    const Vector3 offset = sphere.center_ - center_;
    const float distance = offset.Length();

    // http://mathworld.wolfram.com/Sphere-SphereIntersection.html
    const float R = radius_;
    const float r = sphere.radius_;
    const float d = Min(R + r, distance);
    const float a2 = (-d + r - R) * (-d - r + R) * (-d + r + R) * (d + r + R);
    const float a = Sqrt(Max(0.0f, a2)) / (2 * d);

    const bool isOutside = distance > R + r;
    const bool isInside = a2 < 0.0f;

    const float distanceToCircle = Sqrt(R * R - a * a);
    if (distanceFromCenter)
        *distanceFromCenter = distanceToCircle;

    const Vector3 normal = offset / distance;
    const Vector3 center = center_ + distanceToCircle * normal;
    const float effectiveRadius = isInside || isOutside ? -M_INFINITY : a;
    return Circle{ center, normal, effectiveRadius };
}

}
