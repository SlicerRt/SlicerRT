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

// .NAME vtkSlicerDicomRtExportLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtExportLogic_h
#define __vtkSlicerDicomRtExportLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDicomRtExportModuleLogicExport.h"


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtExportLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerDicomRtExportLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtExportLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SaveDicomRT(char* name);

protected:
  vtkSlicerDicomRtExportLogic();
  virtual ~vtkSlicerDicomRtExportLogic();

private:

  vtkSlicerDicomRtExportLogic(const vtkSlicerDicomRtExportLogic&); // Not implemented
  void operator=(const vtkSlicerDicomRtExportLogic&);               // Not implemented
};

#endif