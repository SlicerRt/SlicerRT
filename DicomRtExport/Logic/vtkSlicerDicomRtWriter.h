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

// .NAME vtkSlicerDicomRtExportReader -
// .SECTION Description
// This class manages the Reader associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtExportReader_h
#define __vtkSlicerDicomRtExportReader_h

// Slicer includes
//#include "vtkSlicerModuleReader.h"

// MRML includes

// STD includes
#include <cstdlib>
#include <vector>


#include "vtkSlicerDicomRtExportModuleLogicExport.h"

#include "vtkObject.h"

class vtkPolyData;
class DcmDataset;


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtWriter :
  public vtkObject
{
public:

  static vtkSlicerDicomRtWriter *New();
  vtkTypeMacro(vtkSlicerDicomRtWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //
  void SetFileName(const char *);

protected:
  vtkSlicerDicomRtWriter();
  virtual ~vtkSlicerDicomRtWriter();
  
  char *FileName;

private:

  vtkSlicerDicomRtWriter(const vtkSlicerDicomRtWriter&); // Not implemented
  void operator=(const vtkSlicerDicomRtWriter&);               // Not implemented
};

#endif