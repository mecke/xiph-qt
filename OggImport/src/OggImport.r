/*
 *  OggImport.r
 *
 *    Information bit definitions for the 'thng' and other OggImport
 *    resources.
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
 *  Last modified: $Id: OggImport.r 13693 2007-09-01 15:09:39Z arek $
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

#include "OggImport.h"

#define kImporterComponentType 'eat '


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
// Ogg Importer

#define kImporterFlags canMovieImportFiles | canMovieImportValidateFile | \
    canMovieImportPartial | canMovieImportInPlace | hasMovieImportMIMEList | \
    canMovieImportDataReferences | canMovieImportWithIdle |             \
    canMovieImportAvoidBlocking | canMovieImportValidateDataReferences | \
    cmpThreadSafe


resource 'thng' (kImporterResID, OggImporterName, purgeable) {
    kImporterComponentType, kCodecFormat, 'vide',
    0, 0, 0, 0,
    'STR ', kImporterNameStringResID,
    'STR ', kImporterInfoStringResID,
    0, 0,		// no icon
    kOgg_eat__Version,
    componentDoAutoVersion|componentHasMultiplePlatforms, 0,
    {
        // COMPONENT PLATFORM INFORMATION ----------------------
        kImporterFlags,
        'dlle',                                 // Code Resource type - Entry point found by symbol name 'dlle' resource
        kImporterResID,                         // ID of 'dlle' resource
        Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
        kImporterFlags,
        'dlle',
        kImporterResID,
        Target_SecondPlatformType,
#endif
    },
    'thnr', kImporterResID
};

// Component Alias
resource 'thga' (kImporterResID, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'OGG ',                             // Subtype - this must be in uppercase.
                                        // It will match an ".ogg" suffix case-insensitively.
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
    0,                                  // Component Flags Mask
    0,                                  // Code Type
    0,                                  // Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0,                                  // Icon Type
    0,                                  // Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags,                     // Component Flags
    0,                                  // Component Flags Mask
    'thnr', kImporterResID, 0
};

// Component Alias
resource 'thga' (kImporterResID + 1, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'OGM ',                             // Subtype - this must be in uppercase.
                                        // It will match an ".ogm" suffix case-insensitively.
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
    0,                                  // Component Flags Mask
    0,                                  // Code Type
    0,                                  // Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0,                                  // Icon Type
    0,                                  // Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags,                     // Component Flags
    0,                                  // Component Flags Mask
    'thnr', kImporterResID, 0
};

// Component Alias
resource 'thga' (kImporterResID + 2, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'SPX ',                             // Subtype - this must be in uppercase.
                                        // It will match an ".spx" suffix case-insensitively.
    'soun',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
    0,                                  // Component Flags Mask
    0,                                  // Code Type
    0,                                  // Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0,                                  // Icon Type
    0,                                  // Icon ID
                                        // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags,                     // Component Flags
    0,                                  // Component Flags Mask
    'thnr', kImporterResID, 0
};

// Component Alias
resource 'thga' (kImporterResID + 3, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'ANX ',                             // Subtype - this must be in uppercase.
                                        // It will match an ".anx" suffix case-insensitively.
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
    0,                                  // Component Flags Mask
    0,                                  // Code Type
    0,                                  // Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0,                                  // Icon Type
    0,                                  // Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags,                     // Component Flags
    0,                                  // Component Flags Mask
    'thnr', kImporterResID, 0
};

// support the new proposed file extensions, see:
// http://wiki.xiph.org/index.php/MIME_Types_and_File_Extensions

//   .ogx - Ogg Multiplex Profile (anything in Ogg)
resource 'thga' (kImporterResID + 4, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'OGX ',                             // Subtype
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,
    0,                                  // Component Flags Mask
    0, 0,                               // Code Type, Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0, 0,                               // Icon Type, Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags, 0,                  // Component Flags and Mask
    'thnr', kImporterResID, 0
};

//   .oga - Ogg Audio Profile (audio in Ogg container)
resource 'thga' (kImporterResID + 5, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'OGA ',                             // Subtype
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,
    0,                                  // Component Flags Mask
    0, 0,                               // Code Type, Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0, 0,                               // Icon Type, Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags, 0,                  // Component Flags and Mask
    'thnr', kImporterResID, 0
};

//   .ogv - Ogg Video Profile (a/v in Ogg container)
resource 'thga' (kImporterResID + 6, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'OGV ',                             // Subtype
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,
    0,                                  // Component Flags Mask
    0, 0,                               // Code Type, Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0, 0,                               // Icon Type, Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags, 0,                  // Component Flags and Mask
    'thnr', kImporterResID, 0
};

//   .axa - Profile for audio in Annodex
resource 'thga' (kImporterResID + 7, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'AXA ',                             // Subtype
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,
    0,                                  // Component Flags Mask
    0, 0,                               // Code Type, Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0, 0,                               // Icon Type, Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags, 0,                  // Component Flags and Mask
    'thnr', kImporterResID, 0
};

//   .axv - Profile for video in Annodex
resource 'thga' (kImporterResID + 8, OggImporterName, purgeable) {
    kImporterComponentType,             // Type
    'AXV ',                             // Subtype
    'vide',                             // Manufacturer
    kImporterFlags | movieImportSubTypeIsFileExtension,
    0,                                  // Component Flags Mask
    0, 0,                               // Code Type, Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID
    0, 0,                               // Icon Type, Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,             // Type
    kCodecFormat,                       // SubType
    'vide',                             // Manufaturer
    kImporterFlags, 0,                  // Component Flags and Mask
    'thnr', kImporterResID, 0
};


resource 'thnr' (kImporterResID, OggImporterName, purgeable) {
    {
        'mime', 1, 0, 'mime', kImporterResID, cmpResourceNoFlags,
        'mcfg', 1, 0, 'mcfg', kImporterResID, cmpResourceNoFlags,
    };
};


resource 'dlle' (kImporterResID, OggImporterName) {
    "OggImportComponentDispatch"
};

// name and info string are shared by the compressor and decompressor
resource 'STR ' (kImporterNameStringResID, OggImporterName, purgeable) {
    "Ogg Importer"
};

resource 'STR ' (kImporterInfoStringResID, OggImporterName, purgeable) {
    "Ogg " "0.1.6" " (See http://www.xiph.org)."
};


/*
    This is an example of how to build an atom container resource to hold mime types.
    This component's GetMIMETypeList implementation simply loads this resource and returns it.
    Please note that atoms of the same type MUST be grouped together within an atom container.
*/
resource 'mime' (kImporterResID, OggImporterName, purgeable) {
    {
        kMimeInfoMimeTypeTag,       1, "application/ogg";
        kMimeInfoMimeTypeTag,       2, "application/x-ogg";
        kMimeInfoMimeTypeTag,       3, "audio/x-speex";
        kMimeInfoMimeTypeTag,       4, "audio/speex";
        kMimeInfoMimeTypeTag,       5, "application/x-annodex";
        kMimeInfoMimeTypeTag,       6, "application/annodex";
        kMimeInfoMimeTypeTag,       7, "application/ogg";
        kMimeInfoMimeTypeTag,       8, "application/x-ogg";
        kMimeInfoMimeTypeTag,       9, "audio/x-ogg";
        kMimeInfoMimeTypeTag,      10, "audio/ogg";
        kMimeInfoMimeTypeTag,      11, "video/x-ogg";
        kMimeInfoMimeTypeTag,      12, "video/ogg";
        kMimeInfoMimeTypeTag,      13, "audio/x-annodex";
        kMimeInfoMimeTypeTag,      14, "audio/annodex";
        kMimeInfoMimeTypeTag,      15, "video/x-annodex";
        kMimeInfoMimeTypeTag,      16, "video/annodex";

        kMimeInfoFileExtensionTag,  1, "ogg";
        kMimeInfoFileExtensionTag,  2, "ogg";
        kMimeInfoFileExtensionTag,  3, "spx";
        kMimeInfoFileExtensionTag,  4, "spx";
        kMimeInfoFileExtensionTag,  5, "anx";
        kMimeInfoFileExtensionTag,  6, "anx";
        kMimeInfoFileExtensionTag,  7, "ogx";
        kMimeInfoFileExtensionTag,  8, "ogx";
        kMimeInfoFileExtensionTag,  9, "oga";
        kMimeInfoFileExtensionTag, 10, "oga";
        kMimeInfoFileExtensionTag, 11, "ogv";
        kMimeInfoFileExtensionTag, 12, "ogv";
        kMimeInfoFileExtensionTag, 13, "axa";
        kMimeInfoFileExtensionTag, 14, "axa";
        kMimeInfoFileExtensionTag, 15, "axv";
        kMimeInfoFileExtensionTag, 16, "axv";

        kMimeInfoDescriptionTag,    1, "Ogg Multimedia Bitstream";
        kMimeInfoDescriptionTag,    2, "Ogg Multimedia Bitstream";
        kMimeInfoDescriptionTag,    3, "Ogg Speex";
        kMimeInfoDescriptionTag,    4, "Ogg Speex";
        kMimeInfoDescriptionTag,    5, "Annodex Media";
        kMimeInfoDescriptionTag,    6, "Annodex Media";
        kMimeInfoDescriptionTag,    7, "Ogg Multimedia Bitstream";
        kMimeInfoDescriptionTag,    8, "Ogg Multimedia Bitstream";
        kMimeInfoDescriptionTag,    9, "Ogg Audio";
        kMimeInfoDescriptionTag,   10, "Ogg Audio";
        kMimeInfoDescriptionTag,   11, "Ogg Video";
        kMimeInfoDescriptionTag,   12, "Ogg Video";
        kMimeInfoDescriptionTag,   13, "Annodex Audio";
        kMimeInfoDescriptionTag,   14, "Annodex Audio";
        kMimeInfoDescriptionTag,   15, "Annodex Video";
        kMimeInfoDescriptionTag,   16, "Annodex Video";
    };
};

resource 'mcfg' (kImporterResID, OggImporterName, purgeable) {
    kVersionDoesntMatter,
    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'OGG ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Ogg Multimedia Bitstream",
            "ogg, ogx",
            "QuickTime Player",
            "Ogg file importer",
            "",
        },

        /* mime types array */
        {
            "application/x-ogg";
            "application/ogg";
        };
//    };
//    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'OGV ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Ogg Video",
            "ogv",
            "QuickTime Player",
            "Ogg Video file importer",
            "",
        },

        /* mime types array */
        {
            "video/x-ogg";
            "video/ogg";
        };
//    };
//    {
        kQTMediaConfigAudioGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'OGA ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Ogg Audio",
            "oga",
            "QuickTime Player",
            "Ogg Audio file importer",
            "",
        },

        /* mime types array */
        {
            "audio/x-ogg";
            "audio/ogg";
        };
//    };
//    {
        kQTMediaConfigAudioGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'SPX ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Ogg Speex audio",
            "spx",
            "QuickTime Player",
            "Ogg Speex file importer",
            "",
        },

        /* mime types array */
        {
            "audio/x-speex";
            "audio/speex";
        };
//    };
//    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'ANX ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Annodex Media",
            "anx",
            "QuickTime Player",
            "Annodex file importer",
            "",
        },

        /* mime types array */
        {
            "application/x-annodex";
            "application/annodex";
        };
//    };
//    {
        kQTMediaConfigAudioGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'AXA ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Annodex Audio",
            "axa",
            "QuickTime Player",
            "Annodex Audio file importer",
            "",
        },

        /* mime types array */
        {
            "audio/x-annodex";
            "audio/annodex";
        };
//    };
//    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigBinaryFile | \
            kQTMediaConfigTakeFileAssociationByDefault | \
            kQTMediaConfigCanUseApp | kQTMediaConfigCanUsePlugin | \
            kQTMediaConfigUsePluginByDefault,
        'OggS',
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, kCodecFormat, kSoundComponentManufacturer,
        0, 0,
        'AXV ',
        kQTMediaInfoNetGroup,

        /* no synonyms */
        {
        },

        {
            "Annodex Video",
            "axv",
            "QuickTime Player",
            "Annodex Video file importer",
            "",
        },

        /* mime types array */
        {
            "video/x-annodex";
            "video/annodex";
        };
    };
};

#if TARGET_OS_WIN32
read 'MDCf' (kImporterResID) "../MetaDataConfig.plist";
#endif
