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

// .NAME vtkSlicerDicomRtReader - 
// .SECTION Description
// This class manages the Reader associated with reading, saving,
// and changing propertied of the volumes

#ifndef __vtkSlicerDicomRtReader_h
#define __vtkSlicerDicomRtReader_h

// Slicer includes
//#include "vtkSlicerModuleReader.h"

// MRML includes

// VTK includes
#include "vtkObject.h"

// STD includes
#include <cstdlib>
#include <vector>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkPolyData;
class DcmDataset;


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTIMPORT_MODULE_LOGIC_EXPORT vtkSlicerDicomRtReader : public vtkObject
{
public:
  static vtkSlicerDicomRtReader *New();
  vtkTypeMacro(vtkSlicerDicomRtReader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Do reading
  void Update();

public:
  /// Get number of created ROIs
  int GetNumberOfROIs();

  /// Get name of a certain ROI
  /// \param ROINumber Number of ROI to get
  char* GetROIName(int ROINumber);

  /// Get display color of a certain ROI
  /// \param ROINumber Number of ROI to get
  double* GetROIDisplayColor(int ROINumber);

  /// Get a certain structure set ROI
  /// \param ROINumber Number of ROI to get
  vtkPolyData* GetROI(int ROINumber);

  /// Set input file name
  vtkSetStringMacro(FileName);

  /*! Get pixel spacing */
  vtkGetVector2Macro(PixelSpacing, double); 

  /// Get load structure set successful flag
  vtkGetMacro(LoadRTStructureSetSuccessful, bool);
  /// Get load dose successful flag
  vtkGetMacro(LoadRTDoseSuccessful, bool);
  /// Get load plan successful flag
  vtkGetMacro(LoadRTPlanSuccessful, bool);

protected:
  //BTX
  /// Structure storing an RT structure set
  class ROIStructureSetEntry
  {
  public:
	  int ROINumber;
	  char* ROIName;
	  double ROIDisplayColor[3];
	  vtkPolyData* ROIPolyData;
  };
  //ETX

protected:
  /// Load RT Structure Set
  void LoadRTStructureSet(DcmDataset*);

  /// Load RT Dose
  void LoadRTDose(DcmDataset*);

protected:
  /*! Set pixel spacing */
  vtkSetVector2Macro(PixelSpacing, double); 

protected:
  /// Structure set contours
  vtkPolyData* ROIContourSequencePolyData;

  /// Input file name
  char* FileName;

  /// List of loaded contour ROIs from structure set
  std::vector<ROIStructureSetEntry*> ROIContourSequenceVector;

  /// Pixel spacing (for dose image)
  double PixelSpacing[2];

  /// Flag indicating if RT Structure Set has been successfully read from the input dataset
  bool LoadRTStructureSetSuccessful;

  /// Flag indicating if RT Dose has been successfully read from the input dataset
  bool LoadRTDoseSuccessful;

  /// Flag indicating if RT Plan has been successfully read from the input dataset
  bool LoadRTPlanSuccessful;

protected:
  vtkSlicerDicomRtReader();
  virtual ~vtkSlicerDicomRtReader();

private:
  vtkSlicerDicomRtReader(const vtkSlicerDicomRtReader&); // Not implemented
  void operator=(const vtkSlicerDicomRtReader&);         // Not implemented
};

#endif
