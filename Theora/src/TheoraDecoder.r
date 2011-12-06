/*
 *  TheoraDecoder.r
 *
 *    Information bit definitions for the 'thng' and other TheoraDecoder
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
 *  Last modified: $Id: TheoraDecoder.r 12346 2007-01-18 13:45:42Z arek $
 *
 */


#define thng_RezTemplateVersion 1

#define cfrg_RezTemplateVersion 1

#ifdef TARGET_REZ_MAC_PPC
#include <CoreServices/CoreServices.r>
#include <QuickTime/QuickTime.r>
#include <QuickTime/QuickTimeComponents.r>
/* vvv ???
#undef __CARBON_R__
#undef __CORESERVICES_R__
#undef __CARBONCORE_R__
#undef __COMPONENTS_R__
   ^^^ */
#else
#include "ConditionalMacros.r"
#include "CoreServices.r"
#include "QuickTimeComponents.r"
#include "ImageCodec.r" //??
//#undef __COMPONENTS_R__ //??
#endif /* TARGET_REZ_MAC_PPC */

#include "TheoraDecoder.h"


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


#define kTheoraDecoderFlags (codecInfoDoes32 | codecInfoDoesTemporal | codecInfoDoesSpool)
//#define kTheoraEncoderFlags (codecInfoDoes32 | codecInfoDoesTemporal | codecInfoDoesRateConstrain | theoraThreadSafe)

#define kTheoraFormatFlags	(codecInfoDepth24)

resource 'cdci' (kTheoraDecoderResID) {
	kTheoraDecoderFormatName, // Type
	1, // Version
        kTheora_imdc_Version,
	kXiphComponentsManufacturer, // Manufacturer
	kTheoraDecoderFlags, // Decompression Flags
	0, // Compression Flags
	kTheoraFormatFlags, // Format Flags
            0, //128, // Compression Accuracy
	128, // Decomression Accuracy
            0, //128, // Compression Speed
	128, // Decompression Speed
	128, // Compression Level
	0, // Reserved
	16, // Minimum Height
	16, // Minimum Width
	0, // Decompression Pipeline Latency
	0, // Compression Pipeline Latency
	0 // Private Data
};

resource 'thng' (kTheoraDecoderResID) {
    decompressorComponentType, kVideoFormatXiphTheora,
    kXiphComponentsManufacturer, // Manufacturer(??)
    0, 0, 0, 0,
    'STR ', kTheoraDecoderNameStringResID,
    'STR ', kTheoraDecoderInfoStringResID,
    0, 0, // no icon
    kTheora_imdc_Version,
    componentDoAutoVersion|componentHasMultiplePlatforms, 0,
    {
        // component platform information
        kTheoraDecoderFlags,
        'dlle',
        kTheoraDecoderResID,
        Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
        kTheoraDecoderFlags,
        'dlle',
        kTheoraDecoderResID,
        Target_SecondPlatformType,
#endif
    };
};

// Component Name
resource 'STR ' (kTheoraDecoderNameStringResID) {
	"Xiph Theora Decoder"
};

// Component Information
resource 'STR ' (kTheoraDecoderInfoStringResID) {
	"Decompresses image sequences stored in the Xiph Theora format."
};

resource 'dlle' (kTheoraDecoderResID) {
    "Theora_ImageCodecComponentDispatch"
};
