/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D MRML
  Module:    $RCSfile: vtkMRMLLogicWin32Header.h,v $
  Date:      $Date: 2006/01/06 17:56:51 $
  Version:   $Revision: 1.4 $

=========================================================================auto=*/
///  vtkMRMLLogicWin32Header - manage Windows system differences
/// 
/// The vtkMRMLLogicWin32Header captures some system differences between Unix
/// and Windows operating systems. 


#ifndef __vtkSlicerRtCommonWin32Header_h
#define __vtkSlicerRtCommonWin32Header_h

#include <vtkSlicerRtCommonConfigure.h>

#if defined(WIN32) && !defined(vtkSlicerRtCommon_STATIC)
#if defined(vtkSlicerRtCommon_EXPORTS)
#define VTK_SLICERRTCOMMON_EXPORT __declspec( dllexport ) 
#else
#define VTK_SLICERRTCOMMON_EXPORT __declspec( dllimport )
#endif
#else
#define VTK_SLICERRTCOMMON_EXPORT
#endif

#endif
