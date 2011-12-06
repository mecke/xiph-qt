/*
 *  TheoraEncoder.r
 *
 *    Information bit definitions for the 'thng' and other TheoraEncoder
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
 *  Last modified: $Id: TheoraEncoder.r 12346 2007-01-18 13:45:42Z arek $
 *
 */


#define thng_RezTemplateVersion 1

#define cfrg_RezTemplateVersion 1

#ifdef TARGET_REZ_MAC_PPC
#include <CoreServices/CoreServices.r>
#include <QuickTime/QuickTime.r>
#include <QuickTime/QuickTimeComponents.r>
#else
#include "ConditionalMacros.r"
#include "CoreServices.r"
#include "QuickTimeComponents.r"
#include "ImageCodec.r"
#endif /* TARGET_REZ_MAC_PPC */

#include "TheoraEncoder.h"


/* How do I do this properly... anybody? */
#if defined(BUILD_UNIVERSAL)
  #define TARGET_CPU_PPC 1
  #define TARGET_CPU_X86 1
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
  #define theoraThreadSafe cmpThreadSafe
#elif TARGET_OS_WIN32
  #define Target_PlatformType platformWin32
  #define theoraThreadSafe 0
#else
  #error get a real platform type
#endif /* TARGET_OS_MAC */

#if !defined(TARGET_REZ_FAT_COMPONENTS)
  #define TARGET_REZ_FAT_COMPONENTS 0
#endif


#define kTheoraEncoderFlags (codecInfoDoes32 | codecInfoDoesTemporal | codecInfoDoesRateConstrain | theoraThreadSafe)

#define kTheoraFormatFlags	(codecInfoDepth24)

resource 'cdci' (kTheoraEncoderResID) {
	kTheoraEncoderFormatName, // Type
	1, // Version
        kTheora_imco_Version,
	kXiphComponentsManufacturer, // Manufacturer
	0, // Decompression Flags
	kTheoraEncoderFlags, // Compression Flags
	kTheoraFormatFlags, // Format Flags
	128, // Compression Accuracy
            0, //128, // Decomression Accuracy
	128, // Compression Speed
            0, //128, // Decompression Speed
	128, // Compression Level
	0, // Reserved
	16, // Minimum Height
	16, // Minimum Width
	0, // Decompression Pipeline Latency
	0, // Compression Pipeline Latency
	0 // Private Data
};

resource 'thng' (kTheoraEncoderResID) {
    compressorComponentType, kVideoFormatXiphTheora,
    kXiphComponentsManufacturer, // Manufacturer(??)
    0, 0, 0, 0,
    'STR ', kTheoraEncoderNameStringResID,
    'STR ', kTheoraEncoderInfoStringResID,
    0, 0, // no icon
    kTheora_imco_Version,
    componentDoAutoVersion | componentHasMultiplePlatforms, 0,
    {
        // component platform information
        kTheoraEncoderFlags,
        'dlle',
        kTheoraEncoderResID,
        Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
        kTheoraEncoderFlags,
        'dlle',
        kTheoraEncoderResID,
        Target_SecondPlatformType,
#endif
    };
};

// Component Name
resource 'STR ' (kTheoraEncoderNameStringResID) {
	"Xiph Theora Encoder"
};

// Component Information
resource 'STR ' (kTheoraEncoderInfoStringResID) {
	"Compresses into the Xiph Theora format."
};

resource 'dlle' (kTheoraEncoderResID) {
    "Theora_ImageEncoderComponentDispatch"
};


/* ========= Settings dialog resources ========= */

resource 'DITL' (kTheoraEncoderDITLResID, "Compressor Options", purgeable) {
{
 //{0, 0, TEXT_HEIGHT, 100}, CheckBox { enabled, "Checkbox" },
 //{TEXT_HEIGHT + INTER_CONTROL_SPACING, 0, TEXT_HEIGHT + INTER_CONTROL_SPACING + POPUP_CONTROL_HEIGHT, 165}, Control { enabled, kTheoraEncoderPopupCNTLResID },
 //{0, 0, TEXT_HEIGHT, 100}, Control { enabled, kTheoraEncoderPopupCNTLResID },
 //{TEXT_HEIGHT + INTER_CONTROL_SPACING, 0, TEXT_HEIGHT + INTER_CONTROL_SPACING + POPUP_CONTROL_HEIGHT, 165}, CheckBox { enabled, "Checkbox" },
 {0, 0, POPUP_CONTROL_HEIGHT, 205}, Control { enabled, kTheoraEncoderPopupCNTLResID },
 {POPUP_CONTROL_HEIGHT + INTER_CONTROL_SPACING, 100, TEXT_HEIGHT + INTER_CONTROL_SPACING + POPUP_CONTROL_HEIGHT, 200}, CheckBox { enabled, "Optimize" },
 }
};

resource 'CNTL' (kTheoraEncoderPopupCNTLResID, "Compressor Popup") {
 {0, 0, 20, 205},
 popupTitleRightJust,
     //1,
 visible,
 100,        /* title width */
 kTheoraEncoderPopupMENUResID,
 //popupMenuCDEFProc + popupFixedWidth,
 popupMenuCDEFProc + popupFixedWidth,
 0,
 "Sharpness:"
};

resource 'MENU' (kTheoraEncoderPopupMENUResID, "Compressor Popup") {
 kTheoraEncoderPopupMENUResID,
 textMenuProc,
 allEnabled,       /* Enable flags */
 enabled,
 "Sharpness",
 { /* array: 8 elements */
  /* [1] */
  "Low", noIcon, noKey, noMark, plain,
  /* [2] */
  "Medium", noIcon, noKey, noMark, plain,
  /* [3] */
  "High", noIcon, noKey, noMark, plain,
 }
};
