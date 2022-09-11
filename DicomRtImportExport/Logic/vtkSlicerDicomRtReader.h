/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada and
  Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program,
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerDicomRtReader -
// .SECTION Description
// This class manages the Reader associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtReader_h
#define __vtkSlicerDicomRtReader_h

#include "vtkSlicerDicomRtImportExportModuleLogicExport.h"

// SlicerRtCommon includes
#include "vtkSlicerDicomReaderBase.h"

// STD includes
#include <vector>
#include <array>

class vtkPolyData;

/// \ingroup SlicerRt_QtModules_DicomRtImport
class VTK_SLICER_DICOMRTIMPORTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtReader : public vtkSlicerDicomReaderBase
{
public:
  static vtkSlicerDicomRtReader *New();
  vtkTypeMacro(vtkSlicerDicomRtReader, vtkSlicerDicomReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Do reading
  void Update();

public:
  /// Get number of created ROIs
  int GetNumberOfRois();

  /// Get name of a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  const char* GetRoiName(unsigned int internalIndex);

  /// Get display color of a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  double* GetRoiDisplayColor(unsigned int internalIndex);

  /// Get model of a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  vtkPolyData* GetRoiPolyData(unsigned int internalIndex);

  /// Get referenced series UID for a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  const char* GetRoiReferencedSeriesUid(unsigned int internalIndex);

  /// Get DICOM ROI number for a certain ROI by internal index
  /// \param internalIndex Internal index of ROI to get
  int GetRoiNumber(unsigned int internalIndex);

  /// Get number of beams
  int GetNumberOfBeams();

  /// Get number of control points for a given beams
  /// \result >= 2 if number of control points are valid, 0 otherwise
  unsigned int GetBeamNumberOfControlPoints(unsigned int beamNumber);

  /// Get beam number (as defined in DICOM) for a beam index (that is between 0 and numberOfBeams-1)
  unsigned int GetBeamNumberForIndex(unsigned int index);

  /// Get name of beam
  const char* GetBeamName(unsigned int beamNumber);

  /// Get type of beam
  const char* GetBeamType(unsigned int beamNumber);

  /// Get radiation type (primary particle) for a given beam
  const char* GetBeamTreatmentDeliveryType(unsigned int beamNumber);

  /// Get radiation type (primary particle) for a given beam
  const char* GetBeamRadiationType(unsigned int beamNumber);

  /// Get isocenter for a given control point of a beam
  double* GetBeamControlPointIsocenterPositionRas( unsigned int beamNumber, 
    unsigned int controlPointIndex);

  /// Get nominal beam energy for a given control point of a beam
  double GetBeamControlPointNominalBeamEnergy( unsigned int beamNumber, 
    unsigned int controlPoint);

  /// Get source axis distance for a given beam
  double GetBeamSourceAxisDistance(unsigned int beamNumber);

  /// Get virtual source axis distance for a given control point of a beam
  /// \return pointer to VSAD[2], or nullptr othervise
  double* GetBeamVirtualSourceAxisDistance(unsigned int beamNumber);

  /// Get gantry angle for a given control point of a beam
  double GetBeamControlPointGantryAngle( unsigned int beamNumber, 
    unsigned int controlPoint);

  /// Get patient support (couch) angle for a given control point of a beam
  double GetBeamControlPointPatientSupportAngle( unsigned int beamNumber, 
    unsigned int controlPoint);

  /// Get beam limiting device (collimator) angle for a given control point of a beam
  double GetBeamControlPointBeamLimitingDeviceAngle( unsigned int beamNumber, 
    unsigned int controlPoint);

  /// Get jaw positions for a given control point of a beam
  /// \param jawPositions Array in which the jaw positions are copied
  /// \return true if jaw positions are valid, false otherwise 
  bool GetBeamControlPointJawPositions( unsigned int beamNumber, 
    unsigned int controlPoint, double jawPositions[2][2]);

  /// Get Scan spot position map and meterset weights for a given 
  /// control point of a modulated ion beam
  /// \param positionMap Array in which the raw position map are copied
  /// \param metersetWeights Array in which the raw meterset weights are copied
  /// \return true if data is valid, false otherwise
  bool GetBeamControlPointScanSpotParameters( unsigned int beamNumber, 
    unsigned int controlPointIndex, std::vector<float>& positionMap, 
    std::vector<float>& metersetWeights);

  /// Get Scan spot size for a given control point of a modulated ion beam
  /// \param ScanSpotSize Array in which the raw scanning spot size is copied
  /// \return true if data is valid, false otherwise
  bool GetBeamControlPointScanningSpotSize( unsigned int beamNumber, 
    unsigned int controlPointIndex, std::array< float, 2 >& ScanSpotSize);

  /// Get source to beam limiting device distance (MLC) for a given beam of RTPlan
  /// or isocenter to beam limiting device distance (MLC) for a given beam of RTIonPlan
  /// \param beamNumber - number of a beam
  /// \return source to beam limiting device distance, 0.0 in case of an error
  double GetBeamSourceToMultiLeafCollimatorDistance(unsigned int beamNumber);
  double GetBeamIsocenterToMultiLeafCollimatorDistance(unsigned int beamNumber);

  /// Get source to beam limiting device distance (Jaws X) for a given beam of RTPlan
  /// or isocenter to beam limiting device distance (Jaws X) for a given beam of RTIonPlan
  /// \param beamNumber - number of a beam
  /// \return source to beam limiting device distance, 0.0 in case of an error
  double GetBeamSourceToJawsDistanceX(unsigned int beamNumber);
  double GetBeamIsocenterToJawsDistanceX(unsigned int beamNumber);

  /// Get source to beam limiting device distance (Jaws Y) for a given beam of RTPlan
  /// or isocenter to beam limiting device distance (Jaws Y) for a given beam of RTIonPlan
  /// \param beamNumber - number of a beam
  /// \return source to beam limiting device distance, 0.0 in case of an error
  double GetBeamSourceToJawsDistanceY(unsigned int beamNumber);
  double GetBeamIsocenterToJawsDistanceY(unsigned int beamNumber);

  /// Get MLC leaves boundaries & leaves positions opening for a given control point of a beam
  /// \param pairBoundaries Array in which the raw leaves boundaries are copied
  /// \param leafPositions Array in which the raw leaf positions are copied
  /// \return "MLCX" or "MLCY" if data is valid, nullptr otherwise
  const char* GetBeamControlPointMultiLeafCollimatorPositions( unsigned int beamNumber, 
    unsigned int controlPoint, std::vector<double>& pairBoundaries, 
    std::vector<double>& leafPositions);

  /// Get number of channels
  int GetNumberOfChannels();

  /// Get number of control points in channel
  int GetChannelNumberOfControlPoints(unsigned int channelNumber);

  /// Get number of control points in channel
  bool GetChannelControlPoint(unsigned int channelNumber, unsigned int controlPointNumber, double controlPointPosition[3]);


  /// Get referenced SOP instance UID list for the loaded structure set
  vtkGetStringMacro(RTStructureSetReferencedSOPInstanceUIDs);
  /// Set referenced SOP instance UID list for the loaded structure set
  vtkSetStringMacro(RTStructureSetReferencedSOPInstanceUIDs);

  /// Get pixel spacing for dose volume
  vtkGetVector2Macro(PixelSpacing, double);

  /// Get dose units
  vtkGetStringMacro(DoseUnits);
  /// Set dose units
  vtkSetStringMacro(DoseUnits);

  /// Get dose grid scaling
  vtkGetStringMacro(DoseGridScaling);
  /// Set dose grid scaling
  vtkSetStringMacro(DoseGridScaling);

  /// Get RT Plan SOP instance UID referenced by RT Dose
  vtkGetStringMacro(RTDoseReferencedRTPlanSOPInstanceUID);
  /// Set RT Plan SOP instance UID referenced by RT Dose
  vtkSetStringMacro(RTDoseReferencedRTPlanSOPInstanceUID);

  /// Get structure set SOP instance UID referenced by RT Plan
  vtkGetStringMacro(RTPlanReferencedStructureSetSOPInstanceUID);
  /// Set structure set SOP instance UID referenced by RT Plan
  vtkSetStringMacro(RTPlanReferencedStructureSetSOPInstanceUID);

  /// Get dose SOP instance UIDs referenced by RT Plan
  vtkGetStringMacro(RTPlanReferencedDoseSOPInstanceUIDs);
  /// Set dose SOP instance UIDs referenced by RT Plan
  vtkSetStringMacro(RTPlanReferencedDoseSOPInstanceUIDs);

  /// Get image type
  vtkGetStringMacro(ImageType);
  /// Set image type
  vtkSetStringMacro(ImageType);

  /// Get RT image label
  vtkGetStringMacro(RTImageLabel);
  /// Set RT image label
  vtkSetStringMacro(RTImageLabel);

  /// Get RT Plan SOP instance UID referenced by RT Image
  vtkGetStringMacro(RTImageReferencedRTPlanSOPInstanceUID);
  /// Set RT Plan SOP instance UID referenced by RT Image
  vtkSetStringMacro(RTImageReferencedRTPlanSOPInstanceUID);

  /// Get referenced beam number
  vtkGetMacro(ReferencedBeamNumber, int);
  /// Set referenced beam number
  vtkSetMacro(ReferencedBeamNumber, int);

  /// Get RT image position
  vtkGetVector2Macro(RTImagePosition, double);
  /// Set RT image position
  vtkSetVector2Macro(RTImagePosition, double);

  /// Get gantry angle
  vtkGetMacro(GantryAngle, double);
  /// Set gantry angle
  vtkSetMacro(GantryAngle, double);

  /// Get beam limiting device (collimator) angle
  vtkGetMacro(BeamLimitingDeviceAngle, double);
  /// Set beam limiting device (collimator) angle
  vtkSetMacro(BeamLimitingDeviceAngle, double);

  /// Get patient support angle
  vtkGetMacro(PatientSupportAngle, double);
  /// Set patient support angle
  vtkSetMacro(PatientSupportAngle, double);

  /// Get radiation machine SAD
  vtkGetMacro(RadiationMachineSAD, double);
  /// Set radiation machine SAD
  vtkSetMacro(RadiationMachineSAD, double);

  /// Get RT Image SID
  vtkGetMacro(RTImageSID, double);
  /// Set RT Image SID
  vtkSetMacro(RTImageSID, double);

  /// Get window center
  vtkGetMacro(WindowCenter, double);
  /// Set window center
  vtkSetMacro(WindowCenter, double);

  /// Get window width
  vtkGetMacro(WindowWidth, double);
  /// Set window width
  vtkSetMacro(WindowWidth, double);


  /// Get load structure set successful flag
  vtkGetMacro(LoadRTStructureSetSuccessful, bool);
  /// Get load dose successful flag
  vtkGetMacro(LoadRTDoseSuccessful, bool);
  /// Get load plan successful flag
  vtkGetMacro(LoadRTPlanSuccessful, bool);
  /// Get load plan successful flag
  vtkGetMacro(LoadRTIonPlanSuccessful, bool); 
  /// Get load image successful flag
  vtkGetMacro(LoadRTImageSuccessful, bool);

protected:
  /// Set pixel spacing for dose volume
  vtkSetVector2Macro(PixelSpacing, double);

protected:
  /// Referenced SOP instance UID list for the loaded structure set (serialized, separated by spaces)
  char* RTStructureSetReferencedSOPInstanceUIDs;

  /// Pixel spacing - for RTDOSE. First element for X spacing, second for Y spacing.
  double PixelSpacing[2];

  /// Dose units (e.g., Gy) - for RTDOSE
  char* DoseUnits;

  /// Dose grid scaling (e.g., 4.4812099e-5) - for RTDOSE
  /// Scaling factor that when multiplied by the dose grid data found in the voxel values,
  /// yields grid doses in the dose units as specified by Dose Units.
  /// Store it as a string, because it will be passed as a MRML node attribute.
  char* DoseGridScaling;

  /// RT Plan SOP instance UID referenced by RT Dose
  char* RTDoseReferencedRTPlanSOPInstanceUID;

  /// Structure set SOP instance UID referenced by RT Plan
  char* RTPlanReferencedStructureSetSOPInstanceUID;

  /// Dose SOP instance UIDs referenced by RT Plan
  char* RTPlanReferencedDoseSOPInstanceUIDs;

  /// Image type for RT images (DRR / PORTAL / SIMULATOR / RADIOGRAPH / BLANK / FLUENCE)
  char* ImageType;

  /// User-defined label for RT image
  char* RTImageLabel;

  /// RT Plan SOP instance UID referenced by RT Image
  char* RTImageReferencedRTPlanSOPInstanceUID;

  /// Referenced beam number (in the referenced RT Plan, \sa RTImageReferencedRTPlanSOPInstanceUID)
  int ReferencedBeamNumber;

  /// RT image position (center of the upper left hand corner of an RT image)
  double RTImagePosition[2];

  /// Gantry angle of an RT Image
  double GantryAngle;

  /// Beam limiting device (collimator) angle for an RT Image
  double BeamLimitingDeviceAngle;

  /// Patient support (bed) angle for an RT Image
  double PatientSupportAngle;

  /// Radiation machine SAD (Radiation source to Gantry rotation axis distance of radiation machine used in acquiring or computing image (mm))
  double RadiationMachineSAD;

  /// RT Image SID (Distance from radiation machine source to image plane (in mm) along radiation beam axis)
  double RTImageSID;

  /// Center of window for an RT Image
  double WindowCenter;

  /// Width of window for an RT Image
  double WindowWidth;


  /// Flag indicating if RT Structure Set has been successfully read from the input dataset
  bool LoadRTStructureSetSuccessful;

  /// Flag indicating if RT Dose has been successfully read from the input dataset
  bool LoadRTDoseSuccessful;

  /// Flag indicating if RT Plan has been successfully read from the input dataset
  bool LoadRTPlanSuccessful;

  /// Flag indicating if RT Ion Plan has been successfully read from the input dataset
  bool LoadRTIonPlanSuccessful;

  /// Flag indicating if RT Image has been successfully read from the input dataset
  bool LoadRTImageSuccessful;

protected:
  vtkSlicerDicomRtReader();
  ~vtkSlicerDicomRtReader() override;

private:
  vtkSlicerDicomRtReader(const vtkSlicerDicomRtReader&) = delete;
  void operator=(const vtkSlicerDicomRtReader&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal; // For access from the callback function
};

#endif
