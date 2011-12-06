/*
 *  stream_theora.c
 *
 *    Theora format related part of OggImporter.
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
 *  Last modified: $Id: stream_theora.c 16093 2009-06-08 23:16:39Z arek $
 *
 */


#include "stream_theora.h"
#if defined(__APPLE_CC__) && defined(XIPHQT_USE_FRAMEWORKS)
#include <TheoraExp/theoradec.h>
#else
#include <theora/theoradec.h>
#endif

#include "debug.h"
#define logg_page_last_packet_incomplete(op) (((unsigned char *)(op)->header)[26 + ((unsigned char *)(op)->header)[26]] == 255)

#include "OggImport.h"

#include "fccs.h"
#include "data_types.h"

#include "samplerefs.h"


#define MAX_FPS_DENOMINATOR 16384
#define DESIRED_MULTIPLIER 16383
#define SEGMENT_RATIO_DENOM 5


#if TARGET_OS_WIN32
EXTERN_API_C(SInt32 ) U64Compare(UInt64 left, UInt64 right)
{
    if (left < right)
        return -1;
    if (left == right)
        return 0;
    return 1;
}
#endif

/*
 * Euclid's GCD algorithm
 * de-recursified, with input check suitable for Theora fps a/b
 */
UInt32 gcd(UInt32 a, UInt32 b)
{
    UInt32 tmp;

    if (b == 0)
        return 0;

    while (b > 0) {
        // python: a, b = b, a % b
        tmp = a;
        a = b;
        b = tmp % b;
    }

    return a;
}

int recognize_header__theora(ogg_page *op)
{
    dbg_printf("! -- - theora_recognise_header: '%4.4s'\n", ((char *)op->body) + 1);
    if (!strncmp("\x80theora", (char *)op->body, 7))
        return 0;

    return 1;
};

int verify_header__theora(ogg_page *op) //?
{
    OSErr err = noErr;

    ogg_stream_state os;
    ogg_packet       opk;

    th_info        ti;
    th_comment     tc;
    th_setup_info *ts = NULL;

    ogg_stream_init(&os, ogg_page_serialno(op));


    th_info_init(&ti);
    th_comment_init(&tc);

    if (ogg_stream_pagein(&os, op) < 0)
        err = invalidMedia;
    else if (ogg_stream_packetout(&os, &opk) != 1)
        err = invalidMedia;
    else if (th_decode_headerin(&ti, &tc, &ts, &opk) < 0)
        err = noVideoTrackInMovieErr;

    ogg_stream_clear(&os);

    if (ts != NULL)         // theoretically this shouldn't happen, but then
        th_setup_free(ts);  // theoretically I don't know that this shouldn't happen

    th_comment_clear(&tc);
    th_info_clear(&ti);

    return err;
};

int initialize_stream__theora(StreamInfo *si)
{
    si->sample_refs_count = 0;
    si->sample_refs_duration = 0;
    si->sample_refs_size = kTSRefsInitial;
    si->sample_refs_increment = kTSRefsIncrement;
    si->sample_refs = calloc(si->sample_refs_size, sizeof(SampleReference64Record));

    if (si->sample_refs == NULL)
        return -1;

    th_info_init(&si->si_theora.ti);
    th_comment_init(&si->si_theora.tc);
    si->si_theora.ts = NULL;

    si->si_theora.state = kTStateInitial;

    si->si_theora.lpkt_data_offset = S64Set(0);
    si->si_theora.lpkt_data_size = 0;
    si->si_theora.lpkt_duration = 0;
    si->si_theora.lpkt_flags = 0;

    si->si_theora.psegment_ratio = SEGMENT_RATIO_DENOM;

    return 0;
};

void clear_stream__theora(StreamInfo *si)
{
    if (si->si_theora.ts != NULL)
        th_setup_free(si->si_theora.ts);
    th_info_clear(&si->si_theora.ti);
    th_comment_clear(&si->si_theora.tc);

    if (si->sample_refs != NULL)
        free(si->sample_refs);
};

ComponentResult create_sample_description__theora(StreamInfo *si)
{
    ComponentResult err = noErr;
    Handle desc = NewHandleClear(sizeof(ImageDescription));
    ImageDescriptionPtr imgdsc = (ImageDescriptionPtr) *desc;

    imgdsc->idSize = sizeof(ImageDescription);
    imgdsc->cType = kVideoFormatXiphTheora;
    imgdsc->version = 1; //major ver num
    imgdsc->revisionLevel = 1; //minor ver num
    imgdsc->vendor = kXiphComponentsManufacturer;
    imgdsc->temporalQuality = codecMaxQuality;
    imgdsc->spatialQuality = codecMaxQuality;
    imgdsc->width = si->si_theora.ti.pic_width;
    imgdsc->height = si->si_theora.ti.pic_height;
    imgdsc->hRes = 72<<16;
    imgdsc->vRes = 72<<16;
    imgdsc->depth = 24;
    imgdsc->clutID = -1;

    si->sampleDesc = (SampleDescriptionHandle) desc;

    return err;
};

ComponentResult create_track__theora(OggImportGlobals *globals, StreamInfo *si)
{
    ComponentResult ret = noErr;
    UInt32 frame_width = si->si_theora.ti.pic_width;
    UInt32 frame_width_fraction = 0;

    if (si->si_theora.ti.aspect_denominator != 0 && si->si_theora.ti.aspect_numerator != si->si_theora.ti.aspect_denominator) {
        frame_width_fraction = (frame_width * si->si_theora.ti.aspect_numerator % si->si_theora.ti.aspect_denominator) * 0x10000 / si->si_theora.ti.aspect_denominator;
        frame_width = frame_width * si->si_theora.ti.aspect_numerator / si->si_theora.ti.aspect_denominator;
    }
    dbg_printf("! -T calling => NewMovieTrack()\n");
    si->theTrack = NewMovieTrack(globals->theMovie,
                                 frame_width << 16 | (frame_width_fraction & 0xffff),
                                 si->si_theora.ti.pic_height << 16, 0);
    if (si->theTrack)
        globals->hasVideoTrack = true;

    return ret;
};

ComponentResult create_track_media__theora(OggImportGlobals *globals, StreamInfo *si, Handle data_ref)
{
    ComponentResult ret = noErr;
    dbg_printf("! -T calling => NewTrackMedia(%lx)\n", si->rate);
    si->theMedia = NewTrackMedia(si->theTrack, VideoMediaType, si->rate, data_ref, globals->dataRefType);

    return ret;
};

int process_first_packet__theora(StreamInfo *si, ogg_page *op, ogg_packet *opckt)
{
    unsigned long serialnoatom[3] = { EndianU32_NtoB(sizeof(serialnoatom)), EndianU32_NtoB(kCookieTypeOggSerialNo),
                                      EndianS32_NtoB(ogg_page_serialno(op)) };
    unsigned long atomhead[2] = { EndianU32_NtoB(opckt->bytes + sizeof(atomhead)), EndianU32_NtoB(kCookieTypeTheoraHeader) };
    unsigned long fps_gcd = 1, multiplier = 1;
    UInt32 fps_N, fps_D;

    th_decode_headerin(&si->si_theora.ti, &si->si_theora.tc, &si->si_theora.ts, opckt); //check errors?

    si->numChannels = 0;
    //si->lastMediaInserted = 0;
    si->mediaLength = 0;

    fps_N = si->si_theora.ti.fps_numerator;
    fps_D = si->si_theora.ti.fps_denominator;

    fps_gcd = gcd(fps_N, fps_D);
    if (fps_gcd < 1)
        return -1; // return some reasonable error code?

    if (fps_D / fps_gcd > MAX_FPS_DENOMINATOR) {
        UInt64 remainder;
        fps_N = U32SetU(U64Divide(U64Multiply(U64Set(fps_N), U64Set(MAX_FPS_DENOMINATOR)), U64Set(fps_D), &remainder));
        if (U64Compare(remainder, U64Set(MAX_FPS_DENOMINATOR / 2)) > 0)
            fps_N += 1;
        fps_D = MAX_FPS_DENOMINATOR;
        fps_gcd = gcd(fps_N, fps_D);
    }

    multiplier = DESIRED_MULTIPLIER / (fps_D / fps_gcd) + 1;
    si->si_theora.fps_framelen = (fps_D / fps_gcd) * multiplier;
    si->rate = (fps_N / fps_gcd) * multiplier;

    dbg_printf("! -T   setting FPS values: [gcd: %8ld, mult: %8ld] fl: %8ld, rate: %8ld (N: %8ld, D: %8ld) (nN: %8ld, nD: %8ld)\n",
               fps_gcd, multiplier, si->si_theora.fps_framelen, si->rate, si->si_theora.ti.fps_numerator, si->si_theora.ti.fps_denominator,
               fps_N, fps_D);
    si->si_theora.granulepos_shift = si->si_theora.ti.keyframe_granule_shift;

    PtrAndHand(serialnoatom, si->soundDescExtension, sizeof(serialnoatom)); //check errors?
    PtrAndHand(atomhead, si->soundDescExtension, sizeof(atomhead)); //check errors?
    PtrAndHand(opckt->packet, si->soundDescExtension, opckt->bytes); //check errors?

    si->si_theora.state = kTStateReadingComments;

    return 0;
};

ComponentResult process_stream_page__theora(OggImportGlobals *globals, StreamInfo *si, ogg_page *opg)
{
    ComponentResult ret = noErr;
    int ovret = 0;
    Boolean loop = true;
    Boolean movie_changed = false;

    TimeValue movieTS = GetMovieTimeScale(globals->theMovie);
    TimeValue mediaTS = 0;
    TimeValue mediaTS_fl = 0.0;

    ogg_packet op;

    switch(si->si_theora.state) {
    case kTStateReadingComments:
    case kTStateReadingCodebooks:
        ogg_stream_pagein(&si->os, opg);
        break;
    default:
        break;
    }

    do {
        switch(si->si_theora.state) {
        case kTStateReadingComments:
            ovret = ogg_stream_packetout(&si->os, &op);
            if (ovret < 0) {
                loop = false;
                ret = invalidMedia;
            } else if (ovret < 1) {
                loop = false;
            } else {
                unsigned long atomhead[2] = { EndianU32_NtoB(op.bytes + sizeof(atomhead)), EndianU32_NtoB(kCookieTypeTheoraComments) };

                PtrAndHand(atomhead, si->soundDescExtension, sizeof(atomhead));
                PtrAndHand(op.packet, si->soundDescExtension, op.bytes);
                th_decode_headerin(&si->si_theora.ti, &si->si_theora.tc, &si->si_theora.ts, &op);

                ret = CreateTrackAndMedia(globals, si, opg);
                if (ret != noErr) {
                    dbg_printf("??? -- CreateTrackAndMedia failed?: %ld\n", (long)ret);
                }

                /*err =*/ DecodeCommentsQT(globals, si, &si->si_theora.tc);
                //NotifyMovieChanged(globals);

                si->si_theora.state = kTStateReadingCodebooks;
            }
            break;

        case kTStateReadingCodebooks:
            ovret = ogg_stream_packetout(&si->os, &op);
            if (ovret < 0) {
                loop = false;
                ret = invalidMedia;
            } else if (ovret < 1) {
                loop = false;
            } else {
                unsigned long atomhead[2] = { EndianU32_NtoB(op.bytes + sizeof(atomhead)), EndianU32_NtoB(kCookieTypeTheoraCodebooks) };
                PtrAndHand(atomhead, si->soundDescExtension, sizeof(atomhead));
                PtrAndHand(op.packet, si->soundDescExtension, op.bytes);

                th_decode_headerin(&si->si_theora.ti, &si->si_theora.tc, &si->si_theora.ts, &op);
                {
                    unsigned long endAtom[2] = { EndianU32_NtoB(sizeof(endAtom)), EndianU32_NtoB(kAudioTerminatorAtomType) };

                    ret = PtrAndHand(endAtom, si->soundDescExtension, sizeof(endAtom));
                    if (ret == noErr) {
                        ret = AddImageDescriptionExtension((ImageDescriptionHandle) si->sampleDesc,
                                                           si->soundDescExtension, kSampleDescriptionExtensionTheora);
                        //dbg_printf("??? -- Adding extension: %ld\n", ret);
                    } else {
                        //dbg_printf("??? -- Hmm, something went wrong: %ld\n", ret);
                    }
                }

                si->si_theora.state = kTStateReadingFirstPacket;
                si->insertTime = 0;
                si->streamOffset = globals->currentGroupOffset;
                mediaTS = GetMediaTimeScale(si->theMedia);
                mediaTS_fl = (Float64) mediaTS;
                si->streamOffsetSamples = (TimeValue) (mediaTS_fl * globals->currentGroupOffsetSubSecond) -
                    ((globals->currentGroupOffset % movieTS) * mediaTS / movieTS);
                dbg_printf("---/  / streamOffset: [%ld, %ld], %lg\n", si->streamOffset, si->streamOffsetSamples, globals->currentGroupOffsetSubSecond);
                si->incompleteCompensation = 0;
                loop = false; //there should be an end of page here according to specs...
            }
            break;

        case kTStateReadingFirstPacket:
            if (ogg_page_pageno(opg) > 3) {
                si->lastGranulePos = ogg_page_granulepos(opg);
                dbg_printf("----==< skipping: %llx, %lx\n", si->lastGranulePos, ogg_page_pageno(opg));
                loop = false;

                if (si->lastGranulePos < 0)
                    si->lastGranulePos = 0;
            }
            si->si_theora.state = kTStateReadingPackets;
            break;

        case kTStateReadingPackets:
            {
                ogg_int64_t pos       = ogg_page_granulepos(opg);
                short       smp_flags = 0;

                long psize = 0, poffset = 0;
                int i, segments;
                Boolean continued = ogg_page_continued(opg);
                TimeValue durationPerSample = 0;
                ogg_int64_t last_packet_pos = si->lastGranulePos >> si->si_theora.granulepos_shift;
                last_packet_pos += si->lastGranulePos - (last_packet_pos << si->si_theora.granulepos_shift);

                loop = false;
                if (ovret < 0) {
                    ret = invalidMedia;
                    break;
                }

                if (continued)
                    smp_flags |= mediaSampleNotSync;

                segments = opg->header[26];
                for (i = 0; i < segments; i++) {
                    int val = opg->header[27 + i];
                    int incomplete = (i == segments - 1) && (val == 255);
                    psize += val;
                    if (val < 255 || incomplete) {
                        TimeValue pduration = si->si_theora.fps_framelen;
                        if (incomplete) {
                            psize += 4; // this should allow decoder to see that if (packet_len % 255 == 4) and
                                        // last 4 bytes contain 'OggS' sync pattern then that's an incomplete packet
                        }
                        if (psize == 0 || (opg->body[poffset] & 0x40) != 0)
                            smp_flags |= mediaSampleNotSync;

                        if (si->si_theora.lpkt_data_size != 0) {
                            /* add last packet to media */
                            durationPerSample = 0;
                            if (!incomplete) {
                                /* packet finishes here */
                                if (si->si_theora.lpkt_duration > 0) {
                                    durationPerSample = si->si_theora.lpkt_duration;
                                } else {
                                    durationPerSample = 1;
                                    pduration += si->si_theora.lpkt_duration - 1;
                                }
                            } else {
                                /* segmented packet */
                                if (si->si_theora.lpkt_duration > 0) {
                                    durationPerSample = si->si_theora.lpkt_duration / si->si_theora.psegment_ratio;
                                    if (durationPerSample == 0)
                                        durationPerSample = 1;
                                    pduration = si->si_theora.lpkt_duration - durationPerSample;
                                } else {
                                    durationPerSample = 1;
                                    pduration = si->si_theora.lpkt_duration - 1 /* durationPerSample */;
                                }
                            }
                            ret = _store_sample_reference(si, &si->si_theora.lpkt_data_offset, si->si_theora.lpkt_data_size,
                                                          durationPerSample, si->si_theora.lpkt_flags);
                            if (ret != noErr)
                                break;
                        } else if (si->streamOffsetSamples > 0) {
                            /* first packet (and stream has a sample offset) */
                            dbg_printf("   -   :++: increasing duration (%ld) by sampleOffset: %ld\n", pduration, si->streamOffsetSamples);
                            pduration += si->streamOffsetSamples;
                        }
                        /* prepending packet with one padding byte (zero-sized samples are not allowed) here
                           (see ...BeginBand() in TheoraDecoder) */
                        si->si_theora.lpkt_data_offset = globals->dataOffset + S64Set(poffset + opg->header_len - 1);
                        si->si_theora.lpkt_data_size = psize + 1;
                        si->si_theora.lpkt_flags = smp_flags;
                        si->si_theora.lpkt_duration = pduration;

                        poffset += psize;
                        psize = 0;
                        continued = false;
                        smp_flags = 0;
                    }
                }

                if (ret == noErr && si->si_theora.lpkt_data_size != 0 && ogg_page_eos(opg)) {
                    /* it's EOS here, flush */
                    durationPerSample = 0;

                    /* this the last page of the stream, packet SHOULD finish here */
                    if (si->si_theora.lpkt_duration > 0) {
                        durationPerSample = si->si_theora.lpkt_duration;
                    } else {
                        durationPerSample = 1;
                    }

                    ret = _store_sample_reference(si, &si->si_theora.lpkt_data_offset, si->si_theora.lpkt_data_size,
                                                  durationPerSample, si->si_theora.lpkt_flags);
                    if (ret != noErr)
                        break;
                }

                if (!globals->usingIdle) {
                    if (si->sample_refs_count >= kTSRefsInitial)
                        ret = _commit_srefs(globals, si, &movie_changed);
                }

                if (pos != -1)
                    si->lastGranulePos = pos;
            }
            loop = false;
            break;

        default:
            loop = false;
        }
    } while(loop);

    if (movie_changed)
        NotifyMovieChanged(globals, false);

    return ret;
};

ComponentResult flush_stream__theora(OggImportGlobals *globals, StreamInfo *si, Boolean notify)
{
    ComponentResult err = noErr;
    Boolean movie_changed = false;

    err = _commit_srefs(globals, si, &movie_changed);

    if (movie_changed && notify)
        NotifyMovieChanged(globals, true);

    return err;
};

ComponentResult granulepos_to_time__theora(StreamInfo *si, ogg_int64_t *gp, TimeRecord *time)
{
    ComponentResult err = noErr;
    SInt64 tmp;

    ogg_int64_t frames = *gp >> si->si_theora.granulepos_shift;
    frames += *gp - (frames << si->si_theora.granulepos_shift);

    tmp = S64Multiply(frames, S64Set(si->si_theora.fps_framelen));
    time->value = SInt64ToWide(tmp);
    time->scale = si->rate;
    time->base = NULL;

    return err;
};
