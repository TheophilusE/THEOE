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

#pragma once

#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/Protocol.h"

namespace Urho3D
{

template <class T>
T ReadNetworkMessage(MemoryBuffer& src)
{
    T msg;
    msg.Load(src);
    return msg;
}

struct MsgPingPong
{
    unsigned magic_{};

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

struct MsgSynchronize
{
    unsigned magic_{};

    unsigned connectionId_{};
    unsigned updateFrequency_{};

    unsigned numTrimmedClockSamples_{};
    unsigned numOngoingClockSamples_{};

    unsigned lastFrame_{};
    unsigned ping_{};

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

struct MsgSynchronizeAck
{
    unsigned magic_{};

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

struct MsgClock
{
    unsigned lastFrame_{};
    unsigned ping_{};

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

}
