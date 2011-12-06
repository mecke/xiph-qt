/*
 *  oggexport_versions.h
 *
 *    The current version of the OggExport component.
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
 *  Last modified: $Id: oggexport_versions.h 16058 2009-05-29 18:07:42Z arek $
 *
 */


#if !defined(__oggexport_versions_h__)
#define __oggexport_versions_h__

#ifdef DEBUG
#define kOgg_spit__Version		(0x00FF0102)
#else
#define kOgg_spit__Version		(0x00000102)
#endif /* DEBUG */

#endif /* __oggexport_versions_h__ */
