/*
 *  CASpeexDecoder.h
 *
 *    CASpeexDecoder class definition.
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
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: CASpeexDecoder.h 12093 2006-11-12 14:44:51Z arek $
 *
 */


#if !defined(__CASpeexDecoder_h__)
#define __CASpeexDecoder_h__


#include "XCACodec.h"

#include <Speex/speex.h>
#include <Speex/speex_stereo.h>
#include <Speex/speex_header.h>

#include <vector>


#define _SHOULD_BE_ZERO_HERE 0
#if defined(TARGET_CPU_X86) && defined(QT_IA32__VBR_BROKEN)
  #undef _SHOULD_BE_ZERO_HERE
  #define _SHOULD_BE_ZERO_HERE 1
#endif


class CASpeexDecoder:
public XCACodec
{
 public:
    CASpeexDecoder(Boolean inSkipFormatsInitialization = false);
    virtual ~CASpeexDecoder();

    virtual void        Initialize(const AudioStreamBasicDescription* inInputFormat,
                                   const AudioStreamBasicDescription* inOutputFormat,
                                   const void* inMagicCookie, UInt32 inMagicCookieByteSize);
    virtual void        Uninitialize();
    virtual void        Reset();

    virtual void        GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
    virtual void        GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable);

    virtual void        SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat);
    virtual void        SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat);
    virtual UInt32      GetVersion() const;

    virtual UInt32      GetMagicCookieByteSize() const;
    virtual void        GetMagicCookie(void* outMagicCookieData, UInt32& ioMagicCookieDataByteSize) const;
    virtual void        SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);

 protected:
    virtual void        BDCInitialize(UInt32 inInputBufferByteSize);
    virtual void        BDCUninitialize();
    virtual void        BDCReset();
    virtual void        BDCReallocate(UInt32 inInputBufferByteSize);

    virtual void        InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription);

    virtual UInt32      FramesReady() const;
    virtual Boolean     GenerateFrames();
    virtual void        OutputFrames(void* outOutputData, UInt32 inNumberFrames, UInt32 inFramesOffset,
                                     AudioStreamPacketDescription* outPacketDescription) const;
    virtual void        Zap(UInt32 inFrames);

    void                SetCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
    virtual void        InitializeCompressionSettings();

    //virtual void        FixFormats();

 protected:
    Byte* mCookie;
    UInt32 mCookieSize;

    Boolean mCompressionInitialized;

    Byte*  mOutBuffer;
    UInt32 mOutBufferSize;
    UInt32 mOutBufferUsedSize;
    UInt32 mOutBufferStart;

    struct SpeexFramePacket {
        SInt32 frames;
        UInt32 bytes;
        UInt32 left;

        SpeexFramePacket() : frames(0), bytes(0), left(0) {};
        SpeexFramePacket(SInt32 inFrames, UInt32 inBytes) : frames(inFrames), bytes(inBytes), left(inBytes) {};
    };

    typedef std::vector<SpeexFramePacket>   SpeexFramePacketList;
    SpeexFramePacketList mSpeexFPList;

    UInt32 mNumFrames;

    SpeexHeader mSpeexHeader;
    SpeexBits mSpeexBits;
    void *mSpeexDecoderState;
    SpeexStereoState mSpeexStereoState;


    enum {
        kSpeexBytesPerPacket = 0,
        kSpeexFramesPerPacket = _SHOULD_BE_ZERO_HERE,
        kSpeexBytesPerFrame = 0,
        kSpeexChannelsPerFrame = 0,
        kSpeexBitsPerChannel = 0,
        kSpeexFormatFlags = 0,

        kSpeexDecoderInBufferSize = 64 * 1024,
        kSpeexDecoderOutBufferSize = 64 * 1024
    };
};


#endif /* __CASpeexDecoder_h__ */
