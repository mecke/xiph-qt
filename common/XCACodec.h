/*
 *  XCACodec.h
 *
 *    XCACodec class definition; abstract interface for shared packet i/o
 *    functionality.
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
 *  Last modified: $Id: XCACodec.h 12093 2006-11-12 14:44:51Z arek $
 *
 */


#if !defined(__XCACodec_h__)
#define __XCACodec_h__

#include "ACBaseCodec.h"

#include "ringbuffer.h"

#include "CAStreamBasicDescription.h"


typedef enum BDCStatus {
    kBDCStatusOK,
    kBDCStatusNeedMoreData,
    kBDCStatusAbort
} BDCStatus;


class XCACodec : public ACBaseCodec
{
 public:
    XCACodec();
    virtual ~XCACodec();

    // AudioCodec interface
    virtual void ReallocateInputBuffer(UInt32 inInputBufferByteSize) { BDCReallocate(inInputBufferByteSize); };
    virtual UInt32 GetInputBufferByteSize() const { return BufferGetBytesSize(); };
    virtual UInt32 GetUsedInputBufferByteSize() const { return BufferGetUsedBytesSize(); };

    virtual void AppendInputData(const void* inInputData, UInt32& ioInputDataByteSize, UInt32& ioNumberPackets,
                                 const AudioStreamPacketDescription* inPacketDescription);
    virtual UInt32 ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                        AudioStreamPacketDescription* outPacketDescription);


#pragma mark Buffer/Decode/Convert interface

 protected:
    RingBuffer mBDCBuffer;
    BDCStatus mBDCStatus;


    virtual void BDCInitialize(UInt32 inInputBufferByteSize);
    virtual void BDCUninitialize();
    virtual void BDCReset();
    virtual void BDCReallocate(UInt32 inInputBufferByteSize);

    virtual BDCStatus BDCGetStatus() const { return mBDCStatus; };

    virtual UInt32 BufferGetBytesSize() const;
    virtual UInt32 BufferGetUsedBytesSize() const;
    virtual UInt32 BufferGetAvailableBytesSize() const;
    virtual Boolean BufferIsEmpty() const;

    virtual void InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription) = 0;

    virtual UInt32 FramesReady() const = 0;
    virtual Boolean GenerateFrames() = 0;
    virtual void OutputFrames(void* outOutputData, UInt32 inNumberFrames, UInt32 inFramesOffset,
                              AudioStreamPacketDescription* outPacketDescription) const = 0;
    virtual void Zap(UInt32 inFrames) = 0;

    virtual UInt32 InPacketsConsumed() const;
};


#endif /* __XCACodec_h__ */
