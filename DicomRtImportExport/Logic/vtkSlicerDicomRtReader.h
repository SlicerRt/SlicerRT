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

  /// Get beam number (as defined in DICOM) for a beam index (that is between 0 and numberOfBeams-1)
  unsigned int GetBeamNumberForIndex(unsigned int index);

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

  /// Get MLCX leaves boundaries & leaves positions opening for a given beam
  /// \param pairBoundaries Array in which the raw leaves boundaries are copied
  /// \param leafPositions Array in which the raw leaf positions are copied
  /// \return true if data is valid, false otherwise
  bool GetBeamMultiLeafCollimatorPositionsX( unsigned int beamNumber, 
    std::vector<double>& pairBoundaries, std::vector<double>& leafPositions);

  /// Get MLCY leaves boundaries & leaves positions opening for a given beam
  /// \param pairBoundaries Array in which the raw leaves boundaries are copied
  /// \param leafPositions Array in which the raw leaf positions are copied
  /// \return true if data is valid, false otherwise
  bool GetBeamMultiLeafCollimatorPositionsY( unsigned int beamNumber, 
    std::vector<double>& pairBoundaries, std::vector<double>& leafPositions);

  /// Get MLC leaves boundaries & leaves positions opening for a given beam
  /// \param pairBoundaries Array in which the raw leaves boundaries are copied
  /// \param leafPositions Array in which the raw leaf positions are copied
  /// \return "MLCX" or "MLCY" if data is valid, nullptr otherwise
  const char* GetBeamMultiLeafCollimatorPositions( unsigned int beamNumber, 
    std::vector<double>& pairBoundaries, std::vector<double>& leafPositions);

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
