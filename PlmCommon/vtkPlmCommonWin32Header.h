///  vtkMRMLLogicWin32Header - manage Windows system differences
/// 
/// The vtkMRMLLogicWin32Header captures some system differences between Unix
/// and Windows operating systems. 


#ifndef __vtkPlmCommonWin32Header_h
#define __vtkPlmCommonWin32Header_h

#include <vtkPlmCommonConfigure.h>

#if defined(WIN32) && !defined(vtkPlmCommon_STATIC)
#if defined(vtkPlmCommon_EXPORTS)
#define VTK_PLMCOMMON_EXPORT __declspec( dllexport ) 
#else
#define VTK_PLMCOMMON_EXPORT __declspec( dllimport )
#endif
#else
#define VTK_PLMCOMMON_EXPORT
#endif

#endif
