/*
 *  OggExport.r
 *
 *    Information bit definitions for the 'thng' and other OggExport
 *    resources.
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
 *  Last modified: $Id: OggExport.r 12441 2007-02-06 19:27:12Z j $
 *
 */


#define thng_RezTemplateVersion 2

#define cfrg_RezTemplateVersion 1

#ifdef TARGET_REZ_MAC_PPC
#include <CoreServices/CoreServices.r>
#include <QuickTime/QuickTime.r>
#include <QuickTime/QuickTimeComponents.r>
#else
#include "ConditionalMacros.r"
#include "CoreServices.r"
#include "QuickTimeComponents.r"
#endif /* TARGET_REZ_MAC_PPC */

#include "OggExport.h"

#define kExporterComponentType 'spit'


/* How do I do this properly... anybody? */
#if defined(BUILD_UNIVERSAL)
  #define TARGET_CPU_PPC 1
  #define TARGET_CPU_X86 1
#endif


#ifndef cmpThreadSafe
#define cmpThreadSafe	0x10000000
#endif

#if TARGET_OS_MAC
  #if TARGET_CPU_PPC && TARGET_CPU_X86
    #define TARGET_REZ_FAT_COMPONENTS 1
    #define Target_PlatformType       platformPowerPCNativeEntryPoint
    #define Target_SecondPlatformType platformIA32NativeEntryPoint
  #elif TARGET_CPU_X86
    #define Target_PlatformType       platformIA32NativeEntryPoint
  #else
    #define Target_PlatformType       platformPowerPCNativeEntryPoint
  #endif
#elif TARGET_OS_WIN32
  #define Target_PlatformType platformWin32
#else
  #error get a real platform type
#endif /* TARGET_OS_MAC */

#if !defined(TARGET_REZ_FAT_COMPONENTS)
  #define TARGET_REZ_FAT_COMPONENTS 0
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Ogg Exporter

#define kExporterFlags canMovieExportFiles | canMovieExportValidateMovie | \
    canMovieExportFromProcedures | movieExportMustGetSourceMediaType | \
    hasMovieExportUserInterface | cmpThreadSafe


resource 'thng' (kExporterResID, OggExporterName, purgeable) {
    kExporterComponentType, kCodecFormat, 'Xiph',
    0, 0, 0, 0,
    'STR ', kExporterNameStringResID,
    'STR ', kExporterInfoStringResID,
    0, 0,		// no icon
    kOgg_spit__Version,
    componentDoAutoVersion|componentHasMultiplePlatforms, 0,
    {
        // COMPONENT PLATFORM INFORMATION ----------------------
        kExporterFlags,
        'dlle',                                 // Code Resource type - Entry point found by symbol name 'dlle' resource
        kExporterResID,                         // ID of 'dlle' resource
        Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
        kExporterFlags,
        'dlle',
        kExporterResID,
        Target_SecondPlatformType,
#endif
    },
    'thnr', kExporterResID
};

resource 'thnr' (kExporterResID, OggExporterName, purgeable) {
    {
        'mime', 1, 0, 'mime', kExporterResID, cmpResourceNoFlags,
        'src#', 1, 0, 'src#', kExporterResID, cmpResourceNoFlags,
        'src#', 2, 0, 'trk#', kExporterResID, cmpResourceNoFlags,
    };
};


resource 'dlle' (kExporterResID, OggExporterName) {
    "OggExportComponentDispatch"
};

resource 'STR ' (kExporterNameStringResID, OggExporterName, purgeable) {
    /* "Ogg Multimedia Bitstream (OGG)" */
    "Ogg"
};

resource 'STR ' (kExporterInfoStringResID, OggExporterName, purgeable) {
    "Ogg " "0.1.1" " (See http://www.xiph.org)."
};


resource 'src#' (kExporterResID, OggExporterName, purgeable) {
    {
        'vide', 0, 65535, isSourceType,
        'soun', 0, 65535, isSourceType;
    };
};

resource 'trk#' (kExporterResID, OggExporterName, purgeable) {
    {
        'eyes', 0, 65535, isMediaCharacteristic,
        'ears', 0, 65535, isMediaCharacteristic;
    };
};
