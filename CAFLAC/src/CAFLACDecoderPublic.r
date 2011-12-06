/*
 *  CAFLACDecoderPublic.r
 *
 *    Information bit definitions for the 'thng' resource.
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
 *  Last modified: $Id: CAFLACDecoderPublic.r 10761 2006-01-28 19:30:37Z arek $
 *
 */


#include "flac_versions.h"
#include "fccs.h"


#define kPrimaryResourceID               -17550
#define kComponentType                   'adec'
#define kComponentSubtype                kAudioFormatXiphOggFramedFLAC
#define kComponentManufacturer           'Xiph'
#define	kComponentFlags                  0
#define kComponentVersion                kCAFLAC_adec_Version
#define kComponentName                   "Xiph (Ogg-framed) FLAC"
#define kComponentInfo                   "An AudioCodec that decodes Xiph (Ogg-framed) FLAC into linear PCM data"
#define kComponentEntryPoint             "CAOggFLACDecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

#include "XCAResources.r"


#define kPrimaryResourceID               -17554
#define kComponentType                   'adec'
#define kComponentSubtype                kAudioFormatXiphFLAC
#define kComponentManufacturer           'Xiph'
#define	kComponentFlags                  0
#define kComponentVersion                kCAFLAC_adec_Version
#define kComponentName                   "Xiph FLAC"
#define kComponentInfo                   "An AudioCodec that decodes Xiph FLAC into linear PCM data"
#define kComponentEntryPoint             "CAFLACDecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

#include "XCAResources.r"
