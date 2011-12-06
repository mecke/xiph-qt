/*
 *  CAOggFLACDecoder.cpp
 *
 *    CAOggFLACDecoder class implementation; translation layer handling
 *    ogg page encapsulation of FLAC frmaes, using CAFLACDecoder
 *    for the actual decoding.
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
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: CAOggFLACDecoder.cpp 12754 2007-03-14 03:51:23Z arek $
 *
 */


#include "CAOggFLACDecoder.h"

#include "fccs.h"
#include "data_types.h"

#include "wrap_ogg.h"

#include "debug.h"


CAOggFLACDecoder::CAOggFLACDecoder() :
    CAFLACDecoder(true),
    mFramesBufferedList(),
    complete_pages(0)
{
    CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAudioFormatXiphOggFramedFLAC,
                                            kFLACBytesPerPacket, kFLACFramesPerPacket,
                                            kFLACBytesPerFrame, kFLACChannelsPerFrame,
                                            kFLACBitsPerChannel, kFLACFormatFlags);
    AddInputFormat(theInputFormat);

    mInputFormat.mSampleRate = 44100;
    mInputFormat.mFormatID = kAudioFormatXiphOggFramedFLAC;
    mInputFormat.mFormatFlags = kFLACFormatFlags;
    mInputFormat.mBytesPerPacket = kFLACBytesPerPacket;
    mInputFormat.mFramesPerPacket = kFLACFramesPerPacket;
    mInputFormat.mBytesPerFrame = kFLACBytesPerFrame;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = kFLACBitsPerChannel;

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

CAOggFLACDecoder::~CAOggFLACDecoder()
{
    if (mCompressionInitialized)
        ogg_stream_clear(&mO_st);
}

void CAOggFLACDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
    if (!mIsInitialized) {
        if (inInputFormat.mFormatID != kAudioFormatXiphOggFramedFLAC) {
            dbg_printf("CAOggFLACDecoder::SetFormats: only support Xiph FLAC (Ogg-framed) for input\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }
        XCACodec::SetCurrentInputFormat(inInputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

UInt32 CAOggFLACDecoder::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                                AudioStreamPacketDescription* outPacketDescription)
{
    dbg_printf("[ oFD]  >> [%08lx] ProduceOutputPackets(%ld [%ld])\n", (UInt32) this, ioNumberPackets, ioOutputDataByteSize);
    UInt32 ret = kAudioCodecProduceOutputPacketSuccess;

    if (mFramesBufferedList.empty()) {
        ioOutputDataByteSize = 0;
        ioNumberPackets = 0;
        ret = kAudioCodecProduceOutputPacketNeedsMoreInputData;
        dbg_printf("<!E [%08lx] CAOggFLACDecoder :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n", (UInt32) this,
                   ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
        return ret;
    }

    OggPagePacket &opp = mFramesBufferedList.front();
    UInt32 flac_frames = opp.packets;
    UInt32 flac_bytes = opp.frames * mOutputFormat.mBytesPerFrame;
    UInt32 ogg_packets = 0;
    UInt32 flac_returned_data = ioOutputDataByteSize;
    UInt32 flac_total_returned_data = 0;
    Byte *the_data = static_cast<Byte*> (outOutputData);
    Boolean empty_packet = false;

    while (true) {
        UInt32 flac_return = kAudioCodecProduceOutputPacketSuccess;
        empty_packet = false;
        if (complete_pages < 1) {
            flac_return = kAudioCodecProduceOutputPacketNeedsMoreInputData;
            flac_frames = 0;
            flac_returned_data = 0;
        } else if (flac_frames == 0) {
            UInt32 one_flac_frame = 1;
            empty_packet = true;
            if (flac_bytes < flac_returned_data) {
                flac_returned_data = flac_bytes;
            }
            flac_return = CAFLACDecoder::ProduceOutputPackets(the_data, flac_returned_data, one_flac_frame, NULL);
        } else {
            flac_return = CAFLACDecoder::ProduceOutputPackets(the_data, flac_returned_data, flac_frames, NULL);
        }

        if (flac_return == kAudioCodecProduceOutputPacketSuccess || flac_return == kAudioCodecProduceOutputPacketSuccessHasMore) {
            if (flac_frames > 0)
                opp.packets -= flac_frames;

            if (flac_returned_data > 0)
                opp.frames -= flac_returned_data / mOutputFormat.mBytesPerFrame;

            dbg_printf("[ oFD]     [%08lx] ProduceOutputPackets() p:%ld, f:%ld, c:%ld\n", (UInt32) this, opp.packets, opp.frames, complete_pages);
            if (opp.packets <= 0 && opp.frames <= 0) {
                ogg_packets++;
                if (!empty_packet)
                    complete_pages--;
                mFramesBufferedList.erase(mFramesBufferedList.begin());
                opp = mFramesBufferedList.front();
            }

            flac_total_returned_data += flac_returned_data;

            if (ogg_packets == ioNumberPackets || flac_total_returned_data == ioOutputDataByteSize ||
                flac_return == kAudioCodecProduceOutputPacketSuccess)
            {
                ioNumberPackets = ogg_packets;
                ioOutputDataByteSize = flac_total_returned_data;

                if (!mFramesBufferedList.empty())
                    ret = kAudioCodecProduceOutputPacketSuccessHasMore;
                else
                    ret = kAudioCodecProduceOutputPacketSuccess;

                break;
            } else {
                the_data += flac_returned_data;
                flac_returned_data = ioOutputDataByteSize - flac_total_returned_data;
                flac_frames = opp.packets;
                flac_bytes = opp.frames * mOutputFormat.mBytesPerFrame;
            }
        } else if (flac_return == kAudioCodecProduceOutputPacketNeedsMoreInputData) {
            if (flac_frames > 0)
                opp.packets -= flac_frames;

            if (flac_returned_data > 0)
                opp.frames -= flac_returned_data / mOutputFormat.mBytesPerFrame;

            if (opp.packets <= 0 && opp.frames <= 0) {
                ogg_packets++;
                if (!empty_packet)
                    complete_pages--;
                mFramesBufferedList.erase(mFramesBufferedList.begin());
                //opp = mFramesBufferedList.front();
            }

            ret = kAudioCodecProduceOutputPacketNeedsMoreInputData;
            ioOutputDataByteSize = flac_total_returned_data + flac_returned_data;
            ioNumberPackets = ogg_packets;
            break;
        } else {
            ret = kAudioCodecProduceOutputPacketFailure;
            ioOutputDataByteSize = flac_total_returned_data;
            ioNumberPackets = ogg_packets;
            break;
        }
    }

    dbg_printf("[ oFD] <.. [%08lx] ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n",
               (UInt32) this, ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
    return ret;
}


void CAOggFLACDecoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    CAFLACDecoder::BDCInitialize(inInputBufferByteSize);
}

void CAOggFLACDecoder::BDCUninitialize()
{
    mFramesBufferedList.clear();
    complete_pages = 0;
    CAFLACDecoder::BDCUninitialize();
}

void CAOggFLACDecoder::BDCReset()
{
    mFramesBufferedList.clear();
    complete_pages = 0;
    if (mCompressionInitialized)
        ogg_stream_reset(&mO_st);
    CAFLACDecoder::BDCReset();
}

void CAOggFLACDecoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    mFramesBufferedList.clear();
    complete_pages = 0;
    CAFLACDecoder::BDCReallocate(inInputBufferByteSize);
}


void CAOggFLACDecoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    if (!mCompressionInitialized)
        CODEC_THROW(kAudioCodecUnspecifiedError);

    ogg_page op;

    if (!WrapOggPage(&op, inInputData, inPacketDescription->mDataByteSize + inPacketDescription->mStartOffset, inPacketDescription->mStartOffset))
        CODEC_THROW(kAudioCodecUnspecifiedError);

    dbg_printf("[ oFD]   : [%08lx] InPacket() [%4.4s] %ld\n", (UInt32) this, (char *) (static_cast<const Byte*> (inInputData) + inPacketDescription->mStartOffset),
               ogg_page_pageno(&op));

    ogg_packet opk;
    SInt32 packet_count = 0;
    int oret;
    AudioStreamPacketDescription flac_packet_desc = {0, 0, 0};
    UInt32 page_packets = ogg_page_packets(&op);

    ogg_stream_pagein(&mO_st, &op);
    while ((oret = ogg_stream_packetout(&mO_st, &opk)) != 0) {
        if (oret < 0) {
            page_packets--;
            continue;
        }

        packet_count++;

        flac_packet_desc.mDataByteSize = opk.bytes;

        CAFLACDecoder::InPacket(opk.packet, &flac_packet_desc);
    }

    if (packet_count > 0)
        complete_pages += 1;

    mFramesBufferedList.push_back(OggPagePacket(packet_count, inPacketDescription->mVariableFramesInPacket));
}


void CAOggFLACDecoder::InitializeCompressionSettings()
{
    if (mCookie != NULL) {
        if (mCompressionInitialized)
            ogg_stream_clear(&mO_st);

        OggSerialNoAtom *atom = reinterpret_cast<OggSerialNoAtom*> (mCookie);

        if (EndianS32_BtoN(atom->type) == kCookieTypeOggSerialNo && EndianS32_BtoN(atom->size) <= mCookieSize) {
            ogg_stream_init(&mO_st, EndianS32_BtoN(atom->serialno));
        }
    }

    ogg_stream_reset(&mO_st);

    CAFLACDecoder::InitializeCompressionSettings();
}
