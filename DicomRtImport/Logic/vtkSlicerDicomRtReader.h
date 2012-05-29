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
  char* GetROINameByROINumber(int ROINumber);

  /// Get display color of a certain ROI
  /// \param ROINumber Number of ROI to get
  double* GetROIDisplayColorByROINumber(int ROINumber);

  /// Get a certain structure set ROI
  /// \param ROINumber Number of ROI to get
  vtkPolyData* GetROIByROINumber(int ROINumber);

  /// Get name of a certain ROI
  /// \param number internal id number of ROI to get
  char* GetROIName(int number);

  /// Get display color of a certain ROI
  /// \param number internal id number of ROI to get
  double* GetROIDisplayColor(int number);

  /// Get a certain structure set ROI
  /// \param number internal id number of ROI to get
  vtkPolyData* GetROI(int number);

  /// Get number of beams
  int GetNumberOfBeams();
  
  /// Get name of beam
  char* GetBeamName(int BeamNumber);

  /// Get beam isocenter
  double* GetBeamIsocenterPosition(int BeamNumber);

  /// Set input file name
  vtkSetStringMacro(FileName);

  /*! Get pixel spacing */
  vtkGetVector2Macro(PixelSpacing, double); 

  /*! Get dose units */
  vtkGetStringMacro(DoseUnits); 
  vtkSetStringMacro(DoseUnits); 

  /*! Get dose units */
  vtkGetStringMacro(DoseGridScaling); 
  vtkSetStringMacro(DoseGridScaling); 

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

  //BTX
  /// Structure storing an RT structure set
  class BeamSequenceEntry
  {
  public:
	  int BeamNumber;
	  char* BeamName;
	  char* BeamType;
	  double BeamIsocenterPosition[3];
  };
  //ETX

protected:
  /// Load RT Structure Set
  void LoadRTStructureSet(DcmDataset*);

  /// Load RT Plan 
  void LoadRTPlan(DcmDataset*);  

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

  /// List of loaded contour ROIs from structure set
  std::vector<BeamSequenceEntry*> BeamSequenceVector;

  /// Pixel spacing - for RTDOSE
  double PixelSpacing[2];

  /// Dose units (e.g., Gy) - for RTDOSE
  char* DoseUnits;

  /// Dose grid scaling (e.g., 4.4812099e-5) - for RTDOSE
  /// Scaling factor that when multiplied by the dose grid data found in the voxel values,
  /// yields grid doses in the dose units as specified by Dose Units.
  /// Store it as a string, because it will be passed as a MRML node attribute.
  char* DoseGridScaling;

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
