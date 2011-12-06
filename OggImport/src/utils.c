/*
 *  utils.c
 *
 *    Small support functions for ogg processing.
 *
 *
 *  Copyright (c) 2006,2007  Arek Korbik
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
 *  Last modified: $Id: utils.c 12754 2007-03-14 03:51:23Z arek $
 *
 */


#include "debug.h"
#include "utils.h"

extern Boolean FindPage(unsigned char **data, const unsigned char *end,
                        ogg_page *og);

int unpack_vorbis_comments(vorbis_comment *vc, const void *data, UInt32 data_size)
{
    int i;
    char *dptr = (char *) data;
    char *dend = dptr + data_size;
    int len = EndianU32_LtoN(*(UInt32 *)dptr);
    int commnum = 0;
    char save;

    dptr += 4 + len;
    if (len >= 0 && dptr < dend) {
        commnum = EndianU32_LtoN(*(UInt32 *)dptr);
        if (commnum >= 0) {
            dptr += 4;

            for (i = 0; i < commnum && dptr <= dend; i++) {
                len = EndianU32_LtoN(*(UInt32 *)dptr);
                dptr += 4;
                if (dptr + len > dend)
                    break;

                save = *(dptr + len);
                *(dptr + len) = '\0';
                vorbis_comment_add(vc, dptr);
                dptr += len;
                *dptr = save;
            }
        }
    }

    return 0;
}

void find_last_page_GP(const unsigned char *data, UInt32 data_size,
                       ogg_int64_t *gp, long *serialno)
{
    unsigned char *ptr = (unsigned char *) data;
    const unsigned char *end = data + data_size;
    ogg_page op;

    *gp = -1;
    *serialno = 0;

    while (FindPage(&ptr, end, &op)) {
        if (ogg_page_granulepos(&op) > 0) {
            *gp = ogg_page_granulepos(&op);
            *serialno = ogg_page_serialno(&op);
        }
    }
}
