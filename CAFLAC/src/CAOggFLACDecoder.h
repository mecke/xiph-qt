/*
 *  CAOggFLACDecoder.h
 *
 *    CAOggFLACDecoder class definition.
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
 *  Last modified: $Id: CAOggFLACDecoder.h 12356 2007-01-20 00:18:04Z arek $
 *
 */


#if !defined(__CAOggFLACDecoder_h__)
#define __CAOggFLACDecoder_h__

#include <Ogg/ogg.h>
#include <vector>

#include "CAFLACDecoder.h"

#include "CAStreamBasicDescription.h"


class CAOggFLACDecoder :
public CAFLACDecoder
{
 public:
    CAOggFLACDecoder();
    virtual ~CAOggFLACDecoder();

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

    struct OggPagePacket {
        UInt32 packets;
        UInt32 frames;
        //UInt32 left;

        OggPagePacket() : packets(0), frames(0) {};
        OggPagePacket(UInt32 inPackets, UInt32 inFrames) : packets(inPackets), frames(inFrames) {};
    };

    typedef std::vector<OggPagePacket> OggPagePacketList;
    OggPagePacketList mFramesBufferedList;

    UInt32 complete_pages;
};



#endif /* __CAOggFLACDecoder_h__ */
