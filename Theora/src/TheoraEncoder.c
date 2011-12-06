/*
 *  TheoraEncoder.c
 *
 *    Theora video encoder (ImageCodec) implementation.
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
 *  Last modified: $Id: TheoraEncoder.c 12346 2007-01-18 13:45:42Z arek $
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

#include "encoder_types.h"
#include "theora_versions.h"

#include "data_types.h"
#include "TheoraEncoder.h"
#include "debug.h"

#if !TARGET_OS_MAC
#undef pascal
#define pascal
#endif

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME()       Theora_ImageEncoder
#define IMAGECODEC_GLOBALS() 	    TheoraGlobalsPtr storage

#define CALLCOMPONENT_BASENAME()    IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()	    IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()	    uppImageCodec
#define COMPONENT_DISPATCH_FILE	    "TheoraEncoderDispatch.h"
#define COMPONENT_SELECT_PREFIX()   kImageCodec


#if defined(__APPLE_CC__)
#include <CoreServices/Components.k.h>
#include <QuickTime/ImageCodec.k.h>
#include <QuickTime/ImageCompression.k.h>
#include <QuickTime/ComponentDispatchHelper.c>
#else
#include <Components.k.h>
#include <ImageCodec.k.h>
#include <ImageCompression.k.h>
#include <ComponentDispatchHelper.c>
#endif /* __APPLE_CC__ */


ComponentResult
yuv_copy__422_to_420(void *b_2vuy, SInt32 b_2vuy_stride, size_t width, size_t height,
                     size_t offset_x, size_t offset_y, yuv_buffer *dst);
ComponentResult
vuy_copy__422_to_420(void *b_2vuy, SInt32 b_2vuy_stride, size_t width, size_t height,
                     size_t offset_x, size_t offset_y, yuv_buffer *dst);
ComponentResult create_pb_attribs(SInt32 width, SInt32 height, const OSType *pixelFormatList,
                                  int pixelFormatCount, CFMutableDictionaryRef *pixelBufferAttributesOut);


ComponentResult
Theora_ImageEncoderOpen(TheoraGlobalsPtr globals, ComponentInstance self)
{
    ComponentResult err = noErr;
    dbg_printf("\n[  TE]  >> [%08lx] :: Open()\n", (UInt32) globals);

    globals = (TheoraGlobalsPtr) calloc(1, sizeof(TheoraGlobals));
    if (!globals)
        err = memFullErr;

    if (!err) {
        SetComponentInstanceStorage(self, (Handle) globals);

        globals->self = self;
        globals->target = self;

        globals->yuv_buffer_size = 0;
        globals->yuv_buffer = NULL;

        globals->set_quality = codecNormalQuality;
        globals->set_fps_numer = 0;
        globals->set_fps_denom = 0;
        globals->set_bitrate = 0;
        globals->set_keyrate = 64;

        globals->set_sharp = 2;
        globals->set_quick = 1;
    }

    dbg_printf("[  TE] <   [%08lx] :: Open() = %ld\n", (UInt32) globals, err);
    return err;
}

ComponentResult
Theora_ImageEncoderClose(TheoraGlobalsPtr globals, ComponentInstance self)
{
    ComponentResult err = noErr;
    dbg_printf("[  TE]  >> [%08lx] :: Close()\n", (UInt32) globals);

    if (globals) {

        if (globals->yuv_buffer)
            free(globals->yuv_buffer);

        free(globals);
        globals = NULL;
    }

    dbg_printf("[  TE] <   [%08lx] :: Close() = %ld\n", (UInt32) globals, err);
    return err;
}

ComponentResult Theora_ImageEncoderVersion(TheoraGlobalsPtr globals)
{
#pragma unused(globals)
    return kTheora_imco_Version;
}

ComponentResult
Theora_ImageEncoderTarget(TheoraGlobalsPtr globals, ComponentInstance target)
{
    globals->target = target;
    return noErr;
}

ComponentResult
Theora_ImageEncoderGetCodecInfo(TheoraGlobalsPtr globals, CodecInfo *info)
{
    ComponentResult err = noErr;
    dbg_printf("[  TE]  >> [%08lx] :: GetCodecInfo()\n", (UInt32) globals);

    if (info == NULL) {
        err = paramErr;
    } else {
        CodecInfo **tempCodecInfo;

        err = GetComponentResource((Component) globals->self,
                                   codecInfoResourceType, kTheoraEncoderResID,
                                   (Handle *) &tempCodecInfo);
        if (!err) {
            *info = **tempCodecInfo;
            DisposeHandle((Handle) tempCodecInfo);
        }
    }

    dbg_printf("[  TE] <   [%08lx] :: GetCodecInfo() = %ld\n", (UInt32) globals, err);
    return err;
}

ComponentResult
Theora_ImageEncoderGetMaxCompressionSize(TheoraGlobalsPtr globals,
                                         PixMapHandle src, const Rect *srcRect,
                                         short depth, CodecQ quality,
                                         long *size)
{
    ComponentResult err = noErr;
    dbg_printf("[  TE]  >> [%08lx] :: GetMaxCompressionSize()\n", (UInt32) globals);

    if (!size) {
        err = paramErr;
    } else {
        *size = (((srcRect->right - srcRect->left) + 0x0f) & ~0x0f) *
            (((srcRect->bottom - srcRect->top) + 0x0f) & ~0x0f) *
            3; // ?!?
    }

    dbg_printf("[  TE] <   [%08lx] :: GetMaxCompressionSize() = %ld [%ld]\n", (UInt32) globals, err, *size);
    return err;
}

ComponentResult
Theora_ImageEncoderRequestSettings(TheoraGlobalsPtr globals, Handle settings,
                                   Rect *rp, ModalFilterUPP filterProc)
{
    ComponentResult err = noErr;

    dbg_printf("[  TE]  >> [%08lx] :: RequestSettings()\n", (UInt32) globals);

    err = badComponentSelector;

    dbg_printf("[  TE] <   [%08lx] :: RequestSettings() = %ld\n", (UInt32) globals, err);

    return err;
}

ComponentResult
Theora_ImageEncoderGetSettings(TheoraGlobalsPtr globals, Handle settings)
{
    ComponentResult err = noErr;

    dbg_printf("[  TE]  >> [%08lx] :: GetSettings()\n", (UInt32) globals);

    if (!settings) {
        err = paramErr;
    } else {
        SetHandleSize(settings, 8);
        ((UInt32 *) *settings)[0] = 'XiTh';
        ((UInt16 *) *settings)[2] = globals->set_sharp;
        ((UInt16 *) *settings)[3] = globals->set_quick;
    }

    dbg_printf("[  TE] <   [%08lx] :: GetSettings() = %ld\n", (UInt32) globals, err);

    return err;
}

ComponentResult
Theora_ImageEncoderSetSettings(TheoraGlobalsPtr globals, Handle settings)
{
    ComponentResult err = noErr;

    dbg_printf("[  TE]  >> [%08lx] :: SetSettings() %ld\n", (UInt32) globals, GetHandleSize(settings));

    if (!settings || GetHandleSize(settings) == 0) {
        globals->set_sharp = 2;
        globals->set_quick = 1;
    } else if (GetHandleSize(settings) == 8 && ((UInt32 *) *settings)[0] == 'XiTh') {
        globals->set_sharp = ((UInt16 *) *settings)[2];
        globals->set_quick = ((UInt16 *) *settings)[3];

        if (globals->set_sharp > 2)
            globals->set_sharp = 2;
        if (globals->set_quick > 1)
            globals->set_quick = 1;
    } else {
        err = paramErr;
    }

    dbg_printf("[  TE] <   [%08lx] :: SetSettings() = %ld\n", (UInt32) globals, err);

    return err;
}


ComponentResult
Theora_ImageEncoderPrepareToCompressFrames(TheoraGlobalsPtr globals,
                                           ICMCompressorSessionRef session,
                                           ICMCompressionSessionOptionsRef sessionOptions,
                                           ImageDescriptionHandle imageDescription,
                                           void *reserved,
                                           CFDictionaryRef *compressorPixelBufferAttributesOut)
{
    ComponentResult err = noErr;
    dbg_printf("[  TE]  >> [%08lx] :: PrepareToCompressFrames()\n", (UInt32) globals);

    CFMutableDictionaryRef compressorPixelBufferAttributes = NULL;
    //OSType pixelFormatList[] = { k422YpCbCr8PixelFormat,    /* '2vuy' */
    //                             kComponentVideoUnsigned }; /* 'yuvs' */
    OSType pixelFormatList[] = { k422YpCbCr8PixelFormat };    /* '2vuy' */
    Fixed gammaLevel;
    //int frameIndex;
    SInt32 widthRoundedUp, heightRoundedUp;
    UInt32 buffer_size_needed;
    int result;

    Fixed cs_fps = 0;
    CodecQ cs_quality = 0;
    SInt32 cs_bitrate = 0;
    SInt32 cs_keyrate = 0;

    // Record the compressor session for later calls to the ICM.
    // Note: this is NOT a CF type and should NOT be CFRetained or CFReleased.
    globals->session = session;

    // Retain the session options for later use.
    ICMCompressionSessionOptionsRelease(globals->sessionOptions);
    globals->sessionOptions = sessionOptions;
    ICMCompressionSessionOptionsRetain(globals->sessionOptions);

    if (globals->sessionOptions) {
        UInt64 _frames = 0;

        err = ICMCompressionSessionOptionsGetProperty(globals->sessionOptions,
                                                      kQTPropertyClass_ICMCompressionSessionOptions,
                                                      kICMCompressionSessionOptionsPropertyID_ExpectedFrameRate,
                                                      sizeof(cs_fps), &cs_fps, NULL);
        dbg_printf("[  TE] fps [%08lx] :: PrepareToCompressFrames() = %ld, %f\n", (UInt32) globals, err, cs_fps / 65536.0f);

        if (!err && cs_fps > 0.0) {
            err = ICMCompressionSessionOptionsGetProperty(globals->sessionOptions,
                                                          kQTPropertyClass_ICMCompressionSessionOptions,
                                                          kICMCompressionSessionOptionsPropertyID_AverageDataRate,
                                                          sizeof(cs_bitrate), &cs_bitrate, NULL);
            dbg_printf("[  TE]  br [%08lx] :: PrepareToCompressFrames() = %ld, %ld\n", (UInt32) globals, err, cs_bitrate);
            //cs_bitrate = 0;
        }

        if (cs_bitrate == 0) {
            err = ICMCompressionSessionOptionsGetProperty(globals->sessionOptions,
                                                          kQTPropertyClass_ICMCompressionSessionOptions,
                                                          kICMCompressionSessionOptionsPropertyID_Quality,
                                                          sizeof(cs_quality), &cs_quality, NULL);
            dbg_printf("[  TE]  qu [%08lx] :: PrepareToCompressFrames() = %ld, %ld\n", (UInt32) globals, err, cs_quality);
            if (!err && cs_quality == 0)
                cs_quality = 1;
        }

        err = ICMCompressionSessionOptionsGetProperty(globals->sessionOptions,
                                                      kQTPropertyClass_ICMCompressionSessionOptions,
                                                      kICMCompressionSessionOptionsPropertyID_MaxKeyFrameInterval,
                                                      sizeof(cs_keyrate), &cs_keyrate, NULL);
        dbg_printf("[  TE]  kr [%08lx] :: PrepareToCompressFrames() = %ld, %ld\n", (UInt32) globals, err, cs_keyrate);
        if (err)
            cs_keyrate = 0;
        else if (cs_keyrate > 32768)
            cs_keyrate = 32768;

        err = ICMCompressionSessionOptionsGetProperty(globals->sessionOptions,
                                                      kQTPropertyClass_ICMCompressionSessionOptions,
                                                      kICMCompressionSessionOptionsPropertyID_SourceFrameCount,
                                                      sizeof(_frames), &_frames, NULL);
        dbg_printf("[  TE]  f# [%08lx] :: PrepareToCompressFrames() = %ld, %lld\n", (UInt32) globals, err, _frames);

        err = noErr;
    }

    // Modify imageDescription here if needed.
    gammaLevel = kQTCCIR601VideoGammaLevel;
    //gammaLevel = kQTUseSourceGammaLevel;
    err = ICMImageDescriptionSetProperty(imageDescription,
                                         kQTPropertyClass_ImageDescription,
                                         kICMImageDescriptionPropertyID_GammaLevel,
                                         sizeof(gammaLevel),
                                         &gammaLevel);
    if (err)
        goto bail;

    // Record the dimensions from the image description.
    globals->width = (*imageDescription)->width;
    globals->height = (*imageDescription)->height;

    // Create a pixel buffer attributes dictionary.
    err = create_pb_attribs(globals->width, globals->height,
                            pixelFormatList, sizeof(pixelFormatList) / sizeof(OSType),
                            &compressorPixelBufferAttributes);
    if (err)
        goto bail;

    *compressorPixelBufferAttributesOut = compressorPixelBufferAttributes;
    compressorPixelBufferAttributes = NULL;

    globals->maxEncodedDataSize = globals->width * globals->height * 3;

    globals->keyFrameCountDown = ICMCompressionSessionOptionsGetMaxKeyFrameInterval( globals->sessionOptions );

    widthRoundedUp = (globals->width + 0x0f) & ~0x0f;
    heightRoundedUp = (globals->height + 0x0f) & ~0x0f;

    theora_info_init(&globals->ti);
    globals->ti.width = widthRoundedUp;
    globals->ti.height = heightRoundedUp;
    globals->ti.frame_width = globals->width;
    globals->ti.frame_height = globals->height;
    globals->ti.offset_x = ((widthRoundedUp - globals->width) / 2) & ~0x01;
    globals->ti.offset_y = ((heightRoundedUp - globals->height) / 2) & ~0x01;
    if (cs_fps != 0) {
        globals->ti.fps_numerator = cs_fps;
        globals->ti.fps_denominator = 0x010000;
    } else {
        globals->ti.fps_numerator = 1;
        globals->ti.fps_denominator = 1;
    }
    globals->ti.aspect_numerator = 1;
    globals->ti.aspect_denominator = 1;
    globals->ti.colorspace = OC_CS_UNSPECIFIED;
    globals->ti.pixelformat = OC_PF_420;
    if (cs_bitrate != 0) {
        globals->ti.target_bitrate = cs_bitrate * 8;
    } else {
        globals->ti.target_bitrate = 0;
    }
    if (cs_quality != 0) {
        globals->ti.quality = 64 * cs_quality / codecLosslessQuality;
    } else if (cs_bitrate == 0) {
        globals->ti.quality = 32; /* medium quality */
    } else {
        globals->ti.quality = 0;
    }

    globals->ti.quick_p = globals->set_quick;
    globals->ti.sharpness = globals->set_sharp;

    globals->ti.dropframes_p = 0;
    globals->ti.keyframe_auto_p = 1;
    if (cs_keyrate == 0) {
        globals->ti.keyframe_frequency = 64;
        globals->ti.keyframe_frequency_force = 64;
        globals->ti.keyframe_mindistance = 8;
    } else {
        globals->ti.keyframe_frequency = cs_keyrate;
        globals->ti.keyframe_frequency_force = cs_keyrate;
        globals->ti.keyframe_mindistance = 8;
        if (globals->ti.keyframe_mindistance > cs_keyrate)
            globals->ti.keyframe_mindistance = cs_keyrate;
    }
    globals->ti.keyframe_data_target_bitrate = globals->ti.target_bitrate * 1.5;
    globals->ti.keyframe_auto_threshold = 80;
    globals->ti.noise_sensitivity = 1;

    result = theora_encode_init(&globals->ts, &globals->ti);
    dbg_printf("[  TE]   i [%08lx] :: PrepareToCompressFrames() = %ld\n", (UInt32) globals, result);
    //theora_info_clear(ti);

    buffer_size_needed = globals->ti.width * globals->ti.height * 3 / 2;

    if (globals->yuv_buffer) {
        if (globals->yuv_buffer_size < buffer_size_needed) {
            globals->yuv_buffer = realloc(globals->yuv_buffer, buffer_size_needed);
            globals->yuv_buffer_size = buffer_size_needed;
        }
    } else {
        globals->yuv_buffer_size = buffer_size_needed;
        globals->yuv_buffer = malloc(buffer_size_needed);
    }

    globals->yuv.y_width = globals->ti.width;
    globals->yuv.y_height = globals->ti.height;
    globals->yuv.y_stride = globals->ti.width;

    globals->yuv.uv_width = globals->ti.width / 2;
    globals->yuv.uv_height = globals->ti.height / 2;
    globals->yuv.uv_stride = globals->ti.width / 2;

    globals->yuv.y = &globals->yuv_buffer[0];
    globals->yuv.u = &globals->yuv_buffer[0] + globals->ti.width * globals->ti.height;
    globals->yuv.v = &globals->yuv_buffer[0] + globals->ti.width * globals->ti.height * 5/4;



    if (err)
        goto bail;

    {
        ogg_packet header, header_tc, header_cb;
        //UInt32 cookie_size;
        UInt8 *cookie;
        UInt32 *qtatom;
        Handle cookie_handle;
        UInt32 atomhead[2];

        theora_encode_header(&globals->ts, &header);
        theora_comment_init(&globals->tc);

        cookie_handle = NewHandleClear(2 * sizeof(UInt32));
        cookie = (UInt8 *) *cookie_handle;

        qtatom = (UInt32 *) cookie;
        *qtatom++ = EndianU32_NtoB(header.bytes + 2 * sizeof(UInt32));
        *qtatom = EndianU32_NtoB(kCookieTypeTheoraHeader);
        PtrAndHand(header.packet, cookie_handle, header.bytes);

        theora_encode_comment(&globals->tc, &header_tc);
        atomhead[0] = EndianU32_NtoB(header_tc.bytes + 2 * sizeof(UInt32));
        atomhead[1] = EndianU32_NtoB(kCookieTypeTheoraComments);
        PtrAndHand(atomhead, cookie_handle, sizeof(atomhead));
        PtrAndHand(header_tc.packet, cookie_handle, header_tc.bytes);

        theora_encode_tables(&globals->ts, &header_cb);
        atomhead[0] = EndianU32_NtoB(header_cb.bytes + 2 * sizeof(UInt32));
        atomhead[1] = EndianU32_NtoB(kCookieTypeTheoraCodebooks);
        PtrAndHand(atomhead, cookie_handle, sizeof(atomhead));
        PtrAndHand(header_cb.packet, cookie_handle, header_cb.bytes);

        atomhead[0] = EndianU32_NtoB(2 * sizeof(UInt32));
        atomhead[1] = EndianU32_NtoB(kAudioTerminatorAtomType);
        PtrAndHand(atomhead, cookie_handle, sizeof(atomhead));

        err = AddImageDescriptionExtension(imageDescription, cookie_handle, kSampleDescriptionExtensionTheora);
        DisposeHandle(cookie_handle);
        free(header_tc.packet);
    }

 bail:
    if (compressorPixelBufferAttributes)
        CFRelease(compressorPixelBufferAttributes);

    
    dbg_printf("[  TE] <   [%08lx] :: PrepareToCompressFrames() = %ld [fps: %f, q: 0x%04lx, kr: %ld, br: %ld, #: %d, O: %d]\n",
               (UInt32) globals, err, cs_fps / 65536.0, cs_quality, cs_keyrate, cs_bitrate, globals->set_sharp, !globals->set_quick);
    return err;
}

ComponentResult
Theora_ImageEncoderEncodeFrame(TheoraGlobalsPtr globals,
                               ICMCompressorSourceFrameRef sourceFrame,
                               UInt32 flags)
{
    ComponentResult err = noErr;
    ICMCompressionFrameOptionsRef frameOptions;
    ICMMutableEncodedFrameRef encodedFrame = NULL;
    UInt8 *dataPtr = NULL;
    CVPixelBufferRef sourcePixelBuffer = NULL;
    ogg_packet op;
    MediaSampleFlags mediaSampleFlags;
    ICMFrameType frameType;
    int result;


    dbg_printf("[  TE]  >> [%08lx] :: EncodeFrame()\n", (UInt32) globals);

    /* 1. copy pixels to a theora's yuv_buffer
     * 2. theora_encode_YUVin()
     * 3. theora_encode_packetout()
     * 4. copy the packet's body back to QT's encodedBuffer */

    memset(&op, 0, sizeof(op));

    // Create the buffer for the encoded frame.
    err = ICMEncodedFrameCreateMutable(globals->session, sourceFrame, globals->maxEncodedDataSize, &encodedFrame);
    dbg_printf("[  TE]   ? [%08lx] :: EncodeFrame() = %ld [%ld]\n", (UInt32) globals, err, globals->maxEncodedDataSize);
    if (err)
        goto bail;

    dataPtr = ICMEncodedFrameGetDataPtr(encodedFrame);

    // Transfer the source frame into glob->currentFrame, converting it from chunky YUV422 to planar YUV420.
    sourcePixelBuffer = ICMCompressorSourceFrameGetPixelBuffer(sourceFrame);
    CVPixelBufferLockBaseAddress(sourcePixelBuffer, 0);
    err = yuv_copy__422_to_420(CVPixelBufferGetBaseAddress(sourcePixelBuffer),
                               CVPixelBufferGetBytesPerRow(sourcePixelBuffer),
                               globals->width, globals->height,
                               globals->ti.offset_x, globals->ti.offset_y,
                               &globals->yuv);
    CVPixelBufferUnlockBaseAddress( sourcePixelBuffer, 0 );
    //dbg_printf("[  TE]  >  [%08lx] :: EncodeFrame() = %ld\n", (UInt32) globals, err);
    if (err)
        goto bail;

    result = theora_encode_YUVin(&globals->ts, &globals->yuv);
    //dbg_printf("[  TE]  Y> [%08lx] :: EncodeFrame() = %d\n", (UInt32) globals, result);

    result = theora_encode_packetout(&globals->ts, 0, &op);
    //dbg_printf("[  TE]  T< [%08lx] :: EncodeFrame() = %d\n", (UInt32) globals, result);


    /* The first byte in the returned encoded frame packet is left
       empty, the actuall data starting at position 1 and the size
       reported is increased by one to compensate for the
       pre-padding. This is because QT doesn't allow zero-size
       packets. See also TheoraDecoder.c, Theora_ImageCodecBeginBand()
       function. */
    memcpy(dataPtr + 1, op.packet, op.bytes);

    err = ICMEncodedFrameSetDataSize(encodedFrame, op.bytes + 1);
    dbg_printf("[  TE]  =  [%08lx] :: EncodeFrame() = %ld [%ld (%ld)]\n", (UInt32) globals, err, op.bytes, op.bytes + 1);
    if (err)
        goto bail;

    mediaSampleFlags = 0;

    if (!theora_packet_iskeyframe(&op)) {
        mediaSampleFlags |= mediaSampleNotSync;
        frameType = kICMFrameType_P;
    } else {
        mediaSampleFlags |= mediaSampleDoesNotDependOnOthers;
        frameType = kICMFrameType_I;
    }

    err = ICMEncodedFrameSetMediaSampleFlags(encodedFrame, mediaSampleFlags);
    //dbg_printf("[  TE]  .  [%08lx] :: EncodeFrame() = %ld\n", (UInt32) globals, err);
    if (err)
        goto bail;

    err = ICMEncodedFrameSetFrameType(encodedFrame, frameType);
    //dbg_printf("[  TE]  *  [%08lx] :: EncodeFrame() = %ld\n", (UInt32) globals, err);
    if (err)
        goto bail;

    // Output the encoded frame.
    err = ICMCompressorSessionEmitEncodedFrame(globals->session, encodedFrame, 1, &sourceFrame);
    //dbg_printf("[  TE] ->  [%08lx] :: EncodeFrame() = %ld\n", (UInt32) globals, err);
    if (err)
        goto bail;

bail:
    // Since we created this, we must also release it.
    ICMEncodedFrameRelease(encodedFrame);

    dbg_printf("[  TE] <   [%08lx] :: EncodeFrame() = %ld\n", (UInt32) globals, err);
    return err;
}

ComponentResult
Theora_ImageEncoderCompleteFrame(TheoraGlobalsPtr globals,
                                 ICMCompressorSourceFrameRef sourceFrame,
                                 UInt32 flags )
{
    ComponentResult err = noErr;
    dbg_printf("[  TE]  >> [%08lx] :: CompleteFrame()\n", (UInt32) globals);

    dbg_printf("[  TE] <   [%08lx] :: CompleteFrame() = %ld\n", (UInt32) globals, err);
    return err;
}

/* ========================================================================= */

ComponentResult
Theora_ImageEncoderGetDITLForSize(TheoraGlobalsPtr globals,
                                  Handle *ditl,
                                  Point *requestedSize)
{
    ComponentResult err = noErr;
    Handle h = NULL;

    dbg_printf("[  TE]  >> [%08lx] :: GetDITLForSize()\n", (UInt32) globals);

    switch (requestedSize->h) {
    case kSGSmallestDITLSize:
        GetComponentResource((Component) globals->self, FOUR_CHAR_CODE('DITL'),
                             kTheoraEncoderDITLResID, &h);
        if (h != NULL)
            *ditl = h;
        else
            err = resNotFound;
        break;
    default:
        err = badComponentSelector;
        break;
    }

    dbg_printf("[  TE] <   [%08lx] :: GetDITLForSize() = %ld\n", (UInt32) globals, err);
    return err;
}

#define kItemSharp 1
#define kItemSlow  2

ComponentResult
Theora_ImageEncoderDITLInstall(TheoraGlobalsPtr globals,
                               DialogRef d,
                               short itemOffset)
{
    ComponentResult err = noErr;
    ControlRef cRef;

    dbg_printf("[  TE]  >> [%08lx] :: DITLInstall(%d)\n", (UInt32) globals, itemOffset);

    UInt32 v_sharp = 3 - globals->set_sharp;
    if (v_sharp > 3)
        v_sharp = 3;
    else if (v_sharp < 1)
        v_sharp = 1;

    GetDialogItemAsControl(d, kItemSlow + itemOffset, &cRef);
    SetControl32BitValue(cRef, (UInt32) (globals->set_quick == 0));

    GetDialogItemAsControl(d, kItemSharp + itemOffset, &cRef);
    SetControl32BitValue(cRef, (UInt32) v_sharp);

    dbg_printf("[  TE] <   [%08lx] :: DITLInstall() = %ld\n", (UInt32) globals, err);

    return err;
}

ComponentResult
Theora_ImageEncoderDITLEvent(TheoraGlobalsPtr globals,
                             DialogRef d,
                             short itemOffset,
                             const EventRecord *theEvent,
                             short *itemHit,
                             Boolean *handled)
{
    ComponentResult err = noErr;
    //dbg_printf("[  TE]  >> [%08lx] :: DITLEvent(%d, %d)\n", (UInt32) globals, itemOffset, *itemHit);

    *handled = false;

    //dbg_printf("[  TE] <   [%08lx] :: DITLEvent() = %ld\n", (UInt32) globals, err);
    return err;
}

ComponentResult
Theora_ImageEncoderDITLItem(TheoraGlobalsPtr globals,
                            DialogRef d,
                            short itemOffset,
                            short itemNum)
{
    ComponentResult err = noErr;
    ControlRef cRef;

    dbg_printf("[  TE]  >> [%08lx] :: DITLItem()\n", (UInt32) globals);

    switch (itemNum - itemOffset) {
    case kItemSlow:
        GetDialogItemAsControl(d, itemOffset + kItemSlow, &cRef);
        SetControl32BitValue(cRef, !GetControl32BitValue(cRef));
        break;
    case kItemSharp:
        break;
    }

    dbg_printf("[  TE] <   [%08lx] :: DITLItem() = %ld\n", (UInt32) globals, err);

    return err;
}

ComponentResult
Theora_ImageEncoderDITLRemove(TheoraGlobalsPtr globals,
                              DialogRef d,
                              short itemOffset)
{
    ComponentResult err = noErr;
    ControlRef cRef;
    UInt32 v_sharp;

    dbg_printf("[  TE]  >> [%08lx] :: DITLRemove()\n", (UInt32) globals);

    GetDialogItemAsControl(d, kItemSlow + itemOffset, &cRef);
    globals->set_quick = GetControl32BitValue(cRef) == 0 ? 1 : 0;

    GetDialogItemAsControl(d, kItemSharp + itemOffset, &cRef);
    v_sharp = 3 - GetControl32BitValue(cRef);
    if (v_sharp > 2)
        v_sharp = 2;
    else if (v_sharp < 0)
        v_sharp = 0;

    globals->set_sharp = v_sharp;

    dbg_printf("[  TE] <   [%08lx] :: DITLRemove() = %ld\n", (UInt32) globals, err);

    return err;
}

ComponentResult
Theora_ImageEncoderDITLValidateInput(TheoraGlobalsPtr globals,
                                     Boolean *ok)
{
    ComponentResult err = noErr;

    dbg_printf("[  TE]  >> [%08lx] :: DITLValidateInput()\n", (UInt32) globals);

    if (ok)
        *ok = true;

    dbg_printf("[  TE] <   [%08lx] :: DITLValidateInput() = %ld\n", (UInt32) globals, err);

    return err;
}


/* ========================================================================= */

ComponentResult
encode_frame(TheoraGlobalsPtr globals, ICMCompressorSourceFrameRef sourceFrame,
             ICMFrameType frameType)
{
    ComponentResult err = noErr;

    return err;
}

ComponentResult
yuv_copy__422_to_420(void *b_2vuy, SInt32 b_2vuy_stride, size_t width, size_t height,
                     size_t offset_x, size_t offset_y, yuv_buffer *dst)
{
    UInt8 *base = b_2vuy;
    size_t off_x = offset_x & ~0x01, off_y = offset_y & ~0x01;
    size_t off_x2 = offset_x >> 1, off_y2 = offset_y >> 1;
    UInt8 *y_base = dst->y + off_y * dst->y_stride + off_x;
    UInt8 *cb_base = dst->u + off_y2 * dst->uv_stride + off_x2;
    UInt8 *cr_base = dst->v + off_y2 * dst->uv_stride + off_x2;
    size_t x, y;

    for (y = 0; y < height; y += 2) {
        UInt8 *b_top = base;
        UInt8 *b_bot = base + b_2vuy_stride;
        UInt8 *y_top = y_base;
        UInt8 *y_bot = y_base + dst->y_stride;
        UInt8 *cb = cb_base;
        UInt8 *cr = cr_base;
        for (x = 0; x < width; x += 2) {
            *cb++ = (UInt8) (((UInt16) *b_top++ + (UInt16) *b_bot++) >> 1);
            *y_top++ = *b_top++;
            *y_bot++ = *b_bot++;
            *cr++ = (UInt8) (((UInt16) *b_top++ + (UInt16) *b_bot++) >> 1);
            *y_top++ = *b_top++;
            *y_bot++ = *b_bot++;
        }
        base += 2 * b_2vuy_stride;
        y_base += 2 * dst->y_stride;
        cb_base += dst->uv_stride;
        cr_base += dst->uv_stride;
    }

    return noErr;
}

ComponentResult
vuy_copy__422_to_420(void *b_2vuy, SInt32 b_2vuy_stride, size_t width, size_t height,
                     size_t offset_x, size_t offset_y, yuv_buffer *dst)
{
    UInt8 *base = b_2vuy;
    size_t off_x = offset_x & ~0x01, off_y = offset_y & ~0x01;
    size_t off_x2 = offset_x >> 1, off_y2 = offset_y >> 1;
    UInt8 *y_base = dst->y + off_y * dst->y_stride + off_x;
    UInt8 *cb_base = dst->u + off_y2 * dst->uv_stride + off_x2;
    UInt8 *cr_base = dst->v + off_y2 * dst->uv_stride + off_x2;
    size_t x, y;

    for (y = 0; y < height; y += 2) {
        UInt8 *b_top = base;
        UInt8 *b_bot = base + b_2vuy_stride;
        UInt8 *y_top = y_base;
        UInt8 *y_bot = y_base + dst->y_stride;
        UInt8 *cb = cb_base;
        UInt8 *cr = cr_base;
        for (x = 0; x < width; x += 2) {
            *y_top++ = *b_top++;
            *y_bot++ = *b_bot++;
            *cb++ = (UInt8) (((UInt16) *b_top++ + (UInt16) *b_bot++) >> 1);
            *y_top++ = *b_top++;
            *y_bot++ = *b_bot++;
            *cr++ = (UInt8) (((UInt16) *b_top++ + (UInt16) *b_bot++) >> 1);
        }
        base += 2 * b_2vuy_stride;
        y_base += 2 * dst->y_stride;
        cb_base += dst->uv_stride;
        cr_base += dst->uv_stride;
    }

    return noErr;
}

// Utility to add an SInt32 to a CFMutableDictionary.
static void
addNumberToDictionary(CFMutableDictionaryRef dictionary, CFStringRef key, SInt32 numberSInt32)
{
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberSInt32Type, &numberSInt32);
    if (!number)
        return;
    CFDictionaryAddValue(dictionary, key, number);
    CFRelease(number);
}

// Utility to add a double to a CFMutableDictionary.
static void
addDoubleToDictionary(CFMutableDictionaryRef dictionary, CFStringRef key, double numberDouble)
{
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberDoubleType, &numberDouble);
    if (!number)
        return;
    CFDictionaryAddValue(dictionary, key, number);
    CFRelease(number);
}

ComponentResult
create_pb_attribs(SInt32 width, SInt32 height,
                  const OSType *pixelFormatList, int pixelFormatCount,
                  CFMutableDictionaryRef *pixelBufferAttributesOut)
{
    ComponentResult err = memFullErr;
    int i;
    CFMutableDictionaryRef pixelBufferAttributes = NULL;
    CFNumberRef number = NULL;
    CFMutableArrayRef array = NULL;
    SInt32 widthRoundedUp, heightRoundedUp, x_l, x_r, x_t, x_b;

    pixelBufferAttributes =
        CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
    if (!pixelBufferAttributes)
        goto bail;

    array = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    if (!array)
        goto bail;

    // Under kCVPixelBufferPixelFormatTypeKey, add the list of source pixel formats.
    // This can be a CFNumber or a CFArray of CFNumbers.
    for (i = 0; i < pixelFormatCount; i++) {
        number = CFNumberCreate(NULL, kCFNumberSInt32Type, &pixelFormatList[i]);
        if (!number)
            goto bail;

        CFArrayAppendValue(array, number);

        CFRelease(number);
        number = NULL;
    }

    CFDictionaryAddValue(pixelBufferAttributes, kCVPixelBufferPixelFormatTypeKey, array);
    CFRelease(array);
    array = NULL;

    // Add kCVPixelBufferWidthKey and kCVPixelBufferHeightKey to specify the dimensions
    // of the source pixel buffers.  Normally this is the same as the compression target dimensions.
    addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferWidthKey, width);
    addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferHeightKey, height);

    // If you want to require that extra scratch pixels be allocated on the edges of source pixel buffers,
    // add the kCVPixelBufferExtendedPixels{Left,Top,Right,Bottom}Keys to indicate how much.
    // Internally our encoded can only support multiples of 16x16 macroblocks;
    // we will round the compression dimensions up to a multiple of 16x16 and encode that size.
    // (Note that if your compressor needs to copy the pixels anyhow (eg, in order to convert to a different
    // format) you may get better performance if your copy routine does not require extended pixels.)

    widthRoundedUp = (width + 0x0f) & ~0x0f;
    heightRoundedUp = (height + 0x0f) & ~0x0f;
    x_l = ((widthRoundedUp - width) / 2) & ~0x01;
    x_t = ((heightRoundedUp - height) / 2) & ~0x01;
    x_r = widthRoundedUp - width - x_l;
    x_b = heightRoundedUp - height - x_t;

    if (x_l)
        addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferExtendedPixelsLeftKey, x_l);
    if (x_t)
        addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferExtendedPixelsTopKey, x_t);
    if (x_r)
        addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferExtendedPixelsRightKey, x_r);
    if (x_b)
        addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferExtendedPixelsBottomKey, x_b);


    // Altivec code is most efficient reading data aligned at addresses that are multiples of 16.
    // Pretending that we have some altivec code, we set kCVPixelBufferBytesPerRowAlignmentKey to
    // ensure that each row of pixels starts at a 16-byte-aligned address.
    addNumberToDictionary(pixelBufferAttributes, kCVPixelBufferBytesPerRowAlignmentKey, 16);

    // This codec accepts YCbCr input in the form of '2vuy' format pixel buffers.
    // We recommend explicitly defining the gamma level and YCbCr matrix that should be used.
    addDoubleToDictionary(pixelBufferAttributes, kCVImageBufferGammaLevelKey, 2.2);
    CFDictionaryAddValue(pixelBufferAttributes, kCVImageBufferYCbCrMatrixKey, kCVImageBufferYCbCrMatrix_ITU_R_601_4);

    err = noErr;
    *pixelBufferAttributesOut = pixelBufferAttributes;
    pixelBufferAttributes = NULL;

bail:
    if (pixelBufferAttributes)
        CFRelease(pixelBufferAttributes);

    if (number)
        CFRelease(number);

    if (array)
        CFRelease(array);

    return err;
}
