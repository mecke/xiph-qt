/*
 *  TheoraDecoder.c
 *
 *    Theora video decoder (ImageCodec) implementation.
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
 *  Last modified: $Id: TheoraDecoder.c 16093 2009-06-08 23:16:39Z arek $
 *
 */

/*
 *  The implementation in this file is based on the 'ElectricImageCodec' and
 *  the 'ExampleIPBCodec' example QuickTime components.
 */

#if defined(__APPLE_CC__)
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#else
#include <ConditionalMacros.h>
#include <Endian.h>
#include <QuickTimeComponents.h>
#include <ImageCodec.h>
#endif /* __APPLE_CC__ */

#include "decoder_types.h"
#include "theora_versions.h"

#include "data_types.h"
#include "TheoraDecoder.h"
#include "debug.h"

#if !TARGET_OS_MAC
#undef pascal
#define pascal
#endif

static OSStatus CopyPlanarYCbCr420ToChunkyYUV422(size_t width, size_t height, th_ycbcr_buffer pb, UInt8 *baseAddr_2vuy, long rowBytes_2vuy,
                                                 size_t offset_x, size_t offset_y);
static OSStatus CopyPlanarYCbCr422ToChunkyYUV422(size_t width, size_t height, th_ycbcr_buffer pb, UInt8 *baseAddr_2vuy, long rowBytes_2vuy,
                                                 size_t offset_x, size_t offset_y);
static OSStatus CopyPlanarYCbCr444ToChunkyYUV422(size_t width, size_t height, th_ycbcr_buffer pb, UInt8 *baseAddr_2vuy, long rowBytes_2vuy,
                                                 size_t offset_x, size_t offset_y);
static OSErr CopyPlanarYCbCr422ToPlanarYUV422(th_ycbcr_buffer ycbcr, ICMDataProcRecordPtr dataProc, UInt8 *baseAddr, long stride, long width, long height,
                                              size_t offset_x, size_t offset_y);

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME() 		Theora_ImageCodec
#define IMAGECODEC_GLOBALS() 		Theora_Globals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"TheoraDecoderDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)

#if defined(__APPLE_CC__)
#include <CoreServices/Components.k.h>
#include <QuickTime/ImageCodec.k.h>
#include <QuickTime/ComponentDispatchHelper.c>
#else
#include <Components.k.h>
#include <ImageCodec.k.h>
#include <ComponentDispatchHelper.c>
#endif /* __APPLE_CC__ */


OSErr grow_buffer(Theora_Globals glob, UInt32 min_size) {
    /* increase the size of the packet buffer, in
       kPacketBufferAllocIncrement blocks, preserving the content of
       the used part of the buffer */
    OSErr err = noErr;
    UInt32 new_size = glob->p_buffer_len;

    dbg_printf("--:Theora:-  _grow_buffer(%08lx, %ld)\n", (long)glob, min_size);

    while (new_size < min_size)
        new_size += kPacketBufferAllocIncrement;

    /* first try to resize in-place */
    SetPtrSize((Ptr) glob->p_buffer, new_size);

    if (err = MemError()) {
        /* resizing failed: allocate new block, memcpy, release the old block */
        Ptr p = NewPtr(new_size);
        if (err = MemError()) goto bail;

        BlockMoveData(glob->p_buffer, p, glob->p_buffer_used);

        DisposePtr((Ptr) glob->p_buffer);
        glob->p_buffer = (UInt8 *) p;
    }

    glob->p_buffer_len = new_size;

 bail:
    if (err)
        dbg_printf("--:Theora:-  _grow_buffer(%08lx, %ld) failed = %d\n", (long)glob, min_size, err);

    return err;
}


OSErr init_theora_decoder(Theora_Globals glob, CodecDecompressParams *p)
{
    OSErr err = noErr;
    Handle ext;
    //OggSerialNoAtom *atom;
    Byte *ptrheader, *mCookie, *cend;
    UInt32 mCookieSize;
    CookieAtomHeader *aheader;
    th_comment tc;
    ogg_packet header, header_tc, header_cb;

    if (glob->info_initialised) {
        dbg_printf("--:Theora:- Decoder already initialised, skipping...\n");
        return err;
    }

    err = GetImageDescriptionExtension(p->imageDescription, &ext, kSampleDescriptionExtensionTheora, 1);
    if (err != noErr) {
        dbg_printf("XXX GetImageDescriptionExtension() failed! ('%4.4s')\n", &(*p->imageDescription)->cType);
        err = codecBadDataErr;
        return err;
    }

    mCookie = (UInt8 *) *ext;
    mCookieSize = GetHandleSize(ext);

    ptrheader = mCookie;
    cend = mCookie + mCookieSize;

    aheader = (CookieAtomHeader*)ptrheader;


    header.bytes = header_tc.bytes = header_cb.bytes = 0;

    while (ptrheader < cend) {
        aheader = (CookieAtomHeader *) ptrheader;
        ptrheader += EndianU32_BtoN(aheader->size);
        if (ptrheader > cend || EndianU32_BtoN(aheader->size) <= 0)
            break;

        switch(EndianS32_BtoN(aheader->type)) {
        case kCookieTypeTheoraHeader:
            header.b_o_s = 1;
            header.e_o_s = 0;
            header.granulepos = 0;
            header.packetno = 0;
            header.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
            header.packet = aheader->data;
            break;

        case kCookieTypeTheoraComments:
            header_tc.b_o_s = 0;
            header_tc.e_o_s = 0;
            header_tc.granulepos = 0;
            header_tc.packetno = 1;
            header_tc.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
            header_tc.packet = aheader->data;
            break;

        case kCookieTypeTheoraCodebooks:
            header_cb.b_o_s = 0;
            header_cb.e_o_s = 0;
            header_cb.granulepos = 0;
            header_cb.packetno = 2;
            header_cb.bytes = EndianS32_BtoN(aheader->size) - 2 * sizeof(long);
            header_cb.packet = aheader->data;
            break;

        default:
            break;
        }
    }

    err = codecBadDataErr;

    if (header.bytes == 0 || header_tc.bytes == 0 || header_cb.bytes == 0)
        return err;

    th_info_init(&glob->ti);
    th_comment_init(&tc);
    glob->ts = NULL;

    if (th_decode_headerin(&glob->ti, &tc, &glob->ts, &header) < 0) {

        if (glob->ts != NULL)
            th_setup_free (glob->ts);
        th_comment_clear(&tc);
        th_info_clear(&glob->ti);

        return err;
    }

    th_decode_headerin(&glob->ti, &tc, &glob->ts, &header_tc);
    th_decode_headerin(&glob->ti, &tc, &glob->ts, &header_cb);

    err = noErr;

    th_comment_clear(&tc);

    dbg_printf("--:Theora:- OK, managed to initialize the decoder somehow...\n");
    glob->info_initialised = true;

    return err;
}


pascal ComponentResult Theora_ImageCodecOpen(Theora_Globals glob, ComponentInstance self)
{
	ComponentResult err;

	glob = (Theora_Globals)NewPtrClear(sizeof(Theora_GlobalsRecord));
        dbg_printf("\n--:Theora:- CodecOpen(%08lx) called\n", (long)glob);
	if (err = MemError()) goto bail;

	SetComponentInstanceStorage(self, (Handle)glob);

	glob->self = self;
	glob->target = self;
	glob->wantedDestinationPixelTypeH = (OSType **)NewHandle(sizeof(OSType) * (kNumPixelFormatsSupported + 1));
	if (err = MemError()) goto bail;
	glob->drawBandUPP = NULL;
        glob->info_initialised = false;
        glob->last_frame = -1;

        glob->p_buffer = NewPtr(kPacketBufferAllocIncrement);
        glob->p_buffer_len = kPacketBufferAllocIncrement;
        glob->p_buffer_used = 0;

        // many of the functions are delegated actually
	err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &glob->delegateComponent);
	if (err) goto bail;

	ComponentSetTarget(glob->delegateComponent, self);

bail:
	return err;
}

pascal ComponentResult Theora_ImageCodecClose(Theora_Globals glob, ComponentInstance self)
{
    dbg_printf("--:Theora:- CodecClose(%08lx) called\n\n", (long)glob);
	// Make sure to close the base component and dealocate our storage
	if (glob) {
		if (glob->delegateComponent) {
			CloseComponent(glob->delegateComponent);
		}
		if (glob->wantedDestinationPixelTypeH) {
			DisposeHandle((Handle)glob->wantedDestinationPixelTypeH);
		}
		if (glob->drawBandUPP) {
			DisposeImageCodecMPDrawBandUPP(glob->drawBandUPP);
		}

                if (glob->p_buffer) {
                    DisposePtr((Ptr) glob->p_buffer);
                    glob->p_buffer = NULL;
                }
		DisposePtr((Ptr)glob);
	}

	return noErr;
}

pascal ComponentResult Theora_ImageCodecVersion(Theora_Globals glob)
{
#pragma unused(glob)
    return kTheora_imdc_Version;
}

pascal ComponentResult Theora_ImageCodecTarget(Theora_Globals glob, ComponentInstance target)
{
    glob->target = target;
    return noErr;
}

pascal ComponentResult Theora_ImageCodecInitialize(Theora_Globals glob, ImageSubCodecDecompressCapabilities *cap)
{
    dbg_printf("--:Theora:- CodecInitalize(%08lx) called (Qsize: %d)\n", (long)glob, cap->suggestedQueueSize);

    cap->decompressRecordSize = sizeof(Theora_DecompressRecord);
    cap->canAsync = true;

    if (cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames) ) {
        cap->subCodecIsMultiBufferAware = true;
        cap->baseCodecShouldCallDecodeBandForAllFrames = true;

        // the following setting is not entirely true, but some applications seem to need it
        cap->subCodecSupportsOutOfOrderDisplayTimes = true; // ?!!
    }

    return noErr;
}

pascal ComponentResult Theora_ImageCodecPreflight(Theora_Globals glob, CodecDecompressParams *p)
{
    CodecCapabilities *capabilities = p->capabilities;
    OSTypePtr         formats = *glob->wantedDestinationPixelTypeH;
    OSErr             ret = noErr;

    dbg_printf("[TD  ]  >> [%08lx] :: CodecPreflight() called (seqid: %08lx, frN: %8ld, first: %d, data1: %02x, flags: %08lx, flags2: %08lx)\n",
               (long) glob, p->sequenceID, p->frameNumber, (p->conditionFlags & codecConditionFirstFrame) != 1, p->bufferSize > 1 ? p->data[1] : 0,
               capabilities->flags, capabilities->flags2);
    dbg_printf("         :- image: %dx%d, pixform: %x\n", (**p->imageDescription).width, (**p->imageDescription).height, glob->ti.pixel_fmt);

    /* only decode full images at the moment */
    capabilities->bandMin = (**p->imageDescription).height;
    capabilities->bandInc = capabilities->bandMin;

    capabilities->wantedPixelSize  = 0;
    p->wantedDestinationPixelTypes = glob->wantedDestinationPixelTypeH;

    capabilities->extendWidth = 0;
    capabilities->extendHeight = 0;

    ret = init_theora_decoder(glob, p);

    if (ret == noErr) {
        *formats++  = k422YpCbCr8PixelFormat;
        if (glob->ti.pixel_fmt == TH_PF_420)
            *formats++  = kYUV420PixelFormat;
        *formats++	= 0;
    }

    dbg_printf("[TD  ] <   [%08lx] :: CodecPreflight() = %ld\n", (long) glob, ret);
    return ret;
}

pascal ComponentResult Theora_ImageCodecBeginBand(Theora_Globals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{
#pragma unused(flags)
    long offsetH, offsetV;
    Theora_DecompressRecord *myDrp = (Theora_DecompressRecord *)drp->userDecompressRecord;

    dbg_printf("--:Theora:- CodecBeginBand(%08lx, %08lx, %08lx) called (seqid: %08lx, frN: %8ld, first: %d, data1: %02x[%02x]) (pixF: '%4.4s') (complP: %8lx)\n",
               (long)glob, (long)drp, (long)myDrp, p->sequenceID, p->frameNumber, (p->conditionFlags & codecConditionFirstFrame) != 1,
               p->bufferSize > 1 ? p->data[1] : 0, p->bufferSize > 1 ? ((unsigned char *)drp->codecData)[1] : 0, &p->dstPixMap.pixelFormat, p->completionProcRecord);
    if (p->frameTime != NULL) {
        dbg_printf("--:Theora:-      BeginBand::frameTime: scale: %8ld, duration: %8ld, rate: %5ld.%05ld (vd: %8ld) [fl: %02lx]\n",
                   p->frameTime->scale, p->frameTime->duration, p->frameTime->rate >> 16, p->frameTime->rate & 0xffff,
                   (p->frameTime->flags & icmFrameTimeHasVirtualStartTimeAndDuration) ? p->frameTime->virtualDuration : -1,
                   p->frameTime->flags);
    }

#if 0
    switch (p->dstPixMap.pixelFormat) {
    case k422YpCbCr8PixelFormat:
        offsetH = (long)(p->dstRect.left - p->dstPixMap.bounds.left) * (long)(p->dstPixMap.pixelSize >> 3);
        offsetV = (long)(p->dstRect.top - p->dstPixMap.bounds.top) * (long)drp->rowBytes;

        drp->baseAddr = p->dstPixMap.baseAddr + offsetH + offsetV;
        break;

    case kYUV420PixelFormat:
        //drp->baseAddr = p->dstPixMap.baseAddr;
        break;

    default:
        //should not happen!
        return codecErr;
    }
#endif /* 0 */

    /* importer should send us samples prepended by a single pading
       byte (this is because QT doesn't allow zero-sized samples) and
       it is PREpended because there is ALWAYS some data BEFORE a
       packet but not necesarrily after */
    /* p->bufferSize -= 1; */
    drp->codecData += 1;

    if (p->bufferSize > 1 && ((unsigned char *)drp->codecData)[0] & 0x40 == 0)
        drp->frameType = kCodecFrameTypeKey;
    else
        drp->frameType = kCodecFrameTypeDifference;

    myDrp->width = (**p->imageDescription).width;
    myDrp->height = (**p->imageDescription).height;
    myDrp->depth = (**p->imageDescription).depth;
    myDrp->dataSize = p->bufferSize - 1;
    myDrp->frameNumber = p->frameNumber;
    myDrp->pixelFormat = p->dstPixMap.pixelFormat;
    myDrp->draw = 0;
    myDrp->decoded = 0;
    if (p->frameTime != NULL && (p->frameTime->flags & icmFrameAlreadyDecoded) != 0)
        myDrp->decoded = 1;

    if (glob->last_frame < 0) {
        dbg_printf("--:Theora:-  calling theora_decode_init()...\n");
        glob->td = th_decode_alloc(&glob->ti, glob->ts);
        glob->last_frame = 0;
    }

    return noErr;
}

pascal ComponentResult Theora_ImageCodecDecodeBand(Theora_Globals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
    OSErr err = noErr;
    Theora_DecompressRecord *myDrp = (Theora_DecompressRecord *)drp->userDecompressRecord;
    unsigned char *dataPtr = (unsigned char *)drp->codecData;
    SInt32 dataConsumed = 0;
    ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : NULL;
    SInt32 dataAvailable = dataProc != NULL ? codecMinimumDataSize : -1;

    dbg_printf("--:Theora:-  CodecDecodeBand(%08lx, %08lx, %08lx) cald (                 frN: %8ld, dataProc: %8lx)\n",
               (long)glob, (long)drp, (long)myDrp, myDrp->frameNumber, (long)dataProc);

    if (dataAvailable > -1) {
#if 1
        dbg_printf("       ! ! ! CodecDecodeBand(): NOT IMPLEMENTED - use dataProc to load data!\n");
        err = codecErr;
#else
        // TODO: implement using dataProc for loading data if not all available at once
        unsigned char *tmpDataPtr = dataPtr;
        UInt32 bytesToLoad = myDrp->dataSize;
        while (dataAvailable > 0) {
            err = dataProc->dataProc((Ptr *)&dataPtr, dataNeeded, dataProc->dataRefCon);
            if (err == eofErr)
                err = noErr;
        }
#endif /* 1 */
    }

    if (err == noErr) {
        ogg_packet op;
        int terr;
        Boolean do_decode = true;
        Boolean continued = (glob->p_buffer_used > 0);
        UInt8 *data_buffer = dataPtr;
        UInt32 data_size = myDrp->dataSize;

        if (glob->last_frame + 1 != myDrp->frameNumber) {
            glob->p_buffer_used = 0;
            continued = false;
        }

        glob->last_frame = myDrp->frameNumber;

        if (myDrp->dataSize % 255 == 4) {
            if (!memcmp(dataPtr + myDrp->dataSize - 4, "OggS", 4)) {
                do_decode = false;
                if (myDrp->dataSize - 4 + glob->p_buffer_used > glob->p_buffer_len) {
                    err = grow_buffer(glob, myDrp->dataSize - 4 + glob->p_buffer_used);
                }
                /* if the we failed to resize, the "frame" will get dropped anyway, otherwise... */
                if (!err) {
                    BlockMoveData(dataPtr, glob->p_buffer + glob->p_buffer_used, myDrp->dataSize - 4);
                    glob->p_buffer_used += myDrp->dataSize - 4;
                }
            } else {
                myDrp->draw = 1;
            }
        } else if (myDrp->dataSize == 0 && !continued) {
            myDrp->draw = 1;
            drp->frameType = kCodecFrameTypeDroppableDifference;
        } else {
            myDrp->draw = 1;
        }

        dataConsumed = myDrp->dataSize;

        if (do_decode && continued) {
            /* this should be the last fragment */
            if (myDrp->dataSize > 0)
                BlockMoveData(dataPtr, glob->p_buffer + glob->p_buffer_used, myDrp->dataSize);
            data_size = myDrp->dataSize + glob->p_buffer_used;
            glob->p_buffer_used = 0;
            data_buffer = glob->p_buffer;
        }

        if (myDrp->draw == 0)
            err = codecDroppedFrameErr;
        else {
            op.b_o_s = 0;
            op.e_o_s = 0;
            op.granulepos = -1;
            op.packetno = myDrp->frameNumber + 3;
            op.bytes = data_size;
            op.packet = data_buffer;
            terr = th_decode_packetin(glob->td, &op, NULL);
            dbg_printf("--:Theora:-  theora_decode_packetin(pktno: %lld, size: %ld, data1: [%02x]) = %d\n", op.packetno, op.bytes, data_buffer[0], terr);

            if (terr != 0) {
                myDrp->draw = 0;
                err = codecDroppedFrameErr;
            }
        }
    }

    myDrp->decoded = 1;
    drp->codecData += dataConsumed;
    return err;
}


pascal ComponentResult Theora_ImageCodecDrawBand(Theora_Globals glob, ImageSubCodecDecompressRecord *drp)
{
    OSErr err = noErr;
    Theora_DecompressRecord *myDrp = (Theora_DecompressRecord *)drp->userDecompressRecord;
    unsigned char *dataPtr = (unsigned char *)drp->codecData;
    ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : NULL;

    dbg_printf("--:Theora:-  CodecDrawBand(%08lx, %08lx, %08lx) called (                 frN: %8ld, dataProc: %8lx)\n",
               (long)glob, (long)drp, (long)myDrp, myDrp->frameNumber, (long)dataProc);

    if (myDrp->decoded == 0) {
        err = Theora_ImageCodecDecodeBand(glob, drp, 0);
    }

    if (err == noErr) {
        if (myDrp->draw == 0) {
            err = codecDroppedFrameErr;
        } else  {
            th_ycbcr_buffer ycbcrB;
            dbg_printf("--:Theora:-  calling theora_decode_YUVout()...\n");
            th_decode_ycbcr_out(glob->td, ycbcrB);
            if (myDrp->pixelFormat == k422YpCbCr8PixelFormat) {
                if (glob->ti.pixel_fmt == TH_PF_420) {
                    err = CopyPlanarYCbCr420ToChunkyYUV422(myDrp->width, myDrp->height, ycbcrB, (UInt8 *)drp->baseAddr, drp->rowBytes, glob->ti.pic_x, glob->ti.pic_y);
                } else if (glob->ti.pixel_fmt == TH_PF_422) {
                    err = CopyPlanarYCbCr422ToChunkyYUV422(myDrp->width, myDrp->height, ycbcrB, (UInt8 *)drp->baseAddr, drp->rowBytes, glob->ti.pic_x, glob->ti.pic_y);
                } else if (glob->ti.pixel_fmt == TH_PF_444) {
                    err = CopyPlanarYCbCr444ToChunkyYUV422(myDrp->width, myDrp->height, ycbcrB, (UInt8 *)drp->baseAddr, drp->rowBytes, glob->ti.pic_x, glob->ti.pic_y);
                } else {
                    dbg_printf("--:Theora:-  'What PLANET is this!?' (%d)\n", glob->ti.pixel_fmt);
                    err = codecBadDataErr;
                }
            } else if (myDrp->pixelFormat == kYUV420PixelFormat) {
                err = CopyPlanarYCbCr422ToPlanarYUV422(ycbcrB, dataProc, (UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, glob->ti.pic_x, glob->ti.pic_y);
            } else {
                dbg_printf("--:Theora:-  'Again, What PLANET is this!?' (%lx)\n", myDrp->pixelFormat);
                err = codecBadDataErr;
            }
        }
    }

    return err;
}

pascal ComponentResult Theora_ImageCodecEndBand(Theora_Globals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
#pragma unused(glob, result, flags)
    OSErr err = noErr;
    Theora_DecompressRecord *myDrp = (Theora_DecompressRecord *)drp->userDecompressRecord;
    dbg_printf("--:Theora:-   CodecEndBand(%08lx, %08lx, %08lx, %08lx) called\n", (long)glob, (long)drp, (long)drp->userDecompressRecord, result);

    if (myDrp->draw == 0)
        err = codecDroppedFrameErr;
    return err;
}

pascal ComponentResult Theora_ImageCodecQueueStarting(Theora_Globals glob)
{
#pragma unused(glob)
    dbg_printf("--:Theora:- CodecQueueStarting(%08lx) called\n", (long)glob);

    return noErr;
}

pascal ComponentResult Theora_ImageCodecQueueStopping(Theora_Globals glob)
{
#pragma unused(glob)
    dbg_printf("--:Theora:- CodecQueueStopping(%08lx) called\n", (long)glob);

    return noErr;
}

pascal ComponentResult Theora_ImageCodecGetCompressedImageSize(Theora_Globals glob, ImageDescriptionHandle desc,
                                                               Ptr data, long dataSize, ICMDataProcRecordPtr dataProc, long *size)
{
#pragma	unused(glob,dataSize,dataProc,desc)
    dbg_printf("--:Theora:- CodecGetCompressedImageSize(%08lx) called (dataSize: %8ld)\n", (long)glob, dataSize);

    if (size == NULL)
        return paramErr;

    //size = 0;
    //return noErr;
    return unimpErr;
}

pascal ComponentResult Theora_ImageCodecGetCodecInfo(Theora_Globals glob, CodecInfo *info)
{
    OSErr err = noErr;
    dbg_printf("--:Theora:- CodecGetCodecInfo(%08lx) called\n", (long)glob);

    if (info == NULL) {
        err = paramErr;
    }
    else {
        CodecInfo **tempCodecInfo;

        err = GetComponentResource((Component)glob->self, codecInfoResourceType, kTheoraDecoderResID, (Handle *)&tempCodecInfo);
        if (err == noErr) {
            *info = **tempCodecInfo;
            DisposeHandle((Handle)tempCodecInfo);
        }
    }

    return err;
}


#pragma mark-

#if TARGET_RT_BIG_ENDIAN
#define PACK_2VUY(Cb, Y1, Cr, Y2) ((UInt32) (((Cb) << 24) | ((Y1) << 16) | ((Cr) << 8) | (Y2)))
#else
#define PACK_2VUY(Cb, Y1, Cr, Y2) ((UInt32) ((Cb) | ((Y1) << 8) | ((Cr) << 16) | ((Y2) << 24)))
#endif /* TARGET_RT_BIG_ENDIAN */

OSStatus CopyPlanarYCbCr420ToChunkyYUV422(size_t width, size_t height, th_ycbcr_buffer pb, UInt8 *baseAddr_2vuy, long rowBytes_2vuy, size_t offset_x, size_t offset_y)
{
    size_t x, y;
    size_t off_x = offset_x & ~0x01, off_y = offset_y & ~0x01;
    size_t off_x2 = offset_x >> 1, off_y2 = offset_y >> 1;
    const UInt8 *lineBase_Y  = pb[0].data + off_y * pb[0].stride + off_x;
    const UInt8 *lineBase_Cb = pb[1].data + off_y2 * pb[1].stride + off_x2;
    const UInt8 *lineBase_Cr = pb[2].data + off_y2 * pb[2].stride + off_x2;
    UInt8 *lineBase_2vuy = baseAddr_2vuy;
    Boolean odd_rows = height & 0x01;

    dbg_printf("BLIT: Yw: %d, Yh: %d, Ys: %d;  w: %ld,  h: %ld; stride: %ld\n", pb[0].width, pb[0].height, pb[0].stride, width, height, rowBytes_2vuy);
    dbg_printf("BLIT: Bw: %d, Bh: %d, Bs: %d; Rw: %d, Rh: %d;     Rs: %d\n", pb[1].width, pb[1].height, pb[1].stride,
               pb[2].width, pb[2].height, pb[2].stride);

    height = height & ~0x01;

    for( y = 0; y < height; y += 2 ) {
        // Take two lines at a time.
        const UInt8 *pixelPtr_Y_top  = lineBase_Y;
        const UInt8 *pixelPtr_Y_bot  = lineBase_Y + pb[0].stride;
        const UInt8 *pixelPtr_Cb = lineBase_Cb;
        const UInt8 *pixelPtr_Cr = lineBase_Cr;
        UInt8 *pixelPtr_2vuy_top = lineBase_2vuy;
        UInt8 *pixelPtr_2vuy_bot = lineBase_2vuy + rowBytes_2vuy;
        for( x = 0; x < width; x += 2 ) {
            *((UInt32 *)pixelPtr_2vuy_top) = PACK_2VUY(*pixelPtr_Cb, *pixelPtr_Y_top, *pixelPtr_Cr, *(pixelPtr_Y_top + 1));
            pixelPtr_2vuy_top += 4;
            pixelPtr_Y_top += 2;

            *((UInt32 *)pixelPtr_2vuy_bot) = PACK_2VUY(*pixelPtr_Cb++, *pixelPtr_Y_bot, *pixelPtr_Cr++, *(pixelPtr_Y_bot + 1));
            pixelPtr_2vuy_bot += 4;
            pixelPtr_Y_bot += 2;
        }

        lineBase_Y += 2 * pb[0].stride;
        lineBase_Cb += pb[1].stride;
        lineBase_Cr += pb[2].stride;
        lineBase_2vuy += 2 * rowBytes_2vuy;
    }

    if (odd_rows) {
        // The last, odd row.
        const UInt8 *pixelPtr_Y_top  = lineBase_Y;
        const UInt8 *pixelPtr_Cb = lineBase_Cb;
        const UInt8 *pixelPtr_Cr = lineBase_Cr;
        UInt8 *pixelPtr_2vuy_top = lineBase_2vuy;
        for (x = 0; x < width; x += 2) {
            *((UInt32 *) pixelPtr_2vuy_top) = PACK_2VUY(*pixelPtr_Cb++, *pixelPtr_Y_top, *pixelPtr_Cr++, *(pixelPtr_Y_top + 1));
            pixelPtr_2vuy_top += 4;
            pixelPtr_Y_top += 2;
        }
    }

    return noErr;
}

OSStatus CopyPlanarYCbCr422ToChunkyYUV422(size_t width, size_t height, th_ycbcr_buffer pb, UInt8 *baseAddr_2vuy, long rowBytes_2vuy, size_t offset_x, size_t offset_y)
{
    size_t x, y;
    size_t off_x = offset_x & ~0x01, off_y = offset_y & ~0x01;
    size_t off_x2 = offset_x >> 1;
    const UInt8 *lineBase_Y  = pb[0].data + off_y * pb[0].stride + off_x;
    const UInt8 *lineBase_Cb = pb[1].data + off_y * pb[1].stride + off_x2;
    const UInt8 *lineBase_Cr = pb[2].data + off_y * pb[2].stride + off_x2;
    UInt8 *lineBase_2vuy = baseAddr_2vuy;
    Boolean odd_rows = height & 0x01;

    dbg_printf("BLIT> Yw: %d, Yh: %d, Ys: %d;  w: %ld,  h: %ld; stride: %ld\n", pb[0].width, pb[0].height, pb[0].stride, width, height, rowBytes_2vuy);
    dbg_printf("BLIT> Bw: %d, Bh: %d, Bs: %d; Rw: %d, Rh: %d;     Rs: %d\n", pb[1].width, pb[1].height, pb[1].stride,
               pb[2].width, pb[2].height, pb[2].stride);

    height = height & ~0x01;

    for( y = 0; y < height; y += 2 ) {
        // Take two lines at a time.
        const UInt8 *pixelPtr_Y_top  = lineBase_Y;
        const UInt8 *pixelPtr_Y_bot  = lineBase_Y  + pb[0].stride;
        const UInt8 *pixelPtr_Cb_top = lineBase_Cb;
        const UInt8 *pixelPtr_Cb_bot = lineBase_Cb + pb[1].stride;
        const UInt8 *pixelPtr_Cr_top = lineBase_Cr;
        const UInt8 *pixelPtr_Cr_bot = lineBase_Cr + pb[2].stride;
        UInt8 *pixelPtr_2vuy_top = lineBase_2vuy;
        UInt8 *pixelPtr_2vuy_bot = lineBase_2vuy + rowBytes_2vuy;
        for( x = 0; x < width; x += 2 ) {
            //*((UInt32 *)pixelPtr_2vuy_top) = (UInt32) ((*pixelPtr_Cb_top++) << 24 | ((*pixelPtr_Y_top)) << 16 | ((*pixelPtr_Cr_top++)) << 8 | ((*(pixelPtr_Y_top+1))));
            *((UInt32 *)pixelPtr_2vuy_top) = PACK_2VUY(*pixelPtr_Cb_top++, *pixelPtr_Y_top, *pixelPtr_Cr_top++, *(pixelPtr_Y_top + 1));
            pixelPtr_2vuy_top += 4;
            pixelPtr_Y_top += 2;

            //*((UInt32 *)pixelPtr_2vuy_bot) = (UInt32) ((*pixelPtr_Cb_bot++) << 24 | ((*pixelPtr_Y_bot)) << 16 | ((*pixelPtr_Cr_bot++)) << 8 | ((*(pixelPtr_Y_bot+1))));
            *((UInt32 *)pixelPtr_2vuy_bot) = PACK_2VUY(*pixelPtr_Cb_bot++, *pixelPtr_Y_bot, *pixelPtr_Cr_bot++, *(pixelPtr_Y_bot + 1));
            pixelPtr_2vuy_bot += 4;
            pixelPtr_Y_bot += 2;
        }

        lineBase_Y += 2 * pb[0].stride;
        lineBase_Cb += 2 * pb[1].stride;
        lineBase_Cr += 2 * pb[2].stride;
        lineBase_2vuy += 2 * rowBytes_2vuy;
    }

    if (odd_rows) {
        // The last, odd row.
        const UInt8 *pixelPtr_Y_top  = lineBase_Y;
        const UInt8 *pixelPtr_Cb_top = lineBase_Cb;
        const UInt8 *pixelPtr_Cr_top = lineBase_Cr;
        UInt8 *pixelPtr_2vuy_top = lineBase_2vuy;
        for (x = 0; x < width; x += 2) {
            *((UInt32 *) pixelPtr_2vuy_top) = PACK_2VUY(*pixelPtr_Cb_top++, *pixelPtr_Y_top, *pixelPtr_Cr_top++, *(pixelPtr_Y_top + 1));
            pixelPtr_2vuy_top += 4;
            pixelPtr_Y_top += 2;
        }
    }

    return noErr;
}


/* !!: At the moment this function does nice 'decimation' rather than subsampling!!(?)
   TODO: proper subsampling? */
OSStatus CopyPlanarYCbCr444ToChunkyYUV422(size_t width, size_t height, th_ycbcr_buffer pb, UInt8 *baseAddr_2vuy, long rowBytes_2vuy, size_t offset_x, size_t offset_y)
{
    size_t x, y;
    size_t off_x = offset_x & ~0x01, off_y = offset_y & ~0x01;
    const UInt8 *lineBase_Y  = pb[0].data + off_y * pb[0].stride + off_x;
    const UInt8 *lineBase_Cb = pb[1].data + off_y * pb[1].stride + off_x;
    const UInt8 *lineBase_Cr = pb[2].data + off_y * pb[2].stride + off_x;
    UInt8 *lineBase_2vuy = baseAddr_2vuy;
    Boolean odd_rows = height & 0x01;

    dbg_printf("BLIT? Yw: %d, Yh: %d, Ys: %d;  w: %ld,  h: %ld; stride: %ld\n", pb[0].width, pb[0].height, pb[0].stride, width, height, rowBytes_2vuy);
    dbg_printf("BLIT? Bw: %d, Bh: %d, Bs: %d; Rw: %d, Rh: %d;     Rs: %d\n", pb[1].width, pb[1].height, pb[1].stride,
               pb[2].width, pb[2].height, pb[2].stride);

    height = height & ~0x01;

    for( y = 0; y < height; y += 2 ) {
        // Take two lines at a time.
        const UInt8 *pixelPtr_Y_top  = lineBase_Y;
        const UInt8 *pixelPtr_Y_bot  = lineBase_Y  + pb[0].stride;
        const UInt8 *pixelPtr_Cb_top = lineBase_Cb;
        const UInt8 *pixelPtr_Cb_bot = lineBase_Cb + pb[1].stride;
        const UInt8 *pixelPtr_Cr_top = lineBase_Cr;
        const UInt8 *pixelPtr_Cr_bot = lineBase_Cr + pb[2].stride;
        UInt8 *pixelPtr_2vuy_top = lineBase_2vuy;
        UInt8 *pixelPtr_2vuy_bot = lineBase_2vuy + rowBytes_2vuy;
        for( x = 0; x < width; x += 2 ) {
            *((UInt32 *)pixelPtr_2vuy_top) = PACK_2VUY(*pixelPtr_Cb_top, *pixelPtr_Y_top, *pixelPtr_Cr_top, *(pixelPtr_Y_top + 1));
            pixelPtr_2vuy_top += 4;
            pixelPtr_Y_top += 2;
            pixelPtr_Cb_top += 2;
            pixelPtr_Cr_top += 2;

            *((UInt32 *)pixelPtr_2vuy_bot) = PACK_2VUY(*pixelPtr_Cb_bot, *pixelPtr_Y_bot, *pixelPtr_Cr_bot, *(pixelPtr_Y_bot + 1));
            pixelPtr_2vuy_bot += 4;
            pixelPtr_Y_bot += 2;
            pixelPtr_Cb_bot += 2;
            pixelPtr_Cr_bot += 2;
        }

        lineBase_Y += 2 * pb[0].stride;
        lineBase_Cb += 2 * pb[1].stride;
        lineBase_Cr += 2 * pb[2].stride;
        lineBase_2vuy += 2 * rowBytes_2vuy;
    }

    if (odd_rows) {
        // The last, odd row.
        const UInt8 *pixelPtr_Y_top  = lineBase_Y;
        const UInt8 *pixelPtr_Cb_top = lineBase_Cb;
        const UInt8 *pixelPtr_Cr_top = lineBase_Cr;
        UInt8 *pixelPtr_2vuy_top = lineBase_2vuy;
        for (x = 0; x < width; x += 2) {
            *((UInt32 *) pixelPtr_2vuy_top) = PACK_2VUY(*pixelPtr_Cb_top, *pixelPtr_Y_top, *pixelPtr_Cr_top, *(pixelPtr_Y_top + 1));
            pixelPtr_2vuy_top += 4;
            pixelPtr_Y_top += 2;
            pixelPtr_Cb_top += 2;
            pixelPtr_Cr_top += 2;
        }
    }

    return noErr;
}

/* Presently, This function assumes YCbCr 4:2:0 as input.
   TODO: take into account different subsampling types? */
OSErr CopyPlanarYCbCr422ToPlanarYUV422(th_ycbcr_buffer ycbcr, ICMDataProcRecordPtr dataProc, UInt8 *baseAddr, long stride, long width, long height,
                                       size_t offset_x, size_t offset_y)
{
    OSErr err = noErr;
    UInt8 *endOfScanLine, *dst_base, *src_base;
    PlanarPixmapInfoYUV420 *pinfo = (PlanarPixmapInfoYUV420 *) baseAddr;
    UInt32 lines;
    SInt32 dst_stride, src_stride;
    size_t off_x = offset_x & ~0x01, off_y = offset_y & ~0x01;
    size_t off_x2 = offset_x >> 1;
    endOfScanLine = baseAddr + (width * 4);

    dbg_printf("BLIT= yw: %d, yh: %d, ys: %d; w: %ld, h: %ld; stride: %ld\n", ycbcr[0].width, ycbcr[0].height, ycbcr[0].stride, width, height, stride);
    dbg_printf("BLIT= Bw: %d, Bh: %d, Bs: %d; Rw: %d, Rh: %d;     Rs: %d\n", ycbcr[1].width, ycbcr[1].height, ycbcr[1].stride,
               ycbcr[2].width, ycbcr[2].height, ycbcr[2].stride);

    lines = height;
    dst_base = baseAddr + pinfo->componentInfoY.offset;
    dst_stride = pinfo->componentInfoY.rowBytes;
    src_base = ycbcr[0].data + off_y * ycbcr[0].stride + off_x;
    src_stride = ycbcr[0].stride;
    while (lines-- > 0) {
        BlockMoveData(src_base, dst_base, width);
        src_base += src_stride;
        dst_base += dst_stride;
    }

    lines = height / 2;
    dst_base = baseAddr + pinfo->componentInfoCb.offset;
    dst_stride = pinfo->componentInfoCb.rowBytes;
    src_base = ycbcr[1].data + off_y * ycbcr[1].stride + off_x2;
    src_stride = ycbcr[1].stride;
    while (lines-- > 0) {
        BlockMoveData(src_base, dst_base, width);
        src_base += src_stride;
        dst_base += dst_stride;
    }

    lines = height / 2;
    dst_base = baseAddr + pinfo->componentInfoCr.offset;
    dst_stride = pinfo->componentInfoCr.rowBytes;
    src_base = ycbcr[2].data + off_y * ycbcr[2].stride + off_x2;;
    src_stride = ycbcr[2].stride;
    while (lines-- > 0) {
        BlockMoveData(src_base, dst_base, width);
        src_base += src_stride;
        dst_base += dst_stride;
    }

    return err;
}
