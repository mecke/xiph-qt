/*
 *  flac_entrypoints.cpp
 *
 *    Declaration of the entry points for the FLAC component.
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
 *  Last modified: $Id: flac_entrypoints.cpp 10761 2006-01-28 19:30:37Z arek $
 *
 */


#include "CAFLACDecoder.h"
#include "CAOggFLACDecoder.h"

#include "ACCodecDispatch.h"

extern "C"
ComponentResult	CAFLACDecoderEntry(ComponentParameters* inParameters, CAFLACDecoder* inThis)
{
    return ACCodecDispatch(inParameters, inThis);
}

extern "C"
ComponentResult	CAOggFLACDecoderEntry(ComponentParameters* inParameters, CAOggFLACDecoder* inThis)
{
    return ACCodecDispatch(inParameters, inThis);
}
