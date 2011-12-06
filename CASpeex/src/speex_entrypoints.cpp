/*
 *  speex_entrypoints.cpp
 *
 *    Declaration of the entry points for the Speex component.
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
 *  Last modified: $Id: speex_entrypoints.cpp 10575 2005-12-10 19:33:16Z arek $
 *
 */


#include "CASpeexDecoder.h"
#include "CAOggSpeexDecoder.h"

#include "ACCodecDispatch.h"

extern "C"
ComponentResult	CASpeexDecoderEntry(ComponentParameters* inParameters, CASpeexDecoder* inThis)
{
    return ACCodecDispatch(inParameters, inThis);
}

extern "C"
ComponentResult	CAOggSpeexDecoderEntry(ComponentParameters* inParameters, CAOggSpeexDecoder* inThis)
{
    return ACCodecDispatch(inParameters, inThis);
}
