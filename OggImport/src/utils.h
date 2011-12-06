/*
 *  utils.h
 *
 *    Support functions header file.
 *
 *
 *  Copyright (c) 2006,2007  Arek Korbik
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
 *  Last modified: $Id: utils.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__ogg_utils_h__)
#define __ogg_utils_h__

#include "config.h"
#include <ogg/ogg.h>
#include <Vorbis/codec.h>

extern int unpack_vorbis_comments(vorbis_comment *vc, const void *data, UInt32 data_size);
extern void find_last_page_GP(const unsigned char *data, UInt32 data_size,
                              ogg_int64_t *gp, long *serialno);


#endif /* __ogg_utils_h__ */
