/*
 *  stream_audio.c
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
 *  Last modified: $Id: stream_audio.c 12399 2007-01-30 21:13:19Z arek $
 *
 */


#include "stream_audio.h"

#include "OggExport.h"

#include "fccs.h"
#include "data_types.h"

#include "debug.h"

extern ComponentResult _audio_settings_to_ac(OggExportGlobalsPtr globals, QTAtomContainer *settings);


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
        si->acc_duration -= ogg_page_granulepos(&si->og) -
            si->og_grpos;
        si->og_grpos = ogg_page_granulepos(&si->og);
    }

    pos = si->og_grpos / si->si_a.stda_asbd.mSampleRate;
    si->og_ts_sec = (UInt32) pos;
    si->og_ts_subsec = (pos - (Float64) si->og_ts_sec);

    if (ogg_page_eos(&si->og))
        si->eos = true;
}

static ComponentResult
_InputDataProc__audio(ComponentInstance ci, UInt32 *ioNumberDataPackets,
                      AudioBufferList *ioData,
                      AudioStreamPacketDescription **outDataPacketDescription,
                      void *inRefCon)
{
    StreamInfoPtr si = (StreamInfoPtr) inRefCon;
    ComponentResult err = noErr;

    dbg_printf("[ aOE]  >> [%08lx] :: _InputDataProc__audio(%ld)\n",
               (UInt32) -1, *ioNumberDataPackets);

    *ioNumberDataPackets = 0;

    if (!si->src_extract_complete || si->gdp.actualSampleCount > 0) {
        if (si->gdp.actualSampleCount == 0) {
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
            si->gdp.requestedSampleCount = *ioNumberDataPackets;
            si->gdp.actualSampleCount = 0;
            si->gdp.durationPerSample = 1;
            si->gdp.sampleFlags = 0;

            err = InvokeMovieExportGetDataUPP(si->refCon, &si->gdp,
                                              si->getDataProc);
            dbg_printf("[ aOE]  <  [%08lx] :: _InputDataProc__audio() = %ld; "
                       "%ld, %ld, %ld, %ld, %ld)\n",
                       (UInt32) -1, err, *ioNumberDataPackets, si->gdp.dataSize,
                       si->gdp.requestedTime, si->gdp.actualTime, ioData->mNumberBuffers);
            if (!err)
                si->time += si->gdp.durationPerSample * si->gdp.actualSampleCount;

            if (err == eofErr) {
                err = noErr;
                si->src_extract_complete = true;
            }
        }

        if (!err) {
            if (si->gdp.actualSampleCount == 0) {
                ioData->mBuffers[0].mDataByteSize = 0;
                ioData->mBuffers[0].mData = NULL;
                si->src_extract_complete = true;
            } else {
                ioData->mBuffers[0].mDataByteSize = si->gdp.dataSize;
                ioData->mBuffers[0].mData = si->gdp.dataPtr;
            }

            *ioNumberDataPackets = si->gdp.actualSampleCount;

            si->gdp.actualSampleCount = 0;
        }
    } else {
        ioData->mBuffers[0].mDataByteSize = 0;
        ioData->mBuffers[0].mData = NULL;
        si->gdp.dataSize = 0;
        si->gdp.requestedTime = si->gdp.actualTime = 0;
    }

    dbg_printf("[ aOE] <   [%08lx] :: _InputDataProc__audio(%ld, "
               "%ld, %ld, %ld, %ld, %d) = %ld\n",
               (UInt32) -1, *ioNumberDataPackets, si->gdp.dataSize,
               si->gdp.requestedTime, si->gdp.actualTime, ioData->mNumberBuffers,
               outDataPacketDescription != NULL, err);
    return err;
}


/* ======================================================================= */

Boolean can_handle_track__audio(OSType trackType, TimeScale scale,
                                MovieExportGetPropertyUPP getPropertyProc,
                                void *refCon)
{
    if (!scale || !trackType || !getPropertyProc)
        return false;

    if (trackType == SoundMediaType)
        return true;

    return false;
}

ComponentResult validate_movie__audio(OggExportGlobals *globals,
                                      Movie theMovie, Track onlyThisTrack,
                                      Boolean *valid)
{
    ComponentResult err = noErr;

    return err;
}

ComponentResult initialize_stream__audio(StreamInfo *si)
{
    ComponentResult err = noErr;

    memset(&si->si_a.op, 0, sizeof(si->si_a.op));

    si->si_a.aspds = NULL;
    si->si_a.abl = NULL;
    si->si_a.op_buffer = NULL;

    si->stream_type = SoundMediaType;

    // allocate initial space for ogg_package
    si->si_a.op_buffer_size = kOES_A_init_op_size;
    si->si_a.op_buffer = calloc(1, kOES_A_init_op_size);

    // allocate space for one AudioBuffer
    si->si_a.abl_size = offsetof(AudioBufferList, mBuffers[1]);
    si->si_a.abl = (AudioBufferList *) calloc(1, si->si_a.abl_size);
    si->si_a.abl->mNumberBuffers = 1;

    // allocate space for one AudioStreamPacketDescription
    si->si_a.aspds_size = sizeof(AudioStreamPacketDescription);
    si->si_a.aspds = (AudioStreamPacketDescription *)
        calloc(1, si->si_a.aspds_size);

    err = MemError();

    if (!err) {
        si->si_a.stdAudio = NULL;
        err = OpenADefaultComponent(StandardCompressionType,
                                    StandardCompressionSubTypeAudio,
                                    &si->si_a.stdAudio);
    }

    if (err) {
        if (si->si_a.aspds) {
            free(si->si_a.aspds);
            si->si_a.aspds = NULL;
            si->si_a.aspds_size = 0;
        }

        if (si->si_a.abl) {
            free(si->si_a.abl);
            si->si_a.abl = NULL;
            si->si_a.abl_size = 0;
        }

        if (si->si_a.op_buffer) {
            free(si->si_a.op_buffer);
            si->si_a.op_buffer = NULL;
            si->si_a.op_buffer_size = 0;
        }
    }

    return err;
}

void clear_stream__audio(StreamInfo *si)
{
    if (si->si_a.aspds) {
        free(si->si_a.aspds);
        si->si_a.aspds = NULL;
        si->si_a.aspds_size = 0;
    }

    if (si->si_a.abl) {
        free(si->si_a.abl);
        si->si_a.abl = NULL;
        si->si_a.abl_size = 0;
    }

    if (si->si_a.op_buffer) {
        free(si->si_a.op_buffer);
        si->si_a.op_buffer = NULL;
        si->si_a.op_buffer_size = 0;
    }

    if (si->si_a.stdAudio)
        CloseComponent(si->si_a.stdAudio);
}

ComponentResult configure_stream__audio(OggExportGlobals *globals,
                                        StreamInfo *si)
{
    ComponentResult err = noErr;

    AudioChannelLayout *acl = NULL;
    UInt32 acl_size = 0;

    /* invoking getDataProc once to get input SampleDescription,
     * there seems to be no other reliable way to find out getDataProc's
     * output sample format...
     */
    si->gdp.requestedTime = 0;
    si->gdp.actualTime = 0;
    si->gdp.dataPtr = NULL;
    si->gdp.dataSize = 0;
    si->gdp.desc = NULL;
    si->gdp.descType = 0;
    si->gdp.descSeed = 0;
    si->gdp.requestedSampleCount = 0;
    si->gdp.actualSampleCount = 0;

    err = InvokeMovieExportGetDataUPP(si->refCon, &si->gdp, si->getDataProc);
    if (err == eofErr) {
        si->src_extract_complete = true;
        err = noErr;
    }

    if (!err) {
        SoundDescriptionHandle sdh;
        SoundDescriptionV2Ptr sd;

        si->time += si->gdp.durationPerSample * si->gdp.actualSampleCount;

        if (si->gdp.descType != SoundMediaType) {
            err = paramErr;
        } else {
            err = QTSoundDescriptionConvert(kQTSoundDescriptionKind_Movie_AnyVersion,
                                            (SoundDescriptionHandle) si->gdp.desc,
                                            kQTSoundDescriptionKind_Movie_Version2,
                                            &sdh);
        }

        if (!err) {
            sd = (SoundDescriptionV2Ptr) *sdh;
            dbg_printf("[ aOE]   i [%08lx] :: configure_stream__audio() = %ld, '%4.4s' ('%4.4s', %d, %d, '%4.4s', [%ld, %ld, %lf] [%ld, %ld, %ld, %ld])\n",
                       (UInt32) globals, err, (char *) &si->gdp.descType,
                       (char *) &sd->dataFormat, sd->version, sd->revlevel, (char *) &sd->vendor,
                       sd->numAudioChannels, sd->constBitsPerChannel, sd->audioSampleRate,
                       sd->constLPCMFramesPerAudioPacket, sd->constBytesPerAudioPacket, sd->formatSpecificFlags, 0l);

            if (sd->dataFormat != kAudioFormatLinearPCM) {
                // doesn't support compressed source samples a.t.m.
                err = paramErr;
            } else {
                AudioStreamBasicDescription *asbd = &si->si_a.qte_out_asbd;

                asbd->mFormatID = kAudioFormatLinearPCM;
                asbd->mFormatFlags = sd->formatSpecificFlags;
                asbd->mFramesPerPacket = sd->constLPCMFramesPerAudioPacket;
                asbd->mBitsPerChannel = sd->constBitsPerChannel;
                asbd->mChannelsPerFrame = sd->numAudioChannels;
                asbd->mBytesPerFrame = sd->constBytesPerAudioPacket;
                asbd->mBytesPerPacket = asbd->mBytesPerFrame *
                    asbd->mFramesPerPacket;
                asbd->mSampleRate = sd->audioSampleRate;

                err = QTSoundDescriptionGetPropertyInfo(sdh, kQTPropertyClass_SoundDescription,
                                                        kQTSoundDescriptionPropertyID_AudioChannelLayout,
                                                        NULL, &acl_size, NULL);
                dbg_printf("[ aOE]  cl [%08lx] :: configure_stream__audio() = %ld, %ld\n",
                           (UInt32) globals, err, acl_size);
                if (!err) {
                    acl = (AudioChannelLayout *) calloc(1, acl_size);
                    err = QTSoundDescriptionGetProperty(sdh, kQTPropertyClass_SoundDescription,
                                                        kQTSoundDescriptionPropertyID_AudioChannelLayout,
                                                        acl_size, acl, NULL);
                    dbg_printf("[ aOE]  CL [%08lx] :: configure_stream__audio() = %ld, {0x%08lx, %ld, %ld}\n",
                               (UInt32) globals, err, acl->mChannelLayoutTag,
                               acl->mChannelBitmap, acl->mNumberChannelDescriptions);
                } else {
                    // try getting channel layout using getPropertyProc? :/
                }
            }

            DisposeHandle((Handle) sdh);

            if (!acl && si->si_a.qte_out_asbd.mChannelsPerFrame < 3) {
                acl_size = sizeof(AudioChannelLayout);
                acl = (AudioChannelLayout *) calloc(1, acl_size);
                acl->mChannelLayoutTag =
                    si->si_a.qte_out_asbd.mChannelsPerFrame == 2 ?
                    kAudioChannelLayoutTag_Stereo : kAudioChannelLayoutTag_Mono;
                acl->mChannelBitmap = 0;
                acl->mNumberChannelDescriptions = 0;
            }

            err = QTSetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_InputBasicDescription,
                                         sizeof(si->si_a.qte_out_asbd), &si->si_a.qte_out_asbd);
            dbg_printf("[ aOE]  iD [%08lx] :: configure_stream__audio() = %ld\n", (UInt32) globals, err);

            if (!err && acl != NULL) {
                err = QTSetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_InputChannelLayout,
                                             acl_size, acl);
                dbg_printf("[ aOE]  iL [%08lx] :: configure_stream__audio() = %ld\n", (UInt32) globals, err);
                if (err)
                    err = noErr;
            }
        }
    }

    if (!err) {
        AudioStreamBasicDescription asbd = {globals->set_a_asbd.mSampleRate, globals->set_a_asbd.mFormatID,
                                            globals->set_a_asbd.mFormatFlags, globals->set_a_asbd.mBytesPerPacket,
                                            globals->set_a_asbd.mFramesPerPacket, globals->set_a_asbd.mBytesPerFrame,
                                            //si->si_a.qte_out_asbd.mChannelsPerFrame,
                                            globals->set_a_asbd.mChannelsPerFrame,
                                            0};

        err = QTSetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio,
                                     kQTSCAudioPropertyID_BasicDescription,
                                     sizeof(asbd), &asbd);
        dbg_printf("[  aOE]  iO [%08lx] :: configure_stream__audio() = %ld\n", (UInt32) globals, err);

        if (!err) {
            err = QTSetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio,
                                         kQTSCAudioPropertyID_RenderQuality,
                                         sizeof(UInt32), &globals->set_a_rquality);
            dbg_printf("[ aOE]  rq [%08lx] :: configure_stream__audio() = %ld\n", (UInt32) globals, err);
        }

        if (!err && globals->set_a_layout != NULL) {
            err = QTSetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio,
                                         kQTSCAudioPropertyID_ChannelLayout,
                                         globals->set_a_layout_size, globals->set_a_layout);
            dbg_printf("[ aOE] ocl [%08lx] :: configure_stream__audio() = %ld, {0x%08lx, %ld, %ld}\n",
                       (UInt32) globals, err, globals->set_a_layout->mChannelLayoutTag,
                       globals->set_a_layout->mChannelBitmap, globals->set_a_layout->mNumberChannelDescriptions);
            err = noErr;
        }

        if (!err && globals->set_a_custom != NULL) {
            err = QTSetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio,
                                         kQTSCAudioPropertyID_CodecSpecificSettingsArray,
                                         sizeof(CFArrayRef), &globals->set_a_custom);
            dbg_printf("[ aOE]  cs [%08lx] :: configure_stream__audio() = %ld\n", (UInt32) globals, err);
        }

        if (!err) {
            err = QTGetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio,
                                         kQTSCAudioPropertyID_BasicDescription,
                                         sizeof(si->si_a.stda_asbd), &si->si_a.stda_asbd, NULL);
        }

        if (!err) {
            if (si->si_a.stda_asbd.mBytesPerPacket) {
                si->si_a.max_packet_size = si->si_a.stda_asbd.mBytesPerPacket;
            } else {
                err = QTGetComponentProperty(si->si_a.stdAudio, kQTPropertyClass_SCAudio,
                                             kQTSCAudioPropertyID_MaximumOutputPacketSize,
                                             sizeof(si->si_a.max_packet_size),
                                             &si->si_a.max_packet_size, NULL);
            }
        }
        dbg_printf("[ aOE]  xp [%08lx] :: configure_stream__audio()  (%ld)\n", (UInt32) globals, si->si_a.max_packet_size);
    }

    if (acl) {
        free(acl);
    }

    return err;
}


ComponentResult
fill_page__audio(OggExportGlobalsPtr globals, StreamInfoPtr si,
                 Float64 max_duration)
{
    ComponentResult err = noErr;

    AudioStreamPacketDescription * aspds = NULL;
    AudioBufferList * abl = NULL;
    UInt32 pullPackets = 8; /* TODO: calculate based on the asbd and
                               the max_duration */
    Boolean eos_hit = false;
    Boolean do_loop = true;

    UInt32 max_page_duration = (UInt32) (max_duration *
                                         si->si_a.stda_asbd.mSampleRate);

    dbg_printf("[ aOE]  >> [%08lx] :: fill_page__audio(%lf)\n",
               (UInt32) globals, max_duration);

    if (ogg_stream_pageout(&si->os, &si->og) > 0) {
        /* there was data left in the internal ogg_stream buffers */
        _ready_page(si);
    } else {
        /* otherwise, get more data */
        if (si->si_a.aspds_size < pullPackets *
            sizeof(AudioStreamPacketDescription)) {
            si->si_a.aspds_size = pullPackets *
                sizeof(AudioStreamPacketDescription);
            si->si_a.aspds = realloc(si->si_a.aspds, si->si_a.aspds_size);
        }
        aspds = si->si_a.aspds;

        if (si->si_a.abl_size < offsetof(AudioBufferList, mBuffers[1])) {
            si->si_a.abl_size = offsetof(AudioBufferList, mBuffers[1]);
            si->si_a.abl = realloc(si->si_a.abl, si->si_a.abl_size);
        }
        abl = si->si_a.abl;
        abl->mNumberBuffers = 1;

        if (si->out_buffer == NULL) {
            si->out_buffer_size = si->si_a.max_packet_size * pullPackets;
            si->out_buffer = calloc(1, si->out_buffer_size);
        } else if (si->out_buffer_size < si->si_a.max_packet_size *
                   pullPackets) {
            si->out_buffer_size = si->si_a.max_packet_size * pullPackets;
            si->out_buffer = realloc(si->out_buffer, si->out_buffer_size);
        }

        if (si->si_a.op_buffer_size < si->si_a.max_packet_size) {
            si->si_a.op_buffer_size = si->si_a.max_packet_size;
            si->si_a.op_buffer = realloc(si->si_a.op_buffer,
                                         si->si_a.op_buffer_size);
        }

        while (do_loop) {
            SInt32 i, pull_packets = pullPackets;
            void *ptr = si->out_buffer;
            int result = 0;

            for (i = 0; i < abl->mNumberBuffers; i++) {
                abl->mBuffers[i].mNumberChannels =
                    si->si_a.stda_asbd.mChannelsPerFrame;
                abl->mBuffers[i].mDataByteSize = si->out_buffer_size;
                abl->mBuffers[i].mData = ptr;
                ptr = (UInt8 *) ptr + si->out_buffer_size;
            }

            err = SCAudioFillBuffer(si->si_a.stdAudio, _InputDataProc__audio,
                                    (void *) si, (UInt32 *) &pull_packets,
                                    abl, aspds);
            dbg_printf("[ aOE]  .  [%08lx] :: fill_page__audio(): "
                       "SCAudioFillBuffer(%ld) = %ld, eos: %d\n",
                       (UInt32) globals, pull_packets, err,
                       pull_packets == 0 &&
                       abl->mBuffers[0].mDataByteSize == 0);

            if (err == eofErr) {
                /* This should never happen if we don't
                   return eofErr from out InputDataProc! */
                err = noErr;
                eos_hit = true;
            }

            if (err)
                break;

            if (pull_packets == 0 && abl->mBuffers[0].mDataByteSize == 0) {
                eos_hit = true;
            }

            if (si->si_a.op.packet != NULL) {
                if (eos_hit)
                    si->si_a.op.e_o_s = 1;
                si->acc_packets++;
                si->acc_duration += si->si_a.op_duration;
                result = ogg_stream_packetin(&si->os, &si->si_a.op);
                dbg_printf("[ aOE] _i  [%08lx] :: fill_page__audio(): "
                           "ogg_stream_packetin(%lld, %ld, %lld) = %d\n",
                           (UInt32) globals, si->si_a.op.packetno,
                           si->si_a.op.bytes, si->si_a.op.granulepos,
                           result);
                si->si_a.op.bytes = 0;
                si->si_a.op.packet = NULL;
            }

            if (pull_packets > 0) {
                pull_packets--;
                for (i = 0; i < pull_packets; i++) {
                    si->si_a.op.bytes = aspds[i].mDataByteSize;
                    si->si_a.op.b_o_s = 0;
                    si->si_a.op.e_o_s = 0;
                    si->si_a.op.packetno = si->packets_total++;
                    si->acc_packets++;
                    si->last_grpos += aspds[i].mVariableFramesInPacket;
                    si->si_a.op.granulepos = si->last_grpos;
                    si->acc_duration += aspds[i].mVariableFramesInPacket;
                    si->si_a.op.packet = (UInt8 *) abl->mBuffers[0].mData +
                        aspds[i].mStartOffset;
                    result = ogg_stream_packetin(&si->os, &si->si_a.op);
                    dbg_printf("[ aOE]  i  [%08lx] :: fill_page__audio(): "
                               "ogg_stream_packetin(%lld, %ld, %lld) = %d\n",
                               (UInt32) globals, si->si_a.op.packetno,
                               si->si_a.op.bytes, si->si_a.op.granulepos,
                               result);
                }

                si->si_a.op.bytes = aspds[i].mDataByteSize;
                si->si_a.op.b_o_s = 0;
                si->si_a.op.e_o_s = 0;
                si->si_a.op.packetno = si->packets_total++;
                si->last_grpos += aspds[i].mVariableFramesInPacket;
                si->si_a.op.granulepos = si->last_grpos;
                si->si_a.op_duration = aspds[i].mVariableFramesInPacket;
                si->si_a.op.packet = (UInt8 *) abl->mBuffers[0].mData +
                    aspds[i].mStartOffset;

                if (si->si_a.op_buffer_size < si->si_a.op.bytes) {
                    si->si_a.op_buffer_size = si->si_a.op.bytes;
                    si->si_a.op_buffer = realloc(si->si_a.op_buffer,
                                                 si->si_a.op_buffer_size);
                }
                BlockMoveData((UInt8 *) abl->mBuffers[0].mData +
                              aspds[i].mStartOffset, si->si_a.op_buffer,
                              aspds[i].mDataByteSize);
                si->si_a.op.packet = si->si_a.op_buffer;
            }

            if (((eos_hit || si->acc_duration > max_page_duration) &&
                 ogg_stream_flush(&si->os, &si->og) > 0) ||
                (ogg_stream_pageout(&si->os, &si->og) > 0)) {
                _ready_page(si);
                do_loop = false;
            }
        }
    }

    dbg_printf("[ aOE] <   [%08lx] :: fill_page__audio() = %ld (%ld, %lf)\n",
               (UInt32) globals, err, si->og_ts_sec, si->og_ts_subsec);
    return err;
}

ComponentResult write_i_header__audio(StreamInfoPtr si, DataHandler data_h,
                                      wide *offset)
{
    // TODO: handle speex and FLAC headers/cookies as well!

    ComponentResult err = noErr;
    void * magicCookie = NULL;
    UInt32 cookieSize = 0;

    err = QTGetComponentPropertyInfo(si->si_a.stdAudio,
                                     kQTPropertyClass_SCAudio,
                                     kQTSCAudioPropertyID_MagicCookie,
                                     NULL, &cookieSize, NULL);

    if (!err) {
        magicCookie = calloc(1, cookieSize);
        err = QTGetComponentProperty(si->si_a.stdAudio,
                                     kQTPropertyClass_SCAudio,
                                     kQTSCAudioPropertyID_MagicCookie,
                                     cookieSize, magicCookie, NULL);

    }

    if (!err) {
        UInt8 *ptrheader = (UInt8 *) magicCookie;
        UInt8 *cend = ptrheader + cookieSize;
        CookieAtomHeader *aheader = (CookieAtomHeader *) ptrheader;
        ogg_packet header, header_vc, header_cb;
        header.bytes = header_vc.bytes = header_cb.bytes = 0;

        while (ptrheader < cend) {
            aheader = (CookieAtomHeader *) ptrheader;
            ptrheader += EndianU32_BtoN(aheader->size);
            if (ptrheader > cend || EndianU32_BtoN(aheader->size) <= 0)
                break;

            switch (EndianS32_BtoN(aheader->type)) {
            case kCookieTypeVorbisHeader:
                header.b_o_s = 1;
                header.e_o_s = 0;
                header.granulepos = 0;
                header.packetno = 0;
                header.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
                header.packet = aheader->data;
                break;

            case kCookieTypeVorbisComments:
                header_vc.b_o_s = 0;
                header_vc.e_o_s = 0;
                header_vc.granulepos = 0;
                header_vc.packetno = 1;
                header_vc.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
                header_vc.packet = aheader->data;
                break;

            case kCookieTypeVorbisCodebooks:
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

        if (header.bytes == 0 || header_vc.bytes == 0 || header_cb.bytes == 0) {
            err = paramErr;
        } else {
            ogg_stream_packetin(&si->os, &header);
            _flush_ogg(si, data_h, offset);

            ogg_stream_packetin(&si->os, &header_vc);
            ogg_stream_packetin(&si->os, &header_cb);
            //_flush_ogg(si, data_h, offset);

            si->packets_total = 3;
            si->acc_packets = 2;
            si->acc_duration = 0;
        }
    }

    if (magicCookie != NULL) {
        free(magicCookie);
        magicCookie = NULL;
    }

    return err;
}

ComponentResult write_headers__audio(StreamInfoPtr si, DataHandler data_h,
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
