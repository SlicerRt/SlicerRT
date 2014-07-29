/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerDicomRtImportModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes

#ifndef __vtkLeakFinder_h
#define __vtkLeakFinder_h

// STD includes
#include <cstdlib>
#include <map>

// VTK includes
#include "vtkObject.h"

class vtkLeakFinderObserver;

/*!
 * \ingroup SlicerRt_LeakFinder
 * \brief Utility class that helps discovering memory leaks by keeping track of the created but not destroyed VTK
 *   objects. Returns a leak report containing the pointers, types and the call stacks at the point of their creation,
 *   when ending tracing manually, or writes a file just before exit (VTK patch needed for this).
 */
class vtkLeakFinder : public vtkObject
{
public:
  static vtkLeakFinder *New();
  vtkTypeMacro(vtkLeakFinder, vtkObject);

public:
  /// Start tracing the VTK objects' lifetime
  void StartTracing();

  /// End tracing manually. The leak report is assembled and written immediately.
  /// It is possible not to call this function, then the leak report is saved at the last possible
  ///   moment before exiting. VTK needs to be patched for this functionality to work.
  void EndTracing();

  /// Set output file name to observer.
  /// The leak report is written in this file.
  void SetOutputFileName(std::string fileName);

  /// Set flag whether Register and Unregister calls are traced (their call stack saved)
  void SetTraceRegisterAndUnregister(bool trace);

protected:
  vtkLeakFinder();
  virtual ~vtkLeakFinder();

protected:
  /// Observer object whose overridden functions are called on certain events
  vtkLeakFinderObserver* Observer;
};

#endif
