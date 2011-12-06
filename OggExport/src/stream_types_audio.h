/*
 *  stream_types_audio.h
 *
 *    Definition of audio stream data structures for OggExport.
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
 *  Last modified: $Id: stream_types_audio.h 12093 2006-11-12 14:44:51Z arek $
 *
 */


#if !defined(__stream_types_audio_h__)
#define __stream_types_audio_h__

enum {
    kOES_A_init_op_size = 1024,
};

typedef struct {
    ComponentInstance stdAudio;

    // quickTimeMovieExporter output asbd
    AudioStreamBasicDescription qte_out_asbd;

    // stdAudioComponent output format asbd
    AudioStreamBasicDescription stda_asbd;

    ogg_packet op;
    UInt32 op_duration;
    void * op_buffer;
    UInt32 op_buffer_size;

    UInt32 max_packet_size;

    AudioBufferList * abl;
    UInt32 abl_size;

    AudioStreamPacketDescription * aspds;
    UInt32 aspds_size;

} StreamInfo__audio;

#define _HAVE__OE_AUDIO 1
#endif /* __stream_types_audio_h__ */
