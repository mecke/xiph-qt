/*
 *  wrap_ogg.cpp
 *
 *    WrapOggPage helper function - constructs an ogg_page 'around'
 *    a block of memory.
 *
 *
 *  Copyright (c) 2005  Arek Korbik
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
 *  Last modified: $Id: wrap_ogg.cpp 10575 2005-12-10 19:33:16Z arek $
 *
 */


#include "wrap_ogg.h"
#include <string>

Boolean WrapOggPage(ogg_page* outOggPage, const void* inRawData, UInt32 inDataByteSize, UInt32 inDataStartOffset)
{
    if (inDataByteSize - inDataStartOffset < 27)
        return false;

    const Byte* data = static_cast<const Byte*> (inRawData) + inDataStartOffset;

    if (memcmp(data, "OggS", 4) != 0)
        return false;

    UInt32 headerBytes = data[26] + 27;

    if (inDataByteSize - inDataStartOffset < headerBytes)
        return false;

    UInt32 bodyBytes = 0;
    UInt32 i;

    /* just checking... */
    for (i = 0; i < data[26]; i++) {
        bodyBytes += data[27 + i];
    }
    if (bodyBytes > inDataByteSize - inDataStartOffset)
        return false;

    outOggPage->header = const_cast<unsigned char*> (data);
    outOggPage->header_len = headerBytes;
    outOggPage->body = const_cast<unsigned char*> (data + headerBytes);
    outOggPage->body_len = bodyBytes;

    return true;
}
