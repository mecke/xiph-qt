/*
 *  CAFLACDecoder.cpp
 *
 *    CAFLACDecoder class implementation; the main part of the FLAC
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
 *  Last modified: $Id: CAFLACDecoder.cpp 12356 2007-01-20 00:18:04Z arek $
 *
 */


#include <Ogg/ogg.h>
#include <FLAC++/decoder.h>

#include "CAFLACDecoder.h"

#include "CABundleLocker.h"

#include "flac_versions.h"
#include "fccs.h"
#include "data_types.h"

#include "debug.h"


CAFLACDecoder::CAFLACDecoder(Boolean inSkipFormatsInitialization /* = false */) :
    mCookie(NULL), mCookieSize(0), mCompressionInitialized(false),
    mOutBuffer(NULL), mOutBufferSize(0), mOutBufferUsedSize(0),
    mFLACsrate(44100), mFLACchannels(2), mFLACbits(16),
    mFLACFPList(),
    mFrame(), mNumFrames(0), mBPtrs(NULL)
{
    if (inSkipFormatsInitialization)
        return;

    CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAudioFormatXiphFLAC,
                                            kFLACBytesPerPacket, kFLACFramesPerPacket,
                                            kFLACBytesPerFrame, kFLACChannelsPerFrame,
                                            kFLACBitsPerChannel, kFLACFormatFlags);
    AddInputFormat(theInputFormat);

    mInputFormat.mSampleRate = 44100.0;
    mInputFormat.mFormatID = kAudioFormatXiphFLAC;
    mInputFormat.mFormatFlags = kFLACFormatFlags;
    mInputFormat.mBytesPerPacket = kFLACBytesPerPacket;
    mInputFormat.mFramesPerPacket = kFLACFramesPerPacket;
    mInputFormat.mBytesPerFrame = kFLACBytesPerFrame;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = 16;

    CAStreamBasicDescription theOutputFormat1(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16,
                                              kAudioFormatFlagsNativeEndian |
                                              kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
    AddOutputFormat(theOutputFormat1);
    CAStreamBasicDescription theOutputFormat2(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 32,
                                              kAudioFormatFlagsNativeFloatPacked);
    AddOutputFormat(theOutputFormat2);

    mOutputFormat.mSampleRate = 44100.0;
    mOutputFormat.mFormatID = kAudioFormatLinearPCM;
    mOutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    mOutputFormat.mBytesPerPacket = 8;
    mOutputFormat.mFramesPerPacket = 1;
    mOutputFormat.mBytesPerFrame = 8;
    mOutputFormat.mChannelsPerFrame = 2;
    mOutputFormat.mBitsPerChannel = 32;
}

CAFLACDecoder::~CAFLACDecoder()
{
    if (mCookie != NULL)
        delete[] mCookie;
}

void CAFLACDecoder::Initialize(const AudioStreamBasicDescription* inInputFormat,
                               const AudioStreamBasicDescription* inOutputFormat,
                               const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
    dbg_printf(" >> [%08lx] :: Initialize(%d, %d, %d)\n", (UInt32) this, inInputFormat != NULL, inOutputFormat != NULL, inMagicCookieByteSize != 0);

    if(inInputFormat != NULL) {
        SetCurrentInputFormat(*inInputFormat);
        /////// TODO: !! what do to with the formats?!!
        mFLACsrate = static_cast<UInt32> (inInputFormat->mSampleRate);
        mFLACchannels = inInputFormat->mChannelsPerFrame;
        mFLACbits = mInputFormat.mBitsPerChannel;
        if (mFLACbits < 4)
            mFLACbits = 16;
    }

    if(inOutputFormat != NULL) {
        SetCurrentOutputFormat(*inOutputFormat);
    }

    if ((mInputFormat.mSampleRate != mOutputFormat.mSampleRate) ||
        (mInputFormat.mChannelsPerFrame != mOutputFormat.mChannelsPerFrame)) {
        CODEC_THROW(kAudioCodecUnsupportedFormatError);
    }

    BDCInitialize(kFLACDecoderInBufferSize);

    if (inMagicCookieByteSize != 0) {
        SetMagicCookie(inMagicCookie, inMagicCookieByteSize);
    }

    //if (mCompressionInitialized)
    //    FixFormats();

    XCACodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
    dbg_printf("<.. [%08lx] :: Initialize(%d, %d, %d)\n", (UInt32) this, inInputFormat != NULL, inOutputFormat != NULL, inMagicCookieByteSize != 0);
}

void CAFLACDecoder::Uninitialize()
{
    dbg_printf(" >> [%08lx] :: Uninitialize()\n", (UInt32) this);
    BDCUninitialize();
    XCACodec::Uninitialize();
    dbg_printf("<.. [%08lx] :: Uninitialize()\n", (UInt32) this);
}

void CAFLACDecoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
    dbg_printf(" >> [%08lx] :: GetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
    switch(inPropertyID)
    {
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
        if(ioPropertyDataSize == sizeof(UInt32)) {
            *reinterpret_cast<UInt32*>(outPropertyData) = kFLACFramesPerPacketReported;
        } else {
            CODEC_THROW(kAudioCodecBadPropertySizeError);
        }
        break;

        /*
        case kAudioCodecPropertyMaximumPacketByteSize:
            if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = 32 * 8 * 1024;
            } else {
                CODEC_THROW(kAudioCodecBadPropertySizeError);
            }
            break;
        */

        //case kAudioCodecPropertyQualitySetting: ???
#if TARGET_OS_MAC
    case kAudioCodecPropertyNameCFString:
        {
            if (ioPropertyDataSize != sizeof(CFStringRef)) CODEC_THROW(kAudioCodecBadPropertySizeError);

            CABundleLocker lock;
            CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Xiph FLAC decoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
            *(CFStringRef*)outPropertyData = name;
            break; 
        }

        //case kAudioCodecPropertyManufacturerCFString:
#endif
    default:
        ACBaseCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
    }
    dbg_printf("<.. [%08lx] :: GetProperty('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CAFLACDecoder::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable)
{
    dbg_printf(" >> [%08lx] :: GetPropertyInfo('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
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

    /*
    case kAudioCodecPropertyMaximumPacketByteSize:
        outPropertyDataSize = sizeof(UInt32);
        outWritable = false;
        break;
    */

    default:
        ACBaseCodec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
        break;

    };
    dbg_printf("<.. [%08lx] :: GetPropertyInfo('%4.4s')\n", (UInt32) this, reinterpret_cast<char*> (&inPropertyID));
}

void CAFLACDecoder::Reset()
{
    dbg_printf(">> [%08lx] :: Reset()\n", (UInt32) this);
    BDCReset();

    XCACodec::Reset();
    dbg_printf("<< [%08lx] :: Reset()\n", (UInt32) this);
}

UInt32 CAFLACDecoder::GetVersion() const
{
    return kCAFLAC_adec_Version;
}

UInt32 CAFLACDecoder::GetMagicCookieByteSize() const
{
    return mCookieSize;
}

void CAFLACDecoder::GetMagicCookie(void* outMagicCookieData, UInt32& ioMagicCookieDataByteSize) const
{
    ioMagicCookieDataByteSize = mCookieSize;

    if (mCookie != NULL)
        outMagicCookieData = mCookie;
}

void CAFLACDecoder::SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
    dbg_printf(" >> [%08lx] :: SetMagicCookie()\n", (UInt32) this);
    if (mIsInitialized)
        CODEC_THROW(kAudioCodecStateError);

    SetCookie(inMagicCookieData, inMagicCookieDataByteSize);

    InitializeCompressionSettings();

    if (!mCompressionInitialized)
        CODEC_THROW(kAudioCodecUnsupportedFormatError);
    dbg_printf("<.. [%08lx] :: SetMagicCookie()\n", (UInt32) this);
}


void CAFLACDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
    if(!mIsInitialized) {
        //	check to make sure the input format is legal
        if(inInputFormat.mFormatID != kAudioFormatXiphFLAC)
        {
            dbg_printf("CAFLACDecoder::SetFormats: only support Xiph Vorbis for input\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentInputFormat(inInputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

void CAFLACDecoder::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
    if(!mIsInitialized) {
        //	check to make sure the output format is legal
        if ((inOutputFormat.mFormatID != kAudioFormatLinearPCM) ||
            !(((inOutputFormat.mFormatFlags == kAudioFormatFlagsNativeFloatPacked) &&
               (inOutputFormat.mBitsPerChannel == 32)) ||
              ((inOutputFormat.mFormatFlags == (kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked)) &&
               (inOutputFormat.mBitsPerChannel == 16))))
        {
            dbg_printf("CAFLACDecoder::SetFormats: only supports either 16 bit native endian signed integer or 32 bit native endian Core Audio floats for output\n");
            CODEC_THROW(kAudioCodecUnsupportedFormatError);
        }

        //	tell our base class about the new format
        XCACodec::SetCurrentOutputFormat(inOutputFormat);
    } else {
        CODEC_THROW(kAudioCodecStateError);
    }
}

#if 0
void CAFLACDecoder::FixFormats()
{
    //mInputFormat.mSampleRate = mV_vi.rate;
    //mInputFormat.mChannelsPerFrame = mV_vi.channels;

    /*
	mInputFormat.mFramesPerPacket = 64;
	mInputFormat.mBytesPerPacket = mInputFormat.mChannelsPerFrame * 34;
	mInputFormat.mBytesPerFrame = 0;
    */
}
#endif /* 0 */


#pragma mark The CORE

void CAFLACDecoder::OutputFrames(void* outOutputData, UInt32 inNumberFrames, UInt32 inFramesOffset,
                                 AudioStreamPacketDescription* /* outPacketDescription */) const
{
    UInt32 i, j;

    if (mOutputFormat.mFormatFlags & kAudioFormatFlagsNativeFloatPacked != 0) {
        float cnvrtr = (float)((1 << (mFLACbits - 1)) - 1);
        for (i = 0; i < mFLACchannels; i++) {
            float* theOutputData = static_cast<float*> (outOutputData) + i + (inFramesOffset * mFLACchannels);
            const FLAC__int32* mono = static_cast<const FLAC__int32*> (mBPtrs[i] + mFrame.header.blocksize - mNumFrames);

            for (j = 0; j < inNumberFrames; j++) {
                *theOutputData = ((float)mono[j]) / cnvrtr;
                theOutputData += mFLACchannels;
            }
        }
    } else {
        for (i = 0; i < mFLACchannels; i++) {
            SInt16* theOutputData = static_cast<SInt16*> (outOutputData) + i + (inFramesOffset * mFLACchannels);
            const FLAC__int32* mono = static_cast<const FLAC__int32*> (mBPtrs[i] + mFrame.header.blocksize - mNumFrames);
            SInt32 bitshift = mFLACbits - mOutputFormat.mBitsPerChannel;

            if (bitshift == 0) {
                for (j = 0; j < inNumberFrames; j++) {
                    *theOutputData = static_cast<SInt16> (mono[j]);
                    theOutputData += mFLACchannels;
                }
            } else if (bitshift > 0) {
                for (j = 0; j < inNumberFrames; j++) {
                    *theOutputData = static_cast<SInt16> (mono[j] >> bitshift);
                    theOutputData += mFLACchannels;
                }
            } else {
                bitshift = -bitshift;
                for (j = 0; j < inNumberFrames; j++) {
                    *theOutputData = static_cast<SInt16> (mono[j] << bitshift);
                    theOutputData += mFLACchannels;
                }
            }
        }
    }
}

void CAFLACDecoder::SetCookie(const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
    if (mCookie != NULL)
        delete[] mCookie;

    mCookieSize = inMagicCookieByteSize;
    if (inMagicCookieByteSize != 0) {
        mCookie = new Byte[inMagicCookieByteSize];
        BlockMoveData(inMagicCookie, mCookie, inMagicCookieByteSize);
    } else {
        mCookie = NULL;
    }
}

void CAFLACDecoder::InitializeCompressionSettings()
{
    if (mCookie == NULL)
        return;

    mCompressionInitialized = false;

    Byte *ptrheader = mCookie;
    Byte *cend = mCookie + mCookieSize;
    CookieAtomHeader *aheader = reinterpret_cast<CookieAtomHeader*> (ptrheader);

    while (ptrheader < cend) {
        aheader = reinterpret_cast<CookieAtomHeader*> (ptrheader);
        ptrheader += EndianU32_BtoN(aheader->size);

        if (EndianS32_BtoN(aheader->type) == kCookieTypeFLACStreaminfo && ptrheader <= cend) {
            UInt32 sib = EndianU32_BtoN(* (UInt32 *) (((Byte *)aheader->data) + 14));

            sib >>= 4;
            mFLACbits = (sib & 0x1f) + 1;
            sib >>= 5;
            mFLACchannels = (sib & 0x07) + 1;
            mFLACsrate = (sib >> 3) & 0xfffff;

            dbg_printf("  = [%08lx] CAFLACDecoder :: InitializeCompressionSettings(sr: %ld, chn: %ld, bi: %ld)\n",
                       (UInt32) this, mFLACsrate, mFLACchannels, mFLACbits);

            mCompressionInitialized = true;
        }
    }
}


#pragma mark BDC functions

void CAFLACDecoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    init();

    if (mOutBuffer)
        delete[] mOutBuffer;
    mOutBuffer = new FLAC__int32[kFLACDecoderOutBufferSize];
    mOutBufferSize = kFLACDecoderOutBufferSize;
    mOutBufferUsedSize = 0;

    XCACodec::BDCInitialize(inInputBufferByteSize);
};

void CAFLACDecoder::BDCUninitialize()
{
    mFLACFPList.clear();

    if (mOutBuffer)
        delete[] mOutBuffer;
    mOutBuffer = NULL;
    mOutBufferSize = 0;
    mOutBufferUsedSize = 0;

    XCACodec::BDCUninitialize();
};

void CAFLACDecoder::BDCReset()
{
    mFLACFPList.clear();

    DFPclear();

    mOutBufferUsedSize = 0;

    flush();
    reset();

    XCACodec::BDCReset();
};

void CAFLACDecoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    mFLACFPList.clear();

    XCACodec::BDCReallocate(inInputBufferByteSize);
};


void CAFLACDecoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    const Byte * theData = static_cast<const Byte *> (inInputData) + inPacketDescription->mStartOffset;
    UInt32 size = inPacketDescription->mDataByteSize;

    if (mBDCBuffer.GetSpaceAvailable() <= size) {
        UInt32 new_size_inc = kFLACDecoderInBufferSize;
        UInt32 space_needed = size - mBDCBuffer.GetSpaceAvailable() + 1;
        while (new_size_inc < space_needed)
            new_size_inc += kFLACDecoderInBufferSize;
        mBDCBuffer.Reallocate(mBDCBuffer.GetBufferByteSize() + new_size_inc);
    }

    mBDCBuffer.In(theData, size);
    mFLACFPList.push_back(FLACFramePacket(inPacketDescription->mVariableFramesInPacket, inPacketDescription->mDataByteSize));
};


UInt32 CAFLACDecoder::FramesReady() const
{
    return mNumFrames;
};

Boolean CAFLACDecoder::GenerateFrames()
{
    Boolean ret = true;

    while (process_single() != true) {
        if ((get_state() != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC && get_state() != FLAC__STREAM_DECODER_READ_FRAME)) {
            ret = false;
            mBDCStatus = kBDCStatusAbort;
            break;
        }
    }

    if (get_state() != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC && get_state() != FLAC__STREAM_DECODER_READ_FRAME) {
        ret = false;
        mBDCStatus = kBDCStatusAbort;
    }

    return ret;
};

void CAFLACDecoder::Zap(UInt32 inFrames)
{
    mNumFrames -= inFrames;

    if (mNumFrames < 0)
        mNumFrames = 0;

    if (mNumFrames == 0) {
        mOutBufferUsedSize = 0;
        DFPclear();
    }
};


void CAFLACDecoder::DFPinit(const ::FLAC__Frame& inFrame, const FLAC__int32 *const inBuffer[])
{
    mFrame.header.blocksize = inFrame.header.blocksize;
    mNumFrames = inFrame.header.blocksize;
    mFrame.header.channels = inFrame.header.channels;
    // fill other values...
    
    mBPtrs = const_cast<const FLAC__int32**> (inBuffer);
};

void CAFLACDecoder::DFPclear()
{
    if (mBPtrs) {
        delete[] mBPtrs; mBPtrs = NULL;
    }

    mNumFrames = 0;
};

#pragma mark Callbacks

::FLAC__StreamDecoderReadStatus CAFLACDecoder::read_callback(FLAC__byte buffer[], size_t *bytes)
{
    dbg_printf("[  FD]| -> [%08lx] :: read_callback(%ld)\n", (UInt32) this, *bytes);
    FLAC__StreamDecoderReadStatus ret = FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;

    if (mFLACFPList.empty()) {
        *bytes = 0;
        ret =  FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    } else {
        FLACFramePacket& ffp = mFLACFPList.front();

        if (*bytes > mBDCBuffer.GetDataAvailable())
            *bytes = mBDCBuffer.GetDataAvailable();
        if (*bytes > ffp.left)
            *bytes = ffp.left;

        BlockMoveData(mBDCBuffer.GetData(), buffer, *bytes);

        mBDCBuffer.Zap(*bytes);
        ffp.left -= *bytes;
        if (ffp.left == 0) {
            mFLACFPList.erase(mFLACFPList.begin());
        }
    }

    dbg_printf("[  FD]|<-. [%08lx] :: read_callback(%ld) = %ld\n", (UInt32) this, *bytes, ret);
    return ret;
}

::FLAC__StreamDecoderWriteStatus CAFLACDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
    ::FLAC__StreamDecoderWriteStatus ret = FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    dbg_printf("[  FD]| +> [%08lx] :: write_callback(%ld) [%ld]\n", (UInt32) this, frame->header.blocksize, frame->header.channels);

    FLAC__int32** lbuffer = new FLAC__int32*[frame->header.channels];
    for (unsigned int i = 0; i < frame->header.channels; i++) {
        // check here for enough space in mOutBuffer !?
        lbuffer[i] = mOutBuffer + mOutBufferUsedSize;
        BlockMoveData(buffer[i], lbuffer[i], frame->header.blocksize * sizeof(FLAC__int32));
        mOutBufferUsedSize += frame->header.blocksize;
    }

    DFPinit(*frame, lbuffer); //MFDFPacket will free the lbuffer on .clear() call

    dbg_printf("[  FD]|<+. [%08lx] :: write_callback()\n", (UInt32) this);
    return ret;
}

void CAFLACDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
    dbg_printf(" |\".\" [%08lx] :: metadata_callback()\n", (UInt32) this);
}

void CAFLACDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
    dbg_printf(" |<!> [%08lx] :: error_callback(%ld)\n", (UInt32) this, status);
}
