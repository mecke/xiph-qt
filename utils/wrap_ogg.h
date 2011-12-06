/*
 *  wrap_ogg.h
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
 *  Last modified: $Id: wrap_ogg.h 10574 2005-12-10 16:22:13Z arek $
 *
 */


#if !defined(__wrap_ogg_h__)
#define __wrap_ogg_h__


#include "config.h"
#include <Ogg/ogg.h>

extern Boolean WrapOggPage(ogg_page* outOggPage, const void* inRawData,
                           UInt32 inDataByteSize, UInt32 inDataStartOffset);


#endif /* __wrap_ogg_h__ */
