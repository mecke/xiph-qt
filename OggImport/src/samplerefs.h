/*
 *  samplerefs.h
 *
 *    SampleReference arrays handling utilities header file.
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
 *  Last modified: $Id: samplerefs.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__samplerefs_h__)
#define __samplerefs_h__

#include "importer_types.h"


extern ComponentResult _store_sample_reference(StreamInfo *si, SInt64 *dataOffset, int size, TimeValue duration, short smp_flags);
extern ComponentResult _commit_srefs(OggImportGlobals *globals, StreamInfo *si, Boolean *movie_changed);


#endif /* __samplerefs_h__ */
