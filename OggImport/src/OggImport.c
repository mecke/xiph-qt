/*
 *  OggImport.c
 *
 *    The main part of the OggImport component.
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
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: OggImport.c 13696 2007-09-02 15:34:00Z arek $
 *
 */


/*
 *  This file is based on the original importer code
 *  written by Steve Nicolai.
 *
 */

#if defined(__APPLE_CC__)
#include <QuickTime/QuickTime.h>
//#include <Ogg/ogg.h>
#else
#include <QuickTimeComponents.h>
//#include <ogg.h>
#endif

#include <Ogg/ogg.h>
//#include <Vorbis/codec.h>

#include "OggImport.h"

//#define NDEBUG

#include "debug.h"


#include "importer_types.h"

#include "common.h"
#include "rb.h"

#if TARGET_OS_WIN32
#include "pxml.h"
#endif

//stream-type support functions
#include "stream_vorbis.h"
#include "stream_speex.h"
#include "stream_flac.h"
#include "stream_theora.h"

static stream_format_handle_funcs s_formats[] = {
#if defined(_HAVE__VORBIS_SUPPORT)
    HANDLE_FUNCTIONS__VORBIS,
#endif
#if defined(_HAVE__FLAC_SUPPORT)
    HANDLE_FUNCTIONS__FLAC,
#endif
#if defined(_HAVE__SPEEX_SUPPORT)
    HANDLE_FUNCTIONS__SPEEX,
#endif
#if defined(_HAVE__THEORA_SUPPORT)
    HANDLE_FUNCTIONS__THEORA,
#endif

    HANDLE_FUNCTIONS__NULL
};




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			Constants
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enum {
    kDataBufferSize = 64 * 1024,
    kDataAsyncBufferSize = 16 * 1024,

    kDefaultChunkSize = 11000,

    kTempFileDuration = 3599,
    kBitrateEstimateBytesNeeded = 1024 * 1024,
    kNoIdlingFileSizeLimit = 7 * 1024 * 1024,
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			prototypes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* sometimes I wonder if using ComponentDispatch.h is worth it.  This is
   one of those times.  MacOS functions are declared pascal, win32 are not.
*/
#if TARGET_OS_MAC
#define COMPONENTFUNC static pascal ComponentResult
#else
#define COMPONENTFUNC static ComponentResult
#endif

COMPONENTFUNC OggImportOpen(OggImportGlobalsPtr globals, ComponentInstance self);
COMPONENTFUNC OggImportClose(OggImportGlobalsPtr globals, ComponentInstance self);
COMPONENTFUNC OggImportVersion(OggImportGlobalsPtr globals);

COMPONENTFUNC OggImportSetOffsetAndLimit64(OggImportGlobalsPtr globals, const wide *offset,
                                           const wide *limit);
COMPONENTFUNC OggImportSetOffsetAndLimit(OggImportGlobalsPtr globals, unsigned long offset,
                                         unsigned long limit);

COMPONENTFUNC OggImportValidate(OggImportGlobalsPtr globals,
                                const FSSpec *      theFile,
                                Handle              theData,
                                Boolean *           valid);
COMPONENTFUNC OggImportValidateDataRef(OggImportGlobalsPtr globals,
                                       Handle              dataRef,
                                       OSType              dataRefType,
                                       UInt8 *             valid);

COMPONENTFUNC OggImportSetChunkSize(OggImportGlobalsPtr globals, long chunkSize);

COMPONENTFUNC OggImportIdle(OggImportGlobalsPtr globals,
                            long                inFlags,
                            long *              outFlags);

COMPONENTFUNC OggImportFile(OggImportGlobalsPtr globals, const FSSpec *theFile,
                            Movie theMovie, Track targetTrack, Track *usedTrack,
                            TimeValue atTime, TimeValue *durationAdded, long inFlags, long *outFlags);
COMPONENTFUNC OggImportGetMIMETypeList(OggImportGlobalsPtr globals, QTAtomContainer *retMimeInfo);
COMPONENTFUNC OggImportDataRef(OggImportGlobalsPtr globals, Handle dataRef,
                               OSType dataRefType, Movie theMovie,
                               Track targetTrack, Track *usedTrack,
                               TimeValue atTime, TimeValue *durationAdded,
                               long inFlags, long *outFlags);
COMPONENTFUNC OggImportGetFileType(OggImportGlobalsPtr globals, OSType *fileType);
COMPONENTFUNC OggImportGetLoadState(OggImportGlobalsPtr globals, long *loadState);
COMPONENTFUNC OggImportGetMaxLoadedTime(OggImportGlobalsPtr globals, TimeValue *time);
COMPONENTFUNC OggImportEstimateCompletionTime(OggImportGlobalsPtr globals, TimeRecord *time);
COMPONENTFUNC OggImportSetDontBlock(OggImportGlobalsPtr globals, Boolean  dontBlock);
COMPONENTFUNC OggImportGetDontBlock(OggImportGlobalsPtr globals, Boolean  *willBlock);
COMPONENTFUNC OggImportSetIdleManager(OggImportGlobalsPtr globals, IdleManager im);
COMPONENTFUNC OggImportSetNewMovieFlags(OggImportGlobalsPtr globals, long flags);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			Component Dispatcher
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define CALLCOMPONENT_BASENAME()     OggImport
#define CALLCOMPONENT_GLOBALS()      OggImportGlobalsPtr storage

#define MOVIEIMPORT_BASENAME()       CALLCOMPONENT_BASENAME()
#define MOVIEIMPORT_GLOBALS()        CALLCOMPONENT_GLOBALS()

#define COMPONENT_UPP_SELECT_ROOT()  MovieImport
#define COMPONENT_DISPATCH_FILE      "OggImportDispatch.h"

#if !TARGET_OS_WIN32
#include <CoreServices/Components.k.h>
#include <QuickTime/QuickTimeComponents.k.h>
#include <QuickTime/ComponentDispatchHelper.c>
#else
#include <Components.k.h>
#include <QuickTimeComponents.k.h>
#include <ComponentDispatchHelper.c>
#endif

#if TARGET_OS_WIN32
EXTERN_API_C(SInt32 ) S64Compare(SInt64 left, SInt64 right)
{
    if (left < right)
        return -1;
    if (left == right)
        return 0;
    return 1;
}
#endif

static void _debug_idles(OggImportGlobalsPtr globals) {
#if !defined(NDEBUG)
    Boolean needs = false;
    TimeRecord ni;

    if (globals->idleManager != NULL) {
        QTIdleManagerNeedsAnIdle(globals->idleManager, &needs);
        QTIdleManagerGetNextIdleTime(globals->idleManager, &ni);
        dbg_printf("[OI  ]  %ci [%08lx] idles: requested: base: %ld, scale: %ld, value: %lld [%ld]\n",
                   needs ? ' ' : '!',
                   (UInt32) globals, (long) ni.base, ni.scale,
                   (ni.value.hi == 2047 && ni.value.lo == 0xffffffff) ? -1LL : WideToSInt64(ni.value),
                   GetTimeBaseTime(ni.base, ni.scale, NULL));
    }

    if (globals->dataIdleManager != NULL) {
        QTIdleManagerNeedsAnIdle(globals->dataIdleManager, &needs);
        QTIdleManagerGetNextIdleTime(globals->dataIdleManager, &ni);
        dbg_printf("[OI  ]  %cd [%08lx] idles: requested: base: %ld, scale: %ld, value: %lld [%ld]\n",
                   needs ? ' ' : '!',
                   (UInt32) globals, (long) ni.base, ni.scale,
                   (ni.value.hi == 2047 && ni.value.lo == 0xffffffff) ? -1LL : WideToSInt64(ni.value),
                   GetTimeBaseTime(ni.base, ni.scale, NULL));
    }
#endif /* NDEBUG */
}

static ComponentResult DoRead(OggImportGlobalsPtr globals, Ptr buffer, SInt64 offset, long size)
{
    ComponentResult err;
    DataHScheduleRecord sched_rec, *sched_rec_ptr = NULL;
    long const_ref = 0;
    DataHCompletionUPP compl_upp = NULL;
    const wide wideOffset = SInt64ToWide(offset);

    dbg_printf("---- DoRead() called\n");

    dbg_printf("--->> READING: %lld [%ld] --> %lld\n", offset, size, offset + size);
    dbg_printf("----> READING: usingIdle: %d, dataCanDoAsyncRead: %d, canDoScheduleData64: %d, "
               "canDoScheduleData: %d, canDoGetFileSize64: %d\n",
               globals->usingIdle, globals->dataCanDoAsyncRead, globals->dataCanDoScheduleData64,
               globals->dataCanDoScheduleData, globals->dataCanDoGetFileSize64);

    if (globals->usingIdle && globals->dataCanDoAsyncRead) {
        SInt64 read_time = S64Set(10);
        TimeValue idle_delay = 80;

        if (globals->idleTimeBase == NULL) {
            globals->idleTimeBase = NewTimeBase();
            SetTimeBaseRate(globals->idleTimeBase, fixed1);
        }

        globals->dataRequested = true;
        const_ref = (long) globals;
        compl_upp = globals->dataReadCompletion;

        GetTimeBaseTime(globals->idleTimeBase, 1000, &sched_rec.timeNeededBy);
        read_time = S64Add(WideToSInt64(sched_rec.timeNeededBy.value), read_time);
        sched_rec.timeNeededBy.value = SInt64ToWide(read_time);
        sched_rec.extendedID = 0;
        sched_rec.extendedVers = 0;
        sched_rec.priority = 0x00640000; // (Fixed) 100.0

        sched_rec_ptr = &sched_rec;

        if (globals->idleManagersLinked)
            idle_delay = 250;
        else if (globals->dataIdleManager == NULL)
            idle_delay = 8;
        _debug_idles(globals);
        err = QTIdleManagerSetNextIdleTimeDelta(globals->idleManager, idle_delay, 1000);
        dbg_printf("----: Delaying Idles: %ld\n", err);
        _debug_idles(globals);
    }

    if (globals->dataCanDoScheduleData64) {
        err = DataHScheduleData64(globals->dataReader, buffer, &wideOffset, size, const_ref, sched_rec_ptr, compl_upp);
        dbg_printf("[OI  ]  sR [%08lx] :: DoRead() = %ld\n", (UInt32) globals, err);
    } else if (globals->dataCanDoScheduleData) {
        err = DataHScheduleData(globals->dataReader, buffer, wideOffset.lo, size, const_ref, sched_rec_ptr, compl_upp);
        dbg_printf("[OI  ]  sr [%08lx] :: DoRead() = %ld\n", (UInt32) globals, err);
    } else {
        err = badComponentSelector;
    }

    dbg_printf("----: READ: %ld [%08lx]\n", err, err);
    if (err == noErr && compl_upp == NULL)
        rb_sync_reserved(&globals->dataRB);
    globals->readError = err;

    return err;
}

static ComponentResult FillBuffer(OggImportGlobalsPtr globals)
{
    int dataLeft;
    SInt64 readDataOffset;

    dbg_printf("---- FillBuffer() called\n");
    dbg_printf("   - dataOffset: %lld, dataEndOffset: %lld\n", globals->dataOffset, globals->dataEndOffset);
    dbg_printf("   - dataOffset != -1: %d, dataOffset >= dataEndOffset: %d\n",
               S64Compare(globals->dataOffset, S64Set(-1)) != 0,
               S64Compare(globals->dataOffset, globals->dataEndOffset) >= 0);

    /* have we hit the end of file or our upper limit? */
    if (globals->sizeInitialised && S64Compare(globals->dataEndOffset, S64Set(-1)) != 0) {
        if (S64Compare(globals->dataOffset, S64Set(-1)) != 0 &&
            S64Compare(globals->dataOffset, globals->dataEndOffset) >= 0)
            return eofErr;
    }

    dbg_printf("--1- FillBuffer() called\n");
    /* can another page from the disk fit in the buffer? */
    if (globals->dataReadChunkSize > rb_space_available(&globals->dataRB))
        return -50;  ///@@@ page won't fit in buffer, they always should

    dbg_printf("--2- FillBuffer() called\n");
    readDataOffset = S64Add(globals->dataOffset, S64Set(rb_data_available(&globals->dataRB)));

    /* figure out how much data is left, and read either a chunk or what's left */
    if (globals->sizeInitialised && S64Compare(globals->dataEndOffset, S64Set(-1)) != 0) {
        dataLeft = S32Set(S64Subtract(globals->dataEndOffset, readDataOffset));
    } else {
        dataLeft = globals->dataReadChunkSize;
    }

    if (dataLeft > globals->dataReadChunkSize)
        dataLeft = globals->dataReadChunkSize;

    if (dataLeft == 0)
        return eofErr;

    return DoRead(globals, (Ptr) rb_reserve(&globals->dataRB, dataLeft), readDataOffset, dataLeft);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static ComponentResult OpenStream(OggImportGlobalsPtr globals, long serialno, ogg_page *opg, stream_format_handle_funcs *ff)
{
    ComponentResult err;

    if (globals->streamInfoHandle)
    {
        globals->streamCount++;
        SetHandleSize((Handle)globals->streamInfoHandle, sizeof(StreamInfo) * globals->streamCount);
    }
    else
    {
        globals->streamInfoHandle = (StreamInfo **)NewHandle(sizeof(StreamInfo));
        globals->streamCount = 1;
    }
    err = MemError();

    if (err == noErr)
    {
        StreamInfo *si = &(*globals->streamInfoHandle)[globals->streamCount - 1];
        si->serialno = serialno;

        ogg_stream_init(&si->os,serialno);

        si->sfhf = ff;

        if (ff->initialize != NULL)
            (*ff->initialize)(si); // check for error here and clean-up if not OK

        si->soundDescExtension = NewHandle(0);

        si->MDmapping = NULL;
        si->UDmapping = NULL;

        // si->sampleOffset = 0;

        globals->numTracksStarted++;
    }

    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static StreamInfoPtr FindStream(OggImportGlobalsPtr globals, long serialno)
{
    int i;

    for (i = 0; i < globals->streamCount; i++)
    {
        if ((*globals->streamInfoHandle)[i].serialno == serialno)
        {
            return &(*globals->streamInfoHandle)[i];
        }
    }

    return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void _close_stream(OggImportGlobalsPtr globals, StreamInfoPtr si)
{
    ogg_stream_clear(&si->os);

    if (si->sfhf->flush != NULL)
        /* ret = */ (*si->sfhf->flush)(globals, si, true);

    if (si->sfhf->clear != NULL)
        (*si->sfhf->clear)(si);

    if (si->MDmapping != NULL)
        CFRelease(si->MDmapping);
    if (si->UDmapping != NULL)
        CFRelease(si->UDmapping);

    DisposeHandle(si->soundDescExtension);
    DisposeHandle((Handle)si->sampleDesc);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void CloseAllStreams(OggImportGlobalsPtr globals)
{
    int i;

    for (i = 0; i < globals->streamCount; i++)
    {
        StreamInfoPtr si = &(*globals->streamInfoHandle)[i];
        _close_stream(globals, si);
    }
    globals->streamCount = 0;
    SetHandleSize((Handle) globals->streamInfoHandle, 0);
}

static void CloseStream(OggImportGlobalsPtr globals, StreamInfoPtr si)
{
    int i;

    for (i = 0; i < globals->streamCount; i++)
    {
        if (si == &(*globals->streamInfoHandle)[i]) {
            _close_stream(globals, si);

            if (i < globals->streamCount - 1) {
                HLock((Handle) globals->streamInfoHandle);
                BlockMove(&(*globals->streamInfoHandle)[i+1], &(*globals->streamInfoHandle)[i], sizeof(StreamInfo) * (globals->streamCount - 1 - i));
                HUnlock((Handle) globals->streamInfoHandle);
            }
            globals->streamCount--;
            SetHandleSize((Handle) globals->streamInfoHandle, sizeof(StreamInfo) * globals->streamCount);
            break;
        }
    }
}

static int InitialiseMetaDataMappings(OggImportGlobalsPtr globals, StreamInfoPtr si) {
    SInt32 ret = 0;
    CFDictionaryRef props = NULL;

    dbg_printf("--= IMDM()\n");
    if (si->MDmapping != NULL && si->UDmapping != NULL) {
        return 1;
    }

    //else? let's assume for now that they are both intialised or both are not initialised

#if TARGET_OS_WIN32
    {
        Handle mapplist;
        OSErr err = GetComponentResource((Component)globals->self, 'MDCf', kImporterResID, &mapplist);
        if (err == noErr) {
            long mpl_size = GetHandleSize(mapplist);
            HLock(mapplist);
            props = pxml_parse_plist((unsigned char *) *mapplist, mpl_size);
            HUnlock(mapplist);
        }
    }
#else
    {
        CFBundleRef bundle;
        CFURLRef mdmurl;
        CFDataRef data;
        SInt32 error = 0;
        CFStringRef errorString;

        bundle = CFBundleGetBundleWithIdentifier(CFSTR(kOggVorbisBundleID));

        if (bundle == NULL)
            return 0;

        mdmurl = CFBundleCopyResourceURL(bundle, CFSTR("MetaDataConfig"), CFSTR("plist"), NULL);
        if (mdmurl != NULL) {
            if (CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, mdmurl, &data, NULL, NULL, &error)) {
                props = (CFDictionaryRef) CFPropertyListCreateFromXMLData(kCFAllocatorDefault, data,
                                                                          kCFPropertyListImmutable, &errorString);
                CFRelease(data);
            }
            CFRelease(mdmurl);
        }
    }
#endif /* TARGET_OS_WIN32 */

    if (props != NULL) {
        if (CFGetTypeID(props) == CFDictionaryGetTypeID()) {
            si->MDmapping = (CFDictionaryRef) CFDictionaryGetValue(props, CFSTR("Vorbis-to-MD"));
            if (si->MDmapping != NULL) {
                dbg_printf("----: MDmapping found\n");
                CFRetain(si->MDmapping);
                ret = 1;
            }
            si->UDmapping = (CFDictionaryRef) CFDictionaryGetValue(props, CFSTR("Vorbis-to-UD"));
            if (si->UDmapping != NULL) {
                dbg_printf("----: UDmapping found\n");
                CFRetain(si->UDmapping);
            } else
                ret = 0;
        }
        CFRelease(props);
    }

    return ret;
}

static int LookupTagUD(OggImportGlobalsPtr globals, StreamInfoPtr si, const char *str, long *osType) {
    int ret = -1;
    long len;

    if (si->UDmapping == NULL)
    {
        if (!InitialiseMetaDataMappings(globals, si))
            return -1;
    }

    len = strcspn(str, "=");

    if (len > 0) {
        CFStringRef tmpkstr = CFStringCreateWithBytes(NULL, (const UInt8 *) str, len + 1, kCFStringEncodingUTF8, true);
        if (tmpkstr != NULL) {
            CFMutableStringRef keystr = CFStringCreateMutableCopy(NULL, len + 1, tmpkstr);
            if (keystr != NULL) {
                //CFLocaleRef loc = CFLocaleCopyCurrent();
                CFLocaleRef loc = NULL;
                CFStringUppercase(keystr, loc);
                //CFRelease(loc);
                dbg_printf("--- luTud: %s [%s]\n", (char *)str, (char *)CFStringGetCStringPtr(keystr,kCFStringEncodingUTF8));
                if (CFDictionaryContainsKey(si->UDmapping, keystr)) {
                    CFStringRef udkey = (CFStringRef) CFDictionaryGetValue(si->UDmapping, keystr);

                    *osType = (CFStringGetCharacterAtIndex(udkey, 0) & 0xff) << 24 |
                        (CFStringGetCharacterAtIndex(udkey, 1) & 0xff) << 16 |
                        (CFStringGetCharacterAtIndex(udkey, 2) & 0xff) << 8 |
                        (CFStringGetCharacterAtIndex(udkey, 3) & 0xff);

                    dbg_printf("--- luTud: %s [%s]\n", (char *)str, (char *)keystr);
                    ret = len + 1;
                }
                CFRelease(keystr);
            }
            CFRelease(tmpkstr);
        }
    }

    return ret;
}

static int LookupTagMD(OggImportGlobalsPtr globals, StreamInfoPtr si, const char *str, long *osType) {
    int ret = -1;
    long len;

    if (si->MDmapping == NULL)
    {
        if (!InitialiseMetaDataMappings(globals, si))
            return -1;
    }

    len = strcspn(str, "=");

    if (len > 0) {
        CFStringRef tmpkstr = CFStringCreateWithBytes(NULL, (const UInt8 *) str, len + 1, kCFStringEncodingUTF8, true);
        if (tmpkstr != NULL) {
            CFMutableStringRef keystr = CFStringCreateMutableCopy(NULL, len + 1, tmpkstr);
            if (keystr != NULL) {
                //CFLocaleRef loc = CFLocaleCopyCurrent();
                CFLocaleRef loc = NULL;
                CFStringUppercase(keystr, loc);
                //CFRelease(loc);
                dbg_printf("--- luTmd: %s [%s]\n", (char *)str, (char *)CFStringGetCStringPtr(keystr,kCFStringEncodingUTF8));
                if (CFDictionaryContainsKey(si->MDmapping, keystr)) {
                    CFStringRef mdkey = (CFStringRef) CFDictionaryGetValue(si->MDmapping, keystr);

                    *osType = (CFStringGetCharacterAtIndex(mdkey, 0) & 0xff) << 24 |
                        (CFStringGetCharacterAtIndex(mdkey, 1) & 0xff) << 16 |
                        (CFStringGetCharacterAtIndex(mdkey, 2) & 0xff) << 8 |
                        (CFStringGetCharacterAtIndex(mdkey, 3) & 0xff);

                    dbg_printf("--- luTmd: %s [%s]\n", (char *)str, (char *)keystr);
                    ret = len + 1;
                }
                CFRelease(keystr);
            }
            CFRelease(tmpkstr);
        }
    }

    return ret;
}

static ComponentResult ConvertUTF8toScriptCode(const char *str, Handle *h)
{
    CFStringEncoding encoding = 0;
    CFIndex numberOfCharsConverted = 0, usedBufferLength = 0;
    OSStatus ret = noErr;

    CFStringRef tmpstr = CFStringCreateWithBytes(NULL, (const UInt8 *) str, strlen(str), kCFStringEncodingUTF8, true);
    if (tmpstr == NULL)
        return  kTextUnsupportedEncodingErr; //!??!?!

    encoding = kCFStringEncodingMacRoman;

    if (ret == noErr) {
        CFRange range = { 0, CFStringGetLength(tmpstr)};
        numberOfCharsConverted = CFStringGetBytes(tmpstr, range, encoding, 0, false,
                                                  NULL, 0, &usedBufferLength);
        if ((numberOfCharsConverted == CFStringGetLength(tmpstr)) && (usedBufferLength > 0)) {
            *h = NewHandleClear(usedBufferLength);
            if (*h != NULL) {
                HLock(*h);

                numberOfCharsConverted = CFStringGetBytes(tmpstr, range, encoding, 0,
                                                          false, (UInt8 *) **h, usedBufferLength,
                                                          &usedBufferLength);
                HUnlock(*h);
            } else {
                ret = MemError();
            }
        } else {
            ret = kTextUnsupportedEncodingErr;
        }
    }

    CFRelease(tmpstr);

    return ret;
}

static ComponentResult AddCommentToMetaData(OggImportGlobalsPtr globals, StreamInfoPtr si, const char *str, int len, QTMetaDataRef md) {
    ComponentResult ret = noErr;
    long tag;

    int tagLen = LookupTagMD(globals, si, str, &tag);
    Handle h;

    if (tagLen != -1 && str[tagLen] != '\0') {
        dbg_printf("-- TAG: %08lx\n", tag);

        ret = QTMetaDataAddItem(md, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon,
                                (UInt8 *) &tag, sizeof(tag), (UInt8 *) (str + tagLen), len - tagLen, kQTMetaDataTypeUTF8, NULL);
        dbg_printf("-- TAG: %4.4s :: QT    = %ld\n", (char *)&tag, (long)ret);
    }

    tagLen = LookupTagUD(globals, si, str, &tag);

    if (tagLen != -1 && str[tagLen] != '\0') {
        QTMetaDataItem mdi;
        char * localestr = "en";
        Handle localeh = NewEmptyHandle();

        PtrAndHand(&localestr, localeh, strlen(localestr));

        dbg_printf("-- TAG: %08lx\n", tag);

        ret = ConvertUTF8toScriptCode(str + tagLen, &h);
        if (ret == noErr) {
            HLock(h);
            ret = QTMetaDataAddItem(md, kQTMetaDataStorageFormatUserData, kQTMetaDataKeyFormatUserData,
                                    (UInt8 *) &tag, sizeof(tag), (UInt8 *) *h, GetHandleSize(h), kQTMetaDataTypeMacEncodedText, &mdi);
            dbg_printf("-- TAG: %4.4s :: QT[X] = %ld\n", (char *)&tag, (long)ret);
            HUnlock(h);
            if (ret == noErr) {
                ret = QTMetaDataSetItemProperty(md, mdi, kPropertyClass_MetaDataItem, kQTMetaDataItemPropertyID_Locale,
                                                GetHandleSize(localeh), *h);
                dbg_printf("-- TAG: %4.4s :: QT[X] locale (%5.5s)= %ld\n", (char *)&tag, *h, (long)ret);
            }
            DisposeHandle(h);
        }
    }

    return ret;
}

ComponentResult DecodeCommentsQT(OggImportGlobalsPtr globals, StreamInfoPtr si, vorbis_comment *vc)
{
    ComponentResult ret = noErr;
    int i;
    QTMetaDataRef md;

    //ret = QTCopyTrackMetaData(si->theTrack, &md);
    ret = QTCopyMovieMetaData(globals->theMovie, &md);

    if (ret != noErr)
        return ret;

    for (i = 0; i < vc->comments; i++)
    {
        ret = AddCommentToMetaData(globals, si, vc->user_comments[i], vc->comment_lengths[i], md);
        if (ret != noErr) {
            //break;
            dbg_printf("AddCommentToMetaData() failed? = %d\n", ret);
        }
    }

    QTMetaDataRelease(md);

    ret = QTCopyTrackMetaData(si->theTrack, &md);
    //ret = QTCopyMovieMetaData(globals->theMovie, &md);

    if (ret != noErr)
        return ret;

    for (i = 0; i < vc->comments; i++)
    {
        ret = AddCommentToMetaData(globals, si, vc->user_comments[i], vc->comment_lengths[i], md);
        if (ret != noErr) {
            //break;
            dbg_printf("AddCommentToMetaData() failed? = %d\n", ret);
        }
    }

    QTMetaDataRelease(md);

    return ret;
}

static ComponentResult CreateSampleDescription(StreamInfoPtr si)
{
    ComponentResult err = noErr;
    if (si->sfhf->sample_description != NULL)
        err = (*si->sfhf->sample_description)(si);
    else
        err = invalidMedia; // ??!

    return err;
}

ComponentResult CreateTrackAndMedia(OggImportGlobalsPtr globals, StreamInfoPtr si, ogg_page *opg)
{
    ComponentResult err = noErr;

    if (err == noErr)
    {
        // build the sample description
        err = CreateSampleDescription(si);
        if (err == noErr)
        {
            dbg_printf("! -- SampleDescription created OK\n");
            if (si->sfhf->track != NULL)
                /* err = */ (*si->sfhf->track)(globals, si);
            else
                si->theTrack = NewMovieTrack(globals->theMovie, 0, 0, kFullVolume);
            if (si->theTrack)
            {
                Handle data_ref = globals->dataRef;
                dbg_printf("! -- MovieTrack created OK\n");

                // for non-url data handlers there seem to be some problems
                // if we use dataRef obtained from the data handler and not
                // the original one... - ? :/
                if (globals->dataRefType == URLDataHandlerSubType)
                    err = DataHGetDataRef(globals->dataReader, &data_ref);

                if (err == noErr) {
                    if (si->sfhf->track_media != NULL)
                        /* err = */ (*si->sfhf->track_media)(globals, si, data_ref);
                    else {
                        dbg_printf("! -- calling => NewTrackMedia(%lx)\n", si->rate);
                        si->theMedia = NewTrackMedia(si->theTrack, SoundMediaType,
                                                     si->rate, data_ref, globals->dataRefType);
                    }

                    if (data_ref != globals->dataRef)
                        DisposeHandle(data_ref);
                }

                if (si->theMedia)
                {
                    dbg_printf("! -- TrackMedia created OK\n");
                    SetTrackEnabled(si->theTrack, true);

                    si->lastGranulePos = 0;
                }
                else
                {
                    err = GetMoviesError();
                    DisposeMovieTrack(si->theTrack);
                }
            } else
                err = GetMoviesError();
        }
    }

    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult NotifyMovieChanged(OggImportGlobalsPtr globals, Boolean force)
{
    //Notify the movie it's changed (from email from Chris Flick)
    QTAtomContainer container = NULL;
    UInt32 tickNow = TickCount();
    OSErr err = noErr;

    if (!globals->usingIdle) {
        dbg_printf("  --  NotifyMovieChanged() = not using idles, skipping\n");
        return err;
    }

    if ((tickNow < globals->tickNotified + globals->notifyStep) && !force) {
        dbg_printf("  --  NotifyMovieChanged() = too short period, skipping\n");
        return err;
    }

    err = QTNewAtomContainer (&container);

    if (err == noErr)
    {
        QTAtom anAction;
        OSType whichAction = EndianU32_NtoB (kActionMovieChanged);

        err = QTInsertChild (container, kParentAtomIsContainer, kAction, 1, 0, 0, NULL, &anAction);
        if (err == noErr)
            err = QTInsertChild (container, anAction, kWhichAction, 1, 0, sizeof (whichAction), &whichAction, NULL);
        if (err == noErr)
            err = MovieExecuteWiredActions (globals->theMovie, 0, container);

        if (!err)
            globals->tickNotified = tickNow;

        dbg_printf("  **  NotifyMovieChanged() = %ld\n", (long)err);
        err = QTDisposeAtomContainer (container);
    }
    return err;
}

static ComponentResult ProcessStreamPage(OggImportGlobalsPtr globals, StreamInfoPtr si, ogg_page *opg) {
    ComponentResult ret = noErr;

    if (si->sfhf->process_page != NULL)
        ret = (*si->sfhf->process_page)(globals, si, opg);
    else {
        // shouldn't happen, but just skip a page here
        ogg_packet op;
        ogg_stream_pagein(&si->os, opg);
        while (ogg_stream_packetout(&si->os, &op) > 0)
            ; // do nothing, just loop
    }

    return ret;
}

static stream_format_handle_funcs* find_stream_support(ogg_page *op) {

    stream_format_handle_funcs *ff = &s_formats[0];
    int i = 0;

    while(ff->recognize != NULL) {
        if ((*ff->recognize)(op) == 0 && (ff->verify == NULL || (*ff->verify)(op) == 0))
            break;
        i += 1;
        ff = &s_formats[i];
    }

    if (ff->recognize == NULL)
        return NULL;

    return ff;
}

/* =============== idle importing support functions ============== */

ComponentResult GetLastFileGP(OggImportGlobalsPtr globals)
{
    ComponentResult err = noErr;
    wide wideOffset = SInt64ToWide(globals->dataEndOffset);
    unsigned char *buffer = NULL, *p = NULL;
    UInt32 size = 8192, i = 16;
    UInt32 ssize = 0;
    SInt64 tmp;

    dbg_printf("[OI  ]  >> [%08lx] :: GetLastFileGP(%lld)\n", (UInt32) globals, globals->dataEndOffset);

    buffer = malloc(size * 16);
    if (buffer == NULL)
        err = MemError();

    if (!err) {
        memset(buffer, 0, size * 16);
        p = buffer + size * 16;

        while (i-- > 0) {
            tmp = S64Subtract(WideToSInt64(wideOffset), S64SetU(size));
            wideOffset = SInt64ToWide(tmp);
            p -= size;
            ssize += size;

            if (globals->dataCanDoScheduleData64) {
                err = DataHScheduleData64(globals->dataReader, (char *) p,
                                          &wideOffset, size, 0, NULL, NULL);
            } else if (globals->dataCanDoScheduleData) {
                err = DataHScheduleData(globals->dataReader, (char *) p,
                                        wideOffset.lo, size, 0, NULL, NULL);
            }

            if (err)
                break;

            find_last_page_GP(p, ssize, &globals->lp_GP, &globals->lp_serialno);
            if (globals->lp_GP > 0)
                break;
        }
    }

    // some sources can only be read sequentially, it's OK
    if (err == notEnoughDataErr)
        err = noErr;

    if (buffer != NULL)
        free(buffer);

    dbg_printf("[OI  ] <   [%08lx] :: GetLastFileGP() = %ld\n", (UInt32) globals, err);
    return err;
}

ComponentResult GetFileDurationFromGP(OggImportGlobalsPtr globals)
{
    ComponentResult err = noErr;
    TimeRecord tr;

    if (globals->lp_GP > 0) {
        StreamInfoPtr si = FindStream(globals, globals->lp_serialno);
        if (si != NULL && si->sfhf->gp_to_time != NULL) {
            err = (*si->sfhf->gp_to_time)(si, &globals->lp_GP, &tr);
        } else if (si != NULL) {
            tr.value = SInt64ToWide(globals->lp_GP);
            tr.scale = si->rate;
            tr.base = NULL;
        }

        if (!err) {
            ConvertTimeScale(&tr, GetMovieTimeScale(globals->theMovie));
            err = GetMoviesError();
        }

        if (!err)
            globals->totalTime = U32SetU(WideToSInt64(tr.value));
    }

    return err;
}

ComponentResult EstimateFileDuration(OggImportGlobalsPtr globals)
{
    ComponentResult err = noErr;

    SInt64 d = S64Multiply(S64Set(globals->timeLoaded), S64Subtract(globals->dataEndOffset, globals->dataStartOffset));
    SInt64 read_size = S64Subtract(globals->dataOffset, globals->dataStartOffset);

    /* Assume a number of 'non-audio' bytes at the beginning of each stream.
       4k at the moment, better to assume more as it will result in a longer
       (idealy it should be slightly longer than the real duration)
       placeholder track. */
    read_size = S64Subtract(read_size, S64Set(globals->streamCount * 4096));

    if (S64Compare(read_size, S64Set(1)) < 0 || S64Compare(d, S64Set(1)) < 0)
        globals->totalTime = kTempFileDuration * GetMovieTimeScale(globals->theMovie);
    else
        globals->totalTime = S32Set(S64Div(d,  read_size));

    dbg_printf("[OI  ]   = [%08lx] :: EstimateFileDuration() : %ld (%ld, %lld, %lld)\n",
               (UInt32) globals, globals->totalTime, globals->timeLoaded,
               S64Subtract(globals->dataEndOffset, globals->dataStartOffset), read_size);
    return err;
}

ComponentResult CreatePlaceholderTrack(OggImportGlobalsPtr globals)
{
    ComponentResult err = noErr;
    Track track = NULL;
    Media media = NULL;
    SampleDescriptionHandle sd;

    if (globals->sizeInitialised && S64Compare(globals->dataEndOffset, S64Set(-1)) == 0)
        return err;

    sd = (SampleDescriptionHandle) NewHandleClear(sizeof(SampleDescription));

    if (sd == NULL)
        err = MemError();

    if (!err) {
        (*sd)->descSize = sizeof(SampleDescriptionPtr);
        (*sd)->dataFormat = BaseMediaType;

        track = NewMovieTrack(globals->theMovie, 0, 0, 0);
        if (track != NULL) {
            media = NewTrackMedia(track, BaseMediaType,
                                  GetMovieTimeScale(globals->theMovie),
                                  NewHandle(0), NullDataHandlerSubType);

            if (media != NULL) {
                err = AddMediaSampleReference(media, 0, 1, globals->totalTime,
                                              sd, 1, 0, NULL);

                if (!err)
                    err = InsertMediaIntoTrack(track, 0, 0, globals->totalTime,
                                               fixed1);
                if (!err) {
                    if (globals->phTrack != NULL)
                        DisposeMovieTrack(globals->phTrack);
                    globals->phTrack = track;
                } else {
                    DisposeMovieTrack(track);
                }
            } else {
                err = GetMoviesError();
                DisposeMovieTrack(track);
            }
        } else {
            err = GetMoviesError();
        }

        DisposeHandle((Handle) sd);
    }

    return err;
}

ComponentResult RemovePlaceholderTrack(OggImportGlobalsPtr globals)
{
    ComponentResult err = noErr;

    if (globals->phTrack) {
        DisposeMovieTrack(globals->phTrack);
        globals->phTrack = NULL;
    }

    return err;
}

ComponentResult FlushAllStreams(OggImportGlobalsPtr globals, Boolean notify)
{
    ComponentResult err = noErr;
    int i;

    for (i = 0; i < globals->streamCount; i++)
    {
        StreamInfoPtr si = &(*globals->streamInfoHandle)[i];
        if (si->sfhf->flush != NULL)
            err = (*si->sfhf->flush)(globals, si, false);
        if (err)
            break;
    }

    if (!err) {
        globals->timeFlushed = globals->timeLoaded;
        globals->tickFlushed = TickCount();
        NotifyMovieChanged(globals, notify);
    }

    return err;
}

/* =============================================================== */

static ComponentResult ProcessPage(OggImportGlobalsPtr globals, ogg_page *op) {
    ComponentResult ret = noErr;
    long serialno;
    StreamInfoPtr si = NULL;
    Boolean process_page = true;

    serialno = ogg_page_serialno(op);

    dbg_printf("   - = page found, nr: %08lx\n", ogg_page_pageno(op));
    if (!globals->groupStreamsFound) {
        if (ogg_page_bos(op)) {
            stream_format_handle_funcs *ff = NULL;
            dbg_printf("   - = new stream found: %08lx\n" , serialno);
            //si = FindStream(globals, serialno);
            //if (si != NULL)
            //    ; // !?!?
            ff = find_stream_support(op);
            if (ff != NULL) {
                dbg_printf("   - == And a supported one!\n");
                ret = OpenStream(globals, serialno, op, ff);
                if (ret == noErr) {
                    ogg_packet opckt;

                    si = FindStream(globals, serialno);
                    if (si != NULL) {
                        ogg_stream_pagein(&si->os, op); //check errors?
                        ogg_stream_packetout(&si->os, &opckt); //check errors?

                        if (si->sfhf->first_packet != NULL)
                            (*si->sfhf->first_packet)(si, op, &opckt); //check errors?

                        if (globals->currentGroupOffset == -1) {
                            globals->currentGroupOffset = globals->timeLoaded;
                            globals->currentGroupOffsetSubSecond = globals->timeLoadedSubSecond;
                        }

                        process_page = false;
                    }
                }
            }
        } else {
            si = FindStream(globals, serialno);
            if (si != NULL) {
                globals->groupStreamsFound = true;
            }
        }
    } else {
        if (!ogg_page_bos(op))
            si = FindStream(globals, serialno);
    }

    if (si != NULL) {
        if (process_page) {
            ret = ProcessStreamPage(globals, si, op);
        }

        if (ogg_page_eos(op)) {
            dbg_printf("   x = closing stream: %08lx\n" , serialno);
            CloseStream(globals, si);
            if (globals->streamCount == 0) {
                globals->groupStreamsFound = false;
                globals->currentGroupOffset = -1;
                globals->currentGroupOffsetSubSecond = 0.0;
            }
        }
    }

    globals->dataOffset = S64Add(globals->dataOffset, S64Set(globals->currentData - globals->dataRB.b_start));
    rb_zap(&globals->dataRB, globals->currentData - globals->dataRB.b_start);

    globals->currentData = rb_data(&globals->dataRB);
    globals->validDataEnd = globals->currentData + rb_data_available(&globals->dataRB);

    return ret;
}

static Boolean ShouldFlush(OggImportGlobals *globals)
{
    Boolean ret = false;
    UInt32 now = TickCount();

    if (now > globals->tickFlushed + globals->flushStepCheck && GetMovieRate(globals->theMovie) != 0) {
        // movie is currently playing:
        // - if video is present don't commit sample refs until
        //   less than 5 seconds of playtime is left (QT will reset video decoder
        //   every time samples are added, QT is just nice like that)
        // - if we only have audio - commit every flushStepNormal ticks, unless near the end of the
        //   currently loaded data - then just commit (practically - every flushStepCheck ticks)
        if ((globals->timeLoaded - GetMovieTime(globals->theMovie, NULL) < globals->flushMinTimeLeft) ||
            (!globals->hasVideoTrack && now > globals->tickFlushed + globals->flushStepNormal))
            ret = true;
    } else if (now > globals->tickFlushed + globals->flushStepNormal) {
        // movie is not playing - commit sample refs every 'flushStepNormal' ticks
        ret = true;
    }

    return ret;
}

static ComponentResult XQTGetFileSize(OggImportGlobalsPtr globals);

static ComponentResult StateProcess(OggImportGlobalsPtr globals) {
    ComponentResult result = noErr;
    ogg_page og;
    Boolean process = true;

    dbg_printf("-----= StateProcess() called\n");
    while (process) {
        switch (globals->state) {
        case kStateInitial:
            dbg_printf("   - (:kStateInitial:)\n");
            globals->dataOffset = globals->dataStartOffset;
            globals->numTracksSeen = 0;
            globals->hasVideoTrack = false;
            globals->timeLoaded = 0;
            globals->timeLoadedSubSecond = 0.0;
            globals->timeFlushed = 0;
            globals->tickFlushed = 0;
            globals->tickNotified = 0;
            globals->dataRequested = false;
            globals->startTickCount = TickCount();

            globals->currentGroupOffset = globals->startTime;
            globals->currentGroupOffsetSubSecond = (Float64) (globals->startTime % GetMovieTimeScale(globals->theMovie)) / (Float64) GetMovieTimeScale(globals->theMovie);
            globals->groupStreamsFound = false;

            if (!globals->sizeInitialised) {
                result = XQTGetFileSize(globals);
            }

            if (result == noErr && globals->calcTotalTime && globals->lp_GP < 0) {
                result = GetLastFileGP(globals);
            }

            if (result != noErr)
                process = false;

            globals->state = kStateReadingPages;
            break;

        case kStateReadingPages:
            dbg_printf("   - (:kStateReadingPages:)\n");
            if (globals->dataRequested) {
                _debug_idles(globals);
                result = DataHTask(globals->dataReader);
                if (globals->dataIsStream) {
                    Boolean needs = false;
                    if (globals->dataIdleManager != NULL)
                        QTIdleManagerNeedsAnIdle(globals->dataIdleManager, &needs);
                    if (needs || globals->dataRequested) {
                        dbg_printf("   - delaying idle, no data\n");
                        QTIdleManagerSetNextIdleTimeDelta(globals->idleManager, 1, 4); // delay 1/4s
                    }
                }
                _debug_idles(globals);
                dbg_printf("[OI  ]  dT [%08lx] :: StateProcess() = %ld\n", (UInt32) globals, result);
                if (result != noErr || globals->dataRequested || true) {
                    process = false;
                    break;
                }
            }
            globals->currentData = rb_data(&globals->dataRB);
            globals->validDataEnd = globals->currentData + rb_data_available(&globals->dataRB);

            while (result == noErr && FindPage(&globals->currentData, globals->validDataEnd, &og)) {
                result = ProcessPage(globals, &og);
                dbg_printf("   - << (:kStateReadingPages:) :: ProcessPage() = %ld\n", (long) result);
            }

            if (result != noErr) {
                process = false;
                break;
            } else {
                if (globals->calcTotalTime && globals->groupStreamsFound) {
                    if (globals->lp_GP > 0 && globals->totalTime == 0)
                        result = GetFileDurationFromGP(globals);

                    if (result != noErr) {
                        process = false;
                        break;
                    }

                    if (globals->phTrack == NULL && globals->totalTime > 0) {
                        // duration: from GP
                        /* result = */ CreatePlaceholderTrack(globals);
                        globals->calcTotalTime = false;
                        process = false;
                        break;
                    } else {
                        if (S64Compare(globals->dataOffset, S64Set(kBitrateEstimateBytesNeeded)) > 0) {
                            // duration: from bitrate
                            // TODO: do 'from GP' calculations here (again) taking into account possible stream start offsets
                            FlushAllStreams(globals, false);
                            result = EstimateFileDuration(globals);
                            /* result = */ CreatePlaceholderTrack(globals);
                            globals->calcTotalTime = false;
                            process = false;
                            break;
                        } else if (globals->phTrack == NULL) {
                            // temporary placeholder
                            globals->totalTime = kTempFileDuration * GetMovieTimeScale(globals->theMovie);
                            /* result = */ CreatePlaceholderTrack(globals);
                            if (globals->dataIsStream) {
                                process = false;
                                break;
                            }
                        }
                    }
                }

                if (globals->usingIdle) {
                    if (ShouldFlush(globals))
                        FlushAllStreams(globals, false);
                }
            }

            result = FillBuffer(globals);
            if (result == eofErr) {
                result = noErr;
                globals->state = kStateReadingLastPages;
            }

            break;

        case kStateReadingLastPages:
            dbg_printf("   - (:kStateReadingLastPages:)\n");
            globals->currentData = rb_data(&globals->dataRB);
            globals->validDataEnd = globals->currentData + rb_data_available(&globals->dataRB);

            dbg_printf("   + (:kStateReadingLastPages:)\n");
            while (result == noErr && FindPage(&globals->currentData, globals->validDataEnd, &og)) {
                result = ProcessPage(globals, &og);
                dbg_printf("   - << (:kStateReadingLastPages:) :: ProcessPage() = %ld\n", (long) result);
            }

            if (result != noErr) {
                process = false;
                break;
            }

            FlushAllStreams(globals, true);
            RemovePlaceholderTrack(globals);
            NotifyMovieChanged(globals, true);
            globals->state = kStateImportComplete;
            break;

        case kStateImportComplete:
            dbg_printf("   - (:kStateImportComplete:)\n");
            RemovePlaceholderTrack(globals);
            process = false;
            break;
        }
    }

    dbg_printf("<----< StateProcess() = %ld (%08lx)\n", result, result);
    return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void ReadCompletion(Ptr request, long refcon, OSErr readErr)
{
    OggImportGlobalsPtr globals = (OggImportGlobalsPtr) refcon;
    ComponentResult result = readErr;

    dbg_printf("---> ReadCompletion() called\n");
    if (readErr == noErr)
    {
        dbg_printf("--1- ReadCompletion() :: noErr\n");

        rb_sync_reserved(&globals->dataRB);
        globals->dataRequested = false;

        if (globals->idleManager != NULL) {
            dbg_printf("--2- ReadCompletion() :: requesting Idle\n");
            _debug_idles(globals);
            QTIdleManagerSetNextIdleTimeNow(globals->idleManager);
            _debug_idles(globals);
        }
    }

    if (result != noErr)
    {
        dbg_printf("--3- ReadCompletion() :: !noErr - %ld (%lx), eofErr: %d\n", result, result, result == eofErr);

        if (result == eofErr) {
            result = noErr;
            globals->dataRequested = false;
            globals->state = kStateImportComplete;
        }

        globals->errEncountered = result;

        /* close off every open stream */
        if (globals->streamCount > 0)
            CloseAllStreams(globals);
    }

    dbg_printf("---< ReadCompletion()\n");
}


static ComponentResult XQTGetFileSize(OggImportGlobalsPtr globals)
{
    ComponentResult err = badComponentSelector;
    SInt64 size64 = -1;

    dbg_printf("---> XQTGetFileSize() called\n");
    if (globals->dataCanDoGetFileSize64) {
        SInt64 tmp = S64Set(-1);
        wide size = SInt64ToWide(tmp);
        err = DataHGetFileSize64(globals->dataReader, &size);
        size64 = WideToSInt64(size);
        dbg_printf("---- :: size: %ld%ld, err: %ld (%lx)\n", size.hi, size.lo, (long)err, (long)err);
    } else {
        long size = -1;
        err = DataHGetFileSize(globals->dataReader, &size);
        size64 = S64Set(size);
        dbg_printf("---- :: size(32): %ld, err: %ld (%lx)\n", size, (long)err, (long)err);
    }

    if (err == noErr) {
        globals->dataEndOffset = size64;
        globals->sizeInitialised = true;
    } else if (err == notEnoughDataErr) {
        globals->dataEndOffset = S64Set(-1);
        globals->sizeInitialised = true;
        err = noErr;
    } else {
        globals->readError = err;
        globals->dataEndOffset = S64Set(-1);
    }

    dbg_printf("---< XQTGetFileSize() = %ld (%lx)\n", (long) err, (long) err);
    return err;
}

static ComponentResult StartImport(OggImportGlobalsPtr globals, Handle dataRef, OSType dataRefType)
{
    ComponentResult err = noErr;

    globals->state = kStateInitial;

    return err;
}

static ComponentResult JustStartImport(OggImportGlobalsPtr globals, Handle dataRef, OSType dataRefType) {
    ComponentResult ret = noErr;
    Boolean using_idles_p = globals->usingIdle;
    Boolean dont_idle = false;

    globals->state = kStateInitial;
    globals->usingIdle = false;
    globals->calcTotalTime = true;

    /* if limits have not been set, then try to get the size of the file. */
    if (S64Compare(globals->dataEndOffset, S64Set(-1)) == 0) {
        ret = XQTGetFileSize(globals);
    }

    if (ret != noErr) {
        return ret;
    } else if (S64Compare(globals->dataEndOffset, S64Set(0)) > 0) {
        if (S64Compare(S64Subtract(globals->dataEndOffset, globals->dataStartOffset), S64Set(kNoIdlingFileSizeLimit)) < 0 && !globals->dataIsStream)
            dont_idle = true;
        /* ret = */ GetLastFileGP(globals);
    }

    while (true) {
        ret = StateProcess(globals);
        if ((ret != noErr && ret != eofErr) || globals->dataIsStream || globals->state == kStateImportComplete || (!dont_idle && !globals->calcTotalTime)) {
            break;
        }
    }

    if (ret == eofErr)
        ret = noErr;

    if (ret == noErr) {
        FlushAllStreams(globals, true);
    }

    globals->usingIdle = using_idles_p;

    dbg_printf("-<<- JustStartImport(): %ld [mt: %8ld]\n", (long) ret, (long) GetMovieTime(globals->theMovie, NULL));
    return ret;
}

static ComponentResult JustImport(OggImportGlobalsPtr globals, Handle dataRef, OSType dataRefType) {
    ComponentResult ret = noErr;

    globals->state = kStateInitial;

    /* if limits have not been set, then try to get the size of the file. */
    if (S64Compare(globals->dataEndOffset, S64Set(-1)) == 0) {
        ret = XQTGetFileSize(globals);
    }

    if (ret != noErr)
        return ret;

    while (true) {
        ret = StateProcess(globals);
        if ((ret != noErr && ret != eofErr) || globals->state == kStateImportComplete)
            break;
    }

    if (ret == eofErr)
        ret = noErr;

    dbg_printf("-<<- JustImport(): %ld\n", (long)ret);
    return ret;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static ComponentResult SetupDataHandler(OggImportGlobalsPtr globals, Handle dataRef, OSType dataRefType)
{
    ComponentResult err = noErr;

    dbg_printf("---> SetupDataHandler(type: '%4.4s') called\n", &dataRefType);
    if (globals->dataReader == NULL)
    {
        Component dataHComponent = NULL;

        dataHComponent = GetDataHandler(dataRef, dataRefType, kDataHCanRead);

        err = OpenAComponent(dataHComponent, &globals->dataReader);

        dbg_printf("---- >> OpenAComponent() = %ld\n", (long)err);
        if (err == noErr)
        {
            err = DataHSetDataRef(globals->dataReader, dataRef);
            dbg_printf("---- >> DataHSetDataRef() = %ld\n", (long)err);
            if (err == noErr)
                err = DataHOpenForRead(globals->dataReader);

            //DataHPlaybackHints(globals->dataReader, 0, 0, -1, 49152);  // Don't care if it fails
            DataHPlaybackHints(globals->dataReader, 0, 0, -1, 2 * 1024 * 1024);  // Don't care if it fails

            if (err == noErr)
            {
                long blockSize = 1024;

                globals->dataOffset = S64Set(0);

                globals->dataRef = dataRef;
                globals->dataRefType = dataRefType;

                globals->dataCanDoAsyncRead = true;

                globals->dataCanDoScheduleData64 = (CallComponentCanDo(globals->dataReader, kDataHScheduleData64Select) == true);
                globals->dataCanDoScheduleData = (CallComponentCanDo(globals->dataReader, kDataHScheduleDataSelect) == true);

                if (!globals->dataCanDoScheduleData && !globals->dataCanDoScheduleData64)
                    err = cantFindHandler; // ??!
                else {
                    globals->dataCanDoGetFileSize64 = (CallComponentCanDo(globals->dataReader, kDataHGetFileSize64Select) == true);

                    globals->dataReadChunkSize = kDataBufferSize;
                    if ((globals->newMovieFlags & newMovieAsyncOK) != 0 && (dataRefType == URLDataHandlerSubType))
                        globals->dataReadChunkSize = kDataAsyncBufferSize;
                    err = DataHGetPreferredBlockSize(globals->dataReader, &blockSize);
                    if (err == noErr && blockSize < globals->dataReadChunkSize && blockSize > 1024)
                        globals->dataReadChunkSize = blockSize;
                    dbg_printf("     - allocating buffer, size: %d (prefBlockSize: %ld); ret = %ld\n",
                               globals->dataReadChunkSize, blockSize, (long)err);
                    err = noErr;	/* ignore any error and use our default read block size */

                    err = rb_init(&globals->dataRB, 2 * globals->dataReadChunkSize); //hmm why was it x2 ?

                    globals->currentData = (unsigned char *)globals->dataRB.buffer;
                    globals->validDataEnd = (unsigned char *)globals->dataRB.buffer;
                }
            }

            if (err == noErr)
            {
                globals->dataReadCompletion = NewDataHCompletionUPP(ReadCompletion);
                dbg_printf("     - dataReadCompletion: %8lx; err = %lx\n",
                           globals->dataReadCompletion, GetMoviesError());
            }

            if (err == noErr && globals->idleManager)
            {
                // purposely ignore the error message here, i.e. set it if the data handler supports it
                OggImportSetIdleManager(globals, globals->idleManager);
            }

            if (err == noErr)
            {
                // This logic is similar to the MP3 importer
                UInt32  flags = 0;

                globals->dataIsStream = (dataRefType == URLDataHandlerSubType);

                err = DataHGetInfoFlags(globals->dataReader, &flags);
                if (err == noErr && (flags & kDataHInfoFlagNeverStreams))
                    globals->dataIsStream = false;
                err = noErr;
                dbg_printf("---- -:: InfoFlags: NeverStreams: %d, CanUpdate...: %d, NeedsNet: %d\n",
                           (flags & kDataHInfoFlagNeverStreams) != 0,
                           (flags & kDataHInfoFlagCanUpdateDataRefs) != 0,
                           (flags & kDataHInfoFlagNeedsNetworkBandwidth) != 0);
            }
        }
    }

    dbg_printf("---< SetupDataHandler() = %ld\n", (long)err);
    return err;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			Component Routines
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportOpen(OggImportGlobalsPtr globals, ComponentInstance self)
{
    OSErr result;

    dbg_printf("-- Open() called\n");
    globals = (OggImportGlobalsPtr)NewPtrClear(sizeof(OggImportGlobals));
    if (globals != nil)
    {
        // set our storage pointer to our globals
        SetComponentInstanceStorage(self, (Handle) globals);
        globals->self = self;

        globals->dataEndOffset = S64Set(-1);
        globals->sizeInitialised = false;
        globals->idleManager = NULL;
        globals->dataIdleManager = NULL;
        globals->idleManagersLinked = false;
        globals->idleTimeBase = NULL;

        globals->totalTime = 0;
        globals->lp_serialno = 0;
        globals->lp_GP = S64Set(-1);
        globals->calcTotalTime = false;

        globals->phTrack = NULL;

        result = noErr;
    }
    else
        result = MemError();

    return (result);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportClose(OggImportGlobalsPtr globals, ComponentInstance self)
{
    ComponentResult result;
    (void)self;

    dbg_printf("-- Close() called\n");
    if (globals != nil) // we have some globals
    {
        if (globals->dataReadCompletion)
            DisposeDataHCompletionUPP(globals->dataReadCompletion);

        if (globals->dataReader)
        {
            result = CloseComponent(globals->dataReader);
            //FailMessage(result != noErr);		//@@@
            globals->dataReader = NULL;
        }

        if (globals->streamInfoHandle)
        {
            if (globals->streamCount > 0)
                CloseAllStreams(globals);
            DisposeHandle((Handle)globals->streamInfoHandle);
        }

        if (globals->dataBuffer)
        {
            DisposePtr(globals->dataBuffer);
            globals->dataBuffer = NULL;
        }

        if (globals->dataRB.buffer) {
            rb_free(&globals->dataRB);
        }

        if (globals->aliasHandle)
            DisposeHandle((Handle)globals->aliasHandle);

        if (globals->dataIdleManager != NULL)
            QTIdleManagerClose(globals->dataIdleManager);

        if (globals->idleTimeBase != NULL)
            DisposeTimeBase(globals->idleTimeBase);

        DisposePtr((Ptr)globals);
    }

    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportVersion(OggImportGlobalsPtr globals)
{
    dbg_printf("-- Version() called\n");
    return kOgg_eat__Version;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportSetOffsetAndLimit64(OggImportGlobalsPtr globals, const wide *offset,
                                           const wide *limit)
{
    dbg_printf("-- SetOffsetAndLimit64(%ld%ld, %ld%ld) called\n", offset->hi, offset->lo, limit->hi, limit->lo);
    globals->dataStartOffset = WideToSInt64(*offset);
    globals->dataEndOffset  = WideToSInt64(*limit);
    globals->sizeInitialised = true;

    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportSetOffsetAndLimit(OggImportGlobalsPtr globals, unsigned long offset,
                                         unsigned long limit)
{
    dbg_printf("-- SetOffsetAndLimit(%ld, %ld) called\n", offset, limit);
    globals->dataStartOffset = S64SetU(offset);
    globals->dataEndOffset = S64SetU(limit);
    globals->sizeInitialised = true;

    return noErr;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportValidate(OggImportGlobalsPtr globals,
                                const FSSpec *         theFile,
                                Handle                 theData,
                                Boolean *              valid)
{
    ComponentResult err = noErr;
    UInt8 extvalid = 0;

    dbg_printf("-- Validate() called\n");
    if (theFile == NULL)
    {
        Handle dataHandle = NewHandle(sizeof(HandleDataRefRecord));
        if (dataHandle != NULL)
        {
            (*(HandleDataRefRecord **)dataHandle)->dataHndl = theData;
            err = MovieImportValidateDataRef(globals->self,
                                             dataHandle,
                                             HandleDataHandlerSubType,
                                             &extvalid);
            DisposeHandle(dataHandle);
        }
    }
    else
    {
        AliasHandle alias = NULL;

        err = NewAliasMinimal(theFile, &alias);
        if (err == noErr)
        {
            err = MovieImportValidateDataRef(globals->self,
                                             (Handle)alias,
                                             rAliasType,
                                             &extvalid);

            DisposeHandle((Handle)alias);
        }
    }

    if (extvalid > 0)
        *valid = true;
    else
        *valid = false;

    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportValidateDataRef(OggImportGlobalsPtr globals,
                                       Handle                 dataRef,
                                       OSType                 dataRefType,
                                       UInt8 *                valid)
{
    ComponentResult err = noErr;
    dbg_printf("-- ValidateDataRef() called\n");

    *valid = 128;

    return err;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportFile(OggImportGlobalsPtr globals, const FSSpec *theFile,
                            Movie theMovie, Track targetTrack, Track *usedTrack,
                            TimeValue atTime, TimeValue *durationAdded,
                            long inFlags, long *outFlags)
{
    ComponentResult err = noErr;
    AliasHandle alias = NULL;

    dbg_printf("-- File() called\n");

    *outFlags = 0;

    err = NewAliasMinimal(theFile, &alias);
    if (err == noErr)
    {
        err = MovieImportDataRef(globals->self,
                                 (Handle)alias,
                                 rAliasType,
                                 theMovie,
                                 targetTrack,
                                 usedTrack,
                                 atTime,
                                 durationAdded,
                                 inFlags,
                                 outFlags);

        if (!(*outFlags & movieImportResultNeedIdles))
            DisposeHandle((Handle) alias);
        else
            globals->aliasHandle = alias;
    }

    return err;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportGetMIMETypeList(OggImportGlobalsPtr globals, QTAtomContainer *retMimeInfo)
{
    dbg_printf("-- GetMIMETypeList() called\n");
    return GetComponentResource((Component)globals->self, FOUR_CHAR_CODE('mime'), kImporterResID, (Handle *)retMimeInfo);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportGetFileType(OggImportGlobalsPtr globals, OSType *fileType)
{
    dbg_printf("-- GetFileType() called\n");
    *fileType = kCodecFormat;
    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportSetChunkSize(OggImportGlobalsPtr globals, long chunkSize)
{
    ComponentResult err = noErr;

    dbg_printf("-- ImportSetChunkSize(%ld) called\n", chunkSize);
    if (chunkSize > 2048 && chunkSize < 204800)
        globals->chunkSize = chunkSize;
    else
        err = paramErr;

    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportIdle(OggImportGlobalsPtr globals,
                            long  inFlags,
                            long *outFlags)
{
    ComponentResult err = noErr;
    dbg_printf("[OI  ]  >> [%08lx] :: Idle()  at: %ld\n", (UInt32) globals, TickCount());

    if (globals->state == kStateImportComplete) {
        *outFlags |= movieImportResultComplete;
        RemovePlaceholderTrack(globals);
    } else {
        /* err = */ QTIdleManagerSetNextIdleTimeNow(globals->idleManager);
        err = StateProcess(globals);
    }

#if 1
    _debug_idles(globals);
#endif

    dbg_printf("[OI  ] <   [%08lx] :: Idle()  at: %ld\n", (UInt32) globals, TickCount());
    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportDataRef(OggImportGlobalsPtr globals, Handle dataRef,
                               OSType dataRefType, Movie theMovie,
                               Track targetTrack, Track *usedTrack,
                               TimeValue atTime, TimeValue *durationAdded,
                               long inFlags, long *outFlags)
{
    ComponentResult err = noErr;

    *outFlags = 0;

    globals->theMovie = theMovie;
    globals->startTime = 0;
    if (atTime >= 0)
        globals->startTime = atTime;

    dbg_printf("-- DataRef(at:%ld) called\n", atTime);
    if (GetHandleSize(dataRef) < 256) {
        dbg_printf("-- - DataRef: \"%s\"\n", *dataRef);
    } else {
        dbg_printf("-- - DataRef: '%c'\n", *dataRef[0]);
    }

    dbg_printf("    theMovie: %lx,  targetTrack: %lx\n", theMovie, targetTrack);
    dbg_printf("    track count: %ld\n", GetMovieTrackCount(theMovie));
    dbg_printf("    flags:\n\tmovieImportCreateTrack:%d\n\tmovieImportInParallel:%d\n"
               "\tmovieImportMustUseTrack:%d\n\tmovieImportWithIdle:%d\n",
               (inFlags & movieImportCreateTrack)  != 0,
               (inFlags & movieImportInParallel)   != 0,
               (inFlags & movieImportMustUseTrack) != 0,
               (inFlags & movieImportWithIdle)     != 0);
    dbg_printf("     : importing at: %ld, added: %ld\n", atTime, *durationAdded);
    err = SetupDataHandler(globals, dataRef, dataRefType);
    if (err == noErr)
        dbg_printf("    SetupDataHandler() succeeded\n");

#if 0
    globals->usingIdle = (globals->dataIsStream
                          && (inFlags & movieImportWithIdle) != 0);

    if (dataRefType != URLDataHandlerSubType)
        globals->usingIdle = false;
#else
    globals->usingIdle = ((inFlags & movieImportWithIdle) != 0);
#endif /* 1 */

    dbg_printf("--> 2: globals->usingIdle: %d\n", globals->usingIdle);

    if (globals->usingIdle) {
        globals->notifyStep = 420; // ~60 ticks per second
        globals->flushStepNormal = 90;
        globals->flushStepCheck = 30;
        // 5 seconds, assuming movie timescale is static
        globals->flushMinTimeLeft = 5 * GetMovieTimeScale(globals->theMovie);
#if 1
        err = JustStartImport(globals, dataRef, dataRefType);
#else
        err = StartImport(globals, dataRef, dataRefType);
#endif /* 1 */
        if (err == noErr && globals->state == kStateImportComplete) {
            *outFlags &= !movieImportResultNeedIdles;
            *outFlags |= movieImportResultComplete;
            *durationAdded = globals->timeLoaded;
            if (globals->numTracksSeen == 1)
                *usedTrack = globals->firstTrack;
        } else {
            *outFlags |= movieImportResultNeedIdles;
            *durationAdded = 0;
        }
    } else {
        err = JustImport(globals, dataRef, dataRefType);
        *outFlags &= !movieImportResultNeedIdles;
        if (err == noErr) {
            *outFlags |= movieImportResultComplete;
            *durationAdded = globals->timeLoaded;
            if (globals->numTracksSeen == 1)
                *usedTrack = globals->firstTrack;
        }
    }

    //globals->usingIdle = true;
    //*outFlags |= movieImportResultNeedIdles;

    dbg_printf("-< DataRef(at:%ld)\n", atTime);
    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportGetLoadState(OggImportGlobalsPtr globals, long *loadState)
{
    ComponentResult err = noErr;

    dbg_printf("-- GetLoadState() called\n");
    switch (globals->state)
    {
    case kStateInitial:
        *loadState = kMovieLoadStateLoading;
        break;

    case kStateReadingPages:
    case kStateReadingLastPages:
        if (globals->timeFlushed > globals->flushMinTimeLeft) {
            if (globals->sizeInitialised && S64Compare(globals->dataEndOffset, S64Set(-1)) == 0) {
                *loadState = kMovieLoadStatePlaythroughOK;
            } else if (globals->sizeInitialised && globals->totalTime > 0) {
                TimeBase wctb;
                *loadState = kMovieLoadStatePlayable;

                if (!QTGetWallClockTimeBase(&wctb)) {
                    TimeRecord dl_tr;
                    TimeRecord wc_tr;
                    TimeValue mt;
                    SInt64 mt_64;
                    TimeScale ts = GetMovieTimeScale(globals->theMovie);

                    err = OggImportEstimateCompletionTime(globals, &dl_tr);
                    if (!err) {
                        mt = GetMovieDuration(globals->theMovie) - GetMovieTime(globals->theMovie, NULL);
                        mt_64 = S64Set(mt);
                        ConvertTimeScale(&dl_tr, ts);
                        GetTimeBaseTime(wctb, ts, &wc_tr);
                        mt_64 = S64Add(WideToSInt64(wc_tr.value), mt_64);
                        if (S64Compare(mt_64, WideToSInt64(dl_tr.value)) > 0)
                            *loadState = kMovieLoadStatePlaythroughOK;
                    } else {
                        err = noErr;
                    }
                }
            } else {
                *loadState = kMovieLoadStatePlayable;
            }
        } else if (globals->timeFlushed > 0) {
            *loadState = kMovieLoadStatePlayable;
        } else {
            *loadState = kMovieLoadStateLoading;
        }

        break;

    case kStateImportComplete:
        *loadState = kMovieLoadStateComplete;
        break;
    }

    dbg_printf("-- GetLoadState returning %ld\n", *loadState);

    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportGetMaxLoadedTime(OggImportGlobalsPtr globals, TimeValue *time)
{
    dbg_printf("-- GetMaxLoadedTime() called: %8ld [%8ld] (at: %ld)\n", globals->timeFlushed, globals->timeLoaded, TickCount());

    *time = globals->timeFlushed;

    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportEstimateCompletionTime(OggImportGlobalsPtr globals, TimeRecord *time)
{
    unsigned long timeUsed = TickCount() - globals->startTickCount;
    SInt64        dataUsed = S64Subtract(globals->dataOffset, globals->dataStartOffset);
    SInt64        dataLeft = S64Subtract(globals->dataEndOffset, globals->dataOffset);
    SInt64        ratio    = S64Div(S64Multiply(S64Set(timeUsed), dataLeft), dataUsed);

    dbg_printf("-- EstimateCompletionTime() called: ratio = %lld\n", ratio);

    time->value = SInt64ToWide(ratio);
    time->scale = 60; // roughly TickCount()'s resolution
    time->base  = NULL;   // this time record is a duration

    return noErr;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportSetDontBlock(OggImportGlobalsPtr globals, Boolean  dontBlock)
{
    dbg_printf("-- SetDontBlock(%d) called\n", dontBlock);
    globals->blocking = dontBlock;
    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportGetDontBlock(OggImportGlobalsPtr globals, Boolean  *willBlock)
{
    dbg_printf("-- GetDontBlock() called\n");
    *willBlock = globals->blocking;
    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportSetIdleManager(OggImportGlobalsPtr globals, IdleManager im)
{
    ComponentResult err = noErr;

    dbg_printf("-- SetIdleManager() called\n");
    globals->idleManager = im;

    if (globals->dataReader) {
        if (CallComponentCanDo(globals->dataReader, kDataHSetIdleManagerSelect) == true) {
            globals->dataIdleManager = QTIdleManagerOpen();
            if (globals->dataIdleManager != NULL) {
                err = DataHSetIdleManager(globals->dataReader, globals->dataIdleManager);
                dbg_printf("--  -- SetIdleManager(dataReader) = %ld\n", (long)err);
                if (err) {
                    QTIdleManagerClose(globals->dataIdleManager);
                    globals->dataIdleManager = NULL;
                    err = noErr;
                } else if (!globals->dataIsStream) {
                    err = QTIdleManagerSetParent(globals->dataIdleManager, globals->idleManager);
                    dbg_printf("--  -- SetParentIdleManager() = %ld\n", (long)err);
                    if (!err)
                        globals->idleManagersLinked = true;
                    //err = noErr;
                }
            }
        } else {
            dbg_printf("--  -- SetIdleManager(dataReader) = DOESN'T SUPPORT IDLE!!\n");
        }
    }

    if (!err) {
        QTIdleManagerSetNextIdleTimeNow(globals->idleManager);
        _debug_idles(globals);
    }

    return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC OggImportSetNewMovieFlags(OggImportGlobalsPtr globals, long flags)
{
    dbg_printf("-- SetNewMovieFlags() called: %08lx\n", flags);

    globals->newMovieFlags = flags;
    return noErr;
}
