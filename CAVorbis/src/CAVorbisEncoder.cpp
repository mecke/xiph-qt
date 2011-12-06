/*
 *  CAVorbisEncoder.cpp
 *
 *    CAVorbisEncoder class implementation; the main part of the Vorbis
 *    encoding functionality.
 *
 *
 *  Copyright (c) 2006-2007  Arek Korbik
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
 *  Last modified: $Id: CAVorbisEncoder.cpp 12399 2007-01-30 21:13:19Z arek $
 *
 */


#include <Ogg/ogg.h>
#include <Vorbis/vorbisenc.h>

#include "CAVorbisEncoder.h"

#include "CABundleLocker.h"

#include "vorbis_versions.h"
#include "fccs.h"
#include "data_types.h"

//#define NDEBUG
#include "debug.h"


#if !defined(NO_ADV_PROPS)
#define NO_ADV_PROPS 1
#endif /* defined(NO_ADV_PROPS) */

#define DBG_STREAMDESC_FMT " [CASBD: sr=%lf, fmt=%4.4s, fl=%lx, bpp=%ld, fpp=%ld, bpf=%ld, ch=%ld, bpc=%ld]"
#define DBG_STREAMDESC_FILL(x) (x)->mSampleRate, reinterpret_cast<const char*> (&((x)->mFormatID)), \
        (x)->mFormatFlags, (x)->mBytesPerPacket, (x)->mFramesPerPacket, (x)->mBytesPerPacket, \
        (x)->mChannelsPerFrame, (x)->mBitsPerChannel

AudioChannelLayoutTag CAVorbisEncoder::gOutChannelLayouts[kVorbisEncoderOutChannelLayouts] = {
    kAudioChannelLayoutTag_Mono,
    kAudioChannelLayoutTag_Stereo,
    //kAudioChannelLayoutTag_ITU_2_1, // there's no tag for [L, C, R] layout!! :(
    kAudioChannelLayoutTag_Quadraphonic,
    kAudioChannelLayoutTag_MPEG_5_0_C,
    kAudioChannelLayoutTag_MPEG_5_1_C,
};

CAVorbisEncoder::CAVorbisEncoder() :
    mCookie(NULL), mCookieSize(0), mCompressionInitialized(false), mEOSHit(false),
    last_granulepos(0), last_packetno(0), mVorbisFPList(), mProducedPList(),
    mCfgMode(kVorbisEncoderModeQuality), mCfgQuality(0.4), mCfgBitrate(128), mCfgDict(NULL)
{
    CAStreamBasicDescription theInputFormat1(kAudioStreamAnyRate, kAudioFormatLinearPCM,
                                             0, 1, 0, 0, 32, kAudioFormatFlagsNativeFloatPacked);
    AddInputFormat(theInputFormat1);

    CAStreamBasicDescription theInputFormat2(kAudioStreamAnyRate, kAudioFormatLinearPCM,
                                             0, 1, 0, 0, 16,
                                             kAudioFormatFlagsNativeEndian |
                                             kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
    AddInputFormat(theInputFormat2);

    mInputFormat.mSampleRate = 44100.0;
    mInputFormat.mFormatID = kAudioFormatLinearPCM;
    mInputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    mInputFormat.mBytesPerPacket = 8;
    mInputFormat.mFramesPerPacket = 1;
    mInputFormat.mBytesPerFrame = 8;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = 32;

    CAStreamBasicDescription theOutputFormat(kAudioStreamAnyRate, kAudioFormatXiphVorbis,
                                             kVorbisBytesPerPacket, kVorbisFramesPerPacket,
                                             kVorbisBytesPerFrame, kVorbisChannelsPerFrame,
                                             kVorbisBitsPerChannel, kVorbisFormatFlags);
    AddOutputFormat(theOutputFormat);

    mOutputFormat.mSampleRate = 44100.0;
    mOutputFormat.mFormatID = kAudioFormatXiphVorbis;
    mOutputFormat.mFormatFlags = kVorbisFormatFlags;
    mOutputFormat.mBytesPerPacket = kVorbisBytesPerPacket;
    mOutputFormat.mFramesPerPacket = kVorbisFramesPerPacket;
    mOutputFormat.mBytesPerFrame = kVorbisBytesPerFrame;
    mOutputFormat.mChannelsPerFrame = 2;
    mOutputFormat.mBitsPerChannel = kVorbisBitsPerChannel;
}

CAVorbisEncoder::~CAVorbisEncoder()
{
    if (mCookie != NULL)
        delete[] mCookie;

    if (mCompressionInitialized) {
        vorbis_block_clear(&mV_vb);
        vorbis_dsp_clear(&mV_vd);

        vorbis_comment_clear(&mV_vc);

        vorbis_info_clear(&mV_vi);
    }

    if (mCfgDict != NULL)
        CFRelease(mCfgDict);
}

void CAVorbisEncoder::Initialize(const AudioStreamBasicDescription* inInputFormat,
                                 const AudioStreamBasicDescription* inOutputFormat,
                                 const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
    dbg_printf("[  VE]  >> [%08lx] :: Initialize(%d, %d, %d)\n", (UInt32) this, inInputFormat != NULL, inOutputFormat != NULL, inMagicCookieByteSize != 0);
    if (inInputFormat)
        dbg_printf("[  VE]   > [%08lx] :: InputFormat :" DBG_STREAMDESC_FMT "\n", (UInt32) this, DBG_STREAMDESC_FILL(inInputFormat));
    if (inOutputFormat)
        dbg_printf("[  VE]   > [%08lx] :: OutputFormat:" DBG_STREAMDESC_FMT "\n", (UInt32) this, DBG_STREAMDESC_FILL(inOutputFormat));

    if(inInputFormat != NULL) {
        SetCurrentInputFormat(*inInputFormat);
    }

    if(inOutputFormat != NULL) {
        SetCurrentOutputFormat(*inOutputFormat);
    }

    if ((mInputFormat.mSampleRate != mOutputFormat.mSampleRate) ||
        (mInputFormat.mChannelsPerFrame != mOutputFormat.mChannelsPerFrame) ||
        (mInputFormat.mSampleRate == 0.0)) {
        CODEC_THROW(kAudioCodecUnsupportedFormatError);
    }

    BDCInitialize(kVorbisEncoderBufferSize);

    InitializeCompressionSettings();

    FixFormats();

    XCACodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);

    dbg_printf("[  VE]  <  [%08lx] :: InputFormat :" DBG_STREAMDESC_FMT "\n", (UInt32) this, DBG_STREAMDESC_FILL(&mInputFormat));
    dbg_printf("[  VE]  <  [%08lx] :: OutputFormat:" DBG_STREAMDESC_FMT "\n", (UInt32) this, DBG_STREAMDESC_FILL(&mOutputFormat));
    dbg_printf("[  VE] <.. [%08lx] :: Initialize(%d, %d, %d)\n", (UInt32) this, inInputFormat != NULL, inOutputFormat != NULL, inMagicCookieByteSize != 0);
}

void CAVorbisEncoder::Uninitialize()
{
    dbg_printf("[  VE]  >> [%08lx] :: Uninitialize()\n", (UInt32) this);
    BDCUninitialize();

    XCACodec::Uninitialize();
    dbg_printf("[  VE] <.. [%08lx] :: Uninitialize()\n", (UInt32) this);
}

/*
 * Property selectors TODO (?):
 *  'gcsc' - ?
 *  'acps' - ?
 */


void CAVorbisEncoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
    dbg_printf("[  VE]  >> [%08lx] :: GetProperty('%4.4s') (%d)\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID), inPropertyID == kAudioCodecPropertyFormatCFString);
    switch(inPropertyID)
    {

        /*
    case kAudioCodecPropertyOutputChannelLayout :
        // by default a codec doesn't support channel layouts.
        CODEC_THROW(kAudioCodecIllegalOperationError);
        break;
    case kAudioCodecPropertyAvailableInputChannelLayouts :
	*/

    case kAudioCodecPropertyInputChannelLayout:
        if (ioPropertyDataSize >= sizeof(AudioChannelLayout)) {
            fill_channel_layout(mInputFormat.mChannelsPerFrame, reinterpret_cast<AudioChannelLayout*>(outPropertyData));
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyOutputChannelLayout:
        if (ioPropertyDataSize >= sizeof(AudioChannelLayout)) {
            fill_channel_layout(mOutputFormat.mChannelsPerFrame, reinterpret_cast<AudioChannelLayout*>(outPropertyData));
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyAvailableOutputChannelLayouts:
        if (ioPropertyDataSize == sizeof(AudioChannelLayoutTag) * kVorbisEncoderOutChannelLayouts) {
            for (int i = 0; i < kVorbisEncoderOutChannelLayouts; i++) {
                (reinterpret_cast<AudioChannelLayoutTag*>(outPropertyData))[i] = gOutChannelLayouts[i];
            }
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyAvailableNumberChannels:
        if(ioPropertyDataSize == sizeof(UInt32) * 1) {
            (reinterpret_cast<UInt32*>(outPropertyData))[0] = 0xffffffff;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyAvailableBitRateRange:
        if (ioPropertyDataSize == sizeof(AudioValueRange) * 1) {
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMinimum = 8.0;
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMaximum = 384.0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyPrimeMethod:
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = (UInt32)kAudioCodecPrimeMethod_None;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyPrimeInfo:
        if(ioPropertyDataSize == sizeof(AudioCodecPrimeInfo) ) {
            (reinterpret_cast<AudioCodecPrimeInfo*>(outPropertyData))->leadingFrames = 0;
            (reinterpret_cast<AudioCodecPrimeInfo*>(outPropertyData))->trailingFrames = 0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

#if !defined(NO_ADV_PROPS) || NO_ADV_PROPS == 0
    case kAudioCodecPropertyApplicableInputSampleRates:
        if (ioPropertyDataSize == sizeof(AudioValueRange)) {
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMinimum = 0.0;
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMaximum = 0.0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyApplicableOutputSampleRates:
        if (ioPropertyDataSize == sizeof(AudioValueRange)) {
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMinimum = 0.0;
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMaximum = 0.0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;
#endif

    case kAudioCodecPropertyAvailableInputSampleRates:
        if (ioPropertyDataSize == sizeof(AudioValueRange)) {
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMinimum = 0.0;
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMaximum = 200000.0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyAvailableOutputSampleRates:
        if (ioPropertyDataSize == sizeof(AudioValueRange)) {
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMinimum = 0.0;
            (reinterpret_cast<AudioValueRange*>(outPropertyData))->mMaximum = 200000.0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertySettings:
        if (ioPropertyDataSize == sizeof(CFDictionaryRef)) {
            
            if (!BuildSettings(outPropertyData))
                CODEC_THROW(kAudioCodecUnspecifiedError);
            
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

        
    case kAudioCodecPropertyRequiresPacketDescription:
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = 1;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyHasVariablePacketByteSizes:
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = 1;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyPacketFrameSize:
        if(ioPropertyDataSize == sizeof(UInt32))
        {
            if (mIsInitialized) {
                long long_blocksize = (reinterpret_cast<long *>(mV_vi.codec_setup))[1];
                *reinterpret_cast<UInt32*>(outPropertyData) = long_blocksize;
                //*reinterpret_cast<UInt32*>(outPropertyData) = 0;
            } else {
                *reinterpret_cast<UInt32*>(outPropertyData) = 0;
            }
            dbg_printf("[  VE]   . [%08lx] :: GetProperty('%4.4s') = %ld\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID),
                       *reinterpret_cast<UInt32*>(outPropertyData));
        }
        else
        {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyMaximumPacketByteSize:
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = mCompressionInitialized ? kVorbisFormatMaxBytesPerPacket : 0;
            //*reinterpret_cast<UInt32*>(outPropertyData) = mCompressionInitialized ? (64 * 1024) : 0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyCurrentInputSampleRate:
        if (ioPropertyDataSize == sizeof(Float64)) {
            *reinterpret_cast<Float64*>(outPropertyData) = mInputFormat.mSampleRate;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyCurrentOutputSampleRate:
        if (ioPropertyDataSize == sizeof(Float64)) {
            *reinterpret_cast<Float64*>(outPropertyData) = mOutputFormat.mSampleRate;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecDoesSampleRateConversion:
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = 0;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    case kAudioCodecPropertyQualitySetting:
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = kAudioCodecQuality_Max;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

#if TARGET_OS_MAC || 1
    case kAudioCodecPropertyFormatCFString:
        {
            if (ioPropertyDataSize != sizeof(CFStringRef))
                CODEC_THROW(kAudioCodecBadPropertySizeError);
            CABundleLocker lock;
            CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Xiph Vorbis"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
            *(CFStringRef*)outPropertyData = name;
            break;
        }

    case kAudioCodecPropertyNameCFString:
        {
            if (ioPropertyDataSize != sizeof(CFStringRef)) CODEC_THROW(kAudioCodecBadPropertySizeError);
            CABundleLocker lock;
            CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Xiph Vorbis encoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
            *(CFStringRef*)outPropertyData = name;
            break;
        }

    case kAudioCodecPropertyManufacturerCFString:
        {
            if (ioPropertyDataSize != sizeof(CFStringRef)) CODEC_THROW(kAudioCodecBadPropertySizeError);
            CABundleLocker lock;
            CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Xiph.Org Foundation"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
            *(CFStringRef*)outPropertyData = name;
            break;
        }
#endif

    default:
        ACBaseCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
    }
    dbg_printf("[  VE] <.. [%08lx] :: GetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CAVorbisEncoder::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable)
{
    dbg_printf("[  VE]  >> [%08lx] :: GetPropertyInfo('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
    switch(inPropertyID)
    {
        /*
    case kAudioCodecPropertyAvailableInputChannelLayouts :
        // by default a codec doesn't support channel layouts.
        CODEC_THROW(kAudioCodecIllegalOperationError);
        break;
        */

    case kAudioCodecPropertyOutputChannelLayout:
    case kAudioCodecPropertyInputChannelLayout:
        outPropertyDataSize = sizeof(AudioChannelLayout);
        outWritable = false;
        break;

    case kAudioCodecPropertyAvailableOutputChannelLayouts:
        outPropertyDataSize = sizeof(AudioChannelLayoutTag) * kVorbisEncoderOutChannelLayouts;
        outWritable = false;
        break;

    case kAudioCodecPropertyAvailableNumberChannels:
        outPropertyDataSize = sizeof(UInt32) * 1; // [0xffffffff - any number]
        outWritable = false;
        break;

    case kAudioCodecPropertyAvailableBitRateRange:
        outPropertyDataSize = sizeof(AudioValueRange) * 1;
        outWritable = false;
        break;

#if !defined(NO_ADV_PROPS) || NO_ADV_PROPS == 0
    case kAudioCodecPropertyApplicableInputSampleRates:
    case kAudioCodecPropertyApplicableOutputSampleRates:
#endif

    case kAudioCodecPropertyAvailableInputSampleRates:
        outPropertyDataSize = sizeof(AudioValueRange);
        outWritable = false;
        break;

    case kAudioCodecPropertyAvailableOutputSampleRates:
        outPropertyDataSize = sizeof(AudioValueRange);
        outWritable = false;
        break;

    case kAudioCodecPropertySettings:
        outPropertyDataSize = sizeof(CFDictionaryRef);
        outWritable = true;
        break;

        
    case kAudioCodecPropertyRequiresPacketDescription:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = false;
        break;

    case kAudioCodecPropertyHasVariablePacketByteSizes:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = false;
        break;

    case kAudioCodecPropertyPacketFrameSize:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = false;
        break;

    case kAudioCodecPropertyMaximumPacketByteSize:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = false;
        break;

    case kAudioCodecPropertyCurrentInputSampleRate:
    case kAudioCodecPropertyCurrentOutputSampleRate:
        outPropertyDataSize = sizeof(Float64);
        outWritable = false;
        break;

    case kAudioCodecDoesSampleRateConversion:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = false;
        break;

    case kAudioCodecPropertyQualitySetting:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = true;
        break;

    default:
        ACBaseCodec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
        break;

    }
    dbg_printf("[  VE] <.. [%08lx] :: GetPropertyInfo('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CAVorbisEncoder::SetProperty(AudioCodecPropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
    dbg_printf("[  VE]  >> [%08lx] :: SetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));

    switch(inPropertyID) {
    case kAudioCodecPropertySettings:
        if (inPropertyDataSize == sizeof(CFDictionaryRef)) {
            if (inPropertyData != NULL)
                ApplySettings(*reinterpret_cast<const CFDictionaryRef*>(inPropertyData));
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

    default:
        ACBaseCodec::SetProperty(inPropertyID, inPropertyDataSize, inPropertyData);
        break;
    }
    dbg_printf("[  VE] <.. [%08lx] :: SetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CAVorbisEncoder::Reset()
{
    dbg_printf("[  VE] > > [%08lx] :: Reset()\n", (UInt32) this);
    BDCReset();

    XCACodec::Reset();
    dbg_printf("[  VE] < < [%08lx] :: Reset()\n", (UInt32) this);
}

UInt32 CAVorbisEncoder::GetVersion() const
{
    return kCAVorbis_aenc_Version;
}


void CAVorbisEncoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
    if (!mIsInitialized) {
        //	check to make sure the input format is legal
        if ((inInputFormat.mFormatID != kAudioFormatLinearPCM) ||
            (inInputFormat.mSampleRate > 200000.0) ||
            !(((inInputFormat.mFormatFlags == kAudioFormatFlagsNativeFloatPacked) &&
               (inInputFormat.mBitsPerChannel == 32)) ||
              ((inInputFormat.mFormatFlags == (kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked)) &&
               (inInputFormat.mBitsPerChannel == 16))))
        {
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentInputFormat(inInputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

void CAVorbisEncoder::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
    if (!mIsInitialized) {
        //	check to make sure the output format is legal
        if (inOutputFormat.mFormatID != kAudioFormatXiphVorbis ||
            inOutputFormat.mSampleRate > 200000.0) {
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentOutputFormat(inOutputFormat);

        // adjust the default config parameters
        mCfgBitrate = BitrateMid();
        if (mOutputFormat.mSampleRate < 8000.0 || mOutputFormat.mSampleRate > 50000.0)
            mCfgMode = kVorbisEncoderModeQuality;
        dbg_printf("[  VE]  of [%08lx] :: InitializeCompressionSettings() = br:%ld, m:%d\n", (UInt32) this, mCfgBitrate, mCfgMode);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

UInt32 CAVorbisEncoder::GetMagicCookieByteSize() const
{
    return mCookieSize;
}

void CAVorbisEncoder::GetMagicCookie(void* outMagicCookieData, UInt32& ioMagicCookieDataByteSize) const
{
    if (mCookie != NULL) {
        ioMagicCookieDataByteSize = mCookieSize;
        BlockMoveData(mCookie, outMagicCookieData, ioMagicCookieDataByteSize);
    } else {
        ioMagicCookieDataByteSize = 0;
    }
}

void CAVorbisEncoder::SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
    dbg_printf("[  VE]  >> [%08lx] :: SetMagicCookie()\n", (UInt32) this);
    CODEC_THROW(kAudioCodecIllegalOperationError);
    dbg_printf("[  VE] <.. [%08lx] :: SetMagicCookie()\n", (UInt32) this);
}

void CAVorbisEncoder::SetCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
#if 0
    if (mCookie != NULL)
        delete[] mCookie;

    mCookieSize = inMagicCookieDataByteSize;
    if (inMagicCookieDataByteSize != 0) {
        mCookie = new Byte[inMagicCookieDataByteSize];
        BlockMoveData(inMagicCookieData, mCookie, inMagicCookieDataByteSize);
    } else {
        mCookie = NULL;
    }
#endif /* 0 */
}



void CAVorbisEncoder::FixFormats()
{
    dbg_printf("[  VE]  >> [%08lx] :: FixFormats()\n", (UInt32) this);
    mOutputFormat.mSampleRate = mInputFormat.mSampleRate;
    mOutputFormat.mBitsPerChannel = 0;
    mOutputFormat.mBytesPerPacket = 0;
    mOutputFormat.mFramesPerPacket = 0;

    //long long_blocksize = (reinterpret_cast<long *>(mV_vi.codec_setup))[1];
    //mOutputFormat.mFramesPerPacket = long_blocksize;

    dbg_printf("[  VE] <.. [%08lx] :: FixFormats()\n", (UInt32) this);
}

void CAVorbisEncoder::InitializeCompressionSettings()
{
    int ret = 0;

    if (mCompressionInitialized) {
        vorbis_block_clear(&mV_vb);
        vorbis_dsp_clear(&mV_vd);

        vorbis_info_clear(&mV_vi);

        if (mCookie != NULL) {
            delete[] mCookie;
            mCookieSize = 0;
            mCookie = NULL;
        }
    }

    mCompressionInitialized = false;

    vorbis_info_init(&mV_vi);

    UInt32 sample_rate = lround(mOutputFormat.mSampleRate);

    if (mCfgMode == kVorbisEncoderModeQuality) {
        ret = vorbis_encode_init_vbr(&mV_vi, mOutputFormat.mChannelsPerFrame, sample_rate, mCfgQuality);
    } else {
        UInt32 total_bitrate = mCfgBitrate * 1000;
        if (mOutputFormat.mChannelsPerFrame > 2)
            total_bitrate *= mOutputFormat.mChannelsPerFrame;

        dbg_printf("[  VE]  .? [%08lx] :: InitializeCompressionSettings() = br:%ld\n", (UInt32) this, total_bitrate);

        if (mCfgMode == kVorbisEncoderModeAverage) {
            ret = vorbis_encode_init(&mV_vi, mOutputFormat.mChannelsPerFrame, sample_rate, -1, total_bitrate, -1);
        } else if (mCfgMode == kVorbisEncoderModeQualityByBitrate) {
            ret = vorbis_encode_setup_managed(&mV_vi, mOutputFormat.mChannelsPerFrame, sample_rate, -1, total_bitrate, -1);
            if (!ret)
                ret = vorbis_encode_ctl(&mV_vi, OV_ECTL_RATEMANAGE2_SET, NULL);
            if (!ret)
                ret = vorbis_encode_setup_init(&mV_vi);
        }
        if (ret)
            mCfgBitrate = 128;
    }

    if (ret) {
        vorbis_info_clear(&mV_vi);
        dbg_printf("[  VE] <!! [%08lx] :: InitializeCompressionSettings() = %ld\n", (UInt32) this, ret);
        CODEC_THROW(kAudioCodecUnspecifiedError);
    }

    vorbis_comment_init(&mV_vc);
    vorbis_comment_add_tag(&mV_vc, "ENCODER", "XiphQT, CAVorbisEncoder.cpp $Rev: 12399 $");

    vorbis_analysis_init(&mV_vd, &mV_vi);
    vorbis_block_init(&mV_vd, &mV_vb);


    {
        ogg_packet header, header_vc, header_cb;
        vorbis_analysis_headerout(&mV_vd, &mV_vc, &header, &header_vc, &header_cb);

        mCookieSize = header.bytes + header_vc.bytes + header_cb.bytes + 4 * 2 * sizeof(UInt32);
        mCookie = new Byte[mCookieSize];

        unsigned long *qtatom = reinterpret_cast<unsigned long*>(mCookie); // reinterpret_cast ?!?
        *qtatom++ = EndianU32_NtoB(header.bytes + 2 * sizeof(UInt32));
        *qtatom++ = EndianU32_NtoB(kCookieTypeVorbisHeader);
        BlockMoveData(header.packet, qtatom, header.bytes);

        qtatom = reinterpret_cast<unsigned long*>(mCookie + 2 * sizeof(UInt32) + header.bytes);
        *qtatom++ = EndianU32_NtoB(header_vc.bytes + 2 * sizeof(UInt32));
        *qtatom++ = EndianU32_NtoB(kCookieTypeVorbisComments);
        BlockMoveData(header_vc.packet, qtatom, header_vc.bytes);

        qtatom = reinterpret_cast<unsigned long*>(mCookie + 4 * sizeof(UInt32) + header.bytes + header_vc.bytes);
        *qtatom++ = EndianU32_NtoB(header_cb.bytes + 2 * sizeof(UInt32));
        *qtatom++ = EndianU32_NtoB(kCookieTypeVorbisCodebooks);
        BlockMoveData(header_cb.packet, qtatom, header_cb.bytes);

        qtatom = reinterpret_cast<unsigned long*>(mCookie + 6 * sizeof(UInt32) + header.bytes + header_vc.bytes + header_cb.bytes);
        *qtatom++ = EndianU32_NtoB(2 * sizeof(UInt32));
        *qtatom++ = EndianU32_NtoB(kAudioTerminatorAtomType);
    }

    dbg_printf("[  VE] < > [%08lx] :: InitializeCompressionSettings() - bru: %ld, brn: %ld, brl: %ld, brw: %ld\n",
               (UInt32) this, mV_vi.bitrate_upper, mV_vi.bitrate_nominal, mV_vi.bitrate_lower, mV_vi.bitrate_window);
    mCompressionInitialized = true;
}

#pragma mark BDC handling

void CAVorbisEncoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    XCACodec::BDCInitialize(inInputBufferByteSize);
}

void CAVorbisEncoder::BDCUninitialize()
{
    mVorbisFPList.clear();
    mProducedPList.clear();

    XCACodec::BDCUninitialize();
}

void CAVorbisEncoder::BDCReset()
{
    mVorbisFPList.clear();
    mProducedPList.clear();

    XCACodec::BDCReset();
}

void CAVorbisEncoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    mVorbisFPList.clear();
    mProducedPList.clear();

    XCACodec::BDCReallocate(inInputBufferByteSize);
}


void CAVorbisEncoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    const Byte * theData = static_cast<const Byte *> (inInputData) + inPacketDescription->mStartOffset;
    UInt32 size = inPacketDescription->mDataByteSize;
    mBDCBuffer.In(theData, size);
    mVorbisFPList.push_back(VorbisFramePacket(inPacketDescription->mVariableFramesInPacket, inPacketDescription->mDataByteSize));
}


UInt32 CAVorbisEncoder::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                             AudioStreamPacketDescription* outPacketDescription)
{
    dbg_printf("[  VE]  >> [%08lx] CAVorbisEncoder :: ProduceOutputPackets(%ld [%ld] %d)\n", (UInt32) this, ioNumberPackets, ioOutputDataByteSize, outPacketDescription != NULL);

    UInt32 theAnswer = kAudioCodecProduceOutputPacketSuccess;

    if (!mIsInitialized)
        CODEC_THROW(kAudioCodecStateError);

    UInt32 fout = 0; //frames produced
    UInt32 bout = 0; //bytes produced
    UInt32 bspace = ioOutputDataByteSize;
    ogg_packet op;

    while (fout < ioNumberPackets && !mEOSHit) {
        if (!mProducedPList.empty()) {
            ogg_packet &op_left = mProducedPList.front();
            if (op_left.bytes > bspace) {
                ioNumberPackets = fout;
                ioOutputDataByteSize = bout;
                theAnswer = kAudioCodecNotEnoughBufferSpaceError;
                dbg_printf("[  VE] <.! [%08lx] CAVorbisEncoder :: ProduceOutputPackets(%ld [%ld]) = %ld\n", (UInt32) this,
                           ioNumberPackets, ioOutputDataByteSize, theAnswer);
                return theAnswer;
            }

            BlockMoveData(op_left.packet, &((static_cast<Byte *>(outOutputData))[bout]), op_left.bytes);
            outPacketDescription[fout].mStartOffset = bout;
            outPacketDescription[fout].mVariableFramesInPacket = op_left.granulepos - last_granulepos;
            outPacketDescription[fout].mDataByteSize = op_left.bytes;
            last_granulepos = op_left.granulepos;
            fout++;
            bout += op_left.bytes;
            bspace -= op_left.bytes;
            if (op_left.e_o_s != 0)
                mEOSHit = true;
            mProducedPList.erase(mProducedPList.begin());
            continue;
        } else if (vorbis_bitrate_flushpacket(&mV_vd, &op)) {
            if (op.bytes > bspace) {
                ioNumberPackets = fout;
                ioOutputDataByteSize = bout;
                theAnswer = kAudioCodecNotEnoughBufferSpaceError;
                mProducedPList.push_back(op);
                dbg_printf("[  VE] <.! [%08lx] CAVorbisEncoder :: ProduceOutputPackets(%ld [%ld]) = %ld\n", (UInt32) this,
                           ioNumberPackets, ioOutputDataByteSize, theAnswer);
                return theAnswer;
            }

            BlockMoveData(op.packet, &((static_cast<Byte *>(outOutputData))[bout]), op.bytes);
            outPacketDescription[fout].mStartOffset = bout;
            outPacketDescription[fout].mVariableFramesInPacket = op.granulepos - last_granulepos;
            outPacketDescription[fout].mDataByteSize = op.bytes;
            last_granulepos = op.granulepos;
            fout++;
            bout += op.bytes;
            bspace -= op.bytes;
            if (op.e_o_s != 0)
                mEOSHit = true;
            continue;
        } else if (vorbis_analysis_blockout(&mV_vd, &mV_vb) == 1) {
            vorbis_analysis(&mV_vb, NULL);
            vorbis_bitrate_addblock(&mV_vb);
            continue;
        }

        // get next block
        if (mVorbisFPList.empty()) {
            if (fout == 0) {
                ioNumberPackets = fout;
                ioOutputDataByteSize = bout;
                theAnswer = kAudioCodecProduceOutputPacketNeedsMoreInputData;
                dbg_printf("[  VE] <!. [%08lx] CAVorbisEncoder :: ProduceOutputPackets(%ld [%ld]) = %ld\n", (UInt32) this,
                           ioNumberPackets, ioOutputDataByteSize, theAnswer);
                return theAnswer;
            }
            break;
        }

        VorbisFramePacket &sfp = mVorbisFPList.front();
        if (sfp.frames > 0) {
            float **buffer = vorbis_analysis_buffer(&mV_vd, sfp.frames);
            void *inData = mBDCBuffer.GetData();
            int j = 0, i = 0;

            if (mInputFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger) {
                for (j = 0; j < mOutputFormat.mChannelsPerFrame; j++) {
                    SInt16 *theInputData = static_cast<SInt16*> (inData) + j;
                    float *theOutputData = buffer[j];
                    for (i = 0; i < sfp.frames; i++) {
                        *theOutputData++ = *theInputData / 32768.0f;
                        theInputData += mInputFormat.mChannelsPerFrame;
                    }
                }
            } else {
                for (j = 0; j < mOutputFormat.mChannelsPerFrame; j++) {
                    float *theInputData = static_cast<float*> (inData) + j;
                    float *theOutputData = buffer[j];
                    for (i = 0; i < sfp.frames; i++) {
                        *theOutputData++ = *theInputData;
                        theInputData += mInputFormat.mChannelsPerFrame;
                    }
                }
            }
        }

        vorbis_analysis_wrote(&mV_vd, sfp.frames);
        mBDCBuffer.Zap(sfp.bytes);
        sfp.frames = sfp.left = 0;
        mVorbisFPList.erase(mVorbisFPList.begin());
    }

    ioOutputDataByteSize = bout; //???
    ioNumberPackets = fout;

    //theAnswer = (!mProducedPList.empty() || !BufferIsEmpty()) ? kAudioCodecProduceOutputPacketSuccessHasMore
    theAnswer = (!mProducedPList.empty() || !mVorbisFPList.empty()) ? kAudioCodecProduceOutputPacketSuccessHasMore
        : kAudioCodecProduceOutputPacketSuccess; // what about possible data inside the vorbis lib (if ..._flushpacket() returns more than 1 packet) ??

    if (mEOSHit)
        theAnswer = kAudioCodecProduceOutputPacketAtEOF;
    dbg_printf("[  VE] <.. [%08lx] CAVorbisEncoder :: ProduceOutputPackets(%ld [%ld]) = %ld\n",
               (UInt32) this, ioNumberPackets, ioOutputDataByteSize, theAnswer);
    return theAnswer;
}

/* ======================== Settings ======================= */
Boolean CAVorbisEncoder::BuildSettings(void *outSettingsDict)
{
    Boolean ret = false;
    SInt32 n, ln;
    CFNumberRef cf_n;
    CFMutableDictionaryRef sd;
    CFMutableDictionaryRef eltd;
    CFMutableArrayRef params;

    if (mCfgDict == NULL) {
        sd = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (sd == NULL)
            return ret;
        CFDictionaryAddValue(sd, CFSTR(kAudioSettings_TopLevelKey), CFSTR("Xiph Vorbis Encoder"));
    }

    params = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

    {
        eltd = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_SettingKey), CFSTR("Encoding Mode"));
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_SettingName), CFSTR("Encoding Mode"));
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Summary), CFSTR("Vorbis encoding mode."));
        n = kAudioSettingsFlags_ExpertParameter | kAudioSettingsFlags_MetaParameter;
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Hint), cf_n);
        CFRelease(cf_n);
        n = 0;
        ln = 3;
        if (mOutputFormat.mSampleRate >= 8000.0 && mOutputFormat.mSampleRate <= 50000.0) {
            if (mCfgMode == kVorbisEncoderModeAverage)
                n = 1;
            else if (mCfgMode == kVorbisEncoderModeQualityByBitrate)
                n = 2;
        } else {
            ln = 1;
        }
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_CurrentValue), cf_n);
        CFRelease(cf_n);
        n = kCFNumberLongType;
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_ValueType), cf_n);
        CFRelease(cf_n);

        CFStringRef values[4] = {CFSTR("Target Quality"),
                                 CFSTR("Average Bit Rate"),
                                 CFSTR("Target Bit Rate Quality"),
                                 CFSTR("Managed Bit Rate")};
        CFArrayRef varr = CFArrayCreate(NULL, reinterpret_cast<const void**>(&values), 4, &kCFTypeArrayCallBacks);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_AvailableValues), varr);
        CFRelease(varr);

        varr = CFArrayCreate(NULL, reinterpret_cast<const void**>(&values), ln, &kCFTypeArrayCallBacks);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_LimitedValues), varr);
        CFRelease(varr);

        CFArrayAppendValue(params, eltd);
        CFRelease(eltd);
    }

    if (mCfgMode == kVorbisEncoderModeQuality) {
        eltd = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_SettingKey), CFSTR("Encoding Quality"));
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_SettingName), CFSTR("Encoding Quality"));
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Summary), CFSTR("Vorbis encoding quality."));
        n = 0; // no flags - basic parameter
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Hint), cf_n);
        CFRelease(cf_n);

        n = lroundf(mCfgQuality * 10.0) + 1;
        if (n < 0)
            n = 0;
        else if (n > 11)
            n = 11;
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_CurrentValue), cf_n);
        CFRelease(cf_n);

        n = kCFNumberLongType;
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_ValueType), cf_n);
        CFRelease(cf_n);

        CFNumberRef values[12];
        for (UInt32 i = 0; i < 12; i++) {
            n = i - 1;
            values[i] = CFNumberCreate(NULL, kCFNumberLongType, &n);
        }
        CFArrayRef varr = CFArrayCreate(NULL, reinterpret_cast<const void**>(&values), 12, &kCFTypeArrayCallBacks);
        for (UInt32 i = 0; i < 12; i++) {
            CFRelease(values[i]);
        }
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_AvailableValues), varr);
        CFRelease(varr);

        CFArrayAppendValue(params, eltd);
        CFRelease(eltd);
    } else {
        eltd = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_SettingKey), CFSTR("Target Bitrate"));
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_SettingName), CFSTR("Target Bitrate"));
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Summary), CFSTR("Target bitrate."));
        n = 0; // no flags - basic parameter
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Hint), cf_n);
        CFRelease(cf_n);

        n = ibrn(mCfgBitrate, kVorbisEncoderBitrateSeriesBase);
        if (n < 0)
            n = 0;
        else if (n >= kVorbisEncoderBitrateSeriesLength)
            n = kVorbisEncoderBitrateSeriesLength - 1;
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_CurrentValue), cf_n);
        CFRelease(cf_n);

        n = kCFNumberLongType;
        cf_n = CFNumberCreate(NULL, kCFNumberLongType, &n);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_ValueType), cf_n);
        CFRelease(cf_n);

        CFNumberRef values[kVorbisEncoderBitrateSeriesLength];
        SInt32 brs_min = ibrn(BitrateMin(), kVorbisEncoderBitrateSeriesBase);
        SInt32 brs_max = ibrn(BitrateMax(), kVorbisEncoderBitrateSeriesBase) - 1;
        if (brs_max >= kVorbisEncoderBitrateSeriesLength)
            brs_max = kVorbisEncoderBitrateSeriesLength - 1;
        if (brs_max < brs_min) {
            CFRelease(eltd);
            CFRelease(params);
            return false;
        }

        for (UInt32 i = 0; i < kVorbisEncoderBitrateSeriesLength; i++) {
            n = brn(i, kVorbisEncoderBitrateSeriesBase);
            values[i] = CFNumberCreate(NULL, kCFNumberLongType, &n);
        }
        CFArrayRef varr = CFArrayCreate(NULL, reinterpret_cast<const void**>(&values), kVorbisEncoderBitrateSeriesLength, &kCFTypeArrayCallBacks);
        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_AvailableValues), varr);
        CFRelease(varr);

        if (brs_min > 0 || brs_max < kVorbisEncoderBitrateSeriesLength - 1) {
            CFNumberRef *limited = &values[brs_min];
            brs_max = brs_max + 1 - brs_min;
            if (brs_max > 0) {
                varr = CFArrayCreate(NULL, reinterpret_cast<const void**>(limited), brs_max, &kCFTypeArrayCallBacks);
                CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_LimitedValues), varr);
                CFRelease(varr);
            }
        }
        for (UInt32 i = 0; i < kVorbisEncoderBitrateSeriesLength; i++) {
            CFRelease(values[i]);
        }

        CFDictionaryAddValue(eltd, CFSTR(kAudioSettings_Unit), mOutputFormat.mChannelsPerFrame > 2 ? CFSTR("kbps/chan.") : CFSTR("kbps"));

        CFArrayAppendValue(params, eltd);
        CFRelease(eltd);
    }

    CFDictionarySetValue(sd, CFSTR(kAudioSettings_Parameters), params);
    CFRelease(params);

    *reinterpret_cast<CFDictionaryRef*>(outSettingsDict) = sd;
    ret = true;

    return ret;
}

Boolean CAVorbisEncoder::ApplySettings(const CFDictionaryRef sd)
{
    Boolean ret = false;
    CFArrayRef params = NULL;
    CFStringRef name = NULL;
    CFIndex i, cf_l;

    dbg_printf("[  VE]  >> [%08lx] :: ApplySettings()\n", (UInt32) this);

    name = (CFStringRef) CFDictionaryGetValue(sd, CFSTR(kAudioSettings_TopLevelKey));
    if (name == NULL || CFStringCompare(name, CFSTR("Xiph Vorbis Encoder"), 0) != kCFCompareEqualTo)
        return ret;

    params = (CFArrayRef) CFDictionaryGetValue(sd, CFSTR(kAudioSettings_Parameters));
    if (params == NULL)
        return ret;

    cf_l = CFArrayGetCount(params);
    for (i = 0; i < cf_l; i++) {
        CFDictionaryRef eltd = (CFDictionaryRef) CFArrayGetValueAtIndex(params, i);
        CFStringRef key = (CFStringRef) CFDictionaryGetValue(eltd, CFSTR(kAudioSettings_SettingKey));
        CFArrayRef available;
        CFIndex current;
        CFNumberRef nval;

        if (key == NULL)
            continue;

        nval = (CFNumberRef) CFDictionaryGetValue(eltd, CFSTR(kAudioSettings_CurrentValue));
        available = (CFArrayRef) CFDictionaryGetValue(eltd, CFSTR(kAudioSettings_AvailableValues));

        if (nval == NULL || available == NULL)
            continue;

        if (!CFNumberGetValue(nval, kCFNumberLongType, &current))
            continue;

        if (CFStringCompare(key, CFSTR("Encoding Mode"), 0) == kCFCompareEqualTo) {
            if (current == 0)
                mCfgMode = kVorbisEncoderModeQuality;
            else if (current == 1)
                mCfgMode = kVorbisEncoderModeAverage;
            else if (current == 2)
                mCfgMode = kVorbisEncoderModeQualityByBitrate;
            else
                return ret;
            dbg_printf("[  VE]   M [%08lx] :: ApplySettings() :: %d\n", (UInt32) this, mCfgMode);
        } else if (CFStringCompare(key, CFSTR("Encoding Quality"), 0) == kCFCompareEqualTo) {
            nval = (CFNumberRef) CFArrayGetValueAtIndex(available, current);
            SInt32 q = 4;
            CFNumberGetValue(nval, kCFNumberLongType, &q);
            if (q < -1 || q > 10)
                continue;
            mCfgQuality = (float) q / 10.0;
            dbg_printf("[  VE]   Q [%08lx] :: ApplySettings() :: %1.1f\n", (UInt32) this, mCfgQuality);
        } else if (CFStringCompare(key, CFSTR("Target Bitrate"), 0) == kCFCompareEqualTo) {
            nval = (CFNumberRef) CFArrayGetValueAtIndex(available, current);
            CFNumberGetValue(nval, kCFNumberLongType, &mCfgBitrate);
            dbg_printf("[  VE]   B [%08lx] :: ApplySettings() :: %ld\n", (UInt32) this, mCfgBitrate);
        }
    }

    if (mOutputFormat.mSampleRate < 8000.0 || mOutputFormat.mSampleRate > 50000.0)
        mCfgMode = kVorbisEncoderModeQuality;
    if (mCfgBitrate < BitrateMin())
        mCfgBitrate = BitrateMin();
    else if (mCfgBitrate > BitrateMax())
       mCfgBitrate = BitrateMax();
    dbg_printf("[  VE]  != [%08lx] :: ApplySettings() :: %d [br: %ld]\n", (UInt32) this, mCfgMode, mCfgBitrate);

    ret = true;
    dbg_printf("[  VE] <   [%08lx] :: ApplySettings() = %d\n", (UInt32) this, ret);
    return ret;
}


UInt32 CAVorbisEncoder::ibrn(UInt32 br, UInt32 bs)
{
    UInt32 n = -3; // number of bits - 3
    UInt32 bn = (br - 1) >> bs;
    UInt32 _br = bn;

    while (bn) {
        n++;
        bn >>= 1;
    };

    if (n < 0)
        return 0;

    return (n << 2) + ((_br >> n) & 0x03) + 1;
}

void CAVorbisEncoder::fill_channel_layout(UInt32 nch, AudioChannelLayout *acl)
{
    acl->mChannelBitmap = 0;
    acl->mNumberChannelDescriptions = 0;
    switch (nch) {
    case 1:
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_Mono;
        break;
    case 2:
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
        break;
    case 3:
        /* ITU_2_1 used a.t.m, but 3-channel layout shouldn't be
           accessible, as it's not the proper layout for vorbis */
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_ITU_2_1;
        break;
    case 4:
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_Quadraphonic;
        break;
    case 5:
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_5_0_C;
        break;
    case 6:
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_5_1_C;
        break;
    default:
        acl->mChannelLayoutTag = kAudioChannelLayoutTag_DiscreteInOrder | nch;
        break;
    }
}


/*
 * Vorbis bitrate limits depend on modes (sample rate + channel
 * (un)coupling). The values in the following methods are taken from
 * the current libvorbis sources, and may need to be changed if they
 * change in libvorbis.
 */

SInt32 CAVorbisEncoder::BitrateMin()
{
    SInt32 br = -1;

    if (mOutputFormat.mChannelsPerFrame == 2) {
        if (mOutputFormat.mSampleRate < 9000.0)
            br = 12;
        else if (mOutputFormat.mSampleRate < 15000.0)
            br = 16;
        else if (mOutputFormat.mSampleRate < 19000.0)
            br = 24;
        else if (mOutputFormat.mSampleRate < 26000.0)
            br = 30;
        else if (mOutputFormat.mSampleRate < 40000.0)
            br = 36;
        else if (mOutputFormat.mSampleRate < 50000.0)
            br = 45;
    } else {
        if (mOutputFormat.mSampleRate < 9000.0)
            br = 8;
        else if (mOutputFormat.mSampleRate < 15000.0)
            br = 12;
        else if (mOutputFormat.mSampleRate < 19000.0)
            br = 16;
        else if (mOutputFormat.mSampleRate < 26000.0)
            br = 16;
        else if (mOutputFormat.mSampleRate < 40000.0)
            br = 30;
        else if (mOutputFormat.mSampleRate < 50000.0)
            br = 32;
    }

    return br;
}

SInt32 CAVorbisEncoder::BitrateMax()
{
    SInt32 br = 0;

    if (mOutputFormat.mChannelsPerFrame == 2) {
        if (mOutputFormat.mSampleRate < 9000.0)
            br = 64;
        else if (mOutputFormat.mSampleRate < 15000.0)
            br = 88;
        else if (mOutputFormat.mSampleRate < 19000.0)
            br = 192;
        else if (mOutputFormat.mSampleRate < 26000.0)
            br = 192;
        else if (mOutputFormat.mSampleRate < 40000.0)
            br = 380;
        else if (mOutputFormat.mSampleRate < 50000.0)
            br = 500;
    } else {
        if (mOutputFormat.mSampleRate < 9000.0)
            br = 42;
        else if (mOutputFormat.mSampleRate < 15000.0)
            br = 50;
        else if (mOutputFormat.mSampleRate < 19000.0)
            br = 100;
        else if (mOutputFormat.mSampleRate < 26000.0)
            br = 90;
        else if (mOutputFormat.mSampleRate < 40000.0)
            br = 190;
        else if (mOutputFormat.mSampleRate < 50000.0)
            br = 240;
    }

    return br;
}

SInt32 CAVorbisEncoder::BitrateMid()
{
    SInt32 br = 128;

    if (mOutputFormat.mChannelsPerFrame == 2) {
        if (mOutputFormat.mSampleRate < 9000.0)
            br = 32;
        else if (mOutputFormat.mSampleRate < 15000.0)
            br = 48;
        else if (mOutputFormat.mSampleRate < 19000.0)
            br = 86;
        else if (mOutputFormat.mSampleRate < 26000.0)
            br = 86;
        else if (mOutputFormat.mSampleRate < 40000.0)
            br = 128;
        else if (mOutputFormat.mSampleRate < 50000.0)
            br = 128;
    } else {
        if (mOutputFormat.mSampleRate < 9000.0)
            br = 20;
        else if (mOutputFormat.mSampleRate < 15000.0)
            br = 24;
        else if (mOutputFormat.mSampleRate < 19000.0)
            br = 48;
        else if (mOutputFormat.mSampleRate < 26000.0)
            br = 48;
        else if (mOutputFormat.mSampleRate < 40000.0)
            br = 96;
        else if (mOutputFormat.mSampleRate < 50000.0)
            br = 96;
    }

    return br;
}
