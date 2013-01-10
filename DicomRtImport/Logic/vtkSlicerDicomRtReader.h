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

/// \ingroup SlicerRt_DicomRtImport
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
  int GetNumberOfRois();

  /// Get name of a certain ROI by its ROI number
  /// \param roiNumber Number (index) of the ROI in question
  const char* GetRoiNameByRoiNumber(unsigned int roiNumber);

  /// Get display color of a certain ROI by its ROI number
  /// \param roiNumber Number (index) of the ROI in question
  double* GetRoiDisplayColorByRoiNumber(unsigned int roiNumber);

  /// Get model of a certain ROI by its ROI number
  /// \param roiNumber Number (index) of the ROI in question
  vtkPolyData* GetRoiPolyDataByRoiNumber(unsigned int roiNumber);

  /// Get name of a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  const char* GetRoiName(unsigned int internalIndex);

  /// Get display color of a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  double* GetRoiDisplayColor(unsigned int internalIndex);

  /// Get model of a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  vtkPolyData* GetRoiPolyData(unsigned int internalIndex);

  /// Get number of beams
  int GetNumberOfBeams();  

  /// Get name of beam
  const char* GetBeamName(unsigned int beamNumber);

  /// Get beam isocenter for a given beam
  double* GetBeamIsocenterPositionRas(unsigned int beamNumber);

  /// Get beam source axis distance for a given beam
  double GetBeamSourceAxisDistance(unsigned int beamNumber);

  /// Get beam gantry angle for a given beam
  double GetBeamGantryAngle(unsigned int beamNumber);

  /// Get beam patient support (couch) angle for a given beam
  double GetBeamPatientSupportAngle(unsigned int beamNumber);

  /// Get beam beam limiting device (collimator) angle for a given beam
  double GetBeamBeamLimitingDeviceAngle(unsigned int beamNumber);

  /// Get beam leaf jaw positions for a given beam
  /// \param jawPositions Array in which the jaw positions are copied
  void GetBeamLeafJawPositions(unsigned int beamNumber, double jawPositions[2][2]);

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
  class RoiEntry
  {
  public:
    RoiEntry();
    virtual ~RoiEntry();
    RoiEntry(const RoiEntry& src);
    RoiEntry &operator=(const RoiEntry &src);

    void SetPolyData(vtkPolyData* roiPolyData);

    unsigned int Number;
    std::string  Name;
    std::string  Description;
    double       DisplayColor[3];
    vtkPolyData* PolyData;
  };

  /// Structure storing an RT structure set
  class BeamEntry
  {
  public:
    BeamEntry()
    {
      Number=-1;
      IsocenterPositionRas[0]=0.0;
      IsocenterPositionRas[1]=0.0;
      IsocenterPositionRas[2]=0.0;
      SourceAxisDistance=0.0;
      GantryAngle=0.0;
      PatientSupportAngle=0.0;
      BeamLimitingDeviceAngle=0.0;
      // TODO: good default values for the jaw positions?
      LeafJawPositions[0][0]=0.0;
      LeafJawPositions[0][1]=0.0;
      LeafJawPositions[1][0]=0.0;
      LeafJawPositions[1][1]=0.0;
    }
    unsigned int Number;
    std::string Name;
    std::string Type;
    std::string Description;
    double IsocenterPositionRas[3];

    // TODO: 
    // In case of VMAT the following parameters can change by each control point
    //   (this is not supported yet!)
    // In case of IMRT, these are fixed (for Slicer visualization, in reality there is
    //   a second control point that defines the CumulativeMetersetWeight to know when
    //   to end irradiation.
    double SourceAxisDistance;
    double GantryAngle;
    double PatientSupportAngle;
    double BeamLimitingDeviceAngle;
    /// Jaw positions: X and Y positions with isocenter as origin (e.g. {{-50,50}{-50,50}} )
    double LeafJawPositions[2][2];
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

  BeamEntry* FindBeamByNumber(unsigned int beamNumber);
  RoiEntry* FindRoiByNumber(unsigned int roiNumber);

protected:
  /// Input file name
  char* FileName;

  /// List of loaded contour ROIs from structure set
  std::vector<RoiEntry> RoiSequenceVector;

  /// List of loaded contour ROIs from structure set
  std::vector<BeamEntry> BeamSequenceVector;

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
