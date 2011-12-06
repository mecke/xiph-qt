/*
 *  CASpeexDecoderPublic.r
 *
 *    Information bit definitions for the 'thng' resource.
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
 *  Last modified: $Id: CASpeexDecoderPublic.r 10574 2005-12-10 16:22:13Z arek $
 *
 */


#include "speex_versions.h"
#include "fccs.h"



#define RES_ID			-17330
#define COMP_TYPE		'adec'
#define COMP_SUBTYPE	kAudioFormatXiphOggFramedSpeex
#define COMP_MANUF		'Xiph'
#define VERSION			kCASpeex_adec_Version
#define NAME			"Xiph Speex (Ogg-framed) Decoder"
#define DESCRIPTION		"An AudioCodec that decodes Xiph Speex (Ogg-framed) into linear PCM data"
#define ENTRY_POINT		"CAOggSpeexDecoderEntry"

#define kPrimaryResourceID               -17330
#define kComponentType                   'adec'
#define kComponentSubtype                kAudioFormatXiphOggFramedSpeex
#define kComponentManufacturer           'Xiph'
#define	kComponentFlags                  0
#define kComponentVersion                kCASpeex_adec_Version
#define kComponentName                   "Xiph (Ogg-framed) Speex"
#define kComponentInfo                   "An AudioCodec that decodes Xiph (Ogg-framed) Speex into linear PCM data"
#define kComponentEntryPoint             "CAOggSpeexDecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

//#include "ACComponentResources.r"
//#include "AUResources.r"
#include "XCAResources.r"


#define RES_ID			-17334
#define COMP_TYPE		'adec'
#define COMP_SUBTYPE	kAudioFormatXiphSpeex
#define COMP_MANUF		'Xiph'
#define VERSION			kCASpeex_adec_Version
#define NAME			"Xiph Speex Decoder"
#define DESCRIPTION		"An AudioCodec that decodes Xiph Speex into linear PCM data"
#define ENTRY_POINT		"CASpeexDecoderEntry"

#define kPrimaryResourceID               -17334
#define kComponentType                   'adec'
#define kComponentSubtype                kAudioFormatXiphSpeex
#define kComponentManufacturer           'Xiph'
#define	kComponentFlags                  0
#define kComponentVersion                kCASpeex_adec_Version
#define kComponentName                   "Xiph Speex"
#define kComponentInfo                   "An AudioCodec that decodes Xiph Speex into linear PCM data"
#define kComponentEntryPoint             "CASpeexDecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

//#include "ACComponentResources.r"
//#include "AUResources.r"
#include "XCAResources.r"
