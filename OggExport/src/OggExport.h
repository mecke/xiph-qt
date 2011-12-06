/*
 *  OggExport.h
 *
 *    OggExport.h - some constants definitions.
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
 *  Last modified: $Id: OggExport.h 12347 2007-01-18 15:16:53Z arek $
 *
 */


#ifndef __OGGEXPORT_H__
#define __OGGEXPORT_H__ 1

#include "config.h"
#include "oggexport_versions.h"

#if !defined(XIPHQT_BUNDLE_ID)
#define kOggExportBundleID "org.xiph.xiph-qt.oggexport"
#else
#define kOggExportBundleID kXiphQTBundleID
#endif  /* XIPHQT_BUNDLE_ID */

#define kExporterResID                  4040
#define kExporterNameStringResID        4040
#define kExporterInfoStringResID        4041

#define kSoundComponentManufacturer     'Xiph'
#define kCodecFormat                    'OggS'


#ifdef _DEBUG
#define OggExporterName        "Ogg Vorbis Exporter"
#else
#define OggExporterName        ""
#endif

#endif /* __OGGEXPORT_H__ */
