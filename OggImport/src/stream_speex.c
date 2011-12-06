/*
 *  stream_speex.c
 *
 *    Speex format related part of OggImporter.
 *
 *
 *  Copyright (c) 2005  Arek Korbik
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
 *  Last modified: $Id: stream_speex.c 12754 2007-03-14 03:51:23Z arek $
 *
 */


#include <Ogg/ogg.h>

#include "stream_speex.h"

#include "debug.h"
#define logg_page_last_packet_incomplete(op) (((unsigned char *)(op)->header)[26 + ((unsigned char *)(op)->header)[26]] == 255)

#include "OggImport.h"

#include "fccs.h"
#include "data_types.h"
#include "utils.h"

#include "samplerefs.h"


int recognize_header__speex(ogg_page *op)
{
    if (!strncmp("Speex   ", (char *)op->body, 8))
        return 0;

    return 1;
};

int verify_header__speex(ogg_page *op) //?
{
    return 0;
};

int initialize_stream__speex(StreamInfo *si)
{
    si->sample_refs_count = 0;
    si->sample_refs_duration = 0;
    si->sample_refs_size = kSSRefsInitial;
    si->sample_refs_increment = kSSRefsIncrement;
    si->sample_refs = calloc(si->sample_refs_size, sizeof(SampleReference64Record));

    if (si->sample_refs == NULL)
        return -1;

    memset(&si->si_speex.header, 0, sizeof(SpeexHeader));
    vorbis_comment_init(&si->si_speex.vc);

    si->si_speex.skipped_headers = 0;
    si->si_speex.state = kSStateInitial;

    return 0;
};

void clear_stream__speex(StreamInfo *si)
{
    if (si->sample_refs != NULL)
        free(si->sample_refs);

    vorbis_comment_clear(&si->si_speex.vc);
};

ComponentResult create_sample_description__speex(StreamInfo *si)
{
    ComponentResult err = noErr;
    Handle desc;
    AudioStreamBasicDescription asbd;
    AudioChannelLayout acl;
    AudioChannelLayout *pacl = &acl;
    ByteCount acl_size = sizeof(acl);

    asbd.mSampleRate = si->rate;
    asbd.mFormatID = kAudioFormatXiphOggFramedSpeex;
    asbd.mFormatFlags = 0;
    asbd.mBytesPerPacket = 0;
    asbd.mFramesPerPacket = 0;
    //asbd.mBytesPerFrame = 2 * si->numChannels;
    asbd.mBytesPerFrame = 0;
    asbd.mChannelsPerFrame = si->numChannels;
    //asbd.mBitsPerChannel = 16;
    asbd.mBitsPerChannel = 0;
    asbd.mReserved = 0;

    if (si->numChannels == 1)
        acl.mChannelLayoutTag = kAudioChannelLayoutTag_Mono;
    else if (si->numChannels == 2)
        acl.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
    else {
        pacl = NULL;
        acl_size = 0;
    }
    acl.mChannelBitmap = 0;
    acl.mNumberChannelDescriptions = 0;

    err = QTSoundDescriptionCreate(&asbd, pacl, acl_size, NULL, 0, kQTSoundDescriptionKind_Movie_Version2, (SoundDescriptionHandle*) &desc);

    if (err == noErr) {
        si->sampleDesc = (SampleDescriptionHandle) desc;
    }

    return err;
};

int process_first_packet__speex(StreamInfo *si, ogg_page *op, ogg_packet *opckt)
{
    unsigned long serialnoatom[3] = { EndianU32_NtoB(sizeof(serialnoatom)), EndianU32_NtoB(kCookieTypeOggSerialNo),
                                      EndianS32_NtoB(ogg_page_serialno(op)) };
    unsigned long atomhead[2] = { EndianU32_NtoB(opckt->bytes + sizeof(atomhead)), EndianU32_NtoB(kCookieTypeSpeexHeader) };
    SpeexHeader *inheader = (SpeexHeader *) opckt->packet;

    si->si_speex.header.bitrate =                 EndianS32_LtoN(inheader->bitrate);
    si->si_speex.header.extra_headers =           EndianS32_LtoN(inheader->extra_headers);
    si->si_speex.header.frame_size =              EndianS32_LtoN(inheader->frame_size);
    si->si_speex.header.frames_per_packet =       EndianS32_LtoN(inheader->frames_per_packet);
    si->si_speex.header.header_size =             EndianS32_LtoN(inheader->header_size);
    si->si_speex.header.mode =                    EndianS32_LtoN(inheader->mode);
    si->si_speex.header.mode_bitstream_version =  EndianS32_LtoN(inheader->mode_bitstream_version);
    si->si_speex.header.nb_channels =             EndianS32_LtoN(inheader->nb_channels);
    si->si_speex.header.rate =                    EndianS32_LtoN(inheader->rate);
    si->si_speex.header.reserved1 =               EndianS32_LtoN(inheader->reserved1);
    si->si_speex.header.reserved2 =               EndianS32_LtoN(inheader->reserved2);
    si->si_speex.header.speex_version_id =        EndianS32_LtoN(inheader->speex_version_id);
    si->si_speex.header.vbr =                     EndianS32_LtoN(inheader->vbr);
    //si->si_speex.header. = EndianS32_LtoN(inheader->);

    dbg_printf("! -- - speex_first_packet: ch: %d, rate: %ld\n", si->si_speex.header.nb_channels, si->si_speex.header.rate);
    si->numChannels = si->si_speex.header.nb_channels;
    si->rate = si->si_speex.header.rate;
    //si->lastMediaInserted = 0;
    si->mediaLength = 0;

    PtrAndHand(serialnoatom, si->soundDescExtension, sizeof(serialnoatom)); //check errors?
    PtrAndHand(atomhead, si->soundDescExtension, sizeof(atomhead)); //check errors?
    PtrAndHand(opckt->packet, si->soundDescExtension, opckt->bytes); //check errors?

    si->si_speex.state = kSStateReadingComments;

    return 0;
};

ComponentResult process_stream_page__speex(OggImportGlobals *globals, StreamInfo *si, ogg_page *opg)
{
    ComponentResult ret = noErr;
    int ovret = 0;
    Boolean loop = true;
    Boolean movie_changed = false;

    TimeValue movieTS = GetMovieTimeScale(globals->theMovie);
    TimeValue mediaTS = 0;
    TimeValue mediaTS_fl = 0.0;

    ogg_packet op;

    switch(si->si_speex.state) {
    case kSStateReadingComments:
    case kSStateReadingAdditionalHeaders:
        ogg_stream_pagein(&si->os, opg);
        break;
    default:
        break;
    }

    do {
        switch(si->si_speex.state) {
        case kSStateReadingComments:
            ovret = ogg_stream_packetout(&si->os, &op);
            if (ovret < 0) {
                loop = false;
                ret = invalidMedia;
            } else if (ovret < 1) {
                loop = false;
            } else {
                unsigned long atomhead[2] = { EndianU32_NtoB(op.bytes + sizeof(atomhead)), EndianU32_NtoB(kCookieTypeSpeexComments) };

                PtrAndHand(atomhead, si->soundDescExtension, sizeof(atomhead));
                PtrAndHand(op.packet, si->soundDescExtension, op.bytes);

                ret = CreateTrackAndMedia(globals, si, opg);
                if (ret != noErr) {
                    dbg_printf("??? -- CreateTrackAndMedia failed?: %ld\n", (long)ret);
                }

                unpack_vorbis_comments(&si->si_speex.vc, op.packet, op.bytes);
                /*err =*/ DecodeCommentsQT(globals, si, &si->si_speex.vc);
                //NotifyMovieChanged(globals);

                si->si_speex.state = kSStateReadingAdditionalHeaders;
            }

            break;

        case kSStateReadingAdditionalHeaders:
            if (si->si_speex.skipped_headers >= si->si_speex.header.extra_headers) {
                unsigned long endAtom[2] = { EndianU32_NtoB(sizeof(endAtom)), EndianU32_NtoB(kAudioTerminatorAtomType) };

                ret = PtrAndHand(endAtom, si->soundDescExtension, sizeof(endAtom));
                if (ret == noErr) {
                    ret = AddSoundDescriptionExtension((SoundDescriptionHandle) si->sampleDesc,
                                                       si->soundDescExtension, siDecompressionParams);
                    //dbg_printf("??? -- Adding extension: %ld\n", ret);
                } else {
                    //dbg_printf("??? -- Hmm, something went wrong: %ld\n", ret);
                }

                si->insertTime = 0;
                si->streamOffset = globals->currentGroupOffset;
                mediaTS = GetMediaTimeScale(si->theMedia);
                mediaTS_fl = (Float64) mediaTS;
                si->streamOffsetSamples = (TimeValue) (mediaTS_fl * globals->currentGroupOffsetSubSecond) -
                    ((globals->currentGroupOffset % movieTS) * mediaTS / movieTS);
                dbg_printf("---/  / streamOffset: [%ld, %ld], %lg\n", si->streamOffset, si->streamOffsetSamples, globals->currentGroupOffsetSubSecond);
                si->incompleteCompensation = 0;
                si->si_speex.state = kSStateReadingFirstPacket;

                loop = false; // ??!
                break;
            }

            ovret = ogg_stream_packetout(&si->os, &op);
            if (ovret < 0) {
                loop = false;
                ret = invalidMedia;
            } else if (ovret < 1) {
                loop = false;
            } else {
                // not much here so far, basically just skip the extra header packet
                unsigned long atomhead[2] = { EndianU32_NtoB(op.bytes + sizeof(atomhead)), EndianU32_NtoB(kCookieTypeSpeexExtraHeader) };
                PtrAndHand(atomhead, si->soundDescExtension, sizeof(atomhead));
                PtrAndHand(op.packet, si->soundDescExtension, op.bytes);

                si->si_speex.skipped_headers += 1;
            }

            break;

        case kSStateReadingFirstPacket:
            if (ogg_page_pageno(opg) > 2) {
                si->lastGranulePos = ogg_page_granulepos(opg);
                dbg_printf("----==< skipping: %llx, %lx\n", si->lastGranulePos, ogg_page_pageno(opg));
                loop = false;

                if (si->lastGranulePos < 0)
                    si->lastGranulePos = 0;
            }
            si->si_speex.state = kSStateReadingPackets;
            break;

        case kVStateReadingPackets:
            {
                ogg_int64_t pos       = ogg_page_granulepos(opg);
                int         len       = opg->header_len + opg->body_len;
                TimeValue   duration  = pos - si->lastGranulePos;
                short       smp_flags = 0;

                if (ogg_page_continued(opg) || si->incompleteCompensation != 0)
                    smp_flags |= mediaSampleNotSync;

                if (duration <= 0) {
                    duration = INCOMPLETE_PAGE_DURATION;
                    si->incompleteCompensation -= INCOMPLETE_PAGE_DURATION;
                } else if (si->incompleteCompensation != 0) {
                    duration += si->incompleteCompensation;
                    si->incompleteCompensation = 0;
                    if (duration <= 0) {
                        ret = badFileFormat;
                        loop = false;
                        break;
                    }
                }

                if (si->insertTime == 0 && si->streamOffsetSamples > 0) {
                    dbg_printf("   -   :++: increasing duration (%ld) by sampleOffset: %ld\n", duration, si->streamOffsetSamples);
                    duration += si->streamOffsetSamples;
                }

                ret = _store_sample_reference(si, &globals->dataOffset, len, duration, smp_flags);
                if (ret != noErr) {
                    loop = false;
                    break;
                }

                if (!globals->usingIdle) {
                    if (si->sample_refs_count >= kSSRefsInitial)
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


ComponentResult flush_stream__speex(OggImportGlobals *globals, StreamInfo *si, Boolean notify)
{
    ComponentResult ret = noErr;
    Boolean movie_changed = false;

    ret = _commit_srefs(globals, si, &movie_changed);

    if (movie_changed && notify)
        NotifyMovieChanged(globals, true);

    return ret;
};
