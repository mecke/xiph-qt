/*
 *  OggFLACTests.h
 *
 *    CAOggFLACDecoder class test cases header file.
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
 *  Last modified: $Id: OggFLACTests.h 12356 2007-01-20 00:18:04Z arek $
 *
 */


#include <CPlusTest/CPlusTest.h>
#include "CAOggFLACDecoder.h"

class OggFLACTests : public TestCase {
public:
    OggFLACTests(TestInvocation* invocation);
    virtual ~OggFLACTests();

    void setUp();
    void tearDown();

    CAOggFLACDecoder *mOggDecoder;

    void noop();
    void append_uninitialized();
    void init_cookie();
};
