/*
 *  CAOggVorbisDecoder.cpp
 *
 *    CAOggVorbisDecoder class implementation; translation layer handling
 *    ogg page encapsulation of Vorbis packets, using CAVorbisDecoder
 *    for the actual decoding.
 *
 *
 *  Copyright (c) 2005-2006  Arek Korbik
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
 *  Last modified: $Id: CAOggVorbisDecoder.cpp 12814 2007-03-27 22:09:32Z arek $
 *
 */


#include "CAOggVorbisDecoder.h"

#include "fccs.h"
#include "data_types.h"

#include "wrap_ogg.h"

#include "debug.h"


CAOggVorbisDecoder::CAOggVorbisDecoder() :
    CAVorbisDecoder(true),
    mFramesBufferedList(),
    mSOBuffer(NULL),
    mSOBufferSize(0), mSOBufferUsed(0),
    mSOBufferWrapped(0), mSOBufferPackets(0),
    mSOBufferPages(0), mSOReturned(0),
    mFirstPageNo(2)
{
    CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAudioFormatXiphOggFramedVorbis,
                                            kVorbisBytesPerPacket, kVorbisFramesPerPacket,
                                            kVorbisBytesPerFrame, kVorbisChannelsPerFrame,
                                            kVorbisBitsPerChannel, kVorbisFormatFlags);
    AddInputFormat(theInputFormat);

    mInputFormat.mSampleRate = 44100.0;
    mInputFormat.mFormatID = kAudioFormatXiphOggFramedVorbis;
    mInputFormat.mFormatFlags = kVorbisFormatFlags;
    mInputFormat.mBytesPerPacket = kVorbisBytesPerPacket;
    mInputFormat.mFramesPerPacket = kVorbisFramesPerPacket;
    mInputFormat.mBytesPerFrame = kVorbisBytesPerFrame;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = kVorbisBitsPerChannel;

    CAStreamBasicDescription theOutputFormat1(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16,
                                              kAudioFormatFlagsNativeEndian |
                                              kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
    AddOutputFormat(theOutputFormat1);
    CAStreamBasicDescription theOutputFormat2(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 32,
                                              kAudioFormatFlagsNativeFloatPacked);
    AddOutputFormat(theOutputFormat2);

    mOutputFormat.mSampleRate = 44100.0;
    mOutputFormat.mFormatID = kAudioFormatLinearPCM;
    mOutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    mOutputFormat.mBytesPerPacket = 8;
    mOutputFormat.mFramesPerPacket = 1;
    mOutputFormat.mBytesPerFrame = 8;
    mOutputFormat.mChannelsPerFrame = 2;
    mOutputFormat.mBitsPerChannel = 32;
}

CAOggVorbisDecoder::~CAOggVorbisDecoder()
{
    if (mCompressionInitialized)
        ogg_stream_clear(&mO_st);

    if (mSOBuffer)
        delete[] mSOBuffer;
}

void CAOggVorbisDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
    if (!mIsInitialized) {
        if (inInputFormat.mFormatID != kAudioFormatXiphOggFramedVorbis) {
            dbg_printf("CAOggVorbisDecoder::SetFormats: only support Xiph Vorbis (Ogg-framed) for input\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }
        XCACodec::SetCurrentInputFormat(inInputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

UInt32 CAOggVorbisDecoder::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                                AudioStreamPacketDescription* outPacketDescription)
{
    dbg_printf("[VDO ]  >> [%08lx] :: ProduceOutputPackets(%ld [%ld]) (%ld, %ld, %ld; %ld[%ld])\n",
               (UInt32) this, ioNumberPackets, ioOutputDataByteSize, mSOBufferSize, mSOBufferUsed, mSOReturned, mSOBufferPages, mSOBufferPackets);
    UInt32 ret = kAudioCodecProduceOutputPacketSuccess;

    if (mFramesBufferedList.empty()) {
        ioOutputDataByteSize = 0;
        ioNumberPackets = 0;
        ret = kAudioCodecProduceOutputPacketNeedsMoreInputData;
        dbg_printf("[VDO ] <!E [%08lx] :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n", (UInt32) this,
                   ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
        return ret;
    }

    UInt32 vorbis_packets = mFramesBufferedList.front();
    UInt32 ogg_packets = 0;
    UInt32 vorbis_returned_data = ioOutputDataByteSize;
    UInt32 vorbis_total_returned_data = 0;
    Byte *the_data = static_cast<Byte*> (outOutputData);

    if (mSOBuffer != NULL) {
        dbg_printf("[VDO ]   + SOBuffering output\n");
        /* stream/sample offset buffer not empty - must be beginning of the stream */
        if (mSOBufferUsed == 0) {
            /* fill the buffer first */
            the_data = mSOBuffer;
            vorbis_packets = mSOBufferPackets;
            vorbis_returned_data = mSOBufferSize;

            while (true) {
                UInt32 vorbis_return = CAVorbisDecoder::ProduceOutputPackets(the_data, vorbis_returned_data, vorbis_packets, NULL);
                if (vorbis_return == kAudioCodecProduceOutputPacketSuccess ||
                    vorbis_return == kAudioCodecProduceOutputPacketSuccessHasMore ||
                    vorbis_return == kAudioCodecProduceOutputPacketNeedsMoreInputData)
                {
                    vorbis_total_returned_data += vorbis_returned_data;

                    if (vorbis_return == kAudioCodecProduceOutputPacketSuccess ||
                        vorbis_return == kAudioCodecProduceOutputPacketNeedsMoreInputData ||
                        vorbis_packets == mSOBufferPackets)
                    {
                        /* ok, all data decoded */
                        mSOBufferUsed = vorbis_total_returned_data;
                        if (vorbis_total_returned_data > mSOBufferSize) {
                            mSOBufferWrapped = vorbis_total_returned_data % mSOBufferSize;
                            mSOBufferUsed = mSOBufferSize;
                        } else if (vorbis_total_returned_data < mSOBufferSize) {
                            memset(mSOBuffer + mSOBufferUsed, 0, mSOBufferSize - mSOBufferUsed);
                            mSOBufferWrapped = mSOBufferUsed;
                        }
                        the_data = static_cast<Byte*> (outOutputData);
                        break;
                    } else {
                        if (vorbis_total_returned_data < mSOBufferSize) {
                            the_data += vorbis_returned_data;
                            mSOBufferUsed = vorbis_total_returned_data;
                            vorbis_returned_data = mSOBufferSize - mSOBufferUsed;
                        } else {
                            /* buffer filled, but the underlying decoder still has more... */
                            the_data = mSOBuffer;
                            mSOBufferUsed = mSOBufferSize;
                            vorbis_returned_data = mSOBufferSize;
                        }
                        mSOBufferPackets -= vorbis_packets;
                        vorbis_packets = mSOBufferPackets;
                    }
                } else {
                    ret = kAudioCodecProduceOutputPacketFailure;
                    ioOutputDataByteSize = 0;
                    ioNumberPackets = mSOBufferPackets;
                    break;
                }
            }
        }

        /* buffer filled, return chunk by chunk */
        if (ret != kAudioCodecProduceOutputPacketFailure) {
            vorbis_returned_data = mSOBufferSize - mSOReturned;
            if (vorbis_returned_data > ioOutputDataByteSize)
                vorbis_returned_data = ioOutputDataByteSize;
            UInt32 offset = (mSOBufferWrapped + mSOReturned) % mSOBufferSize;
            Byte *src_data = mSOBuffer + offset;
            if (offset + vorbis_returned_data > mSOBufferSize) {
                /* copy in two takes */
                UInt32 first_chunk_size = mSOBufferSize - offset;
                BlockMoveData(src_data, the_data, first_chunk_size);
                BlockMoveData(mSOBuffer, the_data + first_chunk_size, vorbis_returned_data - first_chunk_size);
            } else {
                BlockMoveData(src_data, the_data, vorbis_returned_data);
            }
            mSOReturned += vorbis_returned_data;
            ioOutputDataByteSize = vorbis_returned_data;
            // ioNumberPackets = ioNumberPackets;
            if (mSOReturned == mSOBufferSize) {
                /* all data flushed */
                ret = kAudioCodecProduceOutputPacketSuccess;
                ioNumberPackets = mSOBufferPages;
                mFramesBufferedList.erase(mFramesBufferedList.begin());
                delete[] mSOBuffer;
                mSOBufferSize = mSOBufferUsed = mSOBufferWrapped = mSOBufferPackets = mSOBufferPages = mSOReturned = 0;
                mSOBuffer = NULL;
            } else {
                ret = kAudioCodecProduceOutputPacketSuccessHasMore;
                ogg_packets = ioNumberPackets;
                if (ogg_packets >= mSOBufferPages) {
                    ogg_packets = 1;
                }
                if (mSOBufferPages == 1)
                    ogg_packets = 0;
                mSOBufferPages -= ogg_packets;
                ioNumberPackets = ogg_packets;
            }
        }
    } else {
        /* normal output without additional buffering */
        while (true) {
            UInt32 vorbis_return = CAVorbisDecoder::ProduceOutputPackets(the_data, vorbis_returned_data, vorbis_packets, NULL);
            if (vorbis_return == kAudioCodecProduceOutputPacketSuccess || vorbis_return == kAudioCodecProduceOutputPacketSuccessHasMore) {
                if (vorbis_packets > 0)
                    mFramesBufferedList.front() -= vorbis_packets;

                if (mFramesBufferedList.front() <= 0) {
                    ogg_packets++;
                    mFramesBufferedList.erase(mFramesBufferedList.begin());
                }

                vorbis_total_returned_data += vorbis_returned_data;

                if (vorbis_total_returned_data == ioOutputDataByteSize || vorbis_return == kAudioCodecProduceOutputPacketSuccess)
                {
                    ioNumberPackets = ogg_packets;
                    ioOutputDataByteSize = vorbis_total_returned_data;

                    if (!mFramesBufferedList.empty())
                        ret = kAudioCodecProduceOutputPacketSuccessHasMore;
                    else
                        ret = kAudioCodecProduceOutputPacketSuccess;

                    break;
                } else {
                    the_data += vorbis_returned_data;
                    vorbis_returned_data = ioOutputDataByteSize - vorbis_total_returned_data;
                    vorbis_packets = mFramesBufferedList.front();
                }
            } else {
                ret = kAudioCodecProduceOutputPacketFailure;
                ioOutputDataByteSize = vorbis_total_returned_data;
                ioNumberPackets = ogg_packets;
                break;
            }
        }
    }

    if (ret == kAudioCodecProduceOutputPacketSuccess || ret == kAudioCodecProduceOutputPacketSuccessHasMore) {
        ioNumberPackets = ioOutputDataByteSize / mOutputFormat.mBytesPerFrame;
    }

    dbg_printf("[VDO ] <.. [%08lx] :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n",
               (UInt32) this, ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
    return ret;
}


void CAOggVorbisDecoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    CAVorbisDecoder::BDCInitialize(inInputBufferByteSize);
}

void CAOggVorbisDecoder::BDCUninitialize()
{
    mFramesBufferedList.clear();

    delete[] mSOBuffer;
    mSOBufferSize = mSOBufferUsed = mSOBufferWrapped = mSOBufferPackets = mSOBufferPages = mSOReturned = 0;
    mSOBuffer = NULL;

    CAVorbisDecoder::BDCUninitialize();
}

void CAOggVorbisDecoder::BDCReset()
{
    mFramesBufferedList.clear();

    delete[] mSOBuffer;
    mSOBufferSize = mSOBufferUsed = mSOBufferWrapped = mSOBufferPackets = mSOBufferPages = mSOReturned = 0;
    mSOBuffer = NULL;

    if (mCompressionInitialized)
        ogg_stream_reset(&mO_st);
    CAVorbisDecoder::BDCReset();
}

void CAOggVorbisDecoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    mFramesBufferedList.clear();
    CAVorbisDecoder::BDCReallocate(inInputBufferByteSize);
}


void CAOggVorbisDecoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    dbg_printf("[VDO ]  >> [%08lx] :: InPacket({%ld, %ld})\n", (UInt32) this, inPacketDescription->mDataByteSize, inPacketDescription->mVariableFramesInPacket);
    if (!mCompressionInitialized) {
        dbg_printf("[VDO ] <!I [%08lx] :: InPacket()\n", (UInt32) this);
        CODEC_THROW(kAudioCodecUnspecifiedError);
    }

    ogg_page op;

    if (!WrapOggPage(&op, inInputData, inPacketDescription->mDataByteSize + inPacketDescription->mStartOffset, inPacketDescription->mStartOffset)) {
        dbg_printf("[VDO ] <!O [%08lx] :: InPacket()\n", (UInt32) this);
        CODEC_THROW(kAudioCodecUnspecifiedError);
    }

    ogg_packet opk;
    SInt32 packet_count = 0;
    int oret;
    AudioStreamPacketDescription vorbis_packet_desc = {0, 0, 0};
    UInt32 page_packets = ogg_page_packets(&op);

    ogg_stream_pagein(&mO_st, &op);
    while ((oret = ogg_stream_packetout(&mO_st, &opk)) != 0) {
        if (oret < 0) {
            page_packets--;
            continue;
        }

        packet_count++;

        vorbis_packet_desc.mDataByteSize = opk.bytes;

        CAVorbisDecoder::InPacket(opk.packet, &vorbis_packet_desc);
    }

    mFramesBufferedList.push_back(packet_count);

    /* if pageno == FPN from the cookie then this is the first audio page
       - we need to buffer the audio data from the first page, to be able
       to apply the stream offset described in Vorbis spec, section A.2 */
    /* the expected number of audio samples(frames) should be less than
       some reasonable value, like 255 * 8192:
       (max packets per ogg page * max size of a vorbis block) */
    if (ogg_page_pageno(&op) == mFirstPageNo && inPacketDescription->mVariableFramesInPacket > 0 && inPacketDescription->mVariableFramesInPacket < (255 * 8192)) {
        mSOBufferSize = inPacketDescription->mVariableFramesInPacket * mOutputFormat.mBytesPerFrame;
        if (mSOBuffer)
            delete[] mSOBuffer;
        mSOBuffer = new Byte[mSOBufferSize];
        mSOBufferUsed = 0;
        mSOBufferWrapped = 0;
        mSOBufferPackets = packet_count;
        mSOBufferPages = 1; // ?! :/
        mSOReturned = 0;
    }

    dbg_printf("[VDO ] <.. [%08lx] :: InPacket(pn: %ld)\n", (UInt32) this, ogg_page_pageno (&op));
}


void CAOggVorbisDecoder::InitializeCompressionSettings()
{
    if (mCookie != NULL) {
        if (mCompressionInitialized)
            ogg_stream_clear(&mO_st);

        OggSerialNoAtom *atom = reinterpret_cast<OggSerialNoAtom*> (mCookie);
        Byte *ptrheader = mCookie + EndianU32_BtoN(atom->size);
        Byte *cend = mCookie + mCookieSize;
        CookieAtomHeader *aheader = reinterpret_cast<CookieAtomHeader*> (ptrheader);

        if (EndianS32_BtoN(atom->type) == kCookieTypeOggSerialNo && EndianS32_BtoN(atom->size) <= mCookieSize) {
            ogg_stream_init(&mO_st, EndianS32_BtoN(atom->serialno));
        }

        while (ptrheader < cend) {
            aheader = reinterpret_cast<CookieAtomHeader*> (ptrheader);
            ptrheader += EndianU32_BtoN(aheader->size);

            if (EndianS32_BtoN(aheader->type) == kCookieTypeVorbisFirstPageNo && ptrheader <= cend) {
                mFirstPageNo = EndianU32_BtoN((reinterpret_cast<OggSerialNoAtom*> (aheader))->serialno);
                dbg_printf("[VDO ]   = [%08lx] :: InitializeCompressionSettings(fpn: %ld)\n", (UInt32) this, mFirstPageNo);
                break;
            }
        }
    }

    ogg_stream_reset(&mO_st);

    CAVorbisDecoder::InitializeCompressionSettings();
}
