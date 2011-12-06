/*
 *  CASpeexDecoder.cpp
 *
 *    CASpeexDecoder class implementation; the main part of the Speex
 *    codec functionality.
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
 *  Last modified: $Id: CASpeexDecoder.cpp 12093 2006-11-12 14:44:51Z arek $
 *
 */


#include <Ogg/ogg.h>
#include <Speex/speex_callbacks.h>

#include "CASpeexDecoder.h"

#include "CABundleLocker.h"

#include "speex_versions.h"
#include "fccs.h"
#include "data_types.h"

//#define NDEBUG
#include "debug.h"

CASpeexDecoder::CASpeexDecoder(Boolean inSkipFormatsInitialization /* = false */) :
    mCookie(NULL), mCookieSize(0), mCompressionInitialized(false),
    mOutBuffer(NULL), mOutBufferSize(0), mOutBufferUsedSize(0), mOutBufferStart(0),
    mSpeexFPList(),
    mNumFrames(0),
    mSpeexDecoderState(NULL)
{
    mSpeexStereoState.balance = 1.0;
    mSpeexStereoState.e_ratio = 0.5;
    mSpeexStereoState.smooth_left = 1.0;
    mSpeexStereoState.smooth_right = 1.0;

    if (inSkipFormatsInitialization)
        return;

    CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAudioFormatXiphSpeex,
                                            kSpeexBytesPerPacket, kSpeexFramesPerPacket,
                                            kSpeexBytesPerFrame, kSpeexChannelsPerFrame,
                                            kSpeexBitsPerChannel, kSpeexFormatFlags);
    AddInputFormat(theInputFormat);

    mInputFormat.mSampleRate = 44100;
    mInputFormat.mFormatID = kAudioFormatXiphSpeex;
    mInputFormat.mFormatFlags = kSpeexFormatFlags;
    mInputFormat.mBytesPerPacket = kSpeexBytesPerPacket;
    mInputFormat.mFramesPerPacket = kSpeexFramesPerPacket;
    mInputFormat.mBytesPerFrame = kSpeexBytesPerFrame;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = 16;

    CAStreamBasicDescription theOutputFormat1(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16,
                                              kAudioFormatFlagsNativeEndian |
                                              kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
    AddOutputFormat(theOutputFormat1);
    CAStreamBasicDescription theOutputFormat2(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 32,
                                              kAudioFormatFlagsNativeFloatPacked);
    AddOutputFormat(theOutputFormat2);

    mOutputFormat.mSampleRate = 44100;
    mOutputFormat.mFormatID = kAudioFormatLinearPCM;
    mOutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    mOutputFormat.mBytesPerPacket = 8;
    mOutputFormat.mFramesPerPacket = 1;
    mOutputFormat.mBytesPerFrame = 8;
    mOutputFormat.mChannelsPerFrame = 2;
    mOutputFormat.mBitsPerChannel = 32;
}

CASpeexDecoder::~CASpeexDecoder()
{
    if (mCookie != NULL)
        delete[] mCookie;

    if (mOutBuffer != NULL)
        delete[] mOutBuffer;

    if (mSpeexDecoderState != NULL)
        speex_decoder_destroy(mSpeexDecoderState);
}

void CASpeexDecoder::Initialize(const AudioStreamBasicDescription* inInputFormat,
                                const AudioStreamBasicDescription* inOutputFormat,
                                const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
    dbg_printf(" >> [%08lx] CASpeexDecoder :: Initialize(%d, %d, %d)\n", (UInt32) this, inInputFormat != NULL, inOutputFormat != NULL, inMagicCookieByteSize != 0);

    if(inInputFormat != NULL) {
        SetCurrentInputFormat(*inInputFormat);
    }

    if(inOutputFormat != NULL) {
        SetCurrentOutputFormat(*inOutputFormat);
    }

    if ((mInputFormat.mSampleRate != mOutputFormat.mSampleRate) ||
        (mInputFormat.mChannelsPerFrame != mOutputFormat.mChannelsPerFrame)) {
        CODEC_THROW(kAudioCodecUnsupportedFormatError);
    }

    // needs to be called after input & output format have been set
    BDCInitialize(kSpeexDecoderInBufferSize);

    //if (inMagicCookieByteSize == 0)
    //    CODEC_THROW(kAudioCodecUnsupportedFormatError);

    if (inMagicCookieByteSize != 0) {
        SetMagicCookie(inMagicCookie, inMagicCookieByteSize);
    }

    XCACodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
    dbg_printf("<.. [%08lx] CASpeexDecoder :: Initialize(%d, %d, %d)\n", (UInt32) this, inInputFormat != NULL, inOutputFormat != NULL, inMagicCookieByteSize != 0);
}

void CASpeexDecoder::Uninitialize()
{
    dbg_printf(" >> [%08lx] CASpeexDecoder :: Uninitialize()\n", (UInt32) this);

    BDCUninitialize();
    XCACodec::Uninitialize();

    dbg_printf("<.. [%08lx] CASpeexDecoder :: Uninitialize()\n", (UInt32) this);
}

void CASpeexDecoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
    dbg_printf(" >> [%08lx] CASpeexDecoder :: GetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
    switch(inPropertyID)
    {
    case kAudioCodecPropertyRequiresPacketDescription:
        if(ioPropertyDataSize == sizeof(UInt32))
        {
            *reinterpret_cast<UInt32*>(outPropertyData) = 1;
        }
        else
        {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;
    case kAudioCodecPropertyHasVariablePacketByteSizes:
        if(ioPropertyDataSize == sizeof(UInt32))
        {
            *reinterpret_cast<UInt32*>(outPropertyData) = 1;
        }
        else
        {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;
    case kAudioCodecPropertyPacketFrameSize:
        if(ioPropertyDataSize == sizeof(UInt32))
        {
            UInt32 *outProp = reinterpret_cast<UInt32*>(outPropertyData);
            if (!mCompressionInitialized)
                *outProp = kSpeexFramesPerPacket;
            else if (mSpeexHeader.frame_size != 0 * mSpeexHeader.frames_per_packet != 0)
                *outProp = mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet;
            else
                *outProp = 8192;
            if (*outProp < 8192 && mInputFormat.mFormatID == kAudioFormatXiphOggFramedSpeex)
                *outProp = 8192;
            dbg_printf("  = [%08lx] CASpeexDecoder :: GetProperty('pakf'): %ld\n",
                       (UInt32) this, *outProp);
        }
        else
        {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

        //case kAudioCodecPropertyQualitySetting: ???
#if TARGET_OS_MAC
    case kAudioCodecPropertyNameCFString:
        {
            if (ioPropertyDataSize != sizeof(CFStringRef)) CODEC_THROW(kAudioCodecBadPropertySizeError);

            CABundleLocker lock;
            CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Xiph Speex decoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
            *(CFStringRef*)outPropertyData = name;
            break;
        }

        //case kAudioCodecPropertyManufacturerCFString:
#endif
    default:
        ACBaseCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
    }
    dbg_printf("<.. [%08lx] CASpeexDecoder :: GetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CASpeexDecoder::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable)
{
    dbg_printf(" >> [%08lx] CASpeexDecoder :: GetPropertyInfo('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
    switch(inPropertyID)
    {
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

    default:
        ACBaseCodec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
        break;

    }
    dbg_printf("<.. [%08lx] CASpeexDecoder :: GetPropertyInfo('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CASpeexDecoder::Reset()
{
    dbg_printf(">> [%08lx] CASpeexDecoder :: Reset()\n", (UInt32) this);
    BDCReset();

    XCACodec::Reset();
    dbg_printf("<< [%08lx] CASpeexDecoder :: Reset()\n", (UInt32) this);
}

UInt32 CASpeexDecoder::GetVersion() const
{
    return kCASpeex_adec_Version;
}


void CASpeexDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
    if (!mIsInitialized) {
        //	check to make sure the input format is legal
        if (inInputFormat.mFormatID != kAudioFormatXiphSpeex) {
            dbg_printf("CASpeexDecoder::SetFormats: only supports Xiph Speex for input\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentInputFormat(inInputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

void CASpeexDecoder::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
    if (!mIsInitialized)
    {
        //	check to make sure the output format is legal
        if ((inOutputFormat.mFormatID != kAudioFormatLinearPCM) ||
            !(((inOutputFormat.mFormatFlags == kAudioFormatFlagsNativeFloatPacked) &&
               (inOutputFormat.mBitsPerChannel == 32)) ||
              ((inOutputFormat.mFormatFlags == (kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked)) &&
               (inOutputFormat.mBitsPerChannel == 16))))
        {
            dbg_printf("CASpeexDecoder::SetFormats: only supports"
                       " either 16 bit native endian signed integer or 32 bit native endian CoreAudio floats for output\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentOutputFormat(inOutputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

UInt32 CASpeexDecoder::GetMagicCookieByteSize() const
{
    return mCookieSize;
}

void CASpeexDecoder::GetMagicCookie(void* outMagicCookieData, UInt32& ioMagicCookieDataByteSize) const
{
    ioMagicCookieDataByteSize = mCookieSize;

    if (mCookie != NULL)
        outMagicCookieData = mCookie;
}

void CASpeexDecoder::SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
    dbg_printf(" >> [%08lx] CASpeexDecoder :: SetMagicCookie()\n", (UInt32) this);
    if (mIsInitialized)
        CODEC_THROW(kAudioCodecStateError);

    SetCookie(inMagicCookieData, inMagicCookieDataByteSize);

    InitializeCompressionSettings();

    if (!mCompressionInitialized)
        CODEC_THROW(kAudioCodecUnsupportedFormatError);
    dbg_printf("<.. [%08lx] CASpeexDecoder :: SetMagicCookie()\n", (UInt32) this);
}

void CASpeexDecoder::SetCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
    if (mCookie != NULL)
        delete[] mCookie;

    mCookieSize = inMagicCookieDataByteSize;
    if (inMagicCookieDataByteSize != 0) {
        mCookie = new Byte[inMagicCookieDataByteSize];
        BlockMoveData(inMagicCookieData, mCookie, inMagicCookieDataByteSize);
    } else {
        mCookie = NULL;
    }
}

void CASpeexDecoder::InitializeCompressionSettings()
{
    if (mCookie == NULL)
        return;

    if (mCompressionInitialized) {
        memset(&mSpeexHeader, 0, sizeof(mSpeexHeader));

        mSpeexStereoState.balance = 1.0;
        mSpeexStereoState.e_ratio = 0.5;
        mSpeexStereoState.smooth_left = 1.0;
        mSpeexStereoState.smooth_right = 1.0;

        if (mSpeexDecoderState != NULL) {
            speex_decoder_destroy(mSpeexDecoderState);
            mSpeexDecoderState = NULL;
        }
    }

    mCompressionInitialized = false;

    OggSerialNoAtom *atom = reinterpret_cast<OggSerialNoAtom*> (mCookie);
    Byte *ptrheader = mCookie + EndianU32_BtoN(atom->size);
    CookieAtomHeader *aheader = reinterpret_cast<CookieAtomHeader*> (ptrheader);

    // scan quickly through the cookie, check types and packet sizes
    if (EndianS32_BtoN(atom->type) != kCookieTypeOggSerialNo || static_cast<UInt32> (ptrheader - mCookie) > mCookieSize)
        return;
    ptrheader += EndianU32_BtoN(aheader->size);
    if (EndianS32_BtoN(aheader->type) != kCookieTypeSpeexHeader || static_cast<UInt32> (ptrheader - mCookie) > mCookieSize)
        return;
    // we ignore the rest: comments and extra headers

    // all OK, back to the first speex packet
    aheader = reinterpret_cast<CookieAtomHeader*> (mCookie + EndianU32_BtoN(atom->size));
    SpeexHeader *inheader = reinterpret_cast<SpeexHeader *> (&aheader->data[0]);

    // TODO: convert, at some point, mSpeexHeader to a pointer?
    mSpeexHeader.bitrate =                 EndianS32_LtoN(inheader->bitrate);
    mSpeexHeader.extra_headers =           EndianS32_LtoN(inheader->extra_headers);
    mSpeexHeader.frame_size =              EndianS32_LtoN(inheader->frame_size);
    mSpeexHeader.frames_per_packet =       EndianS32_LtoN(inheader->frames_per_packet);
    mSpeexHeader.header_size =             EndianS32_LtoN(inheader->header_size);
    mSpeexHeader.mode =                    EndianS32_LtoN(inheader->mode);
    mSpeexHeader.mode_bitstream_version =  EndianS32_LtoN(inheader->mode_bitstream_version);
    mSpeexHeader.nb_channels =             EndianS32_LtoN(inheader->nb_channels);
    mSpeexHeader.rate =                    EndianS32_LtoN(inheader->rate);
    mSpeexHeader.reserved1 =               EndianS32_LtoN(inheader->reserved1);
    mSpeexHeader.reserved2 =               EndianS32_LtoN(inheader->reserved2);
    mSpeexHeader.speex_version_id =        EndianS32_LtoN(inheader->speex_version_id);
    mSpeexHeader.vbr =                     EndianS32_LtoN(inheader->vbr);

    if (mSpeexHeader.mode >= SPEEX_NB_MODES)
        CODEC_THROW(kAudioCodecUnsupportedFormatError);

    //TODO: check bitstream version here

    mSpeexDecoderState = speex_decoder_init(speex_lib_get_mode(mSpeexHeader.mode));

    if (!mSpeexDecoderState)
        CODEC_THROW(kAudioCodecUnsupportedFormatError);

    //TODO: fix some of the header fields here

    int enhzero = 0;
    speex_decoder_ctl(mSpeexDecoderState, SPEEX_SET_ENH, &enhzero);

    if (mSpeexHeader.nb_channels == 2)
    {
        SpeexCallback callback;
        callback.callback_id = SPEEX_INBAND_STEREO;
        callback.func = speex_std_stereo_request_handler;
        callback.data = &mSpeexStereoState;
        speex_decoder_ctl(mSpeexDecoderState, SPEEX_SET_HANDLER, &callback);
    }

    mCompressionInitialized = true;
}


#pragma mark BDC handling

void CASpeexDecoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    speex_bits_init(&mSpeexBits);

    if (mOutBuffer)
        delete[] mOutBuffer;

    mOutBuffer = new Byte[kSpeexDecoderOutBufferSize];
    mOutBufferSize = kSpeexDecoderOutBufferSize;
    mOutBufferUsedSize = 0;
    mOutBufferStart = 0;

    XCACodec::BDCInitialize(inInputBufferByteSize);
}

void CASpeexDecoder::BDCUninitialize()
{
    speex_bits_destroy(&mSpeexBits);
    if (mSpeexDecoderState != NULL)
        speex_decoder_ctl(mSpeexDecoderState, SPEEX_RESET_STATE, NULL); //??!

    mSpeexFPList.clear();

    if (mOutBuffer)
        delete[] mOutBuffer;
    mOutBuffer = NULL;
    mOutBufferSize = 0;
    mOutBufferUsedSize = 0;
    mOutBufferStart = 0;

    XCACodec::BDCUninitialize();
}

void CASpeexDecoder::BDCReset()
{
    speex_bits_reset(&mSpeexBits);
    if (mSpeexDecoderState != NULL)
        speex_decoder_ctl(mSpeexDecoderState, SPEEX_RESET_STATE, NULL); //??!

    mSpeexFPList.clear();

    mNumFrames = 0;

    mOutBufferUsedSize = 0;
    mOutBufferStart = 0;

    XCACodec::BDCReset();
}

void CASpeexDecoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    speex_bits_reset(&mSpeexBits);

    mSpeexFPList.clear();

    XCACodec::BDCReallocate(inInputBufferByteSize);
}


void CASpeexDecoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    const Byte * theData = static_cast<const Byte *> (inInputData) + inPacketDescription->mStartOffset;
    UInt32 size = inPacketDescription->mDataByteSize;
    mBDCBuffer.In(theData, size);
    mSpeexFPList.push_back(SpeexFramePacket(*reinterpret_cast<const SInt32*> (&inPacketDescription->mVariableFramesInPacket),
                                            inPacketDescription->mDataByteSize));
}


UInt32 CASpeexDecoder::FramesReady() const
{
    return mNumFrames;
}

Boolean CASpeexDecoder::GenerateFrames()
{
    Boolean ret = true;
    int result;

    mBDCStatus = kBDCStatusOK;
    SpeexFramePacket &sfp = mSpeexFPList.front();

    speex_bits_read_from(&mSpeexBits, reinterpret_cast<char*> (mBDCBuffer.GetData()), sfp.bytes);

    if (sfp.frames > 0 && (sfp.frames - mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet > 0)) {
        UInt32 zeroBytes = mOutputFormat.FramesToBytes(sfp.frames - mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet);
        memset(mOutBuffer + mOutBufferUsedSize, 0, zeroBytes);
        mOutBufferUsedSize += zeroBytes;
    }

    for (SInt32 i = 0; i < mSpeexHeader.frames_per_packet; i++) {
        if (mOutputFormat.mFormatFlags & kAudioFormatFlagsNativeFloatPacked != 0)
            result = speex_decode(mSpeexDecoderState, &mSpeexBits, reinterpret_cast<float*> (mOutBuffer + mOutBufferUsedSize));
        else
            result = speex_decode_int(mSpeexDecoderState, &mSpeexBits, reinterpret_cast<spx_int16_t*> (mOutBuffer + mOutBufferUsedSize));

        if (result < 0) {
            mBDCStatus = kBDCStatusAbort;
            return false;
        }

        if (mSpeexHeader.nb_channels == 2) {
            if (mOutputFormat.mFormatFlags & kAudioFormatFlagsNativeFloatPacked != 0)
                speex_decode_stereo(reinterpret_cast<float*> (mOutBuffer + mOutBufferUsedSize), mSpeexHeader.frame_size, &mSpeexStereoState);
            else
                speex_decode_stereo_int(reinterpret_cast<spx_int16_t*> (mOutBuffer + mOutBufferUsedSize), mSpeexHeader.frame_size, &mSpeexStereoState);
        }
        mOutBufferUsedSize += mOutputFormat.FramesToBytes(mSpeexHeader.frame_size);
    }

    if (sfp.frames == 0) {
        mNumFrames += mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet;
    } else if (sfp.frames > 0) {
        mNumFrames += sfp.frames;
        if (mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet - sfp.frames != 0)
            mOutBufferStart += mOutputFormat.FramesToBytes(mSpeexHeader.frame_size * mSpeexHeader.frames_per_packet - sfp.frames);
    } else {
        mNumFrames -= sfp.frames;
    }

    mBDCBuffer.Zap(sfp.bytes);
    mSpeexFPList.erase(mSpeexFPList.begin());

    return ret;
}

void CASpeexDecoder::OutputFrames(void* outOutputData, UInt32 inNumberFrames, UInt32 inFramesOffset,
                                  AudioStreamPacketDescription* /* outPacketDescription */) const
{
    if (mOutputFormat.mFormatFlags & kAudioFormatFlagsNativeFloatPacked != 0) {
        float* theOutputData = static_cast<float*> (outOutputData) + inFramesOffset * mSpeexHeader.nb_channels;
        float* theSourceData = reinterpret_cast<float*> (mOutBuffer + mOutBufferStart);
        UInt32 num_floats = inNumberFrames * mSpeexHeader.nb_channels;
        for (UInt32 i = 0; i < num_floats; i++) {
            *theOutputData = *theSourceData / 32768.0; // !??
            theOutputData += 1;
            theSourceData += 1;
        }
    } else {
        BlockMoveData(mOutBuffer + mOutBufferStart,
                      static_cast<Byte*> (outOutputData) + mOutputFormat.FramesToBytes(inFramesOffset),
                      mOutputFormat.FramesToBytes(inNumberFrames));
    }
}

void CASpeexDecoder::Zap(UInt32 inFrames)
{
    mNumFrames -= inFrames;

    if (mNumFrames < 0)
        mNumFrames = 0;

    if (mNumFrames == 0) {
        mOutBufferUsedSize = 0;
        mOutBufferStart = 0;
    } else {
        mOutBufferStart += mOutputFormat.FramesToBytes(inFrames);
    }
}
