/*
 *  OggImport.h
 *
 *    OggImport.h - some constants definitions.
 *
 *
 *  Copyright (c) 2005  Arek Korbik
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
 *  Last modified: $Id: OggImport.h 12347 2007-01-18 15:16:53Z arek $
 *
 */


#ifndef __OGGIMPORT_H__
#define __OGGIMPORT_H__ 1

#include "config.h"
#include "versions.h"

#if !defined(XIPHQT_BUNDLE_ID)
#define kOggVorbisBundleID "org.xiph.xiph-qt.oggimport"
#else
#define kOggVorbisBundleID kXiphQTBundleID
#endif  /* XIPHQT_BUNDLE_ID */

#define kImporterResID                  4000
#define kImporterNameStringResID        4000
#define kImporterInfoStringResID        4001

#define kSoundComponentManufacturer     'Xiph'	// your company's OSType
#define kCodecFormat                    'OggS'


#ifdef _DEBUG
#define OggImporterName        "Ogg Vorbis Importer"
#else
#define OggImporterName        ""
#endif

#endif /* __OGGIMPORT_H__ */
