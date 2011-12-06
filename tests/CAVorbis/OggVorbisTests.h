/*
 *  OggVorbisTests.h
 *
 *    CAOggVorbisDecoder class test cases header file.
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
 *  Last modified: $Id: OggVorbisTests.h 12814 2007-03-27 22:09:32Z arek $
 *
 */


#include <CPlusTest/CPlusTest.h>
#include "CAOggVorbisDecoder.h"

class OggVorbisTests : public TestCase {
public:
    OggVorbisTests(TestInvocation* invocation);
    virtual ~OggVorbisTests();

    void setUp();
    void tearDown();

    CAOggVorbisDecoder *mOggDecoder;

    void noop();
    void append_uninitialized();
    void init_cookie();
    void append_single();
    void append_multiple();
    void audio_offset();
};
