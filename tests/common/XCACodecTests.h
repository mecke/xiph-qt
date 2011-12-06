/*
 *  XCACodecTests.h
 *
 *    XCACodec class test cases header file.
 *
 *
 *  Copyright (c) 2007  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: XCACodecTests.h 12435 2007-02-06 01:49:26Z arek $
 *
 */


#include <CPlusTest/CPlusTest.h>
#include "XCACodec.h"

class test_XCACodec : public XCACodec {
 protected:
    virtual void InPacket(const void* inInputData,
                          const AudioStreamPacketDescription* inPacketDescription) {};

    virtual UInt32 FramesReady() const { return 0; };
    virtual Boolean GenerateFrames() { return false; };
    virtual void OutputFrames(void* outOutputData, UInt32 inNumberFrames,
                              UInt32 inFramesOffset,
                              AudioStreamPacketDescription* outPacketDescription) const {};
    virtual void Zap(UInt32 inFrames) {};

};

class XCACodecTests : public TestCase {
public:
    XCACodecTests(TestInvocation* invocation);
    virtual ~XCACodecTests();

    void setUp();
    void tearDown();

    XCACodec *mCodec;

    void noop();
    void append_uninitialized();
    void append_zero();
};
