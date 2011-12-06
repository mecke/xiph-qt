/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE QuickTime Components SOURCE CODE.       *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE QuickTime Components SOURCE CODE IS (C) COPYRIGHT 2001       *
 * by Steve Nicolai http://qtcomponents.sourceforge.net/            *
 *                                                                  *
 ********************************************************************

 function: Common Ogg/Vorbis functions across import/decompress
 last mod: $$

 ********************************************************************/

#if __APPLE_CC__
    #include <QuickTime/QuickTime.h>
#else
    #include <MacTypes.h>
    #include <MacErrors.h>
    #include <Endian.h>
    #include <MacMemory.h>
    #include <Components.h>
    #include <Sound.h>
    #include <QuickTimeComponents.h>
    #include <FixMath.h>
    #include <Math64.h>
#endif

#include <Ogg/ogg.h>
#include <Vorbis/codec.h>


Boolean FindPage(unsigned char **data, const unsigned char *end, ogg_page *og);

Boolean FindPageNoCRC(unsigned char **data, const unsigned char *end, ogg_page *og);

OSErr LoadVorbisHeaders(Ptr header, int size, int findPackets, long *serialno, vorbis_info *vi, vorbis_comment *vc);
