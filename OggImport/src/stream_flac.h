/*
 *  stream_flac.h
 *
 *    Declaration of FLAC format related functions of OggImporter.
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
 *  Last modified: $Id: stream_flac.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__stream_flac_h__)
#define __stream_flac_h__

// TODO: #include <FLAC/all.h>

#include "importer_types.h"

#define FLAC_MAPPING_SUPPORTED_MAJOR 1


extern int recognize_header__flac(ogg_page *op);
extern int verify_header__flac(ogg_page *op);

extern int initialize_stream__flac(StreamInfo *si);
extern void clear_stream__flac(StreamInfo *si);
extern ComponentResult create_sample_description__flac(StreamInfo *si);

extern int process_first_packet__flac(StreamInfo *si, ogg_page *op, ogg_packet *opckt);
extern ComponentResult process_stream_page__flac(OggImportGlobals *globals, StreamInfo *si, ogg_page *opg);
extern ComponentResult flush_stream__flac(OggImportGlobals *globals, StreamInfo *si, Boolean notify);

#define HANDLE_FUNCTIONS__FLAC { &process_stream_page__flac, &recognize_header__flac, \
            &verify_header__flac, &process_first_packet__flac, &create_sample_description__flac, \
            NULL, NULL, &initialize_stream__flac, &flush_stream__flac, &clear_stream__flac, NULL }


#endif /* __stream_flac_h__ */
