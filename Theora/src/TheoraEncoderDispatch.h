/*
 *  TheoraEncoderDispatch.h
 *
 *    TheoraEncoder component dispatch helper header.
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
 *  Last modified: $Id: TheoraEncoderDispatch.h 12346 2007-01-18 13:45:42Z arek $
 *
 */


	ComponentSelectorOffset (8)

	ComponentRangeCount (1)
	ComponentRangeShift (8)
	ComponentRangeMask	(FF)

	ComponentRangeBegin (0)
		ComponentError	 (GetMPWorkFunction)
		ComponentError	 (Unregister)
		StdComponentCall (Target)
		ComponentError   (Register)
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)

	ComponentRangeBegin (1)
		ComponentCall (GetCodecInfo)
		ComponentError (GetCompressionTime)
		ComponentCall (GetMaxCompressionSize)
		ComponentError (PreCompress)
		ComponentError (BandCompress)
		ComponentError (PreDecompress)
		ComponentError (BandDecompress)
		ComponentError (Busy)
		ComponentError (GetCompressedImageSize)
		ComponentError (GetSimilarity)
		ComponentError (TrimImage)
		ComponentCall (RequestSettings)
		ComponentCall (GetSettings)
		ComponentCall (SetSettings)
		ComponentError (Flush)
		ComponentError (SetTimeCode)
		ComponentError (IsImageDescriptionEquivalent)
		ComponentError (NewMemory)
		ComponentError (DisposeMemory)
		ComponentError (HitTestData)
		ComponentError (NewImageBufferMemory)
		ComponentError (ExtractAndCombineFields)
		ComponentError (GetMaxCompressionSizeWithSources)
		ComponentError (SetTimeBase)
		ComponentError (SourceChanged)
		ComponentError (FlushLastFrame)
		ComponentError (GetSettingsAsText)
		ComponentError (GetParameterListHandle)
		ComponentError (GetParameterList)
		ComponentError (CreateStandardParameterDialog)
		ComponentError (IsStandardParameterDialogEvent)
		ComponentError (DismissStandardParameterDialog)
		ComponentError (StandardParameterDialogDoAction)
		ComponentError (NewImageGWorld)
		ComponentError (DisposeImageGWorld)
		ComponentError (HitTestDataWithFlags)
		ComponentError (ValidateParameters)
		ComponentError (GetBaseMPWorkFunction)
		ComponentError (LockBits)
		ComponentError (UnlockBits)
		ComponentError (RequestGammaLevel)
		ComponentError (GetSourceDataGammaLevel)
		ComponentError (42)
		ComponentError (GetDecompressLatency)
		ComponentError (MergeFloatingImageOntoWindow)
		ComponentError (RemoveFloatingImage)
		ComponentCall (GetDITLForSize)
		ComponentCall (DITLInstall)
		ComponentCall (DITLEvent)
		ComponentCall (DITLItem)
		ComponentCall (DITLRemove)
		ComponentCall (DITLValidateInput)
		ComponentError (52)
		ComponentError (53)
		ComponentError (GetPreferredChunkSizeAndAlignment)
		ComponentCall (PrepareToCompressFrames)
		ComponentCall (EncodeFrame)
		ComponentCall (CompleteFrame)
		ComponentError (BeginPass)
		ComponentError (EndPass)
		ComponentError (ProcessBetweenPasses)
	ComponentRangeEnd (1)
