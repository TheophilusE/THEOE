//
// Copyright (c) 2020-2021 Theophilus Eriata.
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

#include <EASTL/map.h>

#include "../Math/EasingFunctions.h"

namespace Urho3D
{
double easeInSine(double t) { return sin(1.5707963 * t); }

double easeOutSine(double t) { return 1 + sin(1.5707963 * (--t)); }

double easeInOutSine(double t) { return 0.5 * (1 + sin(3.1415926 * (t - 0.5))); }

double easeInQuad(double t) { return t * t; }

double easeOutQuad(double t) { return t * (2.0 - t); }

double easeInOutQuad(double t) { return t < 0.5 ? 2.0 * t * t : t * (4.0 - 2.0 * t) - 1.0; }

double easeInCubic(double t) { return t * t * t; }

double easeOutCubic(double t) { return 1.0 + (--t) * t * t; }

double easeInOutCubic(double t) { return t < 0.5 ? 4.0 * t * t * t : 1.0 + (--t) * (2.0 * (--t)) * (2.0 * t); }

double easeInQuart(double t)
{
    t *= t;
    return t * t;
}

double easeOutQuart(double t)
{
    t = (--t) * t;
    return 1.0 - t * t;
}

double easeInOutQuart(double t)
{
    if (t < 0.5)
    {
        t *= t;
        return 8.0 * t * t;
    }
    else
    {
        t = (--t) * t;
        return 1.0 - 8.0 * t * t;
    }
}

double easeInQuint(double t)
{
    double t2 = t * t;
    return t * t2 * t2;
}

double easeOutQuint(double t)
{
    double t2 = (--t) * t;
    return 1 + t * t2 * t2;
}

double easeInOutQuint(double t)
{
    double t2;
    if (t < 0.5)
    {
        t2 = t * t;
        return 16.0 * t * t2 * t2;
    }
    else
    {
        t2 = (--t) * t;
        return 1.0 + 16.0 * t * t2 * t2;
    }
}

double easeInExpo(double t) { return (pow(2.0, 8.0 * t) - 1.0) / 255.0; }

double easeOutExpo(double t) { return 1 - pow(2.0, -8.0 * t); }

double easeInOutExpo(double t)
{
    if (t < 0.5)
    {
        return (pow(2.0, 16.0 * t) - 1) / 510.0;
    }
    else
    {
        return 1.0 - 0.5 * pow(2.0, -16.0 * (t - 0.5));
    }
}

double easeInCirc(double t) { return 1.0 - sqrt(1.0 - t); }

double easeOutCirc(double t) { return sqrt(t); }

double easeInOutCirc(double t)
{
    if (t < 0.5)
    {
        return (1 - sqrt(1.0 - 2.0 * t)) * 0.5;
    }
    else
    {
        return (1 + sqrt(2.0 * t - 1.0)) * 0.5;
    }
}

double easeInBack(double t) { return t * t * (2.70158 * t - 1.70158); }

double easeOutBack(double t) { return 1.0 + (--t) * t * (2.70158 * t + 1.70158); }

double easeInOutBack(double t)
{
    if (t < 0.5)
    {
        return t * t * (7.0 * t - 2.5) * 2;
    }
    else
    {
        return 1.0 + (--t) * t * 2.0 * (7.0 * t + 2.5);
    }
}

double easeInElastic(double t)
{
    double t2 = t * t;
    return t2 * t2 * sin(t * M_PI * 4.5);
}

double easeOutElastic(double t)
{
    double t2 = (t - 1) * (t - 1.0);
    return 1.0 - t2 * t2 * cos(t * M_PI * 4.5);
}

double easeInOutElastic(double t)
{
    double t2;
    if (t < 0.45)
    {
        t2 = t * t;
        return 8.0 * t2 * t2 * sin(t * M_PI * 9.0);
    }
    else if (t < 0.55)
    {
        return 0.5 + 0.75 * sin(t * M_PI * 4.0);
    }
    else
    {
        t2 = (t - 1) * (t - 1);
        return 1.0 - 8.0 * t2 * t2 * sin(t * M_PI * 9.0);
    }
}

double easeInBounce(double t) { return pow(2.0, 6.0 * (t - 1)) * abs(sin(t * M_PI * 3.5)); }

double easeOutBounce(double t) { return 1.0 - pow(2.0, -6.0 * t) * abs(cos(t * M_PI * 3.5)); }

double easeInOutBounce(double t)
{
    if (t < 0.5)
    {
        return 8.0 * pow(2.0, 8.0 * (t - 1.0)) * abs(sin(t * M_PI * 7.0));
    }
    else
    {
        return 1.0 - 8.0 * pow(2.0, -8.0 * t) * abs(sin(t * M_PI * 7.0));
    }
}

double easeLinear(double t) { return t; }

easingFunction GetEasingFunction(easing_functions function)
{
    static eastl::map<easing_functions, easingFunction> easingFunctions;
    if (easingFunctions.empty())
    {
        easingFunctions.insert(eastl::make_pair(EaseInSine, easeInSine));
        easingFunctions.insert(eastl::make_pair(EaseOutSine, easeOutSine));
        easingFunctions.insert(eastl::make_pair(EaseInOutSine, easeInOutSine));
        easingFunctions.insert(eastl::make_pair(EaseInQuad, easeInQuad));
        easingFunctions.insert(eastl::make_pair(EaseOutQuad, easeOutQuad));
        easingFunctions.insert(eastl::make_pair(EaseInOutQuad, easeInOutQuad));
        easingFunctions.insert(eastl::make_pair(EaseInCubic, easeInCubic));
        easingFunctions.insert(eastl::make_pair(EaseOutCubic, easeOutCubic));
        easingFunctions.insert(eastl::make_pair(EaseInOutCubic, easeInOutCubic));
        easingFunctions.insert(eastl::make_pair(EaseInQuart, easeInQuart));
        easingFunctions.insert(eastl::make_pair(EaseOutQuart, easeOutQuart));
        easingFunctions.insert(eastl::make_pair(EaseInOutQuart, easeInOutQuart));
        easingFunctions.insert(eastl::make_pair(EaseInQuint, easeInQuint));
        easingFunctions.insert(eastl::make_pair(EaseOutQuint, easeOutQuint));
        easingFunctions.insert(eastl::make_pair(EaseInOutQuint, easeInOutQuint));
        easingFunctions.insert(eastl::make_pair(EaseInExpo, easeInExpo));
        easingFunctions.insert(eastl::make_pair(EaseOutExpo, easeOutExpo));
        easingFunctions.insert(eastl::make_pair(EaseInOutExpo, easeInOutExpo));
        easingFunctions.insert(eastl::make_pair(EaseInCirc, easeInCirc));
        easingFunctions.insert(eastl::make_pair(EaseOutCirc, easeOutCirc));
        easingFunctions.insert(eastl::make_pair(EaseInOutCirc, easeInOutCirc));
        easingFunctions.insert(eastl::make_pair(EaseInBack, easeInBack));
        easingFunctions.insert(eastl::make_pair(EaseOutBack, easeOutBack));
        easingFunctions.insert(eastl::make_pair(EaseInOutBack, easeInOutBack));
        easingFunctions.insert(eastl::make_pair(EaseInElastic, easeInElastic));
        easingFunctions.insert(eastl::make_pair(EaseOutElastic, easeOutElastic));
        easingFunctions.insert(eastl::make_pair(EaseInOutElastic, easeInOutElastic));
        easingFunctions.insert(eastl::make_pair(EaseInBounce, easeInBounce));
        easingFunctions.insert(eastl::make_pair(EaseOutBounce, easeOutBounce));
        easingFunctions.insert(eastl::make_pair(EaseInOutBounce, easeInOutBounce));
        easingFunctions.insert(eastl::make_pair(EaseLinear, easeLinear));
    }

    auto it = easingFunctions.find(function);
    return it == easingFunctions.end() ? nullptr : it->second;
}
} // namespace Urho3D
