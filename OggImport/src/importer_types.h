/*
 *  importer_types.h
 *
 *    Definitions of OggImporter data structures.
 *
 *
 *  Copyright (c) 2005-2007  Arek Korbik
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
 *  Last modified: $Id: importer_types.h 13696 2007-09-02 15:34:00Z arek $
 *
 */


#if !defined(__importer_types_h__)
#define __importer_types_h__

#if defined(__APPLE_CC__)
#include <QuickTime/QuickTime.h>
#include <Ogg/ogg.h>
#else
#include <QuickTimeComponents.h>
#include <ogg/ogg.h>

#if defined(TARGET_OS_WIN32)
#define _WINIOCTL_
#include <windows.h>
#endif

#if defined(__DO_WE_NEED_ALL_THOSE_P__)
#include <MacTypes.h>
#include <MacErrors.h>
#include <Endian.h>
#include <MacMemory.h>
#include <Resources.h>
#include <Components.h>
#include <Sound.h>
#include <QuickTimeComponents.h>
#include <FixMath.h>
#include <Math64.h>
#include <IntlResources.h>
#include <MoviesFormat.h>
#include <Gestalt.h>
#include <TextEncodingConverter.h>
#endif

#endif
#include "rb.h"


#include "stream_types_vorbis.h"
#include "stream_types_speex.h"
#include "stream_types_flac.h"
#include "stream_types_theora.h"

#define INCOMPLETE_PAGE_DURATION 1

typedef enum ImportStates {
    kStateInitial,
    kStateReadingPages,
    kStateReadingLastPages,
    kStateImportComplete
} ImportStates;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			types
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Keep your data long word aligned for best performance

struct stream_format_handle_funcs; //forward declaration

typedef struct {
    long                serialno;

    ogg_stream_state	os;

    int                 numChannels;
    int                 rate;
    Handle              soundDescExtension;

    Track               theTrack;
    Media               theMedia;
    SampleDescriptionHandle sampleDesc;

    ogg_int64_t         lastGranulePos;

    TimeValue           incompleteCompensation;

    TimeValue           lastMediaInserted;
    TimeValue           mediaLength;
    TimeValue           insertTime;

    //SampleReference array
    SampleReference64Record *sample_refs;
    UInt32              sample_refs_size;
    UInt32              sample_refs_count;
    TimeValue           sample_refs_duration;
    UInt32              sample_refs_increment;

    TimeValue           streamOffset;
    SInt32              streamOffsetSamples;

    CFDictionaryRef		MDmapping;
    CFDictionaryRef		UDmapping;

    struct stream_format_handle_funcs *sfhf;

    union {
#if defined(_HAVE__VORBIS_SUPPORT)
        StreamInfo__vorbis si_vorbis;
#endif
#if defined(_HAVE__SPEEX_SUPPORT)
        StreamInfo__speex si_speex;
#endif
#if defined(_HAVE__FLAC_SUPPORT)
        StreamInfo__flac si_flac;
#endif
#if defined(_HAVE__THEORA_SUPPORT)
        StreamInfo__theora si_theora;
#endif
    };

} StreamInfo, *StreamInfoPtr;


typedef struct {
    ComponentInstance	    self;

    Movie               theMovie;
    Handle              dataRef;
    long                dataRefType;
    Boolean             dataIsStream;
    Boolean             usingIdle;
    int                 chunkSize;
    TimeValue           startTime;

    ImportStates            state;
    ComponentResult         errEncountered;

    SInt64                  dataStartOffset;
    SInt64                  dataEndOffset;

    IdleManager             idleManager;
    IdleManager             dataIdleManager;
    Boolean                 idleManagersLinked;

    // ogg grouped and chained streams support variables
    Boolean                 groupStreamsFound;
    TimeValue               currentGroupOffset;
    Float64                 currentGroupOffsetSubSecond;   // same as with timeLoaded

    long                    newMovieFlags;

    ComponentInstance	    dataReader;

    DataHCompletionUPP	    dataReadCompletion;

    wide                    wideTempforFileSize;

    int	                    dataReadChunkSize;

    Boolean                 dataCanDoAsyncRead;

    Boolean                 dataCanDoScheduleData;
    Boolean                 dataCanDoScheduleData64;
    Boolean                 dataCanDoGetFileSize64;

    Boolean                 blocking;
    AliasHandle             aliasHandle;

    ComponentResult         readError;

    /* information about the data buffer */
    Ptr                     dataBuffer;
    int                     maxDataBufferSize;
    SInt64                  dataOffset;
    unsigned char           *validDataEnd;
    unsigned char           *currentData;

    ring_buffer				dataRB;

    int                     numTracksStarted;
    int                     numTracksSeen;		// completed tracks
    Track                   firstTrack;
    Boolean                 hasVideoTrack;

    TimeValue               timeLoaded;
    Float64                 timeLoadedSubSecond;        // last second fraction remainder
    TimeValue               timeFlushed;                // movie timescale precision only

    UInt32                  tickFlushed;
    UInt32                  flushStepCheck;
    UInt32                  flushStepNormal;
    UInt32                  flushMinTimeLeft;
    UInt32                  tickNotified;
    UInt32                  notifyStep;

    unsigned long           startTickCount;

    Track                   phTrack; /**< placeholder track for async importing */

    int                     streamCount;
    StreamInfo              **streamInfoHandle;

    Boolean					dataRequested;
    Boolean					sizeInitialised;

    long                lp_serialno;
    ogg_int64_t         lp_GP;
    TimeValue           totalTime;
    Boolean             calcTotalTime;

    TimeBase            idleTimeBase;
} OggImportGlobals, *OggImportGlobalsPtr;


typedef int (*recognize_header) (ogg_page *op);
typedef int (*verify_header) (ogg_page *op);

typedef int (*initialize_stream) (StreamInfo *si);
typedef void (*clear_stream) (StreamInfo *si);
typedef ComponentResult (*create_sample_description) (StreamInfo *si);
typedef ComponentResult (*create_track) (OggImportGlobals *globals, StreamInfo *si);
typedef ComponentResult (*create_track_media) (OggImportGlobals *globals, StreamInfo *si, Handle data_ref);

typedef int (*process_first_packet) (StreamInfo *si, ogg_page *op, ogg_packet *opckt);
typedef ComponentResult (*process_stream_page) (OggImportGlobals *globals, StreamInfo *si, ogg_page *opg);
typedef ComponentResult (*flush_stream) (OggImportGlobals *globals, StreamInfo *si, Boolean notify);

typedef ComponentResult (*granulepos_to_time) (StreamInfo *si, ogg_int64_t *gp, TimeRecord *time);


typedef struct stream_format_handle_funcs {
    process_stream_page                 process_page;

    recognize_header                    recognize;
    verify_header                       verify;

    process_first_packet                first_packet;
    create_sample_description           sample_description;
    create_track                        track;
    create_track_media                  track_media;

    initialize_stream                   initialize;
    flush_stream                        flush;
    clear_stream                        clear;

    granulepos_to_time                  gp_to_time;
} stream_format_handle_funcs;

#define HANDLE_FUNCTIONS__NULL { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#endif /* __importer_types_h__ */
