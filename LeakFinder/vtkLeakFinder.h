/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

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

class vtkLeakFinder : public vtkObject
{
public:
  static vtkLeakFinder *New();
  vtkTypeMacro(vtkLeakFinder, vtkObject);

public:
  void StartTracing();
  void EndTracing();

  std::string GetLeakReport();

protected:
  vtkLeakFinder();
  virtual ~vtkLeakFinder();

protected:
  vtkLeakFinderObserver* Observer;
};

#endif
