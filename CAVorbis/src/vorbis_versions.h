/*
 *  vorbis_versions.h
 *
 *    The current version of the Vorbis component.
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
 *  Last modified: $Id: vorbis_versions.h 12814 2007-03-27 22:09:32Z arek $
 *
 */


#if !defined(__vorbis_versions_h__)
#define __vorbis_versions_h__


#ifdef DEBUG
#define kCAVorbis_adec_Version		(0x00FF0109)
#define kCAVorbis_aenc_Version		(0x00FF0101)
#else
#define kCAVorbis_adec_Version		(0x00000109)
#define kCAVorbis_aenc_Version		(0x00000101)
#endif /* DEBUG */


#endif /* __vorbis_versions_h__ */
