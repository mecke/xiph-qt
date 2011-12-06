/*
 *  XCACodec.cpp
 *
 *    XCACodec class implementation; shared packet i/o functionality.
 *
 *
 *  Copyright (c) 2005,2007  Arek Korbik
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
 *  Last modified: $Id: XCACodec.cpp 12435 2007-02-06 01:49:26Z arek $
 *
 */


#include "XCACodec.h"

//#define NDEBUG
#include "debug.h"


XCACodec::XCACodec() :
    mBDCBuffer(),
    mBDCStatus(kBDCStatusOK)
{
}

XCACodec::~XCACodec()
{
}

#pragma mark The Core Functions

void XCACodec::AppendInputData(const void* inInputData, UInt32& ioInputDataByteSize, UInt32& ioNumberPackets,
                               const AudioStreamPacketDescription* inPacketDescription)
{
    dbg_printf("[ XC ]  >> [%08lx] AppendInputData(%ld [%ld] %d)\n", (UInt32) this, ioNumberPackets, ioInputDataByteSize, inPacketDescription != NULL);
    if(!mIsInitialized) CODEC_THROW(kAudioCodecStateError);

    UInt32 bytesToCopy = BufferGetAvailableBytesSize();
    if (bytesToCopy > 0) {
        UInt32 packet = 0;
        UInt32 bytes = 0;
        UInt32 packets_added = 0;
        if (inPacketDescription != NULL) {
            while (packet < ioNumberPackets) {
                if (bytes + inPacketDescription[packet].mDataByteSize > bytesToCopy)
                    break;
                dbg_printf("[ XC ]                %ld: %ld [%ld]\n", packet, inPacketDescription[packet].mDataByteSize,
                           inPacketDescription[packet].mVariableFramesInPacket);
                InPacket(inInputData, &inPacketDescription[packet]);

                bytes += inPacketDescription[packet].mDataByteSize;
                packet++;
                packets_added++;
            }
        } else if (mInputFormat.mBytesPerFrame != 0) { /* inPacketDescription == NULL here, but it's and error condition
                                                          if we're doing decoding (mInputFormat.mBytesPerFrame == 0), so
                                                          skip this branch -- thanks to Jan Gerber for spotting it. */

            if (ioInputDataByteSize < bytesToCopy)
                bytesToCopy = ioInputDataByteSize;

            // align the data on a frame boundary
            bytesToCopy -= bytesToCopy % mInputFormat.mBytesPerFrame;

            AudioStreamPacketDescription gen_pd = {0, bytesToCopy / mInputFormat.mBytesPerFrame, bytesToCopy};
            dbg_printf("     -__-  :: %d: %ld [%ld]\n", 0, gen_pd.mDataByteSize, gen_pd.mVariableFramesInPacket);
            InPacket(inInputData, &gen_pd);
            bytes += bytesToCopy;
            packets_added++;
            packet = bytesToCopy / mInputFormat.mBytesPerFrame;
        }

        if (bytes == 0 && packets_added == 0)
            CODEC_THROW(kAudioCodecNotEnoughBufferSpaceError);
        else {
            ioInputDataByteSize = bytes;
            ioNumberPackets = packet;
        }
    } else {
        CODEC_THROW(kAudioCodecNotEnoughBufferSpaceError);
    }
    dbg_printf("[ XC ] <.. [%08lx] AppendInputData(%ld [%ld])\n", (UInt32) this, ioNumberPackets, ioInputDataByteSize);
}

UInt32 XCACodec::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                      AudioStreamPacketDescription* outPacketDescription)
{
    dbg_printf("[ XC ]  >> [%08lx] ProduceOutputPackets(%ld [%ld] %d)\n", (UInt32) this, ioNumberPackets, ioOutputDataByteSize, outPacketDescription != NULL);

    UInt32 theAnswer = kAudioCodecProduceOutputPacketSuccess;

    if (!mIsInitialized)
        CODEC_THROW(kAudioCodecStateError);

    UInt32 frames = 0;
    UInt32 fout = 0; //frames produced
    UInt32 pout = 0; //full (input) packets processed
    UInt32 requested_space_as_frames = ioOutputDataByteSize / mOutputFormat.mBytesPerFrame;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

    //TODO: zamienic fout/pout z ioOutputDataByteSize/ioNumberPackets:
    //      zainicjowac na poczatku, a potem bezposrednio modyfikowac io*, nie [pf]out ...
    while (fout < requested_space_as_frames && pout < ioNumberPackets) {
        while ((frames = FramesReady()) == 0) {
            if (BufferIsEmpty()) {
                ioNumberPackets = pout;
                ioOutputDataByteSize = mOutputFormat.mBytesPerFrame * fout;
                theAnswer = kAudioCodecProduceOutputPacketNeedsMoreInputData;
                dbg_printf("<.! [%08lx] XCACodec :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n", (UInt32) this,
                           ioNumberPackets, ioOutputDataByteSize, theAnswer, FramesReady());
                return theAnswer;
            }

            if (GenerateFrames() != true) {
                if (BDCGetStatus() == kBDCStatusAbort) {
                    ioNumberPackets = pout;
                    ioOutputDataByteSize = mOutputFormat.mBytesPerFrame * fout;
                    theAnswer = kAudioCodecProduceOutputPacketFailure;
                    dbg_printf("[ XC ] <!! [%08lx] ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n", (UInt32) this,
                               ioNumberPackets, ioOutputDataByteSize, theAnswer, FramesReady());
                    return theAnswer;
                }
            }
        }

        if (frames == 0)
            continue;

        if ((fout + frames) * mOutputFormat.mBytesPerFrame > ioOutputDataByteSize)
            frames = requested_space_as_frames - fout;

        OutputFrames(outOutputData, frames, fout, outPacketDescription);

        fout += frames;

        Zap(frames);

        pout += InPacketsConsumed();
    }

    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    ioOutputDataByteSize = mOutputFormat.mBytesPerFrame * fout; //???
    ioNumberPackets = pout;

    theAnswer = (FramesReady() > 0 || !BufferIsEmpty()) ? kAudioCodecProduceOutputPacketSuccessHasMore
        : kAudioCodecProduceOutputPacketSuccess;

    dbg_printf("[ XC ] <.. [%08lx] ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n",
               (UInt32) this, ioNumberPackets, ioOutputDataByteSize, theAnswer, FramesReady());
    return theAnswer;
}

UInt32 XCACodec::InPacketsConsumed() const
{
    // the simplest case, works properly only if _every_ 'in' packet
    // generates positive number of samples (so it won't work with Vorbis)
    return (FramesReady() == 0);
}


#pragma mark Buffer/Decode/Convert interface

void XCACodec::BDCInitialize(UInt32 inInputBufferByteSize)
{
    mBDCBuffer.Initialize(inInputBufferByteSize);
    mBDCStatus = kBDCStatusOK;
}

void XCACodec::BDCUninitialize()
{
    mBDCBuffer.Uninitialize();
}

void XCACodec::BDCReset()
{
    mBDCBuffer.Reset();
}

void XCACodec::BDCReallocate(UInt32 inInputBufferByteSize)
{
    if (mBDCBuffer.GetBufferByteSize() < inInputBufferByteSize) {
        mBDCBuffer.Reallocate(inInputBufferByteSize);
    } else {
        // temporary left here until reallocation allows decreasing buffer size
        mBDCBuffer.Uninitialize();
        mBDCBuffer.Initialize(inInputBufferByteSize);
    }
}


UInt32 XCACodec::BufferGetBytesSize() const
{
    return mBDCBuffer.GetBufferByteSize();
}

UInt32 XCACodec::BufferGetUsedBytesSize() const
{
    return mBDCBuffer.GetDataAvailable();
}

UInt32 XCACodec::BufferGetAvailableBytesSize() const
{
    return mBDCBuffer.GetSpaceAvailable();
}

Boolean XCACodec::BufferIsEmpty() const
{
    return (mBDCBuffer.GetDataAvailable() == 0);
}
