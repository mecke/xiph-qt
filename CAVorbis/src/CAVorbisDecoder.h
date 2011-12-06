/*
 *  CAVorbisDecoder.h
 *
 *    CAVorbisDecoder class definition.
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
 *  Last modified: $Id: CAVorbisDecoder.h 12093 2006-11-12 14:44:51Z arek $
 *
 */


#if !defined(__CAVorbisDecoder_h__)
#define __CAVorbisDecoder_h__


#include "XCACodec.h"

#include <Ogg/ogg.h>
#include <Vorbis/codec.h>

#include <vector>


#define _SHOULD_BE_ZERO_HERE 0
#if defined(TARGET_CPU_X86) && defined(QT_IA32__VBR_BROKEN)
  #undef _SHOULD_BE_ZERO_HERE
  #define _SHOULD_BE_ZERO_HERE 1
#endif


class CAVorbisDecoder:
public XCACodec
{
 public:
    CAVorbisDecoder(Boolean inSkipFormatsInitialization = false);
    ~CAVorbisDecoder();

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

    virtual UInt32      InPacketsConsumed() const;

    void                SetCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
    virtual void        InitializeCompressionSettings();

    virtual void        FixFormats();

 protected:
    Byte* mCookie;
    UInt32 mCookieSize;

    Boolean mCompressionInitialized;

    vorbis_info mV_vi;
    vorbis_dsp_state mV_vd;
    vorbis_block mV_vb;

    struct VorbisFramePacket {
        UInt32 frames;
        UInt32 bytes;
        UInt32 left;

        VorbisFramePacket() : frames(0), bytes(0), left(0) {};
        VorbisFramePacket(UInt32 inFrames, UInt32 inBytes) : frames(inFrames), bytes(inBytes), left(inBytes) {};
    };

    typedef std::vector<VorbisFramePacket>	VorbisFramePacketList;
    VorbisFramePacketList mVorbisFPList;
    VorbisFramePacketList mConsumedFPList;

    UInt32 mFullInPacketsZapped;


    enum {
        kVorbisBytesPerPacket = 0,
        kVorbisFramesPerPacket = _SHOULD_BE_ZERO_HERE,
        kVorbisFramesPerPacketReported = 8192,
        kVorbisBytesPerFrame = 0,
        kVorbisChannelsPerFrame = 0,
        //kVorbisBitsPerChannel = 16,
        kVorbisBitsPerChannel = 0,
        kVorbisFormatFlags = 0,

        /* Just a funny number, and only roughly valid for the 'Xiph (Ogg-Framed) Vorbis'. */
        kVorbisFormatMaxBytesPerPacket = 255 * 255,

        kVorbisDecoderBufferSize = 64 * 1024
    };
};

#endif /* __CAVorbisDecoder_h__ */
