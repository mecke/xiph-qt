/*
 *  XCACodecTests.cpp
 *
 *    XCACodec class test cases.
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
 *  Last modified: $Id: XCACodecTests.cpp 12435 2007-02-06 01:49:26Z arek $
 *
 */


#include "XCACodecTests.h"


XCACodecTests::XCACodecTests(TestInvocation *invocation)
    : TestCase(invocation)
{
}


XCACodecTests::~XCACodecTests()
{
}

void XCACodecTests::setUp()
{
    //mCodec = new XCACodec();
    mCodec = new test_XCACodec();
}

void XCACodecTests::tearDown()
{
    delete mCodec;
    mCodec = NULL;
}

void XCACodecTests::noop()
{
}

void XCACodecTests::append_uninitialized()
{
    UInt32 bytes = 0;
    UInt32 packets = 0;

    Boolean appended = false;

    try {
        mCodec->AppendInputData(NULL, bytes, packets, NULL);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == false);
}

void XCACodecTests::append_zero()
{
    UInt32 bytes = 0;
    UInt32 packets = 0;

    Boolean appended = false;
    Boolean other_error = false;
    ComponentResult ac_error = kAudioCodecNoError;

    mCodec->Initialize(NULL, NULL, NULL, 0);
    CPTAssert(mCodec->IsInitialized());

    try {
        mCodec->AppendInputData(NULL, bytes, packets, NULL);
        appended = true;
    } catch (ComponentResult acError) {
        ac_error = acError;
    } catch (...) {
        other_error = true;
    };

    CPTAssert(appended == false);
    CPTAssert(bytes == 0);
    CPTAssert(packets == 0);
    CPTAssert(other_error == false);

    /* There is no apparent appropriate error code for 0-sized input
       on decoding, NotEnoughBufferSpace seems to do the trick on both
       PPC/Intel. */
    CPTAssert(ac_error == kAudioCodecNotEnoughBufferSpaceError);

    mCodec->Uninitialize();
    CPTAssert(mCodec->IsInitialized() == false);
}

XCACodecTests t_xcac_noop(TEST_INVOCATION(XCACodecTests, noop));
XCACodecTests t_xcac_append_uninitialized(TEST_INVOCATION(XCACodecTests,
                                                          append_uninitialized));
XCACodecTests t_xcac_append_zero(TEST_INVOCATION(XCACodecTests, append_zero));
