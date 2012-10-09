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

// MRML includes

// VTK includes
#include "vtkObject.h"

// STD includes
#include <cstdlib>
#include <vector>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkPolyData;
class DcmDataset;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO: investigate why the wrapping fails

//BTX

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTIMPORT_LOGIC_EXPORT vtkSlicerDicomRtReader : public vtkObject
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
  const char* GetROINameByROINumber(unsigned int ROINumber);

  /// Get display color of a certain ROI
  /// \param ROINumber Number of ROI to get
  double* GetROIDisplayColorByROINumber(unsigned int ROINumber);

  /// Get a certain structure set ROI
  /// \param ROINumber Number of ROI to get
  vtkPolyData* GetROIByROINumber(unsigned int ROINumber);

  /// Get name of a certain ROI
  /// \param number internal id number of ROI to get
  const char* GetROIName(unsigned int number);

  /// Get display color of a certain ROI
  /// \param number internal id number of ROI to get
  double* GetROIDisplayColor(unsigned int number);

  /// Get a certain structure set ROI
  /// \param number internal id number of ROI to get
  vtkPolyData* GetROI(unsigned int number);

  /// Get number of beams
  int GetNumberOfBeams();  

  /// Get name of beam
  const char* GetBeamName(unsigned int BeamNumber);

  /// Get beam isocenter
  double* GetBeamIsocenterPosition(unsigned int BeamNumber);

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
  /// Structure storing an RT structure set
  class ROIStructureSetEntry
  {
  public:
    ROIStructureSetEntry();
    virtual ~ROIStructureSetEntry();
    ROIStructureSetEntry(const ROIStructureSetEntry& src);
    ROIStructureSetEntry &operator=(const ROIStructureSetEntry &src);

    void SetPolyData(vtkPolyData* roiPolyData);

    unsigned int Number;
    std::string Name;
    std::string Description;
    double DisplayColor[3];
    vtkPolyData* PolyData;
  };

  /// Structure storing an RT structure set
  class BeamSequenceEntry
  {
  public:
    BeamSequenceEntry()
    {
      Number=-1;
      IsocenterPosition[0]=0.0;
      IsocenterPosition[1]=0.0;
      IsocenterPosition[2]=0.0;
    }
    unsigned int Number;
    std::string Name;
    std::string Type;
    std::string Description;
    double IsocenterPosition[3]; // in RAS
  };

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

  BeamSequenceEntry* FindBeamByNumber(unsigned int beamNumber);
  ROIStructureSetEntry* FindROIByNumber(unsigned int roiNumber);

protected:
  /// Structure set contours
  vtkPolyData* ROIContourSequencePolyData;

  /// Input file name
  char* FileName;

  /// List of loaded contour ROIs from structure set
  std::vector<ROIStructureSetEntry> ROIContourSequenceVector;

  /// List of loaded contour ROIs from structure set
  std::vector<BeamSequenceEntry> BeamSequenceVector;

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

//ETX

#endif
