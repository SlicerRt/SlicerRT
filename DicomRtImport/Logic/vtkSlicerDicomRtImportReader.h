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

// .NAME vtkSlicerDicomRtImportReader - 
// .SECTION Description
// This class manages the Reader associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtImportReader_h
#define __vtkSlicerDicomRtImportReader_h

// Slicer includes
//#include "vtkSlicerModuleReader.h"

// MRML includes

// STD includes
#include <cstdlib>
#include <vector>


#include "vtkSlicerDicomRtImportModuleLogicExport.h"

#include "vtkObject.h"

class vtkPolyData;
class DcmDataset;


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTIMPORT_MODULE_LOGIC_EXPORT vtkSlicerDicomRtImportReader :
  public vtkObject

{
public:

  static vtkSlicerDicomRtImportReader *New();
  vtkTypeMacro(vtkSlicerDicomRtImportReader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //
  void SetFileName(const char *);

  // Description:
  int GetNumberOfROI();

  //
  char* GetROIName(int ROINumber);

  //
  vtkPolyData* GetROI(int ROINumber);

  //
  void Update();

  //BTX
  //
  class ROIStructureSetEntry
  {
  public:
	  int ROINumber;
	  char *ROIName;
	  vtkPolyData* ROIPolyData;
  };
  //ETX

protected:
  vtkSlicerDicomRtImportReader();
  virtual ~vtkSlicerDicomRtImportReader();
  void LoadRTStructureSet(DcmDataset &);

  vtkPolyData* ROIContourSequencePolyData;
  char *FileName;
  std::vector<ROIStructureSetEntry*> ROIContourSequenceVector;

private:

  vtkSlicerDicomRtImportReader(const vtkSlicerDicomRtImportReader&); // Not implemented
  void operator=(const vtkSlicerDicomRtImportReader&);               // Not implemented
};

#endif
