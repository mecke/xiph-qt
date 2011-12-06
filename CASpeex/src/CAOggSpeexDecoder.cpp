/*
 *  CAOggSpeexDecoder.cpp
 *
 *    CAOggSpeexDecoder class implementation; translation layer handling
 *    ogg page encapsulation of Speex packets, using CASpeexDecoder
 *    for the actual decoding.
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
 *  Last modified: $Id: CAOggSpeexDecoder.cpp 12754 2007-03-14 03:51:23Z arek $
 *
 */


#include "CAOggSpeexDecoder.h"

#include "fccs.h"
#include "data_types.h"

#include "wrap_ogg.h"

#include "debug.h"


CAOggSpeexDecoder::CAOggSpeexDecoder() :
    CASpeexDecoder(true),
    mO_st(), mFramesBufferedList()
{
    CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAudioFormatXiphOggFramedSpeex,
                                            kSpeexBytesPerPacket, kSpeexFramesPerPacket,
                                            kSpeexBytesPerFrame, kSpeexChannelsPerFrame,
                                            kSpeexBitsPerChannel, kSpeexFormatFlags);
    AddInputFormat(theInputFormat);

    mInputFormat.mSampleRate = 44100;
    mInputFormat.mFormatID = kAudioFormatXiphOggFramedSpeex;
    mInputFormat.mFormatFlags = kSpeexFormatFlags;
    mInputFormat.mBytesPerPacket = kSpeexBytesPerPacket;
    mInputFormat.mFramesPerPacket = kSpeexFramesPerPacket;
    mInputFormat.mBytesPerFrame = kSpeexBytesPerFrame;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = 16;

    CAStreamBasicDescription theOutputFormat1(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16,
                                              kAudioFormatFlagsNativeEndian |
                                              kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
    AddOutputFormat(theOutputFormat1);
    CAStreamBasicDescription theOutputFormat2(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 32,
                                              kAudioFormatFlagsNativeFloatPacked);
    AddOutputFormat(theOutputFormat2);

    mOutputFormat.mSampleRate = 44100;
    mOutputFormat.mFormatID = kAudioFormatLinearPCM;
    mOutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    mOutputFormat.mBytesPerPacket = 8;
    mOutputFormat.mFramesPerPacket = 1;
    mOutputFormat.mBytesPerFrame = 8;
    mOutputFormat.mChannelsPerFrame = 2;
    mOutputFormat.mBitsPerChannel = 32;
}

CAOggSpeexDecoder::~CAOggSpeexDecoder()
{
    if (mCompressionInitialized)
        ogg_stream_clear(&mO_st);
}

void CAOggSpeexDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
    if (!mIsInitialized) {
        //	check to make sure the input format is legal
        if (inInputFormat.mFormatID != kAudioFormatXiphOggFramedSpeex) {
            dbg_printf("CASpeexDecoder::SetFormats: only supports Xiph Speex (Ogg-framed)for input\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentInputFormat(inInputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

UInt32 CAOggSpeexDecoder::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                               AudioStreamPacketDescription* outPacketDescription)
{
    dbg_printf(" >> [%08lx] CAOggSpeexDecoder :: ProduceOutputPackets(%ld [%ld])\n", (UInt32) this, ioNumberPackets, ioOutputDataByteSize);
    UInt32 ret = kAudioCodecProduceOutputPacketSuccess;

    if (mFramesBufferedList.empty()) {
        ioOutputDataByteSize = 0;
        ioNumberPackets = 0;
        ret = kAudioCodecProduceOutputPacketNeedsMoreInputData;
        dbg_printf("<!E [%08lx] CAOggSpeexDecoder :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n", (UInt32) this,
                   ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
        return ret;
    }

    UInt32 speex_packets = mFramesBufferedList.front();
    UInt32 ogg_packets = 0;
    UInt32 speex_returned_data = ioOutputDataByteSize;
    UInt32 speex_total_returned_data = 0;
    Byte *the_data = static_cast<Byte*> (outOutputData);

    while (true) {
        UInt32 speex_return = CASpeexDecoder::ProduceOutputPackets(the_data, speex_returned_data, speex_packets, NULL);
        if (speex_return == kAudioCodecProduceOutputPacketSuccess || speex_return == kAudioCodecProduceOutputPacketSuccessHasMore) {
            if (speex_packets > 0)
                mFramesBufferedList.front() -= speex_packets;

            if (mFramesBufferedList.front() <= 0) {
                ogg_packets++;
                mFramesBufferedList.erase(mFramesBufferedList.begin());
            }

            speex_total_returned_data += speex_returned_data;

            if (speex_total_returned_data == ioOutputDataByteSize || speex_return == kAudioCodecProduceOutputPacketSuccess)
            {
                ioNumberPackets = ogg_packets;
                ioOutputDataByteSize = speex_total_returned_data;

                if (!mFramesBufferedList.empty())
                    ret = kAudioCodecProduceOutputPacketSuccessHasMore;
                else
                    ret = kAudioCodecProduceOutputPacketSuccess;

                break;
            } else {
                the_data += speex_returned_data;
                speex_returned_data = ioOutputDataByteSize - speex_total_returned_data;
                speex_packets = mFramesBufferedList.front();
            }
        } else {
            ret = kAudioCodecProduceOutputPacketFailure;
            ioOutputDataByteSize = speex_total_returned_data;
            ioNumberPackets = ogg_packets;
        }
    }

    dbg_printf("<.. [%08lx] CAOggSpeexDecoder :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n",
               (UInt32) this, ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
    return ret;
}


void CAOggSpeexDecoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    CASpeexDecoder::BDCInitialize(inInputBufferByteSize);
}

void CAOggSpeexDecoder::BDCUninitialize()
{
    mFramesBufferedList.clear();
    CASpeexDecoder::BDCUninitialize();
}

void CAOggSpeexDecoder::BDCReset()
{
    mFramesBufferedList.clear();
    if (mCompressionInitialized)
        ogg_stream_reset(&mO_st);
    CASpeexDecoder::BDCReset();
}

void CAOggSpeexDecoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    mFramesBufferedList.clear();
    CASpeexDecoder::BDCReallocate(inInputBufferByteSize);
}


void CAOggSpeexDecoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    dbg_printf(" >> [%08lx] CAOggSpeexDecoder :: InPacket({%ld, %ld})\n", (UInt32) this, inPacketDescription->mDataByteSize, inPacketDescription->mVariableFramesInPacket);
    if (!mCompressionInitialized)
        CODEC_THROW(kAudioCodecUnspecifiedError);

    ogg_page op;

    if (!WrapOggPage(&op, inInputData, inPacketDescription->mDataByteSize + inPacketDescription->mStartOffset, inPacketDescription->mStartOffset))
        CODEC_THROW(kAudioCodecUnspecifiedError);

    ogg_packet opk;
    UInt32 packet_count = 0;
    int oret;
    AudioStreamPacketDescription speex_packet_desc = {0, mSpeexHeader.frame_size, 0};
    UInt32 page_packets = ogg_page_packets(&op);
    SInt32 packet_length_adjust = 0;

    dbg_printf("  > [%08lx] CAOggSpeexDecoder :: InPacket(): no: %ld, fs: %ld, fpp: %ld, np: %ld\n",
               (UInt32) this, ogg_page_pageno(&op), mSpeexHeader.frame_size, mSpeexHeader.frames_per_packet, page_packets);

    if (mSpeexHeader.frame_size != 0 && mSpeexHeader.frames_per_packet != 0) {
        if (mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet * page_packets != inPacketDescription->mVariableFramesInPacket) {
            packet_length_adjust = mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet * page_packets - inPacketDescription->mVariableFramesInPacket;
            if (ogg_page_eos(&op)) {
                if  (packet_length_adjust > mSpeexHeader.frame_size) {
                    page_packets -= packet_length_adjust / mSpeexHeader.frame_size;
                    packet_length_adjust = packet_length_adjust % mSpeexHeader.frame_size;
                }
                packet_length_adjust = -packet_length_adjust;
            }
            dbg_printf("  > [%08lx] CAOggSpeexDecoder :: InPacket(): p_l_adjust: %ld\n",
                       (UInt32) this, packet_length_adjust);
        }
    }

    ogg_stream_pagein(&mO_st, &op);
    while ((oret = ogg_stream_packetout(&mO_st, &opk)) != 0) {
        if (oret < 0) {
            page_packets--;
            if (packet_length_adjust > 0)
                packet_length_adjust = 0;
            continue;
        }

        packet_count++;

        speex_packet_desc.mDataByteSize = opk.bytes;
        speex_packet_desc.mVariableFramesInPacket = mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet;
        if (packet_count == 1 && packet_length_adjust > 0) {
            speex_packet_desc.mVariableFramesInPacket -= packet_length_adjust;
            packet_length_adjust = 0;
        } else if (packet_count == page_packets && packet_length_adjust < 0) {
            speex_packet_desc.mVariableFramesInPacket = - speex_packet_desc.mVariableFramesInPacket - packet_length_adjust;
        }

        CASpeexDecoder::InPacket(opk.packet, &speex_packet_desc);
    }

    mFramesBufferedList.push_back(packet_count);

    dbg_printf("<.. [%08lx] CAOggSpeexDecoder :: InPacket(): packet_count: %ld\n", (UInt32) this, packet_count);
}


void CAOggSpeexDecoder::InitializeCompressionSettings()
{
    if (mCookie != NULL) {
        if (mCompressionInitialized)
            ogg_stream_clear(&mO_st);

        OggSerialNoAtom *atom = reinterpret_cast<OggSerialNoAtom*> (mCookie);

        if (EndianS32_BtoN(atom->type) == kCookieTypeOggSerialNo && (mCookieSize - EndianS32_BtoN(atom->size) >= 0)) {
            ogg_stream_init(&mO_st, EndianS32_BtoN(atom->serialno));
        }
    }

    ogg_stream_reset(&mO_st);

    CASpeexDecoder::InitializeCompressionSettings();
}
