/*
 *  stream_video.c
 *
 *    Audio tracks related part of OggExporter.
 *
 *
 *  Copyright (c) 2006-2007  Arek Korbik
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
 *  Last modified: $Id: stream_video.c 13634 2007-08-26 20:28:21Z arek $
 *
 */


#include "stream_video.h"

#include "OggExport.h"

#include "fccs.h"
#include "data_types.h"

#include "debug.h"


static void
_frame_decompressed(void *decompressionTrackingRefCon, OSStatus err,
                    ICMDecompressionTrackingFlags dtf,
                    CVPixelBufferRef pixelBuffer, TimeValue64 displayTime,
                    TimeValue64 displayDuration,
                    ICMValidTimeFlags validTimeFlags, void *reserved,
                    void *sourceFrameRefCon)
{
    dbg_printf("[ vOE]  >> [%08lx] :: _frame_decompressed()\n", (UInt32) -1);
    if (!err) {
        StreamInfoPtr si = (StreamInfoPtr) decompressionTrackingRefCon;
        if (dtf & kICMDecompressionTracking_ReleaseSourceData) {
            // if we were responsible for managing source data buffers,
            //  we should release the source buffer here,
            //  using sourceFrameRefCon to identify it.
        }

        if ((dtf & kICMDecompressionTracking_EmittingFrame) && pixelBuffer) {
            ICMCompressionFrameOptionsRef frameOptions = NULL;
            OSType pf = CVPixelBufferGetPixelFormatType(pixelBuffer);

            dbg_printf("[ vOE]   > [%08lx] :: _frame_decompressed() = %ld; %ld,"
                       " %lld, %lld, %ld [%ld '%4.4s' (%ld x %ld)]\n",
                       (UInt32) -1, err,
                       dtf, displayTime, displayDuration, validTimeFlags,
                       CVPixelBufferGetDataSize(pixelBuffer), (char *) &pf,
                       CVPixelBufferGetWidth(pixelBuffer),
                       CVPixelBufferGetHeight(pixelBuffer));

            displayDuration = 25; //?

            // Feed the frame to the compression session.
            err = ICMCompressionSessionEncodeFrame(si->si_v.cs, pixelBuffer,
                                                   displayTime, displayDuration,
                                                   validTimeFlags, frameOptions,
                                                   NULL, NULL );
        }
    }

    dbg_printf("[ vOE] <   [%08lx] :: _frame_decompressed() = %ld\n",
               (UInt32) -1, err);
}

static ComponentResult
_setup_ds(StreamInfoPtr si, ImageDescriptionHandle imgDesc)
{
    ComponentResult err = noErr;
    CFNumberRef number = NULL;
    CFMutableDictionaryRef pba = NULL;
    ICMDecompressionTrackingCallbackRecord dtcr;
    SInt32 w, h;
    OSType pbf = k422YpCbCr8PixelFormat;

    dbg_printf("[ vOE]  >> [%08lx] :: _setup_ds()\n", (UInt32) -1);

    if (si->si_v.ds) {
        ICMDecompressionSessionFlush(si->si_v.ds);
        ICMDecompressionSessionRelease(si->si_v.ds);
        si->si_v.ds = NULL;
    }

    pba = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                    &kCFTypeDictionaryValueCallBacks);

    w = si->si_v.width >> 16;
    h = si->si_v.height >> 16;

    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &w);
    CFDictionaryAddValue(pba, kCVPixelBufferWidthKey, number);
    CFRelease(number);

    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &h);
    CFDictionaryAddValue(pba, kCVPixelBufferHeightKey, number);
    CFRelease(number);

    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &pbf);
    CFDictionaryAddValue(pba, kCVPixelBufferPixelFormatTypeKey, number);
    CFRelease(number);

    dtcr.decompressionTrackingCallback = _frame_decompressed;
    dtcr.decompressionTrackingRefCon = (void *) si;

    err = ICMDecompressionSessionCreate(NULL, imgDesc,
                                        /* sessionOptions */ NULL,
                                        pba,
                                        &dtcr,
                                        &si->si_v.ds);

    CFRelease(pba);

    dbg_printf("[ vOE] <   [%08lx] :: _setup_ds() = %ld\n", (UInt32) -1, err);
    return err;
}

static OSStatus
_frame_compressed(void *efRefCon, ICMCompressionSessionRef session,
                  OSStatus err, ICMEncodedFrameRef ef, void *reserved)
{
    dbg_printf("[ vOE]  >> [%08lx] :: _frame_compressed()\n", (UInt32) -1);
    if (!err) {
        StreamInfoPtr si = (StreamInfoPtr) efRefCon;
        ImageDescriptionHandle imgDesc = NULL;
        ImageDescription *id;
        UInt32 enc_size = ICMEncodedFrameGetDataSize(ef);

        err = ICMEncodedFrameGetImageDescription(ef, &imgDesc);
        if (!err) {
            id = *imgDesc;

            dbg_printf("[ vOE]  f> [%08lx] :: _frame_compressed() = %ld, '%4.4s'"
                       " %08lx %08lx [%d x %d] [%f x %f] %ld %d %d %d\n",
                       (UInt32) -1, err, (char *) &id->cType,
                       id->temporalQuality, id->spatialQuality, id->width,
                       id->height, id->hRes / 65536.0, id->vRes / 65536.0,
                       id->dataSize, id->frameCount, id->depth, id->clutID);
            dbg_printf("[ vOE]  fi [%08lx] :: _frame_compressed() = %lld %ld %ld\n",
                       (UInt32) -1, ICMEncodedFrameGetDecodeDuration(ef),
                       enc_size, ICMEncodedFrameGetBufferSize(ef));
        }

        if (si->si_v.cs_imdsc == NULL)
            si->si_v.cs_imdsc = imgDesc;

        if (si->si_v.op_buffer_size < enc_size) {
            si->si_v.op_buffer = realloc(si->si_v.op_buffer, enc_size);
            si->si_v.op_buffer_size = enc_size;
        }
        memcpy(si->si_v.op_buffer, ICMEncodedFrameGetDataPtr(ef), enc_size);

        /* skip one byte (pre-padding);
           see TheoraEncoder.c, Theora_ImageEncoderEncodeFrame() */
        si->si_v.op.packet = si->si_v.op_buffer + 1;
        si->si_v.op.bytes = enc_size - 1;
        //si->si_v.op.packetno = si->packets_total++;
        //si->si_v.op.granulepos = si->last_grpos++;
        si->si_v.op_flags = ICMEncodedFrameGetMediaSampleFlags(ef);
    }

    dbg_printf("[ vOE] <   [%08lx] :: _frame_compressed() = %ld\n",
               (UInt32) -1, err);
    return err;
}

static ComponentResult _setup_cs(StreamInfoPtr si)
{
    ComponentResult err = noErr;
    ICMEncodedFrameOutputRecord efor;
    SInt32 tmpval = 0;

    dbg_printf("[ vOE]  >> [%08lx] :: _setup_cs()\n", (UInt32) -1);

    if (si->si_v.cs) {
        ICMCompressionSessionCompleteFrames(si->si_v.cs, true, 0, 0);
        ICMCompressionSessionRelease(si->si_v.cs);
        si->si_v.cs = NULL;
    }

    err = ICMCompressionSessionOptionsCreate(NULL, &si->si_v.cs_opts);
    if (err)
        goto bail;


    // We must set this flag to enable P or B frames.
    err = ICMCompressionSessionOptionsSetAllowTemporalCompression(si->si_v.cs_opts,
                                                                  true);
    dbg_printf("[ vOE]  ?1 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    // We must set this flag to enable B frames.
    err = ICMCompressionSessionOptionsSetAllowFrameReordering(si->si_v.cs_opts,
                                                              true);
    dbg_printf("[ vOE]  ?2 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    // Set the maximum key frame rate.
    err = ICMCompressionSessionOptionsSetMaxKeyFrameInterval(si->si_v.cs_opts,
                                                             si->si_v.keyrate);
    dbg_printf("[ vOE]  ?3 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    // This allows the compressor more flexibility
    //  (ie, dropping and coalescing frames).
    err = ICMCompressionSessionOptionsSetAllowFrameTimeChanges(si->si_v.cs_opts,
                                                               true);
    dbg_printf("[ vOE]  ?4 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    // We need durations when we store frames.
    err = ICMCompressionSessionOptionsSetDurationsNeeded(si->si_v.cs_opts, true);
    dbg_printf("[ vOE]  ?5 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    tmpval = si->si_v.quality;
    err = ICMCompressionSessionOptionsSetProperty(si->si_v.cs_opts,
                                                  kQTPropertyClass_ICMCompressionSessionOptions,
                                                  kICMCompressionSessionOptionsPropertyID_Quality,
                                                  sizeof(tmpval),
                                                  &tmpval);
    dbg_printf("[ vOE]  ?6 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    err = ICMCompressionSessionOptionsSetProperty(si->si_v.cs_opts,
                                                  kQTPropertyClass_ICMCompressionSessionOptions,
                                                  kICMCompressionSessionOptionsPropertyID_ExpectedFrameRate,
                                                  sizeof(si->si_v.fps),
                                                  &si->si_v.fps);
    dbg_printf("[ vOE]  ?7 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    tmpval = si->si_v.bitrate;
    err = ICMCompressionSessionOptionsSetProperty(si->si_v.cs_opts,
                                                  kQTPropertyClass_ICMCompressionSessionOptions,
                                                  kICMCompressionSessionOptionsPropertyID_AverageDataRate,
                                                  sizeof(tmpval),
                                                  &tmpval);
    dbg_printf("[ vOE]  ?8 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
    if (err)
        goto bail;

    if (si->si_v.custom != NULL) {
        err = ICMCompressionSessionOptionsSetProperty(si->si_v.cs_opts,
                                                      kQTPropertyClass_ICMCompressionSessionOptions,
                                                      kICMCompressionSessionOptionsPropertyID_CompressorSettings,
                                                      sizeof(si->si_v.custom),
                                                      &si->si_v.custom);
        dbg_printf("[ vOE]  ?9 [%08lx] :: _setup_cs() = %ld\n", (UInt32) -1, err);
        if (err)
            goto bail;
    }

    if (!err) {
        efor.encodedFrameOutputCallback = _frame_compressed;
        efor.encodedFrameOutputRefCon = (void *) si;
        efor.frameDataAllocator = NULL;

        err = ICMCompressionSessionCreate(NULL, si->si_v.width >> 16,
                                          si->si_v.height >> 16,
                                          kVideoFormatXiphTheora, /* fixed for now... */
                                          si->sourceTimeScale, si->si_v.cs_opts,
                                          NULL, &efor, &si->si_v.cs);
        dbg_printf("[ vOE]  ?A [%08lx] :: _setup_cs() = %ld [%lx x %lx]\n",
                   (UInt32) -1, err, si->si_v.width, si->si_v.height);
    }

 bail:
    dbg_printf("[ vOE] <   [%08lx] :: _setup_cs() = %lx (%ld)\n",
               (UInt32) -1, err, err);
    return err;
}

static ComponentResult
_flush_ogg(StreamInfoPtr si, DataHandler data_h, wide *offset)
{
    ComponentResult err = noErr;
    int result = 0;
    ogg_page og;
    Boolean have_pages = true;
    wide tmp;

    while (have_pages) {
        result = ogg_stream_pageout(&si->os, &og);

        if (!result) {
            have_pages = false;
            result = ogg_stream_flush(&si->os, &og);
            if (!result)
                break;
        }

        err = DataHWrite64(data_h, (Ptr) og.header, offset, og.header_len,
                         NULL, 0);
        if (!err) {
            tmp.hi = 0;
            tmp.lo = og.header_len;
            WideAdd(offset, &tmp);
            err = DataHWrite64(data_h, (Ptr) og.body, offset, og.body_len,
                             NULL, 0);
            if (!err) {
                tmp.hi = 0;
                tmp.lo = og.body_len;
                WideAdd(offset, &tmp);
            }
        }
    }

    return err;
}

static void _ready_page(StreamInfoPtr si)
{
    UInt32 len = si->og.header_len + si->og.body_len;
    Float64 pos;

    if (si->og_buffer_size < len) {
        si->og_buffer = realloc(si->og_buffer, len);
        si->og_buffer_size = len;
    }
    BlockMoveData(si->og.header, si->og_buffer, si->og.header_len);
    BlockMoveData(si->og.body, si->og_buffer + si->og.header_len,
                  si->og.body_len);
    si->og.header = si->og_buffer;
    si->og.body = si->og_buffer + si->og.header_len;
    si->og_ready = true;
    si->acc_packets -= ogg_page_packets(&si->og);
    if (ogg_page_granulepos(&si->og) != -1) {
        si->og_grpos = ogg_page_granulepos(&si->og);
        if (si->si_v.grpos_shift > 0) {
            /* with theora, si->og_grpos represents total number of frames */
            ogg_int64_t frames = si->og_grpos >> si->si_v.grpos_shift;
            si->og_grpos = frames + si->og_grpos -
                (frames << si->si_v.grpos_shift);
        }
    }

    pos = si->og_grpos / (si->si_v.fps / 65536.0);
    si->og_ts_sec = (UInt32) pos;
    si->og_ts_subsec = (pos - (Float64) si->og_ts_sec);

    if (ogg_page_eos(&si->og))
        si->eos = true;
}

static ComponentResult _get_frame(StreamInfoPtr si)
{
    ComponentResult err = noErr;
    ICMFrameTimeRecord ft;

    dbg_printf("[ vOE]  >> [%08lx] :: _get_frame()\n", (UInt32) -1);

    if (!si->src_extract_complete) {
        si->gdp.recordSize = sizeof(MovieExportGetDataParams);
        si->gdp.trackID = si->trackID;
        si->gdp.requestedTime = si->time;
        si->gdp.sourceTimeScale = si->sourceTimeScale;
        si->gdp.actualTime = 0;
        si->gdp.dataPtr = NULL;
        si->gdp.dataSize = 0;
        si->gdp.desc = NULL;
        si->gdp.descType = 0;
        si->gdp.descSeed = 0;
        si->gdp.requestedSampleCount = 0;
        si->gdp.actualSampleCount = 0;
        si->gdp.durationPerSample = 1;
        si->gdp.sampleFlags = 0;

        err = InvokeMovieExportGetDataUPP(si->refCon, &si->gdp,
                                          si->getDataProc);
        dbg_printf("[ vOE]  D> [%08lx] :: _get_frame() = %ld; %ld [%ld]"
                   " %ld [%ld] [%ld @ %ld] %ld '%4.4s'\n",
                   (UInt32) -1, err, si->gdp.requestedSampleCount,
                   si->gdp.actualSampleCount, si->gdp.requestedTime,
                   si->gdp.actualTime, si->gdp.durationPerSample,
                   si->gdp.sourceTimeScale, si->gdp.dataSize,
                   (char *) &si->gdp.descType);

        if (!err) {
            //si->time += si->gdp.durationPerSample * si->gdp.actualSampleCount;
            if (si->si_v.fps == 0) {
                si->time += si->gdp.durationPerSample;
            } else {
                si->si_v.frames_time += si->gdp.sourceTimeScale /
                    (si->si_v.fps / 65536.0);
                si->time = si->si_v.frames_time;
            }
        }

        if (err == eofErr) {
            err = noErr;
            si->src_extract_complete = true;

            if (si->si_v.ds) {
                err = ICMDecompressionSessionFlush(si->si_v.ds);
                dbg_printf("[ vOE]  dF [%08lx] :: _get_frame() = %ld\n",
                           (UInt32) -1, err);
            }

            /* TODO: flush compressor by one frame only, not all at once */
            if (si->si_v.cs) {
                err = ICMCompressionSessionCompleteFrames(si->si_v.cs, true, 0, 0);
                dbg_printf("[ vOE]  cF [%08lx] :: _get_frame() = %ld\n",
                           (UInt32) -1, err);
            }
        }

        if (!err) {
            if (si->gdp.descType == 0) {
                si->src_extract_complete = true;
            } else if (si->gdp.descType != VideoMediaType) {
                err = paramErr;
                goto bail;
            } else {
                                                        
                ImageDescription *id = *(ImageDescriptionHandle) si->gdp.desc;
                dbg_printf("[ vOE]  I> [%08lx] :: _get_frame() = '%4.4s' %08lx"
                           " %08lx [%d x %d] [%f x %f] %ld %d %d %d\n",
                           (UInt32) -1, (char *) &id->cType,
                           id->temporalQuality, id->spatialQuality, id->width,
                           id->height, id->hRes / 65536.0, id->vRes / 65536.0,
                           id->dataSize, id->frameCount, id->depth, id->clutID);

                if (si->gdp.descSeed != si->lastDescSeed) {

                    if (si->out_buffer) {
                        free(si->out_buffer);
                        si->out_buffer = NULL;
                        si->out_buffer_size = 0;
                    }

                    if (si->si_v.width == 0)
                        si->si_v.width = id->width << 16;
                    else
                        si->si_v.width = FixRound(si->si_v.width);

                    if (si->si_v.height == 0)
                        si->si_v.height = id->height << 16;
                    else
                        si->si_v.height = FixRound(si->si_v.height);

                    if (si->si_v.depth == 0)
                        si->si_v.depth = id->depth;


                    err = _setup_cs(si);
                    if (err)
                        goto bail;

                    err = _setup_ds(si, (ImageDescriptionHandle) si->gdp.desc);
                    if (err)
                        goto bail;

                    // Allocate memory enough to store maximum compressed data
                    si->out_buffer_size = si->si_v.width * si->si_v.height * 3;
                    si->out_buffer = calloc(1, si->out_buffer_size);

                    err = MemError();
                    if (err) goto bail;

                    si->lastDescSeed = si->gdp.descSeed;

                    dbg_printf("[ vOE]   x [%08lx] :: _get_frame() = [%f x %f] @ %d\n",
                               (UInt32) -1, si->si_v.width / 65536.0,
                               si->si_v.height / 65536.0, si->si_v.depth);
                }

                memset(&ft, 0, sizeof(ICMFrameTimeRecord));
                ft.recordSize = sizeof(ICMFrameTimeRecord);
                *(TimeValue64 *) &ft.value = si->time;
                ft.scale = si->sourceTimeScale;
                ft.rate = fixed1;
                //ft.frameNumber = 0;
                ft.duration = si->gdp.durationPerSample;
                //ft.flags = 0;
                //ft.flags = icmFrameTimeIsNonScheduledDisplayTime;
                ft.flags = icmFrameTimeDecodeImmediately;

                err = ICMDecompressionSessionDecodeFrame(si->si_v.ds,
                                                         (UInt8 *) si->gdp.dataPtr,
                                                         si->gdp.dataSize,
                                                         /* sess_opts */ NULL,
                                                         &ft,
                                                         si);
                if (err) goto bail;
                                                        
            }

            si->gdp.actualSampleCount = 0;
        }
    }

 bail:
    dbg_printf("[ vOE] <   [%08lx] :: _get_frame() = %ld\n", (UInt32) -1, err);
    return err;
}


/* ======================================================================= */

Boolean can_handle_track__video(OSType trackType, TimeScale scale,
                                MovieExportGetPropertyUPP getPropertyProc,
                                void *refCon)
{
    if (!scale || !trackType || !getPropertyProc)
        return false;

    if (trackType == VideoMediaType)
        return true;

    return false;
}

ComponentResult validate_movie__video(OggExportGlobals *globals,
                                      Movie theMovie, Track onlyThisTrack,
                                      Boolean *valid)
{
    ComponentResult err = noErr;

    return err;
}

ComponentResult initialize_stream__video(StreamInfo *si)
{
    ComponentResult err = noErr;

    memset(&si->si_v.op, 0, sizeof(si->si_v.op));

    si->si_v.cs = NULL;
    si->si_v.cs_opts = NULL;
    si->si_v.ds = NULL;
    si->si_v.ds_opts = NULL;

    si->si_v.cs_imdsc = NULL;

    si->si_v.width = 0;
    si->si_v.height = 0;
    si->si_v.fps = kEOS_V_default_fps;
    si->si_v.quality = 0;
    si->si_v.bitrate = 0;
    si->si_v.keyrate = kEOS_V_default_keyrate;
    si->si_v.custom = NULL;

    si->si_v.grpos_shift = 0;
    si->si_v.op_flags = 0;
    si->si_v.frames_time = 0;

    si->stream_type = VideoMediaType;

    // allocate initial space for ogg_package
    si->si_v.op_buffer_size = kOES_V_init_op_size;
    si->si_v.op_buffer = calloc(1, kOES_V_init_op_size);

    err = MemError();

    if (!err) {
        si->si_v.stdVideo = NULL;
#if 1
        err = OpenADefaultComponent(StandardCompressionType,
                                    StandardCompressionSubType,
                                    &si->si_v.stdVideo);
#else
        // the 'vide' one is not documented :/
        err = OpenADefaultComponent(StandardCompressionType,
                                    'vide',
                                    &si->si_v.stdVideo);
#endif
        if (err) {
            if (si->si_v.op_buffer) {
                free(si->si_v.op_buffer);
                si->si_v.op_buffer = NULL;
                si->si_v.op_buffer_size = 0;
            }
        }
    }

    return err;
}

void clear_stream__video(StreamInfo *si)
{
    if (si->si_v.op_buffer) {
        free(si->si_v.op_buffer);
        si->si_v.op_buffer = NULL;
        si->si_v.op_buffer_size = 0;
    }

    if (si->si_v.ds) {
        ICMDecompressionSessionFlush(si->si_v.ds); // !?
        ICMDecompressionSessionRelease(si->si_v.ds);
        si->si_v.ds = NULL;
    }

    if (si->si_v.cs) {
        ICMCompressionSessionCompleteFrames(si->si_v.cs, true, 0, 0); // !?
        ICMCompressionSessionRelease(si->si_v.cs);
        si->si_v.cs = NULL;
    }
}

ComponentResult configure_stream__video(OggExportGlobals *globals,
                                        StreamInfo *si)
{
    ComponentResult err = noErr;
    SCSpatialSettings ss = {0, NULL, 0, 0};
    SCTemporalSettings ts = {0, 0, 0};
    SCDataRateSettings ds = {0, 0, 0, 0};
    UInt32 tmp = 0;
    Fixed tmp_fixed = 0;
    Boolean useConfiguredSettings;

    dbg_printf("[ vOE]  >> [%08lx] :: configure_stream()\n", (UInt32) globals);

    err = InvokeMovieExportGetPropertyUPP(si->refCon, si->trackID,
                                          movieExportUseConfiguredSettings, &useConfiguredSettings,
                                          si->getPropertyProc);
    if (!err && useConfiguredSettings)
    {
        // useConfiguredSettings -> if this is set to true, the current settings
        // should be used as a starting point.  Otherwise,  we should use
        // default settings as the starting point.  Current settings would
        // originate, for example, from MovieExportSetSettingsFromAtomContainer
        // or from doing a MovieExportDoUserDialog

        si->si_v.quality = globals->set_v_quality;
        si->si_v.bitrate = globals->set_v_bitrate;
        if (globals->set_v_keyrate != 0)
            si->si_v.keyrate = globals->set_v_keyrate;

        if (globals->set_v_fps != 0)
            si->si_v.fps = globals->set_v_fps;
        else if (globals->movie_fps != 0)
            si->si_v.fps = globals->movie_fps;

    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Temporal Settings

    ts.temporalQuality = codecNormalQuality;
    ts.frameRate = si->si_v.fps;
    ts.frameRate = si->si_v.keyrate;

    err = InvokeMovieExportGetPropertyUPP(si->refCon, si->trackID,
                                          scTemporalSettingsType, &ts,
                                          si->getPropertyProc);
    dbg_printf("[ vOE]  ts [%08lx] :: configure_stream() = %ld, [%ld, %f, %ld]\n", (UInt32) globals, err,
               ts.temporalQuality, ts.frameRate / 65536.0, ts.keyFrameRate);
    if (!err) {
        if (ts.frameRate != 0)
            si->si_v.fps = ts.frameRate;
        else if (globals->set_v_fps != 0)
            si->si_v.fps = globals->set_v_fps;
        else if (globals->movie_fps != 0)
            si->si_v.fps = globals->movie_fps;
        //else leave the default setting

        if (ts.keyFrameRate != 0)
            si->si_v.keyrate = ts.keyFrameRate;
        else if (globals->set_v_keyrate != 0)
            si->si_v.keyrate = globals->set_v_keyrate;
        //else leave the default setting
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Spatial Settings

    ss.spatialQuality = si->si_v.quality;
    ss.depth = 24;

    err = InvokeMovieExportGetPropertyUPP(si->refCon, si->trackID,
                                          scSpatialSettingsType, &ss,
                                          si->getPropertyProc);
    dbg_printf("[ vOE]  ss [%08lx] :: configure_stream() = %ld, ['%4.4s', %08lx, %d, %ld]\n", (UInt32) globals, err,
               (char *) &ss.codecType, (UInt32) ss.codec, ss.depth, ss.spatialQuality);
    if (!err) {
        si->si_v.quality = ss.spatialQuality;
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Data Rate Settings

    ds.dataRate = globals->set_v_bitrate;
    ds.frameDuration = -1;
    ds.minSpatialQuality = codecMinQuality;
    ds.minTemporalQuality = codecMinQuality;

    err = InvokeMovieExportGetPropertyUPP(si->refCon, si->trackID,
                                          scDataRateSettingsType, &ds,
                                          si->getPropertyProc);
    dbg_printf("[ vOE]  ds [%08lx] :: configure_stream() = %ld, [%ld, %ld, %ld, %ld]\n", (UInt32) globals, err,
               ds.dataRate, ds.frameDuration, ds.minSpatialQuality, ds.minTemporalQuality);
    if (!err) {
        if (ds.dataRate != 0)
            si->si_v.bitrate = ds.dataRate;
        else if (globals->set_v_bitrate != 0)
            si->si_v.bitrate = globals->set_v_bitrate;
    }

    tmp_fixed = si->si_v.width;
    if (InvokeMovieExportGetPropertyUPP(si->refCon, si->trackID,
                                        movieExportWidth, &tmp_fixed,
                                        si->getPropertyProc) == noErr)
        si->si_v.width = tmp_fixed;

    tmp_fixed = si->si_v.height;
    if (InvokeMovieExportGetPropertyUPP(si->refCon, si->trackID,
                                        movieExportHeight, &tmp_fixed,
                                        si->getPropertyProc) == noErr)
        si->si_v.height = tmp_fixed;

    tmp = si->si_v.keyrate - 1;
    si->si_v.grpos_shift = 0;
    while (tmp > 0) {
        si->si_v.grpos_shift += 1;
        tmp >>= 1;
    }

    if (globals->set_v_custom != NULL)
        si->si_v.custom = globals->set_v_custom;

    dbg_printf("[ vOE]   x [%08lx] :: configure_stream() = [%f x %f]\n",
               (UInt32) globals, si->si_v.width / 65536.0,
               si->si_v.height / 65536.0);

    if (err == paramErr) {
        // don't return the last error from InvokeMovieExportGetPropertyUPP
        // since it is allowed to return paramErr when it does not want to
        // specify a parameter...
        err = noErr;
    }

    dbg_printf("[ vOE] <   [%08lx] :: configure_stream() = %ld\n", (UInt32) globals, err);
    return err;
}


ComponentResult
fill_page__video(OggExportGlobalsPtr globals, StreamInfoPtr si,
                 Float64 max_duration)
{
    ComponentResult err = noErr;

    Boolean eos_hit = false;
    Boolean do_loop = true;

    UInt32 max_page_packets = (UInt32) (max_duration *
                                        si->si_v.fps / 65536.0);
    if (max_page_packets < 1)
        max_page_packets = 1;

    dbg_printf("[ vOE]  >> [%08lx] :: fill_page(%lf [%ld / %f])\n",
               (UInt32) globals, max_duration, max_page_packets,
               si->si_v.fps / 65536.0);

    if (ogg_stream_pageout(&si->os, &si->og) > 0) {
        _ready_page(si);
    } else {
        while (do_loop) {
            int result = 0;
            if (si->si_v.op.packet != NULL) {
                si->acc_packets++;
                //si->acc_duration = si->acc_packets / (si->si_v.fps / 65536.0);
                si->si_v.op.b_o_s = 0;

                /* TODO: make eos identification more general, now it
                   works only if the compressor queues frames ahead */
                if (si->src_extract_complete)
                    si->si_v.op.e_o_s = 1;
                else
                    si->si_v.op.e_o_s = 0;

                si->si_v.op.packetno = si->packets_total++;
                if (si->si_v.op_flags & mediaSampleNotSync ||
                    si->si_v.grpos_shift == 0) {
                    si->last_grpos++;
                } else {
                    ogg_int64_t frames = 0;
                    if (si->last_grpos >= 0) {
                        frames = si->last_grpos >> si->si_v.grpos_shift;
                        frames += si->last_grpos -
                            (frames << si->si_v.grpos_shift) + 1;
                    }
                    si->last_grpos = frames << si->si_v.grpos_shift;
                }
                si->si_v.op.granulepos = si->last_grpos;
                result = ogg_stream_packetin(&si->os, &si->si_v.op);
                dbg_printf("[ vOE] _i  [%08lx] :: fill_page(): "
                           "ogg_stream_packetin(%lld, %ld, %016llx) = %d\n",
                           (UInt32) globals, si->si_v.op.packetno,
                           si->si_v.op.bytes, si->si_v.op.granulepos,
                           result);
                si->si_v.op.bytes = 0;
                si->si_v.op.packet = NULL;
                si->si_v.op_flags = 0;
            }

            if (si->acc_packets > 0) {
            if (((eos_hit || si->acc_packets > max_page_packets) &&
                 ogg_stream_flush(&si->os, &si->og) > 0) ||
                (ogg_stream_pageout(&si->os, &si->og) > 0)) {
                _ready_page(si);
                do_loop = false;
                break;
            }
            }

            err = _get_frame(si);
            dbg_printf("[ vOE]  1# [%08lx] :: fill_page() = %ld\n",
                       (UInt32) globals, err);
            if (err)
                break;
        }
    }

    if (si->src_extract_complete) {
        si->eos = true;
    }

    dbg_printf("[ vOE] <   [%08lx] :: fill_page() = %ld (%ld, %lf)\n",
               (UInt32) globals, err, si->og_ts_sec, si->og_ts_subsec);
    return err;
}

ComponentResult write_i_header__video(StreamInfoPtr si, DataHandler data_h,
                                      wide *offset)
{
    ComponentResult err = noErr;
    Handle ext;

    /* pull frames until we get one, thus being able to get
       video-encoded frame's sample description and the magic
       cookie */

    while (si->si_v.op.packet == NULL || si->src_extract_complete) {
        err = _get_frame(si);
        if (err)
            break;
    }

    if (si->si_v.op.packet == NULL) {
        // flush en/de-coder queues?
    }

    if (!err)
        err = GetImageDescriptionExtension(si->si_v.cs_imdsc, &ext,
                                           kSampleDescriptionExtensionTheora, 1);
    if (!err) {
        Byte *ptrheader, *mCookie, *cend;
        UInt32 mCookieSize;
        CookieAtomHeader *aheader;
        //th_comment tc;
        ogg_packet header, header_tc, header_cb;

        mCookie = (UInt8 *) *ext;
        mCookieSize = GetHandleSize(ext);

        ptrheader = mCookie;
        cend = mCookie + mCookieSize;

        aheader = (CookieAtomHeader *) ptrheader;

        header.bytes = header_tc.bytes = header_cb.bytes = 0;

        while (ptrheader < cend) {
            aheader = (CookieAtomHeader *) ptrheader;
            ptrheader += EndianU32_BtoN(aheader->size);
            if (ptrheader > cend || EndianU32_BtoN(aheader->size) <= 0)
                break;

            switch(EndianS32_BtoN(aheader->type)) {
            case kCookieTypeTheoraHeader:
                header.b_o_s = 1;
                header.e_o_s = 0;
                header.granulepos = 0;
                header.packetno = 0;
                header.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
                header.packet = aheader->data;
                break;

            case kCookieTypeTheoraComments:
                header_tc.b_o_s = 0;
                header_tc.e_o_s = 0;
                header_tc.granulepos = 0;
                header_tc.packetno = 1;
                header_tc.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
                header_tc.packet = aheader->data;
                break;

            case kCookieTypeTheoraCodebooks:
                header_cb.b_o_s = 0;
                header_cb.e_o_s = 0;
                header_cb.granulepos = 0;
                header_cb.packetno = 2;
                header_cb.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
                header_cb.packet = aheader->data;
                break;

            default:
                break;
            }
        }

        if (header.bytes == 0 || header_tc.bytes == 0 || header_cb.bytes == 0) {
            err = codecBadDataErr;
        } else {
            ogg_stream_packetin(&si->os, &header);
            _flush_ogg(si, data_h, offset);

            ogg_stream_packetin(&si->os, &header_tc);
            ogg_stream_packetin(&si->os, &header_cb);
            //_flush_ogg(si, data_h, offset);

            si->packets_total = 3;
            si->acc_packets = 2;
            si->acc_duration = 0;
        }

        DisposeHandle(ext);
    }

    return err;
}

ComponentResult write_headers__video(StreamInfoPtr si, DataHandler data_h,
                                     wide *offset)
{
    /* Simplified at the moment - packets are pulled into the stream in the
     * initial header setup, and here we assume they are still in the stream
     * and just flush them.
     */
    ComponentResult err = noErr;

    err = _flush_ogg(si, data_h, offset);

    if (!err) {
        si->acc_packets = 0;
        si->acc_duration = 0;
    }

    return err;
}
