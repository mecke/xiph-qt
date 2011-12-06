/*
 *  OggExportDispatch.h
 *
 *    OggExport component dispatch helper header.
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
 *  Last modified: $Id: OggExportDispatch.h 16023 2009-05-24 15:06:20Z arek $
 *
 */


#if __MACH__
        ComponentSelectorOffset (-kQTRemoveComponentPropertyListenerSelect)
#else
        ComponentSelectorOffset (-kComponentVersionSelect)
#endif

        ComponentRangeCount (3)
        ComponentRangeShift (7)
        ComponentRangeMask	(7F)

        ComponentRangeBegin (0)
#if __MACH__
		ComponentError (RemoveComponentPropertyListener)
		ComponentError (AddComponentPropertyListener)
		StdComponentCall (SetComponentProperty)
		StdComponentCall (GetComponentProperty)
		StdComponentCall (GetComponentPropertyInfo)

		ComponentError   (GetPublicResource)
		ComponentError   (ExecuteWiredAction)
		ComponentError   (GetMPWorkFunction)
		ComponentError   (Unregister)

		ComponentError   (Target)
		ComponentError   (Register)
#endif
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)

	ComponentRangeUnused (1)

	ComponentRangeBegin (2)
		ComponentError (ToHandle)
		ComponentCall  (ToFile)
		ComponentError (130)
		ComponentError (GetAuxiliaryData)
		ComponentCall  (SetProgressProc)
		ComponentError (SetSampleDescription)
		ComponentCall  (DoUserDialog)
		ComponentError (GetCreatorType)
		ComponentCall  (ToDataRef)
		ComponentCall  (FromProceduresToDataRef)
		ComponentCall  (AddDataSource)
		ComponentCall  (Validate)
		ComponentCall  (GetSettingsAsAtomContainer)
		ComponentCall  (SetSettingsFromAtomContainer)
		ComponentCall  (GetFileNameExtension)
		ComponentCall  (GetShortFileTypeString)
		ComponentCall  (GetSourceMediaType)
		ComponentError (SetGetMoviePropertyProc)
	ComponentRangeEnd (2)

	ComponentRangeBegin (3)
		ComponentCall (NewGetDataAndPropertiesProcs)
		ComponentCall (DisposeGetDataAndPropertiesProcs)
	ComponentRangeEnd (3)
