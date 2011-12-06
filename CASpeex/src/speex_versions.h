/*
 *  speex_versions.h
 *
 *    The current version of the Speex component.
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
 *  Last modified: $Id: speex_versions.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__speex_versions_h__)
#define __speex_versions_h__


#ifdef DEBUG
#define kCASpeex_adec_Version		(0x00FF0104)
#else
#define kCASpeex_adec_Version		(0x00000104)
#endif /* DEBUG */


#endif /* __speex_versions_h__ */
