/*
 *  stream_theora.h
 *
 *    Declaration of Theora format related functions of OggImporter.
 *
 *
 *  Copyright (c) 2006  Arek Korbik
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
 *  Last modified: $Id: stream_theora.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__stream_theora_h__)
#define __stream_theora_h__

#include <Ogg/ogg.h>
//#include <Theora/theora.h>

#include "importer_types.h"

extern int recognize_header__theora(ogg_page *op);
extern int verify_header__theora(ogg_page *op); //?

extern int initialize_stream__theora(StreamInfo *si);
extern void clear_stream__theora(StreamInfo *si);

extern ComponentResult create_sample_description__theora(StreamInfo *si);
extern ComponentResult create_track__theora(OggImportGlobals *globals, StreamInfo *si);
extern ComponentResult create_track_media__theora(OggImportGlobals *globals, StreamInfo *si, Handle data_ref);

extern int process_first_packet__theora(StreamInfo *si, ogg_page *op, ogg_packet *opckt);
extern ComponentResult process_stream_page__theora(OggImportGlobals *globals, StreamInfo *si, ogg_page *opg);
extern ComponentResult flush_stream__theora(OggImportGlobals *globals, StreamInfo *si, Boolean notify);

extern ComponentResult granulepos_to_time__theora(StreamInfo *si, ogg_int64_t *gp, TimeRecord *time);

#define HANDLE_FUNCTIONS__THEORA { &process_stream_page__theora, &recognize_header__theora, \
            &verify_header__theora, &process_first_packet__theora, &create_sample_description__theora, \
            &create_track__theora, &create_track_media__theora, &initialize_stream__theora, flush_stream__theora, \
            &clear_stream__theora, &granulepos_to_time__theora }


#endif /* __stream_theora_h__ */
