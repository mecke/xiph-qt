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


#include "common.h"

#include <string.h>


/* returns false for not enough data, true for found a page
   the page checksum isn't checked here, it's checked in the
   next layer up, FindPage.  Advances the data pointer past
   data as it returns it.
 */
static Boolean InternalFindPage(unsigned char **data, const unsigned char *end, ogg_page *op)
{
    unsigned char *p = *data;
    int headerBytes, i, bodyBytes = 0;
    
    if (end - p < 27)
        return false;

    /* look for the page start marker */
    while (p != NULL && p < end && (memcmp(p, "OggS", 4)))
    {
        *data = p;
        p = (unsigned char *)memchr(p + 1, 'O', (end - p) - 1);
    }
    
    if (p == NULL || end - p < 27)
        return false;

    headerBytes = p[26] + 27;
    
    if (end - p < headerBytes)
        return false;
    
    for (i = 0; i < p[26]; i++)
      bodyBytes += p[27 + i];
    
    if (p + headerBytes + bodyBytes > end)
        return false;
    
    op->header = p;
    op->header_len = headerBytes;
    op->body = p + headerBytes;
    op->body_len = bodyBytes;
    
    *data = p + headerBytes + bodyBytes;

    return true;
}

static Boolean CheckChecksum(ogg_page *og)
{
    /* Grab the checksum bytes, set the header field to zero */
    char chksum[4];
    
    memcpy(chksum,og->header+22,4);
    memset(og->header+22,0,4);
    
    ogg_page_checksum_set(og);
    
    /* Compare */
    if(memcmp(chksum,og->header+22,4))
    {
        /* replace the computed checksum with the one actually read in */
        memcpy(og->header+22,chksum,4);
        return false;
    }
    return true;
}


Boolean FindPage(unsigned char **data, const unsigned char *end, ogg_page *og)
{
    Boolean	foundPage;
    do {
        foundPage = InternalFindPage(data, end, og);
        
        if (!foundPage)
            break;	/* didn't find a page, indicate more data needed */

        if (CheckChecksum(og)) /* everything checks out, start next search after this page */
            break;
        else
        { /* checksum is wrong, look for next page start */
            unsigned char *t = memchr(*data + 1, 'O', (end - *data) - 1);
            if (t == NULL)
            { /* no candidate found, indicate more data needed */
                foundPage = false;
                break;
            }
            *data = t;
        }
    } while (1);
    
    return foundPage;
}


inline Boolean FindPageNoCRC(unsigned char **data, const unsigned char *end, ogg_page *og)
{
	return InternalFindPage(data, end, og);
}


OSErr LoadVorbisHeaders(Ptr header, int size, int findPackets, long *serialno, vorbis_info *vi, vorbis_comment *vc)
{
    OSErr	     err = noErr;
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */

    int				 numPackets = 0;
    unsigned char *	 page = (unsigned char *)header;
    const unsigned char *    end = page + size;

    /* Get the first page. */
    if (!InternalFindPage(&page, end, &og))
    {
        /* have we simply run out of data?  If so, we're done. */
        return endOfDataReached;
    }
    
    /* Get the serial number and set up the rest of decode. */
    /* serialno first; use it to set up a logical stream */
    *serialno = ogg_page_serialno(&og);
    ogg_stream_init(&os, *serialno);
    
    /* extract the initial header from the first page and verify that the
       Ogg bitstream is in fact Vorbis data */
    
    /* I handle the initial header first instead of just having the code
       read all three Vorbis headers at once because reading the initial
       header is an easy way to identify a Vorbis bitstream and it's
       useful to see that functionality seperated out. */
    
    vorbis_info_clear(vi);
    vorbis_info_init(vi);
    vorbis_comment_init(vc);
    do {
        if (ogg_stream_pagein(&os, &og) < 0)
        { 
            err = invalidMedia;			/* error; stream version mismatch perhaps */
            break;
        }
        
        while (numPackets < findPackets)
        {
            if (ogg_stream_packetout(&os, &op) != 1)
                break;
            
            numPackets++;
            if (vorbis_synthesis_headerin(vi, vc, &op) < 0)
            { 
                err = noSoundTrackInMovieErr;	/* This Ogg bitstream does not contain Vorbis audio data */
                break;
            }
        }
        if (numPackets < findPackets) {
            if (!InternalFindPage(&page, end, &og))
            {
                err = endOfDataReached;	/* have we simply run out of data?  If so, we're done. */
                break;
            }
        }
    } while (numPackets < findPackets);

    ogg_stream_clear(&os);
    
    return err;
}


