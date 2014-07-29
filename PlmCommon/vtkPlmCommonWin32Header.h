/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Gregory C. Sharp, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

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
