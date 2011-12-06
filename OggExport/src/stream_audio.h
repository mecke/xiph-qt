/*
 *  stream_audio.h
 *
 *    Declaration of audio stream related functions of OggExporter.
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
 *  Last modified: $Id: stream_audio.h 12346 2007-01-18 13:45:42Z arek $
 *
 */


#if !defined(__stream_audio_h__)
#define __stream_audio_h__

#include "exporter_types.h"

extern Boolean
can_handle_track__audio(OSType trackType, TimeScale scale,
                        MovieExportGetPropertyUPP getPropertyProc,
                        void *refCon);
extern ComponentResult
validate_movie__audio(OggExportGlobals *globals, Movie theMovie,
                      Track onlyThisTrack, Boolean *valid);
extern ComponentResult
configure_stream__audio(OggExportGlobals *globals, StreamInfo *si);

extern ComponentResult
write_i_header__audio(StreamInfoPtr si, DataHandler data_h,
                      wide *offset);
extern ComponentResult
write_headers__audio(StreamInfoPtr si, DataHandler data_h,
                     wide *offset);
extern ComponentResult
fill_page__audio(OggExportGlobalsPtr globals, StreamInfoPtr si,
                 Float64 max_duration);

extern ComponentResult initialize_stream__audio(StreamInfo *si);
extern void clear_stream__audio(StreamInfo *si);


#define HANDLE_FUNCTIONS__AUDIO { &can_handle_track__audio,     \
            &validate_movie__audio, &configure_stream__audio,   \
            &write_i_header__audio, &write_headers__audio,      \
            &fill_page__audio,                                  \
            &initialize_stream__audio, &clear_stream__audio }


#endif /* __stream_audio_h__ */
