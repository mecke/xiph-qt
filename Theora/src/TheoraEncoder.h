/*
 *  TheoraEncoder.h
 *
 *    TheoraEncoder.h - some constants definitions.
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
 *  Last modified: $Id: TheoraEncoder.h 12346 2007-01-18 13:45:42Z arek $
 *
 */


#ifndef __theoraencoder_h__
#define __theoraencoder_h__ 1

#include "theora_versions.h"
#include "fccs.h"

#define kTheoraEncoderResID                  -17790
#define kTheoraEncoderNameStringResID        -17790
#define kTheoraEncoderInfoStringResID        -17791

#define	kTheoraEncoderFormatName  "Xiph Theora"


#define kTheoraEncoderDITLResID              -17790
#define kTheoraEncoderPopupCNTLResID         -17790
#define kTheoraEncoderPopupMENUResID         -17790

#define TEXT_HEIGHT            16
#define INTER_CONTROL_SPACING  12
#define POPUP_CONTROL_HEIGHT   22


#ifdef _DEBUG
#define TheoraEncoderName        "Xiph Theora Decoder"
#else
#define TheoraEncoderName        ""
#endif

#endif /* __theoraencoder_h__ */
