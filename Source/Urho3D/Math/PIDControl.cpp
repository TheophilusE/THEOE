//
// Copyright (c) 2020-2022 Theophilus Eriata.
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

#include "../Math/PIDControl.h"

namespace Urho3D
{
// Init ctor
FPIDController::FPIDController(float InP, float InI, float InD, float InMaxOutAbs)
    : P(InP)
    , I(InI)
    , D(InD)
    , MaxOutAbs(InMaxOutAbs)
{
    // Reset errors, bind update function ptr
    FPIDController::Init();
}

// Init
void FPIDController::Init(float InP, float InI, float InD, float InMaxOutAbs, bool bClearErrors /*= true*/)
{
    P = InP;
    I = InI;
    D = InD;
    MaxOutAbs = InMaxOutAbs;
    // Reset errors, bind update function ptr
    FPIDController::Init(bClearErrors);
}

// Default init
void FPIDController::Init(bool bClearErrors /*= true*/)
{
    if (bClearErrors)
    {
        PrevErr = 0.f;
        IErr = 0.f;
    }

    // Bind the update type function ptr
    if (P > 0.f && I > 0.f && D > 0.f)
    {
        UpdateFunctionPtr = &FPIDController::UpdateAsPID;
    }
    else if (P > 0.f && I > 0.f)
    {
        UpdateFunctionPtr = &FPIDController::UpdateAsPI;
    }
    else if (P > 0.f && D > 0.f)
    {
        UpdateFunctionPtr = &FPIDController::UpdateAsPD;
    }
    else if (P > 0.f)
    {
        UpdateFunctionPtr = &FPIDController::UpdateAsP;
    }
    else
    {
        // Default
        UpdateFunctionPtr = &FPIDController::UpdateAsPID;
    }
}

// Call the update function pointer
float FPIDController::Update(const float InError, const float InDeltaTime)
{
    return (this->*UpdateFunctionPtr)(InError, InDeltaTime);
}

float FPIDController::UpdateAsPID(const float InError, const float InDeltaTime)
{
    if (InDeltaTime == 0.0f || IsNaN(InError))
    {
        return 0.0f;
    }

    // Calculate proportional output
    const float POut = P * InError;

    // Calculate integral error / output
    IErr += InDeltaTime * InError;
    const float IOut = I * IErr;

    // Calculate the derivative error / output
    const float DErr = (InError - PrevErr) / InDeltaTime;
    const float DOut = D * DErr;

    // Set previous error
    PrevErr = InError;

    // Calculate the output
    const float Out = POut + IOut + DOut;

    // Clamp output
    return Clamp(Out, -MaxOutAbs, MaxOutAbs);
}

float FPIDController::UpdateAsP(const float InError, const float /*InDeltaTime*/)
{
    if (IsNaN(InError))
    {
        return 0.0f;
    }

    // Calculate proportional output
    const float Out = P * InError;

    // Clamp output
    return Clamp(Out, -MaxOutAbs, MaxOutAbs);
}

float FPIDController::UpdateAsPD(const float InError, const float InDeltaTime)
{
    if (InDeltaTime == 0.0f || IsNaN(InError))
    {
        return 0.0f;
    }

    // Calculate proportional output
    const float POut = P * InError;

    // Calculate the derivative error / output
    const float DErr = (InError - PrevErr) / InDeltaTime;
    const float DOut = D * DErr;

    // Set previous error
    PrevErr = InError;

    // Calculate the output
    const float Out = POut + DOut;

    // Clamp output
    return Clamp(Out, -MaxOutAbs, MaxOutAbs);
}

float FPIDController::UpdateAsPI(const float InError, const float InDeltaTime)
{
    if (InDeltaTime == 0.0f || IsNaN(InError))
    {
        return 0.0f;
    }

    // Calculate proportional output
    const float POut = P * InError;

    // Calculate integral error / output
    IErr += InDeltaTime * InError;
    const float IOut = I * IErr;

    // Calculate the output
    const float Out = POut + IOut;

    // Clamp output
    return Clamp(Out, -MaxOutAbs, MaxOutAbs);
}

// Init ctor
FPIDController3D::FPIDController3D(float InP, float InI, float InD, float InMaxOutAbs)
    : P(InP)
    , I(InI)
    , D(InD)
    , MaxOutAbs(InMaxOutAbs)
{
    // Reset errors, bind update function ptr
    FPIDController3D::Init();
}

// Init
void FPIDController3D::Init(float InP, float InI, float InD, float InMaxOutAbs, bool bClearErrors /*= true*/)
{
    P = InP;
    I = InI;
    D = InD;
    MaxOutAbs = InMaxOutAbs;
    // Reset errors, bind update function ptr
    FPIDController3D::Init(bClearErrors);
}

// Default init
void FPIDController3D::Init(bool bClearErrors /*= true*/)
{
    if (bClearErrors)
    {
        PrevErr = Vector3(Vector3::ZERO);
        IErr = Vector3(Vector3::ZERO);
    }

    // Bind the update type function ptr
    if (P > 0.f && I > 0.f && D > 0.f)
    {
        UpdateFunctionPtr = &FPIDController3D::UpdateAsPID;
    }
    else if (P > 0.f && I > 0.f)
    {
        UpdateFunctionPtr = &FPIDController3D::UpdateAsPI;
    }
    else if (P > 0.f && D > 0.f)
    {
        UpdateFunctionPtr = &FPIDController3D::UpdateAsPD;
    }
    else if (P > 0.f)
    {
        UpdateFunctionPtr = &FPIDController3D::UpdateAsP;
    }
    else
    {
        // Default
        UpdateFunctionPtr = &FPIDController3D::UpdateAsPID;
    }
}

// Call the update function pointer
Vector3 FPIDController3D::Update(const Vector3 InError, const float InDeltaTime)
{
    return (this->*UpdateFunctionPtr)(InError, InDeltaTime);
}

Vector3 FPIDController3D::UpdateAsPID(const Vector3 InError, const float InDeltaTime)
{
    if (InDeltaTime == 0.0f || InError.IsNaN())
    {
        return Vector3(Vector3::ZERO);
    }

    // Calculate proportional output
    const Vector3 POut = P * InError;

    // Calculate integral error / output
    IErr += InDeltaTime * InError;
    const Vector3 IOut = I * IErr;

    // Calculate the derivative error / output
    const Vector3 DErr = (InError - PrevErr) / InDeltaTime;
    const Vector3 DOut = D * DErr;

    // Set previous error
    PrevErr = InError;

    // Calculate the output
    const Vector3 Out = POut + IOut + DOut;

    // Clamp the vector values
    return Out.BoundToCube(MaxOutAbs);
}

Vector3 FPIDController3D::UpdateAsP(const Vector3 InError, const float /*InDeltaTime*/)
{
    if (InError.IsNaN())
    {
        return Vector3(Vector3::ZERO);
    }

    // Calculate proportional output
    const Vector3 Out = P * InError;

    // Clamp output
    return Out.BoundToCube(MaxOutAbs);
}

Vector3 FPIDController3D::UpdateAsPD(const Vector3 InError, const float InDeltaTime)
{
    if (InDeltaTime == 0.0f || InError.IsNaN())
    {
        return Vector3(Vector3::ZERO);
    }

    // Calculate proportional output
    const Vector3 POut = P * InError;

    // Calculate the derivative error / output
    const Vector3 DErr = (InError - PrevErr) / InDeltaTime;
    const Vector3 DOut = D * DErr;

    // Set previous error
    PrevErr = InError;

    // Calculate the output
    const Vector3 Out = POut + DOut;

    // Clamp output
    return Out.BoundToCube(MaxOutAbs);
}

Vector3 FPIDController3D::UpdateAsPI(const Vector3 InError, const float InDeltaTime)
{
    if (InDeltaTime == 0.0f || InError.IsNaN())
    {
        return Vector3(Vector3::ZERO);
    }

    // Calculate proportional output
    const Vector3 POut = P * InError;

    // Calculate integral error / output
    IErr += InDeltaTime * InError;
    const Vector3 IOut = I * IErr;

    // Calculate the output
    const Vector3 Out = POut + IOut;

    // Clamp output
    return Out.BoundToCube(MaxOutAbs);
}
} // namespace Urho3D
