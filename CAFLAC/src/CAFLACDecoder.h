/*
 *  CAFLACDecoder.h
 *
 *    CAFLACDecoder class definition.
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
 *  Last modified: $Id: CAFLACDecoder.h 12346 2007-01-18 13:45:42Z arek $
 *
 */


#if !defined(__CAFLACDecoder_h__)
#define __CAFLACDecoder_h__


#include "XCACodec.h"

//#include <Ogg/ogg.h>
#include <FLAC++/decoder.h>

#include <vector>


#define _SHOULD_BE_ZERO_HERE 0
#if defined(TARGET_CPU_X86) && defined(QT_IA32__VBR_BROKEN)
  #undef _SHOULD_BE_ZERO_HERE
  #define _SHOULD_BE_ZERO_HERE 1
#endif


class CAFLACDecoder:
public XCACodec, FLAC::Decoder::Stream
{
public:
    CAFLACDecoder(Boolean inSkipFormatsInitialization = false);
    ~CAFLACDecoder();

    virtual void        Initialize(const AudioStreamBasicDescription* inInputFormat, \
                                   const AudioStreamBasicDescription* inOutputFormat, \
                                   const void* inMagicCookie, UInt32 inMagicCookieByteSize);
    virtual void        Uninitialize();
    virtual void        Reset();
    //virtual void        FixFormats();

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

//    virtual UInt32      InPacketsConsumed() const;

    void                SetCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
    virtual void        InitializeCompressionSettings();

protected:
    /* FLAC callback interface functions */

    virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);
    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);
    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status);

 protected:
    Byte* mCookie;
    UInt32 mCookieSize;

    Boolean mCompressionInitialized;

    FLAC__int32* mOutBuffer;
    UInt32       mOutBufferSize;
    UInt32       mOutBufferUsedSize;

    UInt32 mFLACsrate;
    UInt32 mFLACchannels;
    UInt32 mFLACbits;

    struct FLACFramePacket {
        UInt32 frames;
        UInt32 bytes;
        UInt32 left;

        FLACFramePacket() : frames(0), bytes(0), left(0) {};
        FLACFramePacket(UInt32 inFrames, UInt32 inBytes) : frames(inFrames), bytes(inBytes), left(inBytes) {};
    };

    typedef std::vector<FLACFramePacket> FLACFramePacketList;
    FLACFramePacketList mFLACFPList;

    ::FLAC__Frame mFrame;
    UInt32 mNumFrames;
    const FLAC__int32** mBPtrs;

    void DFPinit(const ::FLAC__Frame& inFrame, const FLAC__int32 *const inBuffer[]);
    void DFPclear();


    enum {
        kFLACBytesPerPacket = 0,
        kFLACFramesPerPacket = _SHOULD_BE_ZERO_HERE,
        kFLACFramesPerPacketReported = 8192,
        kFLACBytesPerFrame = 0,
        kFLACChannelsPerFrame = 0,
        kFLACBitsPerChannel = 0,
        kFLACFormatFlags = 0,

        kFLACFormatMaxBytesPerPacket = 65535,

        kFLACDecoderInBufferSize = 96 * 1024,
        kFLACDecoderOutBufferSize = 32 * 1024
    };
};


#endif /* __CAFLACDecoder_h__ */
