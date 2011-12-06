/*
 *  config.h
 *
 *    General compilation environment configuration file.
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
 *  Last modified: $Id: config.h 12347 2007-01-18 15:16:53Z arek $
 *
 */


#if !defined(__xiphqt_config_h__)
#define __xiphqt_config_h__


#if !defined(XIPHQT_NO_ENCODERS)
#define kXiphQTBundleID "org.xiph.xiph-qt.xiphqt"
#else
#define kXiphQTBundleID "org.xiph.xiph-qt.xiphqt-decoders"
#endif

#endif /* __data_types_h__ */
