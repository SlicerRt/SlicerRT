///  vtkSlicerRtCommonWin32Header - manage Windows system differences
/// 
/// The vtkSlicerRtCommonWin32Header captures some system differences between Unix
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
