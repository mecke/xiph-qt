/*
 *  CAOggVorbisDecoder.h
 *
 *    CAOggVorbisDecoder class definition.
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
 *  Last modified: $Id: CAOggVorbisDecoder.h 11273 2006-04-24 22:28:46Z arek $
 *
 */


#if !defined(__CAOggVorbisDecoder_h__)
#define __CAOggVorbisDecoder_h__

#include <Ogg/ogg.h>
#include <vector>

#include "CAVorbisDecoder.h"

#include "CAStreamBasicDescription.h"


class CAOggVorbisDecoder :
public CAVorbisDecoder
{
 public:
    CAOggVorbisDecoder();
    virtual ~CAOggVorbisDecoder();

    virtual UInt32          ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                                 AudioStreamPacketDescription* outPacketDescription);
    virtual void            SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat);

 protected:
    virtual void            BDCInitialize(UInt32 inInputBufferByteSize);
    virtual void            BDCUninitialize();
    virtual void            BDCReset();
    virtual void            BDCReallocate(UInt32 inInputBufferByteSize);

    virtual void            InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription);

    void                    InitializeCompressionSettings();

    ogg_stream_state        mO_st;
    std::vector<SInt32>     mFramesBufferedList;

    /* SampleOffsetBuffer */
    Byte                    *mSOBuffer;
    UInt32                  mSOBufferSize;
    UInt32                  mSOBufferUsed;
    UInt32                  mSOBufferWrapped;
    UInt32                  mSOBufferPackets;
    UInt32                  mSOBufferPages;
    UInt32                  mSOReturned;

    UInt32                  mFirstPageNo;
};



#endif /* __CAOggVorbisDecoder_h__ */
