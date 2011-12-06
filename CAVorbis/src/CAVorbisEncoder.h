/*
 *  CAVorbisEncoder.h
 *
 *    CAVorbisEncoder class definition.
 *
 *
 *  Copyright (c) 2006  Arek Korbik
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
 *  Last modified: $Id: CAVorbisEncoder.h 12394 2007-01-30 01:21:22Z arek $
 *
 */


#if !defined(__CAVorbisEncoder_h__)
#define __CAVorbisEncoder_h__


#include "XCACodec.h"

#include <Ogg/ogg.h>
#include <Vorbis/codec.h>

#include <vector>


class CAVorbisEncoder:
public XCACodec
{
 public:
    CAVorbisEncoder();
    virtual ~CAVorbisEncoder();

    virtual void        Initialize(const AudioStreamBasicDescription* inInputFormat,
                                   const AudioStreamBasicDescription* inOutputFormat,
                                   const void* inMagicCookie, UInt32 inMagicCookieByteSize);
    virtual void        Uninitialize();
    virtual void        Reset();

    virtual UInt32      ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                             AudioStreamPacketDescription* outPacketDescription);

    virtual void        GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
    virtual void        GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable);
    virtual void        SetProperty(AudioCodecPropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData);

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

    virtual UInt32      FramesReady() const { return 0; };
    virtual Boolean     GenerateFrames() { return false; };
    virtual void        OutputFrames(void* outOutputData, UInt32 inNumberFrames, UInt32 inFramesOffset,
                                     AudioStreamPacketDescription* outPacketDescription) const {};
    virtual void        Zap(UInt32 inFrames) {};

    virtual UInt32      InPacketsConsumed() const { return 0; };

    void                SetCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
    virtual void        InitializeCompressionSettings();

    virtual void        FixFormats();

    SInt32              BitrateMin();
    SInt32              BitrateMax();
    SInt32              BitrateMid();

    /* bitrate-series generator and inverse generator functions */
    static UInt32       brn(UInt32 n, UInt32 bs) { return (4 + n % 4) << ((n >> 2) + bs); };
    static UInt32       ibrn(UInt32 br, UInt32 bs);

    static void         fill_channel_layout(UInt32 nch, AudioChannelLayout *acl);

 private:
    Boolean             BuildSettings(void *outSettingsDict);
    Boolean             ApplySettings(CFDictionaryRef sd);

 protected:
    Byte* mCookie;
    UInt32 mCookieSize;

    Boolean mCompressionInitialized;
    Boolean mEOSHit;

    vorbis_info mV_vi;
    vorbis_comment mV_vc;
    vorbis_dsp_state mV_vd;
    vorbis_block mV_vb;

    ogg_int64_t last_granulepos;
    ogg_int64_t last_packetno; // do I need this one?

    struct VorbisFramePacket {
        UInt32 frames;
        UInt32 bytes;
        UInt32 left;

        VorbisFramePacket() : frames(0), bytes(0), left(0) {};
        VorbisFramePacket(UInt32 inFrames, UInt32 inBytes) : frames(inFrames), bytes(inBytes), left(inBytes) {};
    };

    typedef std::vector<VorbisFramePacket>	VorbisFramePacketList;
    VorbisFramePacketList mVorbisFPList;
    typedef std::vector<ogg_packet> VorbisPacketList;
    VorbisPacketList mProducedPList;

    //UInt32 mFullInPacketsZapped;


    enum {
        kVorbisBytesPerPacket = 0,
        kVorbisFramesPerPacket = 0,
        kVorbisFramesPerPacketReported = 8192,
        kVorbisBytesPerFrame = 0,
        kVorbisChannelsPerFrame = 0,
        kVorbisBitsPerChannel = 0,
        kVorbisFormatFlags = 0,

        /* Just a funny number, and only roughly valid for the 'Xiph (Ogg-Framed) Vorbis'. */
        kVorbisFormatMaxBytesPerPacket = 255 * 255,

        kVorbisEncoderBufferSize = 256 * 1024,

        kVorbisEncoderOutChannelLayouts = 5,

        kVorbisEncoderBitrateSeriesLength = 23,
        kVorbisEncoderBitrateSeriesBase = 1,
    };

    static AudioChannelLayoutTag gOutChannelLayouts[kVorbisEncoderOutChannelLayouts];

    /* settings */
    enum VorbisEncoderMode {
        kVorbisEncoderModeQuality = 0,
        kVorbisEncoderModeAverage,
        kVorbisEncoderModeQualityByBitrate,
    };
    VorbisEncoderMode mCfgMode;
    float mCfgQuality; // [-0.1; 1.0] quality range
    SInt32 mCfgBitrate; // in kbits

    CFMutableDictionaryRef mCfgDict;
};

#endif /* __CAVorbisEncoder_h__ */
