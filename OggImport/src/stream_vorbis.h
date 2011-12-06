/*
 *  stream_vorbis.h
 *
 *    Declaration of Vorbis format related functions of OggImporter.
 *
 *
 *  Copyright (c) 2005-2006  Arek Korbik
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
 *  Last modified: $Id: stream_vorbis.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__stream_vorbis_h__)
#define __stream_vorbis_h__

#include <Ogg/ogg.h>
#include <Vorbis/codec.h>

#include "importer_types.h"

extern int recognize_header__vorbis(ogg_page *op);
extern int verify_header__vorbis(ogg_page *op); //?

extern int initialize_stream__vorbis(StreamInfo *si);
extern void clear_stream__vorbis(StreamInfo *si);
extern ComponentResult create_sample_description__vorbis(StreamInfo *si);

extern int process_first_packet__vorbis(StreamInfo *si, ogg_page *op, ogg_packet *opckt);
extern ComponentResult process_stream_page__vorbis(OggImportGlobals *globals, StreamInfo *si, ogg_page *opg);
extern ComponentResult flush_stream__vorbis(OggImportGlobals *globals, StreamInfo *si, Boolean notify);

#define HANDLE_FUNCTIONS__VORBIS { &process_stream_page__vorbis, &recognize_header__vorbis, \
            &verify_header__vorbis, &process_first_packet__vorbis, &create_sample_description__vorbis, \
            NULL, NULL, &initialize_stream__vorbis, &flush_stream__vorbis, &clear_stream__vorbis, NULL }


#endif /* __stream_vorbis_h__ */
