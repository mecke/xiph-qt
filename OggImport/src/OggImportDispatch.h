/*
 *  OggImportDispatch.h
 *
 *    OggImport component dispatch helper header.
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
 *  Last modified: $Id: OggImportDispatch.h 12093 2006-11-12 14:44:51Z arek $
 *
 */


        ComponentSelectorOffset (6)
	
	ComponentRangeCount (1)
	ComponentRangeShift (7)
	ComponentRangeMask  (7F)
	
	ComponentRangeBegin (0)
		ComponentError (Target)
		ComponentError (Register)
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)
	
	ComponentRangeBegin (1)
		ComponentError (0)
		ComponentError (Handle)
		ComponentCall (File)
		ComponentError (SetSampleDuration)
		ComponentError (SetSampleDescription)
		ComponentError (SetMediaFile)
		ComponentError (SetDimensions)
		ComponentCall (SetChunkSize)
		ComponentError (SetProgressProc)
		ComponentError (SetAuxiliaryData)
		ComponentError (SetFromScrap)
		ComponentError (DoUserDialog)
		ComponentError (SetDuration)
		ComponentError (GetAuxiliaryDataType)
		ComponentCall (Validate)
		ComponentCall (GetFileType)
		ComponentCall (DataRef)
		ComponentError (GetSampleDescription)
		ComponentCall (GetMIMETypeList)
		ComponentCall (SetOffsetAndLimit)
		ComponentError (GetSettingsAsAtomContainer)
		ComponentError (SetSettingsFromAtomContainer)
		ComponentCall (SetOffsetAndLimit64)
		ComponentCall (Idle)
		ComponentCall (ValidateDataRef)
		ComponentCall (GetLoadState)
		ComponentCall (GetMaxLoadedTime)
		ComponentCall (EstimateCompletionTime)
		ComponentCall (SetDontBlock)
		ComponentCall (GetDontBlock)
		ComponentCall (SetIdleManager)
                ComponentCall (SetNewMovieFlags)
	ComponentRangeEnd (1)
