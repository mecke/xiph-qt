/*
 *  TheoraDecoder.h
 *
 *    TheoraDecoder.h - some constants definitions.
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
 *  Last modified: $Id: TheoraDecoder.h 11325 2006-04-30 22:46:08Z arek $
 *
 */


#ifndef __theoradecoder_h__
#define __theoradecoder_h__ 1

#include "theora_versions.h"
#include "fccs.h"

#define kTheoraDecoderResID                  -17770
#define kTheoraDecoderNameStringResID        -17770
#define kTheoraDecoderInfoStringResID        -17771

#define	kTheoraDecoderFormatName  "Xiph Theora"


#ifdef _DEBUG
#define TheoraDecoderName        "Xiph Theora Decoder"
#else
#define TheoraDecoderName        ""
#endif

#endif /* __theoradecoder_h__ */
