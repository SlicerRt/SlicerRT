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

// DicomRtImportExportModuleLogic includes
#include "vtkSlicerDicomRtReader.h"

// SlicerRt includes
#include "vtkSlicerRtCommon.h"

// VTK includes
#include <vtkCellArray.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// STD includes
#include <array>
#include <vector>
#include <map>

// DCMTK includes
#include <dcmtk/config/osconfig.h>    /* make sure OS specific configuration is included first */

#include <dcmtk/ofstd/ofconapp.h>

#include <dcmtk/dcmrt/drtdose.h>
#include <dcmtk/dcmrt/drtimage.h>
#include <dcmtk/dcmrt/drtplan.h>
#include <dcmtk/dcmrt/drtstrct.h>
#include <dcmtk/dcmrt/drttreat.h>
#include <dcmtk/dcmrt/drtionpl.h>
#include <dcmtk/dcmrt/drtiontr.h>

// Qt includes
#include <QSettings>

vtkStandardNewMacro(vtkSlicerDicomRtReader);

//----------------------------------------------------------------------------
class vtkSlicerDicomRtReader::vtkInternal
{
public:
  vtkInternal(vtkSlicerDicomRtReader* external);
  ~vtkInternal();

public:
  /// Structure storing a Beam Limiting Device Parameters (MLC)
  /// BeamLimitingDeviceEntry is a description of MLC,
  /// or any beam limiting device such as symmetric or asymmetric jaws.

  /// DICOM standard describes two kinds of multi-leaf collimators: 
  /// "MLCX" - leaves moves along X-axis, "MCLY" - leaves moves along Y-axis.
  class BeamLimitingDeviceEntry
  {
  public:
    BeamLimitingDeviceEntry()
      :
      SourceIsoDistance(400.),
      NumberOfLeafJawPairs(0)
    {
    }
    /// Source to BeamLimitingDevice (MLC) distance for RTPlan
    /// or Isocenter to BeamLimitingDevice (MLC) distance for RTIonPlan
    double SourceIsoDistance;
    /// Number of leaf pairs for MLC, or 1 for symmetric or asymmetric jaws
    unsigned int NumberOfLeafJawPairs;
    /// MLC position boundaries: Raw DICOM values
    std::vector<double> LeafPositionBoundary;
  };

  /// Structure storing a beam range shifter parameters associated with ion beam
  class RangeShifterEntry
  {
  public:
    RangeShifterEntry()
      :
      Number(-1),
      Type("BINARY")
    {
    }
    int Number;
    std::string ID;
    std::string AccessoryCode;
    std::string Type; // "ANALOG", "BINARY"
    std::string Description;
  };

  //TODO: Add support of compensators
  //Structure storing a treatment compensator parameters associated with beam
  class CompensatorEntry
  {
  public:
    CompensatorEntry()
    {
    }
  };

  //TODO: Add support of shielding blocks
  //Structure storing a shielding block parameters associated with beam
  class BlockEntry
  {
  public:
    BlockEntry()
    {
    }
  };

  /// Structure storing a ROI of an RT structure set
  class RoiEntry
  {
  public:
    RoiEntry();
    virtual ~RoiEntry();
    RoiEntry(const RoiEntry& src);
    RoiEntry &operator=(const RoiEntry &src);

    void SetPolyData(vtkPolyData* roiPolyData);

    unsigned int Number;
    std::string Name;
    std::string Description;
    std::array< double, 3 > DisplayColor;
    vtkPolyData* PolyData;
    std::string ReferencedSeriesUID;
    std::string ReferencedFrameOfReferenceUID;
    std::map<int,std::string> ContourIndexToSOPInstanceUIDMap;
  };

  /// List of loaded contour ROIs from structure set
  std::vector<RoiEntry> RoiSequenceVector;

  //TODO: Use referenced beams to load beams in correct order
  class ReferencedBeamEntry
  {
    public:
      ReferencedBeamEntry()
        :
        Number(-1)
      {
      }
      unsigned int Number; /// Referenced Beam Number (300C,0006)
      /// pair.first = Cumulative Meterset Weight (300A,0134)
      /// pair.second = Referenced Control Point Index (300C,00F0)
      std::vector< std::pair< double, unsigned int > > BeamDoseVerificationControlPointSequenceVector;
  };

  //TODO: Use fraction entry to check that RTPlan data is valid
  /// Structure storing Fraction information of RT plan or RT Ion plan
  class FractionEntry
  {
  public:
    FractionEntry()
      :
      Number(-1),
      NumberOfFractionsPlanned(0),
      NumberOfBeams(0),
      NumberOfBrachyApplicationSetups(0)
    {
    }
    unsigned int Number;
    std::string Description;
    unsigned int NumberOfFractionsPlanned;
    unsigned int NumberOfBeams;
    unsigned int NumberOfBrachyApplicationSetups;
    std::vector< ReferencedBeamEntry > ReferencedBeamSequenceVector;
  };

  /// List of loaded fractions from RT plan or RT Ion plan 
  std::vector< FractionEntry > FractionSequenceVector;

  /// Structure storing Control Point information for the current beam
  class ControlPointEntry
  {
  public:
    ControlPointEntry();
    ControlPointEntry(const ControlPointEntry& src);
    ControlPointEntry& operator=(const ControlPointEntry& src);

    unsigned int Index;
    std::array< double, 3 > IsocenterPositionRas;
    
    //TODO:
    // In case of VMAT the following parameters can change by each control point
    //   (this is not supported yet!)
    // In case of IMRT, these are fixed (for Slicer visualization, in reality there is
    //   a second control point that defines the CumulativeMetersetWeight to know when
    //   to end irradiation.
    double CumulativeMetersetWeight;
    double GantryAngle;
    double PatientSupportAngle;
    double BeamLimitingDeviceAngle;
    double NominalBeamEnergy; // MeV/u
    double MetersetRate;

    /// Jaw positions: X and Y positions with isocenter as origin (e.g. {{-50,50}{-50,50}} )
    std::array< double, 4 > JawPositions; // X[0], X[1], Y[0], Y[1]
    std::string MultiLeafCollimatorType; // "MLCX" or "MLCY"
    /// MLC positions: Raw DICOM values
    std::vector<double> LeafPositions;

    int ReferencedRangeShifterNumber;
    std::string RangeShifterSetting;
    double IsocenterToRangeShifterDistance;
    double RangeShifterWaterEquivalentThickness;

    // Parameters taken from plastimatch (for future use)
    std::string GantryRotationDirection;
    float GantryPitchAngle;
    std::string GantryPitchRotationDirection;
    std::string BeamLimitingDeviceRotationDirection;

    std::string ScanSpotTuneId;
    unsigned int NumberOfScanSpotPositions;
    std::string ScanSpotReorderingAllowed;
    std::vector<float> ScanSpotPositionMap;
    std::vector<float> ScanSpotMetersetWeights;
    unsigned int NumberOfPaintings;
    std::array< float, 2 > ScanningSpotSize;
    std::string PatientSupportRotationDirection;
    
    float TableTopPitchAngle;
    std::string TableTopPitchRotationDirection;
    float TableTopRollAngle;
    std::string TableTopRollRotationDirection;
    float TableTopVerticalPosition;
    float TableTopLongitudinalPosition;
    float TableTopLateralPosition;
  };

  /// Structure storing an RT beam
  class BeamEntry
  {
  public:
    BeamEntry()
      :
      Number(-1),
      Type("STATIC"),
      RadiationIon({ 0, 0, 0 }),
      SourceAxisDistance({ 2000., 2000. }),
      SourceIsoToJawsDistance({ 500., 500. }),
      NumberOfCompensators(0),
      NumberOfBlocks(0),
      NumberOfRangeShifters(0),
      NumberOfControlPoints(0),
      FinalCumulativeMetersetWeight(-1.),
      ScanMode("NONE")
    {
    }
    unsigned int Number;
    std::string Name;
    std::string Type;
    std::string Description;
    std::string RadiationType;

    /// RadiationIon[0] = RadiationMassNumber;
    /// RadiationIon[1] = RadiationAtomicNumber;
    /// RadiationIon[2] = RadiationChargeState;
    std::array< unsigned int, 3 > RadiationIon;
    /// SourceAxisDistance (SAD) for RTPlan (first element)
    /// VirtualSourceAxisDistance (VSAD) for RTIonPlan    
    std::array< double, 2 > SourceAxisDistance;

    /// Source to jaw distance: Distance from source to Jaws X and Jaws Y for RTPlan
    /// Distance from isocenter to Jaws X and Jaws Y for RTIonPlan
    /// SourceIsoToJawsDistance[0] - X or ASYMX
    /// SourceIsoToJawsDistance[1] - Y or ASYMY
    std::array< double, 2 > SourceIsoToJawsDistance;

    /// MLC parameters: Raw DICOM values
    std::string MultiLeafCollimatorType; // "MLCX" or "MLCY"
    BeamLimitingDeviceEntry MultiLeafCollimator;

    unsigned int NumberOfCompensators;
    std::vector< CompensatorEntry > CompensatorSequenceVector;

    unsigned int NumberOfBlocks;
    std::vector< BlockEntry > BlockSequenceVector;

    unsigned int NumberOfRangeShifters; // for RTIonPlan only 
    std::vector< RangeShifterEntry > RangeShifterSequenceVector;

    unsigned int NumberOfControlPoints;
    std::vector< ControlPointEntry > ControlPointSequenceVector;

    /// Meterset at end of all control points
    double FinalCumulativeMetersetWeight;
    /// Scan mode type;
    std::string ScanMode; // MODULATED, NONE, UNIFORM
    // Parameters taken from plastimatch (for future use)
    std::string TreatmentMachineName;
    std::string TreatmentDeliveryType;
    std::string Manufacturer;
    std::string InstitutionName;
    std::string InstitutionAddress;
    std::string InstitutionalDepartmentName;
    std::string ManufacturerModelName;
  };

  /// List of loaded beams from external beam plan
  std::vector<BeamEntry> BeamSequenceVector;

  /// Structure storing a channel in an RT application setup (for brachytherapy plan)
  class ChannelEntry
  {
  public:
    ChannelEntry()
    {
      Number = -1;
      NumberOfControlPoints = 0;
      Length = 0.0;
      TotalTime = 0.0;
    }
    unsigned int Number;
    unsigned int NumberOfControlPoints;
    double Length;
    double TotalTime;
    std::vector< std::array<double,3> > ControlPointVector;
    //TODO: Additional fields? (source applicator properties, material, etc.)
  };

  /// List of loaded channels from brachytherapy plan
  std::vector<ChannelEntry> ChannelSequenceVector;

public:
  /// Load RT Dose
  void LoadRTDose(DcmDataset* dataset);

  /// Load RT Plan 
  void LoadRTPlan(DcmDataset* dataset);
  /// Load RT Ion Plan
  void LoadRTIonPlan(DcmDataset* dataset);

  /// Load RT Structure Set
  void LoadRTStructureSet(DcmDataset* dataset);
  /// Load contours from a structure sequence
  void LoadContoursFromRoiSequence(DRTStructureSetROISequence* roiSequence);
  /// Load individual contour from RT Structure Set
  vtkSlicerDicomRtReader::vtkInternal::RoiEntry* LoadContour(DRTROIContourSequence::Item &roiObject, DRTStructureSetIOD* rtStructureSet);

  /// Load RT Image
  void LoadRTImage(DcmDataset* dataset);

public:
  /// Find and return a beam entry according to its beam number
  BeamEntry* FindBeamByNumber(unsigned int beamNumber);

  /// Find and return a ROI entry according to its ROI number
  RoiEntry* FindRoiByNumber(unsigned int roiNumber);

  /// Find and return a channel entry according to its channel number
  ChannelEntry* FindChannelByNumber(unsigned int channelNumber);

  /// Get frame of reference for an SOP instance
  DRTRTReferencedSeriesSequence* GetReferencedSeriesSequence(DRTStructureSetIOD* rtStructureSet);

  /// Get referenced series instance UID for the structure set (0020,000E)
  OFString GetReferencedSeriesInstanceUID(DRTStructureSetIOD* rtStructureSet);

  /// Get contour image sequence object in the referenced frame of reference sequence for a structure set
  DRTContourImageSequence* GetReferencedFrameOfReferenceContourImageSequence(DRTStructureSetIOD* rtStructureSet);

public:
  vtkSlicerDicomRtReader* External;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::vtkInternal(vtkSlicerDicomRtReader* external)
  : External(external)
{
  this->RoiSequenceVector.clear();
  this->BeamSequenceVector.clear();
  this->ChannelSequenceVector.clear();
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::~vtkInternal()
{
  this->RoiSequenceVector.clear();
  this->BeamSequenceVector.clear();
  this->ChannelSequenceVector.clear();
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry::RoiEntry()
{
  this->Number = 0;
  this->DisplayColor = { 1.0, 0.0, 0.0 };
  this->PolyData = nullptr;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry::~RoiEntry()
{
  this->SetPolyData(nullptr);
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry::RoiEntry(const RoiEntry& src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor = src.DisplayColor;
  this->PolyData = nullptr;
  this->SetPolyData(src.PolyData);
  this->ReferencedSeriesUID = src.ReferencedSeriesUID;
  this->ReferencedFrameOfReferenceUID = src.ReferencedFrameOfReferenceUID;
  this->ContourIndexToSOPInstanceUIDMap = src.ContourIndexToSOPInstanceUIDMap;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry& vtkSlicerDicomRtReader::vtkInternal::RoiEntry::operator=(const RoiEntry &src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor = src.DisplayColor;
  this->SetPolyData(src.PolyData);
  this->ReferencedSeriesUID = src.ReferencedSeriesUID;
  this->ReferencedFrameOfReferenceUID = src.ReferencedFrameOfReferenceUID;
  this->ContourIndexToSOPInstanceUIDMap = src.ContourIndexToSOPInstanceUIDMap;

  return (*this);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::RoiEntry::SetPolyData(vtkPolyData* roiPolyData)
{
  if (roiPolyData == this->PolyData)
  {
    // not changed
    return;
  }
  if (this->PolyData != nullptr)
  {
    this->PolyData->UnRegister(nullptr);
  }

  this->PolyData = roiPolyData;

  if (this->PolyData != nullptr)
  {
    this->PolyData->Register(nullptr);
  }
}

vtkSlicerDicomRtReader::vtkInternal::ControlPointEntry::ControlPointEntry()
  :
  Index(0),
  IsocenterPositionRas({ 0.0, 0.0, 0.0 }),
  CumulativeMetersetWeight(-1.0),
  GantryAngle(0.0),
  PatientSupportAngle(0.0),
  BeamLimitingDeviceAngle(0.0),
  NominalBeamEnergy(0.0),
  MetersetRate(0.0),
  JawPositions({ -100.0, 100.0, -100.0, 100.0 })
{
}

vtkSlicerDicomRtReader::vtkInternal::ControlPointEntry::ControlPointEntry(const ControlPointEntry& src)
  :
  Index(src.Index),
  IsocenterPositionRas(src.IsocenterPositionRas),
  CumulativeMetersetWeight(src.CumulativeMetersetWeight),
  GantryAngle(src.GantryAngle),
  PatientSupportAngle(src.PatientSupportAngle),
  BeamLimitingDeviceAngle(src.BeamLimitingDeviceAngle),
  NominalBeamEnergy(src.NominalBeamEnergy),
  MetersetRate(src.MetersetRate),
  JawPositions(src.JawPositions),
  MultiLeafCollimatorType(src.MultiLeafCollimatorType),
  LeafPositions(src.LeafPositions),
  ReferencedRangeShifterNumber(src.ReferencedRangeShifterNumber),
  RangeShifterSetting(src.RangeShifterSetting),
  IsocenterToRangeShifterDistance(src.IsocenterToRangeShifterDistance),
  RangeShifterWaterEquivalentThickness(src.RangeShifterWaterEquivalentThickness),
  GantryRotationDirection(src.GantryRotationDirection),
  GantryPitchAngle(src.GantryPitchAngle),
  GantryPitchRotationDirection(src.GantryPitchRotationDirection),
  BeamLimitingDeviceRotationDirection(src.BeamLimitingDeviceRotationDirection),
  ScanSpotTuneId(src.ScanSpotTuneId),
  NumberOfScanSpotPositions(src.NumberOfScanSpotPositions),
  ScanSpotReorderingAllowed(src.ScanSpotReorderingAllowed),
  ScanSpotPositionMap(src.ScanSpotPositionMap),
  ScanSpotMetersetWeights(src.ScanSpotMetersetWeights),
  NumberOfPaintings(src.NumberOfPaintings),
  ScanningSpotSize(src.ScanningSpotSize),
  PatientSupportRotationDirection(src.PatientSupportRotationDirection),
  TableTopPitchAngle(src.TableTopPitchAngle),
  TableTopPitchRotationDirection(src.TableTopPitchRotationDirection),
  TableTopRollAngle(src.TableTopRollAngle),
  TableTopRollRotationDirection(src.TableTopRollRotationDirection),
  TableTopVerticalPosition(src.TableTopVerticalPosition),
  TableTopLongitudinalPosition(src.TableTopLongitudinalPosition),
  TableTopLateralPosition(src.TableTopLateralPosition)
{
}

vtkSlicerDicomRtReader::vtkInternal::ControlPointEntry&
vtkSlicerDicomRtReader::vtkInternal::ControlPointEntry::operator=(const ControlPointEntry& src)
{
  this->Index = src.Index;
  this->IsocenterPositionRas = src.IsocenterPositionRas;
  
  this->CumulativeMetersetWeight = src.CumulativeMetersetWeight;
  
  this->GantryAngle = src.GantryAngle;
  this->PatientSupportAngle = src.PatientSupportAngle;
  this->BeamLimitingDeviceAngle = src.BeamLimitingDeviceAngle;
  
  this->NominalBeamEnergy = src.NominalBeamEnergy;
  this->MetersetRate = src.MetersetRate;
  
  this->JawPositions = src.JawPositions;
  this->MultiLeafCollimatorType = src.MultiLeafCollimatorType;
  this->LeafPositions = src.LeafPositions;
  
  this->ReferencedRangeShifterNumber = src.ReferencedRangeShifterNumber;
  this->RangeShifterSetting = src.RangeShifterSetting;
  this->IsocenterToRangeShifterDistance = src.IsocenterToRangeShifterDistance;
  this->RangeShifterWaterEquivalentThickness = src.RangeShifterWaterEquivalentThickness;

  this->GantryRotationDirection = src.GantryRotationDirection;
  this->GantryPitchAngle = src.GantryPitchAngle;
  this->GantryPitchRotationDirection = src.GantryPitchRotationDirection;
  this->BeamLimitingDeviceRotationDirection = src.BeamLimitingDeviceRotationDirection;

  this->ScanSpotTuneId = src.ScanSpotTuneId;
  this->NumberOfScanSpotPositions = src.NumberOfScanSpotPositions;
  this->ScanSpotReorderingAllowed = src.ScanSpotReorderingAllowed;
  this->ScanSpotPositionMap = src.ScanSpotPositionMap;
  this->ScanSpotMetersetWeights = src.ScanSpotMetersetWeights;
  this->NumberOfPaintings = src.NumberOfPaintings;
  this->ScanningSpotSize = src.ScanningSpotSize;

  this->PatientSupportRotationDirection = src.PatientSupportRotationDirection;
    
  this->TableTopPitchAngle = src.TableTopPitchAngle;
  this->TableTopPitchRotationDirection = src.TableTopPitchRotationDirection;
  this->TableTopRollAngle = src.TableTopRollAngle;
  this->TableTopRollRotationDirection = src.TableTopRollRotationDirection;
  this->TableTopVerticalPosition = src.TableTopVerticalPosition;
  this->TableTopLongitudinalPosition = src.TableTopLongitudinalPosition;
  this->TableTopLateralPosition = src.TableTopLateralPosition;
  return *this;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::BeamEntry* vtkSlicerDicomRtReader::vtkInternal::FindBeamByNumber(unsigned int beamNumber)
{
  for (unsigned int i=0; i<this->BeamSequenceVector.size(); i++)
  {
    if (this->BeamSequenceVector[i].Number == beamNumber)
    {
      return &this->BeamSequenceVector[i];
    }
  }

  // Not found
  vtkErrorWithObjectMacro(this->External, "FindBeamByNumber: Beam cannot be found for number " << beamNumber);
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry* vtkSlicerDicomRtReader::vtkInternal::FindRoiByNumber(unsigned int roiNumber)
{
  for (unsigned int i=0; i<this->RoiSequenceVector.size(); i++)
  {
    if (this->RoiSequenceVector[i].Number == roiNumber)
    {
      return &this->RoiSequenceVector[i];
    }
  }

  // Not found
  vtkErrorWithObjectMacro(this->External, "FindRoiByNumber: ROI cannot be found for number " << roiNumber);
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::ChannelEntry* vtkSlicerDicomRtReader::vtkInternal::FindChannelByNumber(unsigned int channelNumber)
{
  for (unsigned int i=0; i<this->ChannelSequenceVector.size(); i++)
  {
    if (this->ChannelSequenceVector[i].Number == channelNumber)
    {
      return &this->ChannelSequenceVector[i];
    }
  }

  // Not found
  vtkErrorWithObjectMacro(this->External, "FindChannelByNumber: Beam cannot be found for number " << channelNumber);
  return nullptr;
}

//----------------------------------------------------------------------------
DRTRTReferencedSeriesSequence* vtkSlicerDicomRtReader::vtkInternal::GetReferencedSeriesSequence(DRTStructureSetIOD* rtStructureSet)
{
  DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSet->getReferencedFrameOfReferenceSequence();
  if (!rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: No referenced frame of reference sequence object item is available");
    return nullptr;
  }

  DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
  if (!currentReferencedFrameOfReferenceSequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: Frame of reference sequence object item is invalid");
    return nullptr;
  }

  DRTRTReferencedStudySequence &rtReferencedStudySequence = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
  if (!rtReferencedStudySequence.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: No referenced study sequence object item is available");
    return nullptr;
  }

  DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequence.getCurrentItem();
  if (!rtReferencedStudySequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: Referenced study sequence object item is invalid");
    return nullptr;
  }

  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequence = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
  if (!rtReferencedSeriesSequence.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: No referenced series sequence object item is available");
    return nullptr;
  }

  return &rtReferencedSeriesSequence;
}

//----------------------------------------------------------------------------
OFString vtkSlicerDicomRtReader::vtkInternal::GetReferencedSeriesInstanceUID(DRTStructureSetIOD* rtStructureSet)
{
  OFString invalidUid("");
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequence = this->GetReferencedSeriesSequence(rtStructureSet);
  if (!rtReferencedSeriesSequence || !rtReferencedSeriesSequence->gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesInstanceUID: No referenced series sequence object item is available");
    return invalidUid;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequence->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesInstanceUID: Referenced series sequence object item is invalid");
    return invalidUid;
  }

  OFString referencedSeriesInstanceUID("");
  rtReferencedSeriesSequenceItem.getSeriesInstanceUID(referencedSeriesInstanceUID);
  return referencedSeriesInstanceUID;
}

//----------------------------------------------------------------------------
DRTContourImageSequence* vtkSlicerDicomRtReader::vtkInternal::GetReferencedFrameOfReferenceContourImageSequence(DRTStructureSetIOD* rtStructureSet)
{
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequence = this->GetReferencedSeriesSequence(rtStructureSet);
  if (!rtReferencedSeriesSequence || !rtReferencedSeriesSequence->gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedFrameOfReferenceContourImageSequence: No referenced series sequence object item is available");
    return nullptr;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequence->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedFrameOfReferenceContourImageSequence: Referenced series sequence object item is invalid");
    return nullptr;
  }

  DRTContourImageSequence &rtContourImageSequence = rtReferencedSeriesSequenceItem.getContourImageSequence();
  if (!rtContourImageSequence.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedFrameOfReferenceContourImageSequence: No contour image sequence object item is available");
    return nullptr;
  }

  return &rtContourImageSequence;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTDose(DcmDataset* dataset)
{
  this->External->LoadRTDoseSuccessful = false;

  DRTDoseIOD rtDose;
  if (rtDose.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to read RT Dose dataset");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTDose: Load RT Dose object");

  OFString doseGridScaling("");
  if (rtDose.getDoseGridScaling(doseGridScaling).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to get Dose Grid Scaling for dose object");
    return; // mandatory DICOM value
  }
  else if (doseGridScaling.empty())
  {
    vtkWarningWithObjectMacro(this->External, "LoadRTDose: Dose grid scaling tag is missing or empty! Using default dose grid scaling 0.0001.");
    doseGridScaling = "0.0001";
  }

  this->External->SetDoseGridScaling(doseGridScaling.c_str());

  OFString doseUnits("");
  if (rtDose.getDoseUnits(doseUnits).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to get Dose Units for dose object");
    return; // mandatory DICOM value
  }
  this->External->SetDoseUnits(doseUnits.c_str());

  OFVector<vtkTypeFloat64> pixelSpacingOFVector;
  if (rtDose.getPixelSpacing(pixelSpacingOFVector).bad() || pixelSpacingOFVector.size() < 2)
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to get Pixel Spacing for dose object");
    return; // mandatory DICOM value
  }
  // According to the DICOM standard:
  // Physical distance in the patient between the center of each pixel,specified by a numeric pair -
  // adjacent row spacing (delimiter) adjacent column spacing in mm. See Section 10.7.1.3 for further explanation.
  // So X spacing is the second element of the vector, while Y spacing is the first.
  this->External->SetPixelSpacing(pixelSpacingOFVector[1], pixelSpacingOFVector[0]);
  vtkDebugWithObjectMacro(this->External, "Pixel Spacing: (" << pixelSpacingOFVector[1] << ", " << pixelSpacingOFVector[0] << ")");

  // Get referenced RTPlan instance UID
  DRTReferencedRTPlanSequence &referencedRTPlanSequence = rtDose.getReferencedRTPlanSequence();
  if (referencedRTPlanSequence.gotoFirstItem().good())
  {
    DRTReferencedRTPlanSequence::Item &referencedRTPlanSequenceItem = referencedRTPlanSequence.getCurrentItem();
    if (referencedRTPlanSequenceItem.isValid())
    {
      OFString referencedSOPInstanceUID("");
      if (referencedRTPlanSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
      {
        this->External->SetRTDoseReferencedRTPlanSOPInstanceUID(referencedSOPInstanceUID.c_str());
      }
    }
  }

  // Get SOP instance UID
  OFString sopInstanceUid("");
  if (rtDose.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to get SOP instance UID for RT dose");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->External->GetAndStoreRtHierarchyInformation(&rtDose);

  this->External->LoadRTDoseSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTPlan(DcmDataset* dataset)
{
  this->External->LoadRTPlanSuccessful = false; 

  DRTPlanIOD rtPlan;
  if (rtPlan.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Failed to read RT Plan object");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTPlan: Load RT Plan object");
  if (rtPlan.isRTFractionSchemeModulePresent(OFTrue) == OFTrue)
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTPlan: Fraction Scheme is correct");
  }
  else if (rtPlan.isRTFractionSchemeModulePresent() == OFTrue)
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Fraction Scheme is partially correct");
  }
  else if (rtPlan.isRTFractionSchemeModulePresent() == OFTrue)
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Fraction Scheme is partially correct");
  }
  else
  {
    vtkErrorWithObjectMacro( this->External, "LoadRTPlan: Fraction Scheme is absent");
    return;
  }

  // Check beams module
  bool hasBeamsModule = true, hasBrachyApplicationSetupsModule = true;
  if (rtPlan.isRTBeamsModulePresent(OFTrue) == OFTrue)
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTPlan: Beams module is correct");
    
  }
  else if (rtPlan.isRTBeamsModulePresent() == OFTrue)
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Beams module is partially correct");
  }
  else
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Beams module is absent");
    hasBeamsModule = false;
  }

  // Check brachy setups module
  if (rtPlan.isRTBrachyApplicationSetupsModulePresent(OFTrue) == OFTrue)
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTPlan: Brachy application setups module is correct");
  }
  else if (rtPlan.isRTBrachyApplicationSetupsModulePresent() == OFTrue)
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Brachy application setups module is partially correct");
  }
  else
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Brachy application setups module is absent");
    hasBrachyApplicationSetupsModule = false;
  }

  bool haveBeams = false, haveBrachy = false;
  DRTFractionGroupSequence& rtPlanFractionsSequence = rtPlan.getFractionGroupSequence();
  // RTPlan fraction sequence
  if (rtPlanFractionsSequence.isValid() && rtPlanFractionsSequence.gotoFirstItem().good())
  {
    do
    {
      DRTFractionGroupSequence::Item &fractionSequenceItem = rtPlanFractionsSequence.getCurrentItem();
      if (!fractionSequenceItem.isValid())
      {
        // possibly reach the end of the sequence
        if (rtPlanFractionsSequence.gotoNextItem().good())
        {
          // Only log warning if this is not the last item (the loop reaches here after the final item)
          vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Found an invalid fraction item in dataset");
        }
        continue;
      }

      Sint32 Number = -1;
      fractionSequenceItem.getFractionGroupNumber(Number);

      Sint32 nofBeams = -1;
      fractionSequenceItem.getNumberOfBeams(nofBeams);

      Sint32 nofBrachy = -1;
      fractionSequenceItem.getNumberOfBrachyApplicationSetups(nofBrachy);

      if (nofBeams > 0)
      {
        vtkDebugWithObjectMacro( this->External, "LoadRTPlan: Number of beams are " 
          << nofBeams << " in fraction group item number " << Number);
        haveBeams = true;
      }
      if (nofBrachy > 0)
      {
        vtkDebugWithObjectMacro( this->External, "LoadRTPlan: Number of brachy application setups are " 
          << nofBrachy << " in fraction group item number " << Number);
        haveBrachy = true;
      }
    }
    while (rtPlanFractionsSequence.gotoNextItem().good());

  }

  if ((hasBeamsModule && haveBeams) || (hasBrachyApplicationSetupsModule && haveBrachy))
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTPlan: Data is available for loading");
  }
  else
  {
    vtkErrorWithObjectMacro( this->External, "LoadRTPlan: No data for loading");
    return;
  }

  DRTBeamSequence &rtPlanBeamSequence = rtPlan.getBeamSequence();
  DRTApplicationSetupSequence &rtPlanApplicationSetupSequence = rtPlan.getApplicationSetupSequence();
  // RTPlan beam sequence (external beam plan)
  if (rtPlanBeamSequence.isValid() && rtPlanBeamSequence.gotoFirstItem().good())
  {
    do
    {
      DRTBeamSequence::Item &currentBeamSequenceItem = rtPlanBeamSequence.getCurrentItem();
      if (!currentBeamSequenceItem.isValid())
      {
        // possibly reach the end of the sequence
        if (rtPlanBeamSequence.gotoNextItem().good())
        {
          // Only log warning if this is not the last item (the loop reaches here after the final item)
          vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Found an invalid beam item in dataset");
        }
        continue;
      }

      // Read item into the BeamSequenceVector
      BeamEntry beamEntry;

      OFString beamName("");
      currentBeamSequenceItem.getBeamName(beamName);
      beamEntry.Name=beamName.c_str();

      OFString beamDescription("");
      currentBeamSequenceItem.getBeamDescription(beamDescription);
      beamEntry.Description=beamDescription.c_str();

      OFString treatmentDeliveryType("");
      currentBeamSequenceItem.getTreatmentDeliveryType(treatmentDeliveryType);
      beamEntry.TreatmentDeliveryType=treatmentDeliveryType.c_str();

      OFString beamType("");
      currentBeamSequenceItem.getBeamType(beamType);
      beamEntry.Type=beamType.c_str();

      Sint32 beamNumber = -1;
      currentBeamSequenceItem.getBeamNumber( beamNumber );        
      beamEntry.Number = beamNumber;

      OFString radiationType("");
      currentBeamSequenceItem.getRadiationType(radiationType);
      beamEntry.RadiationType = radiationType.c_str();

      Float64 sourceAxisDistance = 0.0;
      currentBeamSequenceItem.getSourceAxisDistance(sourceAxisDistance);
      beamEntry.SourceAxisDistance[0] = sourceAxisDistance;

      Float64 finalCumulativeMetersetWeight = -1.;
      currentBeamSequenceItem.getFinalCumulativeMetersetWeight(finalCumulativeMetersetWeight);
      beamEntry.FinalCumulativeMetersetWeight = finalCumulativeMetersetWeight;

      // Get multi leaf collimators parameters: distance, number of leaf pairs, leaf position boundaries
      DRTBeamLimitingDeviceSequenceInRTBeamsModule& rtBeamLimitingDeviceSequence = 
        currentBeamSequenceItem.getBeamLimitingDeviceSequence();
      if (rtBeamLimitingDeviceSequence.isValid() && rtBeamLimitingDeviceSequence.gotoFirstItem().good())
      {
        do
        {
          DRTBeamLimitingDeviceSequenceInRTBeamsModule::Item &collimatorItem =
            rtBeamLimitingDeviceSequence.getCurrentItem();
          if (collimatorItem.isValid())
          {
            OFString deviceType("");
            Sint32 nofPairs = -1;

            // source to beam limiting device (any collimator) distance
            Float64 distance = -1.;
            collimatorItem.getSourceToBeamLimitingDeviceDistance(distance);

            collimatorItem.getRTBeamLimitingDeviceType(deviceType);
            if (!deviceType.compare("X") || !deviceType.compare("ASYMX"))
            {
              beamEntry.SourceIsoToJawsDistance[0] = distance;
            }
            else if (!deviceType.compare("Y") || !deviceType.compare("ASYMY"))
            {
              beamEntry.SourceIsoToJawsDistance[1] = distance;
            }
            else if (!deviceType.compare("MLCX") || !deviceType.compare("MLCY"))
            {              
              OFCondition getNumberOfLeafJawPairsCondition = collimatorItem.getNumberOfLeafJawPairs(nofPairs);
              if (getNumberOfLeafJawPairsCondition.good())
              {
                std::string& type = beamEntry.MultiLeafCollimatorType;
                double& mlcDistance = beamEntry.MultiLeafCollimator.SourceIsoDistance;
                unsigned int& pairs = beamEntry.MultiLeafCollimator.NumberOfLeafJawPairs;
                std::vector<double>& bounds = beamEntry.MultiLeafCollimator.LeafPositionBoundary;
                OFVector<Float64> leafPositionBoundaries;
                OFCondition getBoundariesCondition = collimatorItem.getLeafPositionBoundaries(leafPositionBoundaries);
                if (getBoundariesCondition.good())
                {
                  type = deviceType.c_str();
                  mlcDistance = distance;
                  pairs = nofPairs;
                  bounds.resize(leafPositionBoundaries.size());
                  std::copy( leafPositionBoundaries.begin(), leafPositionBoundaries.end(), bounds.begin());
                  vtkDebugWithObjectMacro( this->External, "LoadRTPlan: multi-leaf collimator type is " 
                    << type << ", number of leaf pairs are " << nofPairs);
                }
              }
            }
          }
        }
        while (rtBeamLimitingDeviceSequence.gotoNextItem().good());

      }

      Sint32 beamNumberOfControlPoints = -1;
      currentBeamSequenceItem.getNumberOfControlPoints(beamNumberOfControlPoints);
      beamEntry.NumberOfControlPoints = beamNumberOfControlPoints;

      DRTControlPointSequence &rtControlPointSequence = currentBeamSequenceItem.getControlPointSequence();
      if (!rtControlPointSequence.isValid() || !rtControlPointSequence.gotoFirstItem().good())
      {
        vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Found an invalid RT control point sequence in dataset");
        continue;
      }
    
      // Initialize control point vector so that it can be correctly filled even if control points arrive in random order
      for ( Sint32 i = 0; i < beamNumberOfControlPoints; ++i)
      {
        ControlPointEntry dummyControlPoint;
        beamEntry.ControlPointSequenceVector.push_back(dummyControlPoint);
      }
      unsigned int controlPointCount = 0;
    
      do
      {
        DRTControlPointSequence::Item &controlPointItem = rtControlPointSequence.getCurrentItem();

        if (!controlPointItem.isValid())
        {
          // possibly reach the end of the sequence
          if (rtControlPointSequence.gotoNextItem().good())
          {
            vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Found an invalid RT control point in dataset, "
              << "control point counter = " << controlPointCount);
          }
          continue;
        }

        Sint32 controlPointIndex = -1;
        controlPointItem.getControlPointIndex(controlPointIndex);
        if (controlPointIndex < 0 || controlPointIndex >= beamNumberOfControlPoints)
        {
          vtkErrorWithObjectMacro( this->External, 
            "LoadRTPlan: Invalid control point index value " 
            << controlPointIndex << ", for a beam number "
            << beamEntry.Number << " named \"" << beamEntry.Name << "\"");
          continue;
        }

        // Control point item from the ControlPointSequenceVector
        ControlPointEntry& controlPoint = beamEntry.ControlPointSequenceVector.at(controlPointIndex);
        // assign previous point control point to a current control point
        if (controlPointIndex > 0)
        {
          ControlPointEntry& prevControlPoint = beamEntry.ControlPointSequenceVector.at(controlPointIndex - 1);
          controlPoint = ControlPointEntry(prevControlPoint);
        }
//        ControlPointEntry& controlPointEntry = beamEntry.ControlPointSequenceVector.at(controlPointCount); 
        controlPoint.Index = controlPointIndex;

        OFVector<Float64> isocenterPositionDataLps;
        OFString isocenterStringX, isocenterStringY, isocenterStringZ;

        controlPointItem.getIsocenterPosition( isocenterStringX, 0);
        controlPointItem.getIsocenterPosition( isocenterStringY, 1);
        controlPointItem.getIsocenterPosition( isocenterStringZ, 2);

        controlPointItem.getIsocenterPosition(isocenterPositionDataLps);
        if (!isocenterStringX.empty() && !isocenterStringY.empty() && !isocenterStringZ.empty())
        {
          // Convert from DICOM LPS -> Slicer RAS
          controlPoint.IsocenterPositionRas[0] = -isocenterPositionDataLps[0];
          controlPoint.IsocenterPositionRas[1] = -isocenterPositionDataLps[1];
          controlPoint.IsocenterPositionRas[2] = isocenterPositionDataLps[2];
        }

        Float64 nominalBeamEnergy = 0.0;
        OFCondition dataCondition = controlPointItem.getNominalBeamEnergy(nominalBeamEnergy);
        if (dataCondition.good())
        {
          controlPoint.NominalBeamEnergy = nominalBeamEnergy;
        }

        Float64 gantryAngle = 0.0;
        dataCondition = controlPointItem.getGantryAngle(gantryAngle);
        if (dataCondition.good())
        {
          controlPoint.GantryAngle = gantryAngle;
        }

        Float64 patientSupportAngle = 0.0;
        dataCondition = controlPointItem.getPatientSupportAngle(patientSupportAngle);
        if (dataCondition.good())
        {
          controlPoint.PatientSupportAngle = patientSupportAngle;
        }
        
        Float64 beamLimitingDeviceAngle = 0.0;
        dataCondition = controlPointItem.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
        if (dataCondition.good())
        {
          controlPoint.BeamLimitingDeviceAngle = beamLimitingDeviceAngle;
        }

        Float64 cumulativeMetersetWeight = -1.;
        dataCondition = controlPointItem.getCumulativeMetersetWeight(cumulativeMetersetWeight);
        if (dataCondition.good())
        {
          controlPoint.CumulativeMetersetWeight = cumulativeMetersetWeight;
        }

        DRTBeamLimitingDevicePositionSequence &currentCollimatorPositionSequence =
          controlPointItem.getBeamLimitingDevicePositionSequence();
        if (currentCollimatorPositionSequence.isValid() && 
          currentCollimatorPositionSequence.gotoFirstItem().good())
        {
          do
          {
            DRTBeamLimitingDevicePositionSequence::Item &collimatorPositionItem =
              currentCollimatorPositionSequence.getCurrentItem();
            if (collimatorPositionItem.isValid())
            {
              OFString rtBeamLimitingDeviceType("");
              collimatorPositionItem.getRTBeamLimitingDeviceType(rtBeamLimitingDeviceType);

              OFVector<Float64> leafJawPositions;
              OFCondition getJawPositionsCondition = collimatorPositionItem.getLeafJawPositions(leafJawPositions);

              if ( !rtBeamLimitingDeviceType.compare("ASYMX") || !rtBeamLimitingDeviceType.compare("X") )
              {
                if (getJawPositionsCondition.good())
                {
                  controlPoint.JawPositions[0] = leafJawPositions[0];
                  controlPoint.JawPositions[1] = leafJawPositions[1];
                }
                else
                {
                  vtkWarningWithObjectMacro(this->External, "LoadRTPlan: No jaw position found in collimator entry");
                }
              }
              else if ( !rtBeamLimitingDeviceType.compare("ASYMY") || !rtBeamLimitingDeviceType.compare("Y") )
              {
                if (getJawPositionsCondition.good())
                {
                  controlPoint.JawPositions[2] = leafJawPositions[0];
                  controlPoint.JawPositions[3] = leafJawPositions[1];
                }
                else
                {
                  vtkWarningWithObjectMacro(this->External, "LoadRTPlan: No jaw position found in collimator entry");
                }
              }
              else if ( !rtBeamLimitingDeviceType.compare("MLCX") || !rtBeamLimitingDeviceType.compare("MLCY") )
              {
                // Get MLC leaves position (opening)
                if (getJawPositionsCondition.good())
                {
                  std::vector<double>& mlcPositions = controlPoint.LeafPositions;
                  std::string& mlcType = controlPoint.MultiLeafCollimatorType;

                  mlcType = rtBeamLimitingDeviceType.c_str();
                  mlcPositions.resize(leafJawPositions.size());
                  std::copy( leafJawPositions.begin(), leafJawPositions.end(), mlcPositions.begin());
                  vtkDebugWithObjectMacro( this->External, "LoadRTPlan: " 
                    << mlcType << " leaf positions have been loaded");
                }
                else
                {
                  vtkWarningWithObjectMacro( this->External, "LoadRTPlan: No MLC position found in collimator entry");
                }
              }
              else
              {
                vtkErrorWithObjectMacro( this->External, 
                  "LoadRTPlan: Unsupported collimator type: " << rtBeamLimitingDeviceType);
              }
            }
          }
          while (currentCollimatorPositionSequence.gotoNextItem().good());
 
        }
        ++controlPointCount;
      }
      while (rtControlPointSequence.gotoNextItem().good());

      if (Sint32(controlPointCount) != beamNumberOfControlPoints)
      {
        vtkErrorWithObjectMacro( this->External, "LoadRTPlan: Number of control points expected ("
          << beamNumberOfControlPoints << ") and found (" << controlPointCount << ") do not match. Invalid points remain among control points");
      }
      this->BeamSequenceVector.push_back(beamEntry);
    }
    while (rtPlanBeamSequence.gotoNextItem().good());

  }
  // RT brachy plan
  else if (rtPlanApplicationSetupSequence.gotoFirstItem().good())
  {
    // Get channel sequence
    // (relevant section in DICOM standard: http://dicom.nema.org/dicom/2013/output/chtml/part03/sect_C.8.html#sect_C.8.8.15)

    DRTApplicationSetupSequence::Item &currentApplicationSetupSequence = rtPlanApplicationSetupSequence.getCurrentItem();
    if (!currentApplicationSetupSequence.isValid())
    {
      // Only consider first application setup item //TODO: There may be plans where there are more
      vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Application setup sequence is invalid");
      return;
    }

    DRTChannelSequence &channelSequence = currentApplicationSetupSequence.getChannelSequence();
    channelSequence.gotoFirstItem();

    do
    {
      DRTChannelSequence::Item &currentChannelSequenceItem = channelSequence.getCurrentItem();  
      if (!currentChannelSequenceItem.isValid())
      {
        // possibly reach the end of the sequence
        if (channelSequence.gotoNextItem().good())
        {
          // Only log warning if this is not the last item (the loop reaches here after the final item)
          vtkWarningWithObjectMacro(this->External, "LoadRTPlan: Found an invalid channel in dataset");
        }
        continue;
      }

      // Read item into the ChannelSequenceVector
      ChannelEntry channelEntry;

      Sint32 channelNumber;
      currentChannelSequenceItem.getChannelNumber(channelNumber);
      channelEntry.Number = channelNumber;

      Float64 channelLength;
      currentChannelSequenceItem.getChannelLength(channelLength);
      channelEntry.Length = channelLength;

      Float64 channelTotalTime;
      currentChannelSequenceItem.getChannelTotalTime(channelTotalTime);
      channelEntry.TotalTime = channelTotalTime;

      Sint32 channelNumberOfControlPoints;
      currentChannelSequenceItem.getNumberOfControlPoints(channelNumberOfControlPoints);
      channelEntry.NumberOfControlPoints = channelNumberOfControlPoints;

      DRTBrachyControlPointSequence &rtBrachyControlPointSequence = currentChannelSequenceItem.getBrachyControlPointSequence();

      if (!rtBrachyControlPointSequence.gotoFirstItem().good())
      {
        vtkWarningWithObjectMacro(this->External, "LoadRTPlan: Found an invalid brachy control point sequence in dataset");
        continue;
      }

      // Initialize control point vector so that it can be correctly filled even if control points arrive in random order
      for (int i=0; i<channelNumberOfControlPoints; ++i)
      {
        std::array<double,3> dummyControlPoint = { 0.0, 0.0, 0.0 };
        channelEntry.ControlPointVector.push_back(dummyControlPoint);
      }
      unsigned int controlPointCount = 0;
      do
      {
        DRTBrachyControlPointSequence::Item &brachyControlPointItem = rtBrachyControlPointSequence.getCurrentItem();

        if (!brachyControlPointItem.isValid())
        {
          // possibly reach the end of the sequence
          if (rtBrachyControlPointSequence.gotoNextItem().good())
          {
            vtkWarningWithObjectMacro( this->External, "LoadRTPlan: Found an invalid brachy control point in dataset, "
              << "control point counter = " << controlPointCount);
          }
          continue;
        }

        Sint32 controlPointIndex;
        brachyControlPointItem.getControlPointIndex(controlPointIndex);

        OFVector<vtkTypeFloat64> controlPointPositionLps;
        brachyControlPointItem.getControlPoint3DPosition(controlPointPositionLps);
        channelEntry.ControlPointVector[controlPointIndex][0] = -controlPointPositionLps[0];
        channelEntry.ControlPointVector[controlPointIndex][1] = -controlPointPositionLps[1];
        channelEntry.ControlPointVector[controlPointIndex][2] =  controlPointPositionLps[2];

        //TODO: Get additional control point properties? (ControlPointRelativePosition, CumulativeTimeWeight)

        ++controlPointCount;
      }
      while (rtBrachyControlPointSequence.gotoNextItem().good());

      if (Sint32(controlPointCount) != channelNumberOfControlPoints)
      {
        vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Number of brachy control points expected ("
          << channelNumberOfControlPoints << ") and found (" << controlPointCount << ") do not match. Invalid points remain among control points");
      }

      this->ChannelSequenceVector.push_back(channelEntry);
    }
    while (channelSequence.gotoNextItem().good());
  }
  else
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: No beams or application setup found in RT plan");
    return;
  }

  // Get SOP instance UID
  OFString sopInstanceUid("");
  if (rtPlan.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Failed to get SOP instance UID for RT plan");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Referenced structure set UID
  DRTReferencedStructureSetSequence &referencedStructureSetSequence = rtPlan.getReferencedStructureSetSequence();
  if (referencedStructureSetSequence.gotoFirstItem().good())
  {
    DRTReferencedStructureSetSequence::Item &referencedStructureSetSequenceItem = referencedStructureSetSequence.getCurrentItem();
    if (referencedStructureSetSequenceItem.isValid())
    {
      OFString referencedSOPInstanceUID("");
      if (referencedStructureSetSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
      {
        this->External->SetRTPlanReferencedStructureSetSOPInstanceUID(referencedSOPInstanceUID.c_str());
      }
    }
  }

  // Referenced dose UID
  DRTReferencedDoseSequence &referencedDoseSequence = rtPlan.getReferencedDoseSequence();
  std::string serializedDoseUidList("");
  if (referencedDoseSequence.gotoFirstItem().good())
  {
    do
    {
      DRTReferencedDoseSequence::Item &currentDoseSequenceItem = referencedDoseSequence.getCurrentItem();
      if (currentDoseSequenceItem.isValid())
      {
        OFString referencedSOPInstanceUID("");
        if (currentDoseSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
        {
          serializedDoseUidList.append(referencedSOPInstanceUID.c_str());
          serializedDoseUidList.append(" ");
        }
      }
    }
    while (referencedDoseSequence.gotoNextItem().good());
  }
  // Strip last space
  serializedDoseUidList = serializedDoseUidList.substr(0, serializedDoseUidList.size()-1);
  this->External->SetRTPlanReferencedDoseSOPInstanceUIDs(serializedDoseUidList.size() > 0 ? serializedDoseUidList.c_str() : nullptr);

  // Get and store patient, study and series information
  this->External->GetAndStoreRtHierarchyInformation(&rtPlan);

  this->External->LoadRTPlanSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTIonPlan(DcmDataset* dataset)
{
  this->External->LoadRTIonPlanSuccessful = false;

  DRTIonPlanIOD ionPlan;
  if (ionPlan.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro( this->External, "LoadRTIonPlan: Failed to read RT Ion Plan object!");
    return;
  }

  vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: Load RT Ion Plan object");
  if (ionPlan.isRTFractionSchemeModulePresent(OFTrue) == OFTrue)
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: Fraction Scheme is correct");
  }
  else if (ionPlan.isRTFractionSchemeModulePresent() == OFTrue)
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTIonPlan: Fraction Scheme is partially correct");
  }
  else
  {
    vtkErrorWithObjectMacro( this->External, "LoadRTIonPlan: Fraction Scheme is absent");
    return;
  }

  bool hasBeamsModule = true;
  if (ionPlan.isRTIonBeamsModulePresent(OFTrue) == OFTrue)
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: Ion beams module is correct");
  }
  else if (ionPlan.isRTIonBeamsModulePresent() == OFTrue)
  {
    vtkWarningWithObjectMacro( this->External, "LoadRTIonPlan: Ion beams module is partially correct");
  }
  else
  {
    vtkErrorWithObjectMacro( this->External, "LoadRTIonPlan: Ion beams module is absent");
    hasBeamsModule = false;
  }

  bool haveBeams = false;
  DRTFractionGroupSequence& rtPlanFractionsSequence = ionPlan.getFractionGroupSequence();
  // RTPlan fraction sequence
  if (rtPlanFractionsSequence.isValid() && rtPlanFractionsSequence.gotoFirstItem().good())
  {
    do
    {
      DRTFractionGroupSequence::Item &fractionSequenceItem = rtPlanFractionsSequence.getCurrentItem();
      if (!fractionSequenceItem.isValid())
      {
        // possibly reach the end of the sequence
        if (rtPlanFractionsSequence.gotoNextItem().good())
        {
          // Only log warning if this is not the last item (the loop reaches here after the final item)
          vtkWarningWithObjectMacro( this->External, "LoadRTIonPlan: Found an invalid fraction item in dataset");
        }
        continue;
      }

      Sint32 Number = -1;
      fractionSequenceItem.getFractionGroupNumber(Number);

      Sint32 nofBeams = -1;
      fractionSequenceItem.getNumberOfBeams(nofBeams);
      if (nofBeams > 0)
      {
        vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: Number of beams are " 
          << nofBeams << " in fraction group item number " << Number);
        haveBeams = true;
      }
    }
    while (rtPlanFractionsSequence.gotoNextItem().good());

  }

  if (hasBeamsModule && haveBeams)
  {
    vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: Data is available for loading");
  }
  else
  {
    vtkErrorWithObjectMacro( this->External, "LoadRTIonPlan: No data for loading");
    return;
  }

  DRTIonBeamSequence &ionBeamSequence = ionPlan.getIonBeamSequence();
  // RTPlan ion beam sequence (external ion beam plan)
  if (ionBeamSequence.isValid() && ionBeamSequence.gotoFirstItem().good())
  {
    do
    {
      DRTIonBeamSequence::Item &currentIonBeamSequenceItem = ionBeamSequence.getCurrentItem();
      if (!currentIonBeamSequenceItem.isValid())
      {
        // possibly reach the end of the sequense
        if (ionBeamSequence.gotoNextItem().good())
        {
          vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: Found an invalid ion beam item in dataset");
        }
        continue;
      }

      // Read item into the BeamSequenceVector
      BeamEntry beamEntry;

      OFString beamName("");
      currentIonBeamSequenceItem.getBeamName(beamName);
      beamEntry.Name = beamName.c_str();

      OFString beamDescription("");
      currentIonBeamSequenceItem.getBeamDescription(beamDescription);
      beamEntry.Description = beamDescription.c_str();

      OFString treatmentDeliveryType("");
      currentIonBeamSequenceItem.getTreatmentDeliveryType(treatmentDeliveryType);
      beamEntry.TreatmentDeliveryType=treatmentDeliveryType.c_str();

      OFString beamType("");
      currentIonBeamSequenceItem.getBeamType(beamType);
      beamEntry.Type = beamType.c_str();

      Sint32 beamNumber = -1;
      currentIonBeamSequenceItem.getBeamNumber(beamNumber);
      beamEntry.Number = beamNumber;

      OFString scanMode("");
      currentIonBeamSequenceItem.getScanMode(scanMode);
      beamEntry.ScanMode = scanMode.c_str();

      OFString radiationType("");
      currentIonBeamSequenceItem.getRadiationType(radiationType);
      beamEntry.RadiationType = radiationType.c_str();
      if (!radiationType.compare("ION"))
      {
        Sint32 Z = 0;
        currentIonBeamSequenceItem.getRadiationAtomicNumber(Z);
        beamEntry.RadiationIon[0] = Z;
        Sint32 A = 0;
        currentIonBeamSequenceItem.getRadiationMassNumber(A);
        beamEntry.RadiationIon[1] = A;
        Sint16 Charge = 0;
        currentIonBeamSequenceItem.getRadiationChargeState(Charge);
        beamEntry.RadiationIon[2] = Charge;
      }

      Float32 VSADx = -1.0f, VSADy = -1.0f; // virtual source axis distance components
      currentIonBeamSequenceItem.getVirtualSourceAxisDistances( VSADx, 0);
      currentIonBeamSequenceItem.getVirtualSourceAxisDistances( VSADy, 1);
      beamEntry.SourceAxisDistance = { VSADx, VSADy };

      Float64 finalCumulativeMetersetWeight = -1.;
      currentIonBeamSequenceItem.getFinalCumulativeMetersetWeight(finalCumulativeMetersetWeight);
      beamEntry.FinalCumulativeMetersetWeight = finalCumulativeMetersetWeight;

      DRTIonBeamLimitingDeviceSequence& rtIonBeamLimitingDeviceSequence = 
        currentIonBeamSequenceItem.getIonBeamLimitingDeviceSequence();
      if (rtIonBeamLimitingDeviceSequence.isValid() && 
        rtIonBeamLimitingDeviceSequence.gotoFirstItem().good())
      {
        do
        {
          DRTIonBeamLimitingDeviceSequence::Item &collimatorItem =
            rtIonBeamLimitingDeviceSequence.getCurrentItem();
          if (collimatorItem.isValid())
          {
            OFString deviceType("");
            Sint32 nofPairs = -1;

            // isocenter to beam limiting device distance
            Float32 distance = -1.0f;
            collimatorItem.getIsocenterToBeamLimitingDeviceDistance(distance);

            collimatorItem.getRTBeamLimitingDeviceType(deviceType);
            if (!deviceType.compare("X") || !deviceType.compare("ASYMX"))
            {
              beamEntry.SourceIsoToJawsDistance[0] = distance;
            }
            else if (!deviceType.compare("Y") || !deviceType.compare("ASYMY"))
            {
              beamEntry.SourceIsoToJawsDistance[1] = distance;
            }
            else if (!deviceType.compare("MLCX") || !deviceType.compare("MLCY"))
            {              
              OFCondition getNumberOfLeafJawPairsCondition = collimatorItem.getNumberOfLeafJawPairs(nofPairs);
              if (getNumberOfLeafJawPairsCondition.good())
              {
                std::string& type = beamEntry.MultiLeafCollimatorType;
                double& mlcDistance = beamEntry.MultiLeafCollimator.SourceIsoDistance;
                unsigned int& pairs = beamEntry.MultiLeafCollimator.NumberOfLeafJawPairs;
                std::vector<double>& bounds = beamEntry.MultiLeafCollimator.LeafPositionBoundary;
                OFVector<Float64> leafPositionBoundaries;
                OFCondition getBoundariesCondition = collimatorItem.getLeafPositionBoundaries(leafPositionBoundaries);
                if (getBoundariesCondition.good())
                {
                  type = deviceType.c_str();
                  mlcDistance = distance;
                  pairs = nofPairs;
                  bounds.resize(leafPositionBoundaries.size());
                  std::copy( leafPositionBoundaries.begin(), leafPositionBoundaries.end(), bounds.begin());
                  vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: multi-leaf collimator type is " 
                    << type << ", number of leaf pairs are " << nofPairs);
                }
              }
            }
          }
        } while (rtIonBeamLimitingDeviceSequence.gotoNextItem().good());
      }

      Sint32 beamNumberOfControlPoints = -1;
      currentIonBeamSequenceItem.getNumberOfControlPoints(beamNumberOfControlPoints);
      beamEntry.NumberOfControlPoints = beamNumberOfControlPoints;

      DRTIonControlPointSequence &rtIonControlPointSequence = currentIonBeamSequenceItem.getIonControlPointSequence();
      if (!rtIonControlPointSequence.isValid() || !rtIonControlPointSequence.gotoFirstItem().good())
      {
        vtkWarningWithObjectMacro( this->External, "LoadRTIonPlan: Found an invalid RT ion control point sequence in dataset");
        continue;
      }

      // Initialize control point vector so that it can be correctly filled even if control points arrive in random order
      for ( Sint32 i = 0; i < beamNumberOfControlPoints; ++i)
      {
        ControlPointEntry dummyControlPoint;
        beamEntry.ControlPointSequenceVector.push_back(dummyControlPoint);
      }
      unsigned int controlPointCount = 0;

      do
      {
        DRTIonControlPointSequence::Item &controlPointItem = rtIonControlPointSequence.getCurrentItem();
        if (!controlPointItem.isValid())
        {
          // possibly reach the end of the sequense
          if (rtIonControlPointSequence.gotoNextItem().good())
          {
            vtkWarningWithObjectMacro( this->External, "LoadRTIonPlan: Found an invalid RT ion control point in dataset");
          }
          continue;
        }

        Sint32 controlPointIndex = -1;
        controlPointItem.getControlPointIndex(controlPointIndex);
        if (controlPointIndex < 0 || controlPointIndex >= beamNumberOfControlPoints)
        {
          vtkErrorWithObjectMacro( this->External, 
            "LoadRTIonPlan: Invalid control point index value " 
            << controlPointIndex << ", for a beam number "
            << beamEntry.Number << " named \"" << beamEntry.Name << "\"");
          continue;
        }

        // Control point item from the ControlPointSequenceVector
        ControlPointEntry& controlPoint = beamEntry.ControlPointSequenceVector.at(controlPointIndex);
        // assign previous point control point to a current control point
        if (controlPointIndex > 0)
        {
          ControlPointEntry& prevControlPoint = beamEntry.ControlPointSequenceVector.at(controlPointIndex - 1);
          controlPoint = ControlPointEntry(prevControlPoint);
        }
//        ControlPointEntry& controlPointEntry = beamEntry.ControlPointSequenceVector.at(controlPointCount);        
        controlPoint.Index = controlPointIndex;

        OFVector<Float64> isocenterPositionDataLps;
        OFString isocenterStringX, isocenterStringY, isocenterStringZ;

        controlPointItem.getIsocenterPosition( isocenterStringX, 0);
        controlPointItem.getIsocenterPosition( isocenterStringY, 1);
        controlPointItem.getIsocenterPosition( isocenterStringZ, 2);

        controlPointItem.getIsocenterPosition(isocenterPositionDataLps);
        if (!isocenterStringX.empty() && !isocenterStringY.empty() && !isocenterStringZ.empty())
        {
          // Convert from DICOM LPS -> Slicer RAS
          controlPoint.IsocenterPositionRas[0] = -isocenterPositionDataLps[0];
          controlPoint.IsocenterPositionRas[1] = -isocenterPositionDataLps[1];
          controlPoint.IsocenterPositionRas[2] = isocenterPositionDataLps[2];
        }

        OFCondition dataCondition;

        OFString kvpString, beamEnergyString;
        controlPointItem.getNominalBeamEnergy(beamEnergyString);
        controlPointItem.getKVP(kvpString);

        if (!beamEnergyString.empty() && kvpString.empty())
        {
          Float64 nominalBeamEnergy = 0.0;
          dataCondition = controlPointItem.getNominalBeamEnergy(nominalBeamEnergy);
          if (dataCondition.good())
          {
            controlPoint.NominalBeamEnergy = nominalBeamEnergy;
          }
        }
        else if (beamEnergyString.empty() && !kvpString.empty())
        {
          // Do something with KVP value
        }

        Float64 gantryAngle = 0.0;
        dataCondition = controlPointItem.getGantryAngle(gantryAngle);
        if (dataCondition.good())
        {
          controlPoint.GantryAngle = gantryAngle;
        }

        Float64 patientSupportAngle = 0.0;
        dataCondition = controlPointItem.getPatientSupportAngle(patientSupportAngle);
        if (dataCondition.good())
        {
          controlPoint.PatientSupportAngle = patientSupportAngle;
        }

        Float64 beamLimitingDeviceAngle = 0.0;
        dataCondition = controlPointItem.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
        if (dataCondition.good())
        {
          controlPoint.BeamLimitingDeviceAngle = beamLimitingDeviceAngle;
        }

        Float64 cumulativeMetersetWeight = -1.;
        dataCondition = controlPointItem.getCumulativeMetersetWeight(cumulativeMetersetWeight);
        if (dataCondition.good())
        {
          controlPoint.CumulativeMetersetWeight = cumulativeMetersetWeight;
        }

        if (!scanMode.compare("MODULATED") || !scanMode.compare("MODULATED_SPEC"))
        {
          OFString scanSpotTuneId("");
          dataCondition = controlPointItem.getScanSpotTuneID(scanSpotTuneId);
          if (dataCondition.good())
          {
            controlPoint.ScanSpotTuneId = scanSpotTuneId.c_str();
          }
 
          Sint32 nofScanSpotPositions = -1;
          dataCondition = controlPointItem.getNumberOfScanSpotPositions(nofScanSpotPositions);
          if (dataCondition.good())
          {
            controlPoint.NumberOfScanSpotPositions = nofScanSpotPositions;
          }

          OFString scanSpotReorderingAllowed("");
          dataCondition = controlPointItem.getScanSpotReorderingAllowed(scanSpotReorderingAllowed);
          if (dataCondition.good())
          {
            controlPoint.ScanSpotReorderingAllowed = scanSpotReorderingAllowed.c_str();
          }
          
          Float32 scanningSpotSizeX = -1.f, scanningSpotSizeY = -1.f;
          dataCondition = controlPointItem.getScanningSpotSize( scanningSpotSizeX, 0);
          if (dataCondition.good())
          {
            controlPoint.ScanningSpotSize[0] = scanningSpotSizeX;
          }
          dataCondition = controlPointItem.getScanningSpotSize( scanningSpotSizeY, 1);
          if (dataCondition.good())
          {
            controlPoint.ScanningSpotSize[1] = scanningSpotSizeY;
          }
          if (controlPoint.NumberOfScanSpotPositions > 0)
          {
            controlPoint.ScanSpotPositionMap.resize(2 * controlPoint.NumberOfScanSpotPositions);
            controlPoint.ScanSpotMetersetWeights.resize(controlPoint.NumberOfScanSpotPositions);

            for ( unsigned int i = 0; i < controlPoint.NumberOfScanSpotPositions; ++i)
            {
              Float32 x = -1.f, y = -1.f, w = -1.f;
              dataCondition = controlPointItem.getScanSpotPositionMap( x, 2 * i);
              if (dataCondition.good())
              {
                controlPoint.ScanSpotPositionMap.at(2 * i) = float(x);
              }
              dataCondition = controlPointItem.getScanSpotPositionMap( y, 2 * i + 1);
              if (dataCondition.good())
              {
                controlPoint.ScanSpotPositionMap.at(2 * i + 1) = float(y);
              }

              dataCondition = controlPointItem.getScanSpotMetersetWeights( w, i);
              if (dataCondition.good())
              {
                controlPoint.ScanSpotMetersetWeights.at(i) = float(w);
              }
            }
          }
          Sint32 nofPaintings = -1;
          dataCondition = controlPointItem.getNumberOfPaintings(nofPaintings);
          if (dataCondition.good())
          {
            controlPoint.NumberOfPaintings = nofPaintings;
          }
        }

        DRTBeamLimitingDevicePositionSequence &currentCollimatorPositionSequence =
          controlPointItem.getBeamLimitingDevicePositionSequence();
        if (currentCollimatorPositionSequence.isValid() &&
          currentCollimatorPositionSequence.gotoFirstItem().good())
        {
          do
          {
            DRTBeamLimitingDevicePositionSequence::Item &collimatorPositionItem =
              currentCollimatorPositionSequence.getCurrentItem();
            if (collimatorPositionItem.isValid())
            {
              OFString rtBeamLimitingDeviceType("");
              collimatorPositionItem.getRTBeamLimitingDeviceType(rtBeamLimitingDeviceType);

              OFVector<Float64> leafJawPositions;
              OFCondition getJawPositionsCondition = collimatorPositionItem.getLeafJawPositions(leafJawPositions);
              if (!rtBeamLimitingDeviceType.compare("ASYMX") || !rtBeamLimitingDeviceType.compare("X"))
              {
                if (getJawPositionsCondition.good())
                {
                  controlPoint.JawPositions[0] = leafJawPositions[0];
                  controlPoint.JawPositions[1] = leafJawPositions[1];
                }
                else
                {
                  vtkDebugWithObjectMacro(this->External, "LoadRTIonPlan: No jaw position found in collimator entry");
                }
              }
              else if (!rtBeamLimitingDeviceType.compare("ASYMY") || !rtBeamLimitingDeviceType.compare("Y"))
              {
                if (getJawPositionsCondition.good())
                {
                  controlPoint.JawPositions[2] = leafJawPositions[0];
                  controlPoint.JawPositions[3] = leafJawPositions[1];
                }
                else
                {
                  vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: No jaw position found in collimator entry");
                }
              }
              else if (!rtBeamLimitingDeviceType.compare("MLCX") || !rtBeamLimitingDeviceType.compare("MLCY"))
              {
                // Get MLC leaves position (opening)
                if (getJawPositionsCondition.good())
                {
                  std::vector<double>& mlcPositions = controlPoint.LeafPositions;
                  std::string& mlcType = controlPoint.MultiLeafCollimatorType;

                  mlcType = rtBeamLimitingDeviceType.c_str();
                  mlcPositions.resize(leafJawPositions.size());
                  std::copy( leafJawPositions.begin(), leafJawPositions.end(), mlcPositions.begin());
                  vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: " 
                    << mlcType << " leaf positions have been loaded");
                }
                else
                {
                  vtkDebugWithObjectMacro( this->External, "LoadRTIonPlan: No MLC position found in collimator entry");
                }
              }
              else
              {
                vtkErrorWithObjectMacro( this->External, 
                  "LoadRTIonPlan: Unsupported collimator type: " 
                  << rtBeamLimitingDeviceType);
              }
            }
          }
          while (currentCollimatorPositionSequence.gotoNextItem().good());

        }
        ++controlPointCount;
      }
      while (rtIonControlPointSequence.gotoNextItem().good());

      if (Sint32(controlPointCount) != beamNumberOfControlPoints)
      {
        vtkErrorWithObjectMacro( this->External, "LoadRTIonPlan: Number of control points expected ("
          << beamNumberOfControlPoints << ") and found (" << controlPointCount << ") do not match. Invalid points remain among control points");
      }
      this->BeamSequenceVector.push_back(beamEntry);
    }
    while (ionBeamSequence.gotoNextItem().good());

  }
  else
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTIonPlan: No beams found in RT ion plan!");
    return;
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (ionPlan.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTIonPlan: Failed to get SOP instance UID for RT plan!");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Referenced structure set UID
  DRTReferencedStructureSetSequence &referencedStructureSetSequence = ionPlan.getReferencedStructureSetSequence();
  if (referencedStructureSetSequence.gotoFirstItem().good())
  {
    DRTReferencedStructureSetSequence::Item &referencedStructureSetSequenceItem = referencedStructureSetSequence.getCurrentItem();
    if (referencedStructureSetSequenceItem.isValid())
    {
      OFString referencedSOPInstanceUID("");
      if (referencedStructureSetSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
      {
        this->External->SetRTPlanReferencedStructureSetSOPInstanceUID(referencedSOPInstanceUID.c_str());
      }
    }
  }

  // Referenced dose UID
  DRTReferencedDoseSequence &referencedDoseSequence = ionPlan.getReferencedDoseSequence();
  std::string serializedDoseUidList("");
  if (referencedDoseSequence.gotoFirstItem().good())
  {
    do
    {
      DRTReferencedDoseSequence::Item &currentDoseSequenceItem = referencedDoseSequence.getCurrentItem();
      if (currentDoseSequenceItem.isValid())
      {
        OFString referencedSOPInstanceUID("");
        if (currentDoseSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
        {
          serializedDoseUidList.append(referencedSOPInstanceUID.c_str());
          serializedDoseUidList.append(" ");
        }
      }
    } while (referencedDoseSequence.gotoNextItem().good());
  }
  // Strip last space
  serializedDoseUidList = serializedDoseUidList.substr(0, serializedDoseUidList.size() - 1);
  this->External->SetRTPlanReferencedDoseSOPInstanceUIDs(serializedDoseUidList.size() > 0 ? serializedDoseUidList.c_str() : NULL);

  // Get and store patient, study and series information
  this->External->GetAndStoreRtHierarchyInformation(&ionPlan);

  this->External->LoadRTIonPlanSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTStructureSet(DcmDataset* dataset)
{
  this->External->LoadRTStructureSetSuccessful = false;

  DRTStructureSetIOD* rtStructureSet = new DRTStructureSetIOD();
  if (rtStructureSet->read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTStructureSet: Could not load strucure set object from dataset");
    delete rtStructureSet;
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTStructureSet: RT Structure Set object");

  // Read ROI name, description, and number into the ROI contour sequence vector (StructureSetROISequence)
  DRTStructureSetROISequence* rtStructureSetROISequence = new DRTStructureSetROISequence(rtStructureSet->getStructureSetROISequence());
  this->LoadContoursFromRoiSequence(rtStructureSetROISequence);

  // Get referenced anatomical image
  OFString referencedSeriesInstanceUID = this->GetReferencedSeriesInstanceUID(rtStructureSet);

  // Get ROI contour sequence
  DRTROIContourSequence &rtROIContourSequence = rtStructureSet->getROIContourSequence();
  // Reset the ROI contour sequence to the start
  if (!rtROIContourSequence.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTStructureSet: No ROIContourSequence found");
    delete rtStructureSet;
    return;
  }

  // Read ROIs, iterate over ROI contour sequence
  do 
  {
    DRTROIContourSequence::Item &currentRoi = rtROIContourSequence.getCurrentItem();
    RoiEntry* currentRoiEntry = this->LoadContour(currentRoi, rtStructureSet);
    if (currentRoiEntry)
    {
      // Set referenced series UID
      currentRoiEntry->ReferencedSeriesUID = (std::string)referencedSeriesInstanceUID.c_str();
    }
  }
  while (rtROIContourSequence.gotoNextItem().good());

  // Get SOP instance UID
  OFString sopInstanceUid("");
  if (rtStructureSet->getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTStructureSet: Failed to get SOP instance UID for RT structure set");
    delete rtStructureSet;
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->External->GetAndStoreRtHierarchyInformation(rtStructureSet);

  this->External->LoadRTStructureSetSuccessful = true;
  delete rtStructureSet;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadContoursFromRoiSequence(DRTStructureSetROISequence* rtStructureSetROISequence)
{
  if (!rtStructureSetROISequence->gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadContoursFromRoiSequence: No structure sets were found");
    return;
  }
  do
  {
    DRTStructureSetROISequence::Item &currentROISequence = rtStructureSetROISequence->getCurrentItem();
    if (!currentROISequence.isValid())
    {
      continue;
    }

    RoiEntry roiEntry;

    OFString roiName("");
    currentROISequence.getROIName(roiName);
    roiEntry.Name = roiName.c_str();

    OFString roiDescription("");
    currentROISequence.getROIDescription(roiDescription);
    roiEntry.Description = roiDescription.c_str();                   

    OFString referencedFrameOfReferenceUid("");
    currentROISequence.getReferencedFrameOfReferenceUID(referencedFrameOfReferenceUid);
    roiEntry.ReferencedFrameOfReferenceUID = referencedFrameOfReferenceUid.c_str();

    Sint32 roiNumber = -1;
    currentROISequence.getROINumber(roiNumber);
    roiEntry.Number=roiNumber;

    // Save to vector          
    this->RoiSequenceVector.push_back(roiEntry);
  }
  while (rtStructureSetROISequence->gotoNextItem().good());
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry* vtkSlicerDicomRtReader::vtkInternal::LoadContour(
  DRTROIContourSequence::Item &roi, DRTStructureSetIOD* rtStructureSet)
{
  if (!roi.isValid())
  {
    return nullptr;
  }

  // Used for connection from one planar contour ROI to the corresponding anatomical volume slice instance
  std::map<int, std::string> contourToSliceInstanceUIDMap;
  std::set<std::string> referencedSopInstanceUids;

  // Get ROI entry created for the referenced ROI
  Sint32 referencedRoiNumber = -1;
  roi.getReferencedROINumber(referencedRoiNumber);
  RoiEntry* roiEntry = this->FindRoiByNumber(referencedRoiNumber);
  if (roiEntry == nullptr)
  {
    vtkErrorWithObjectMacro(this->External, "LoadContour: ROI with number " << referencedRoiNumber << " is not found");      
    return nullptr;
  } 

  // Get contour sequence
  DRTContourSequence &rtContourSequence = roi.getContourSequence();
  if (!rtContourSequence.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadContour: Contour sequence for ROI named '"
      << roiEntry->Name << "' with number " << referencedRoiNumber << " is empty");
    return roiEntry;
  }

  // Create containers for contour poly data
  vtkSmartPointer<vtkPoints> currentRoiContourPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> currentRoiContourCells = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType pointId = 0;

  // Read contour data, iterate over contour sequence
  do
  {
    // Get contour
    DRTContourSequence::Item &contourItem = rtContourSequence.getCurrentItem();
    if (!contourItem.isValid())
    {
      continue;
    }

    // Get number of contour points
    OFString numberOfPointsString("");
    contourItem.getNumberOfContourPoints(numberOfPointsString);
    std::stringstream ss;
    ss << numberOfPointsString;
    int numberOfPoints;
    ss >> numberOfPoints;

    // Get contour point data
    OFVector<vtkTypeFloat64> contourData_LPS;
    contourItem.getContourData(contourData_LPS);
    if (contourData_LPS.size() != size_t(numberOfPoints * 3))
    {
      vtkErrorWithObjectMacro(this->External, "LoadContour: Contour sequence object item is invalid: "
        << " number of contour points is " << numberOfPoints << " therefore expected "
        << numberOfPoints * 3 << " values in contour data but only found " << contourData_LPS.size());
      continue;
    }

    unsigned int contourIndex = currentRoiContourCells->InsertNextCell(numberOfPoints+1);
    for (int k=0; k<numberOfPoints; k++)
    {
      // Convert from DICOM LPS -> Slicer RAS
      currentRoiContourPoints->InsertPoint(pointId, -contourData_LPS[3*k], -contourData_LPS[3*k+1], contourData_LPS[3*k+2]);
      currentRoiContourCells->InsertCellPoint(pointId);
      pointId++;
    }

    // Close the contour
    currentRoiContourCells->InsertCellPoint(pointId-numberOfPoints);

    // Add map to the referenced slice instance UID
    // This is not a mandatory field so no error logged if not found. The reason why
    // it is still read and stored is that it references the contours individually
    DRTContourImageSequence &rtContourImageSequence = contourItem.getContourImageSequence();
    if (rtContourImageSequence.gotoFirstItem().good())
    {
      DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequence.getCurrentItem();
      if (rtContourImageSequenceItem.isValid())
      {
        OFString referencedSOPInstanceUID("");
        rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
        contourToSliceInstanceUIDMap[contourIndex] = referencedSOPInstanceUID.c_str();
        referencedSopInstanceUids.insert(referencedSOPInstanceUID.c_str());

        // Check if multiple SOP instance UIDs are referenced
        if (rtContourImageSequence.getNumberOfItems() > 1)
        {
          vtkWarningWithObjectMacro(this->External, "LoadContour: Contour in ROI " << roiEntry->Number << ": " << roiEntry->Name << " contains multiple referenced instances. This is not yet supported");
        }
      }
      else
      {
        vtkErrorWithObjectMacro(this->External, "LoadContour: Contour image sequence object item is invalid");
      }
    }
  }
  while (rtContourSequence.gotoNextItem().good());

  // Read slice reference UIDs from referenced frame of reference sequence if it was not included in the ROIContourSequence above
  if (contourToSliceInstanceUIDMap.empty())
  {
    DRTContourImageSequence* rtContourImageSequence = this->GetReferencedFrameOfReferenceContourImageSequence(rtStructureSet);
    if (rtContourImageSequence && rtContourImageSequence->gotoFirstItem().good())
    {
      int currentSliceNumber = -1; // Use negative keys to indicate that the slice instances cannot be directly mapped to the ROI planar contours
      do 
      {
        DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequence->getCurrentItem();
        if (rtContourImageSequenceItem.isValid())
        {
          OFString referencedSOPInstanceUID("");
          rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
          contourToSliceInstanceUIDMap[currentSliceNumber] = referencedSOPInstanceUID.c_str();
          referencedSopInstanceUids.insert(referencedSOPInstanceUID.c_str());
        }
        else
        {
          vtkErrorWithObjectMacro(this->External, "LoadContour: Contour image sequence object item in referenced frame of reference sequence is invalid");
        }
        currentSliceNumber--;
      }
      while (rtContourImageSequence->gotoNextItem().good());
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadContour: No items in contour image sequence object item in referenced frame of reference sequence");
    }
  }

  // Save just loaded contour data into ROI entry
  vtkSmartPointer<vtkPolyData> currentRoiPolyData = vtkSmartPointer<vtkPolyData>::New();
  currentRoiPolyData->SetPoints(currentRoiContourPoints);
  if (currentRoiContourPoints->GetNumberOfPoints() == 1)
  {
    // Point ROI
    currentRoiPolyData->SetVerts(currentRoiContourCells);
  }
  else if (currentRoiContourPoints->GetNumberOfPoints() > 1)
  {
    // Contour ROI
    currentRoiPolyData->SetLines(currentRoiContourCells);
  }
  roiEntry->SetPolyData(currentRoiPolyData);

  // Get structure color
  Sint32 roiDisplayColor = -1;
  for (int j=0; j<3; j++)
  {
    roi.getROIDisplayColor(roiDisplayColor,j);
    roiEntry->DisplayColor[j] = roiDisplayColor/255.0;
  }

  // Set referenced SOP instance UIDs
  roiEntry->ContourIndexToSOPInstanceUIDMap = contourToSliceInstanceUIDMap;

  // Serialize referenced SOP instance UID set
  std::set<std::string>::iterator uidIt;
  std::string serializedUidList("");
  for (uidIt = referencedSopInstanceUids.begin(); uidIt != referencedSopInstanceUids.end(); ++uidIt)
  {
    serializedUidList.append(*uidIt);
    serializedUidList.append(" ");
  }
  // Strip last space
  serializedUidList = serializedUidList.substr(0, serializedUidList.size()-1);
  this->External->SetRTStructureSetReferencedSOPInstanceUIDs(serializedUidList.c_str());

  return roiEntry;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTImage(DcmDataset* dataset)
{
  this->External->LoadRTImageSuccessful = false;

  DRTImageIOD rtImage;
  if (rtImage.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to read RT Image object");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTImage: Load RT Image object");

  // ImageType
  OFString imageType("");
  if (rtImage.getImageType(imageType).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get Image Type for RT Image object");
    return; // mandatory DICOM value
  }
  this->External->SetImageType(imageType.c_str());

  // RTImageLabel
  OFString rtImagelabel("");
  if (rtImage.getRTImageLabel(rtImagelabel).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get RT Image Label for RT Image object");
    return; // mandatory DICOM value
  }
  this->External->SetRTImageLabel(imageType.c_str());

  // RTImagePlane (confirm it is NORMAL)
  OFString rtImagePlane("");
  if (rtImage.getRTImagePlane(rtImagePlane).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get RT Image Plane for RT Image object");
    return; // mandatory DICOM value
  }
  if (rtImagePlane.compare("NORMAL"))
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Only value 'NORMAL' is supported for RTImagePlane tag for RT Image objects");
    return;
  }

  // ReferencedRTPlanSequence
  DRTReferencedRTPlanSequenceInRTImageModule &rtReferencedRtPlanSequence = rtImage.getReferencedRTPlanSequence();
  if (rtReferencedRtPlanSequence.gotoFirstItem().good())
  {
    DRTReferencedRTPlanSequenceInRTImageModule::Item &currentReferencedRtPlanSequence = rtReferencedRtPlanSequence.getCurrentItem();

    OFString referencedSOPClassUID("");
    currentReferencedRtPlanSequence.getReferencedSOPClassUID(referencedSOPClassUID);
    if (referencedSOPClassUID.compare(UID_RTPlanStorage))
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Referenced RT Plan SOP class has to be RTPlanStorage");
    }
    else
    {
      // Read Referenced RT Plan SOP instance UID
      OFString referencedSOPInstanceUID("");
      currentReferencedRtPlanSequence.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
      this->External->SetRTImageReferencedRTPlanSOPInstanceUID(referencedSOPInstanceUID.c_str());
    }

    if (rtReferencedRtPlanSequence.getNumberOfItems() > 1)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Referenced RT Plan sequence object can contain one item! It contains " << rtReferencedRtPlanSequence.getNumberOfItems());
    }
  }

  // ReferencedBeamNumber
  Sint32 referencedBeamNumber = -1;
  if (rtImage.getReferencedBeamNumber(referencedBeamNumber).good())
  {
    this->External->ReferencedBeamNumber = (int)referencedBeamNumber;
  }
  else if (rtReferencedRtPlanSequence.getNumberOfItems() == 1)
  {
    // Type 3
    vtkDebugWithObjectMacro(this->External, "LoadRTImage: Unable to get referenced beam number in referenced RT Plan for RT image");
  }

  // XRayImageReceptorTranslation
  OFVector<vtkTypeFloat64> xRayImageReceptorTranslation;
  if (rtImage.getXRayImageReceptorTranslation(xRayImageReceptorTranslation).good())
  {
    if (xRayImageReceptorTranslation.size() == 3)
    {
      if ( xRayImageReceptorTranslation[0] != 0.0
        || xRayImageReceptorTranslation[1] != 0.0
        || xRayImageReceptorTranslation[2] != 0.0 )
      {
        vtkErrorWithObjectMacro(this->External, "LoadRTImage: Non-zero XRayImageReceptorTranslation vectors are not supported");
        return;
      }
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: XRayImageReceptorTranslation tag should contain a vector of 3 elements (it has " << xRayImageReceptorTranslation.size() << "");
    }
  }

  // XRayImageReceptorAngle
  vtkTypeFloat64 xRayImageReceptorAngle = 0.0;
  if (rtImage.getXRayImageReceptorAngle(xRayImageReceptorAngle).good())
  {
    if (xRayImageReceptorAngle != 0.0)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Non-zero XRayImageReceptorAngle spacingValues are not supported");
      return;
    }
  }

  // ImagePlanePixelSpacing
  OFVector<vtkTypeFloat64> imagePlanePixelSpacing;
  if (rtImage.getImagePlanePixelSpacing(imagePlanePixelSpacing).good())
  {
    if (imagePlanePixelSpacing.size() == 2)
    {
      //this->SetImagePlanePixelSpacing(imagePlanePixelSpacing[0], imagePlanePixelSpacing[1]);
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: ImagePlanePixelSpacing tag should contain a vector of 2 elements (it has " << imagePlanePixelSpacing.size() << "");
    }
  }

  // RTImagePosition
  OFVector<vtkTypeFloat64> rtImagePosition;
  if (rtImage.getRTImagePosition(rtImagePosition).good())
  {
    if (rtImagePosition.size() == 2)
    {
      this->External->SetRTImagePosition(rtImagePosition[0], rtImagePosition[1]);
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: RTImagePosition tag should contain a vector of 2 elements (it has " << rtImagePosition.size() << ")");
    }
  }

  // RTImageOrientation
  OFVector<vtkTypeFloat64> rtImageOrientation;
  if (rtImage.getRTImageOrientation(rtImageOrientation).good())
  {
    if (rtImageOrientation.size() > 0)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: RTImageOrientation is specified but not supported yet");
    }
  }

  // GantryAngle
  vtkTypeFloat64 gantryAngle = 0.0;
  if (rtImage.getGantryAngle(gantryAngle).good())
  {
    this->External->SetGantryAngle(gantryAngle);
  }

  // GantryPitchAngle
  vtkTypeFloat32 gantryPitchAngle = 0.0;
  if (rtImage.getGantryPitchAngle(gantryPitchAngle).good())
  {
    if (gantryPitchAngle != 0.0)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Non-zero GantryPitchAngle tag spacingValues are not supported yet");
      return;
    }
  }

  // BeamLimitingDeviceAngle
  vtkTypeFloat64 beamLimitingDeviceAngle = 0.0;
  if (rtImage.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle).good())
  {
    this->External->SetBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
  }

  // PatientSupportAngle
  vtkTypeFloat64 patientSupportAngle = 0.0;
  if (rtImage.getPatientSupportAngle(patientSupportAngle).good())
  {
    this->External->SetPatientSupportAngle(patientSupportAngle);
  }

  // RadiationMachineSAD
  vtkTypeFloat64 radiationMachineSAD = 0.0;
  if (rtImage.getRadiationMachineSAD(radiationMachineSAD).good())
  {
    this->External->SetRadiationMachineSAD(radiationMachineSAD);
  }

  // RadiationMachineSSD
  vtkTypeFloat64 radiationMachineSSD = 0.0;
  if (rtImage.getRadiationMachineSSD(radiationMachineSSD).good())
  {
    //this->External->SetRadiationMachineSSD(radiationMachineSSD);
  }

  // RTImageSID
  vtkTypeFloat64 rtImageSID = 0.0;
  if (rtImage.getRTImageSID(rtImageSID).good())
  {
    this->External->SetRTImageSID(rtImageSID);
  }

  // SourceToReferenceObjectDistance
  vtkTypeFloat64 sourceToReferenceObjectDistance = 0.0;
  if (rtImage.getSourceToReferenceObjectDistance(sourceToReferenceObjectDistance).good())
  {
    //this->External->SetSourceToReferenceObjectDistance(sourceToReferenceObjectDistance);
  }

  // WindowCenter
  vtkTypeFloat64 windowCenter = 0.0;
  if (rtImage.getWindowCenter(windowCenter).good())
  {
    this->External->SetWindowCenter(windowCenter);
  }

  // WindowWidth
  vtkTypeFloat64 windowWidth = 0.0;
  if (rtImage.getWindowWidth(windowWidth).good())
  {
    this->External->SetWindowWidth(windowWidth);
  }

  // Get SOP instance UID
  OFString sopInstanceUid("");
  if (rtImage.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get SOP instance UID for RT image");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->External->GetAndStoreRtHierarchyInformation(&rtImage);

  this->External->LoadRTImageSuccessful = true;
}


//----------------------------------------------------------------------------
// vtkSlicerDicomRtReader methods

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkSlicerDicomRtReader()
{
  this->Internal = new vtkInternal(this);

  this->RTStructureSetReferencedSOPInstanceUIDs = nullptr;

  this->SetPixelSpacing(0.0,0.0);
  this->DoseUnits = nullptr;
  this->DoseGridScaling = nullptr;
  this->RTDoseReferencedRTPlanSOPInstanceUID = nullptr;

  this->RTPlanReferencedStructureSetSOPInstanceUID = nullptr;
  this->RTPlanReferencedDoseSOPInstanceUIDs = nullptr;

  this->ImageType = nullptr;
  this->RTImageLabel = nullptr;
  this->RTImageReferencedRTPlanSOPInstanceUID = nullptr;
  this->ReferencedBeamNumber = -1;
  this->SetRTImagePosition(0.0,0.0);
  this->RTImageSID = 0.0;
  this->WindowCenter = 0.0;
  this->WindowWidth = 0.0;

  this->LoadRTStructureSetSuccessful = false;
  this->LoadRTDoseSuccessful = false;
  this->LoadRTPlanSuccessful = false;
  this->LoadRTIonPlanSuccessful = false;
  this->LoadRTImageSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::~vtkSlicerDicomRtReader()
{
  if (this->Internal)
  {
    delete this->Internal;
    this->Internal = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::Update()
{
  if ((this->FileName != nullptr) && (strlen(this->FileName) > 0))
  {
    // Set DICOM database file name
    //TODO: Get rid of Qt code
    QSettings settings;
    QString databaseDirectory = settings.value("DatabaseDirectory").toString();
    QString databaseFile = databaseDirectory + DICOMREADER_DICOM_DATABASE_FILENAME.c_str();
    this->SetDatabaseFile(databaseFile.toUtf8().constData());

    // Load DICOM file or dataset
    DcmFileFormat fileformat;

    OFCondition result = EC_TagNotFound;
    result = fileformat.loadFile(this->FileName, EXS_Unknown);
    if (result.good())
    {
      DcmDataset *dataset = fileformat.getDataset();

      // Check SOP Class UID for one of the supported RT objects
      //   TODO: One series can contain composite information, e.g, an RTPLAN series can contain structure sets and plans as well
      OFString sopClass("");
      if (dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() && !sopClass.empty())
      {
        if (sopClass == UID_RTDoseStorage)
        {
          this->Internal->LoadRTDose(dataset);
        }
        else if (sopClass == UID_RTImageStorage)
        {
          this->Internal->LoadRTImage(dataset);
        }
        else if (sopClass == UID_RTPlanStorage)
        {
          this->Internal->LoadRTPlan(dataset);  
        }
        else if (sopClass == UID_RTStructureSetStorage)
        {
          this->Internal->LoadRTStructureSet(dataset);
        }
        else if (sopClass == UID_RTTreatmentSummaryRecordStorage)
        {
          //result = dumpRTTreatmentSummaryRecord(out, *dataset);
        }
        else if (sopClass == UID_RTIonPlanStorage)
        {
          this->Internal->LoadRTIonPlan(dataset);
        }
        else if (sopClass == UID_RTIonBeamsTreatmentRecordStorage)
        {
          //result = dumpRTIonBeamsTreatmentRecord(out, *dataset);
        }
        else
        {
          //OFLOG_ERROR(drtdumpLogger, "unsupported SOPClassUID (" << sopClass << ") in file: " << ifname);
        }
      } 
      else 
      {
        //OFLOG_ERROR(drtdumpLogger, "SOPClassUID (0008,0016) missing or empty in file: " << ifname);
      }
    } 
    else 
    {
      //OFLOG_FATAL(drtdumpLogger, OFFIS_CONSOLE_APPLICATION << ": error (" << result.text() << ") reading file: " << ifname);
    }
  } 
  else 
  {
    //OFLOG_FATAL(drtdumpLogger, OFFIS_CONSOLE_APPLICATION << ": invalid filename: <empty string>");
  }
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfRois()
{
  return this->Internal->RoiSequenceVector.size();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiName(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiName: Cannot get ROI with internal index: " << internalIndex);
    return nullptr;
  }
  return (this->Internal->RoiSequenceVector[internalIndex].Name.empty() ? vtkSlicerRtCommon::DICOMRTIMPORT_NO_NAME : this->Internal->RoiSequenceVector[internalIndex].Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetRoiDisplayColor(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiDisplayColor: Cannot get ROI with internal index: " << internalIndex);
    return nullptr;
  }
  return this->Internal->RoiSequenceVector[internalIndex].DisplayColor.data();
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetRoiPolyData(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiPolyData: Cannot get ROI with internal index: " << internalIndex);
    return nullptr;
  }
  return this->Internal->RoiSequenceVector[internalIndex].PolyData;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiReferencedSeriesUid(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiReferencedSeriesUid: Cannot get ROI with internal index: " << internalIndex);
    return nullptr;
  }
  return this->Internal->RoiSequenceVector[internalIndex].ReferencedSeriesUID.c_str();
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetRoiNumber(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiNumber: Cannot get ROI with internal index: " << internalIndex);
    return -1;
  }
  return this->Internal->RoiSequenceVector[internalIndex].Number;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->Internal->BeamSequenceVector.size();
}

//----------------------------------------------------------------------------
unsigned int vtkSlicerDicomRtReader::GetBeamNumberOfControlPoints(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam)
  {
    return (beam->ControlPointSequenceVector.size() > 1) ? beam->ControlPointSequenceVector.size() : 0;
  }
  return 0;
}

//----------------------------------------------------------------------------
unsigned int vtkSlicerDicomRtReader::GetBeamNumberForIndex(unsigned int index)
{
  return this->Internal->BeamSequenceVector[index].Number;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamName(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (!beam)
  {
    return nullptr;
  }
  return (beam->Name.empty() ? vtkSlicerRtCommon::DICOMRTIMPORT_NO_NAME : beam->Name).c_str();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamType(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (!beam)
  {
    return nullptr;
  }
  return (beam->Type.empty() ? nullptr : beam->Type.c_str());
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamTreatmentDeliveryType(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (!beam)
  {
    return nullptr;
  }
  return beam->TreatmentDeliveryType.c_str();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamRadiationType(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (!beam)
  {
    return nullptr;
  }
  return beam->RadiationType.c_str();
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamControlPointNominalBeamEnergy( unsigned int beamNumber, 
  unsigned int controlPointIndex)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
    return controlPoint.NominalBeamEnergy;
  }
  return 0.;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamControlPointIsocenterPositionRas( unsigned int beamNumber,
  unsigned int controlPointIndex)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
    return controlPoint.IsocenterPositionRas.data();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceAxisDistance(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (!beam)
  {
    vtkErrorMacro("GetBeamSourceAxisDistance: Unable to find beam of number" << beamNumber);
    return 0.0;
  }
  return beam->SourceAxisDistance[0];
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamVirtualSourceAxisDistance(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (!beam)
  {
    vtkErrorMacro("GetBeamVirtualSourceAxisDistance: Unable to find beam of number" << beamNumber);
    return nullptr;
  }
  return beam->SourceAxisDistance.data();
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamControlPointGantryAngle( unsigned int beamNumber, 
  unsigned int controlPointIndex)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
    return controlPoint.GantryAngle;
  }
  else if (!beam)
  {
    vtkErrorMacro("GetControlPointGantryAngle: " \
      "Unable to find beam of number" << beamNumber);
  }
  else
  {
    vtkErrorMacro("GetControlPointGantryAngle: " \
     "No control point sequence data for current beam: " << beam->Name);
  }
  return 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamControlPointPatientSupportAngle( unsigned int beamNumber, 
  unsigned int controlPointIndex)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
    return controlPoint.PatientSupportAngle;
  }
  else if (!beam)
  {
    vtkErrorMacro("GetControlPointPatientSupportAngle: " \
      "Unable to find beam of number" << beamNumber);
  }
  else
  {
    vtkErrorMacro("GetControlPointPatientSupportAngle: " \
     "No control point sequence data for current beam: " << beam->Name);
  }
  return 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamControlPointBeamLimitingDeviceAngle( unsigned int beamNumber, 
  unsigned int controlPointIndex)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
    return controlPoint.BeamLimitingDeviceAngle;
  }
  else if (!beam)
  {
    vtkErrorMacro("GetControlPointBeamLimitingDeviceAngle: " \
      "Unable to find beam of number" << beamNumber);
  }
  else
  {
    vtkErrorMacro("GetControlPointBeamLimitingDeviceAngle: " \
     "No control point sequence data for current beam: " << beam->Name);
  }
  return 0.0;
}

//----------------------------------------------------------------------------
bool vtkSlicerDicomRtReader::GetBeamControlPointJawPositions( unsigned int beamNumber, 
  unsigned int controlPointIndex, double jawPositions[2][2])
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
    jawPositions[0][0] = controlPoint.JawPositions[0];
    jawPositions[0][1] = controlPoint.JawPositions[1];
    jawPositions[1][0] = controlPoint.JawPositions[2];
    jawPositions[1][1] = controlPoint.JawPositions[3];
    return true;
  }
  else if (!beam)
  {
    vtkErrorMacro("GetControlPointJawPositions: " \
      "Unable to find beam of number" << beamNumber);
  }
  else
  {
    vtkErrorMacro("GetControlPointJawPositions: " \
     "No control point sequence data for current beam: " << beam->Name);
  }
  return false;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamControlPointMultiLeafCollimatorPositions( unsigned int beamNumber, 
  unsigned int controlPointIndex, std::vector<double>& pairBoundaries, 
  std::vector<double>& leafPositions)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam)
  {
    const unsigned int& pairs = beam->MultiLeafCollimator.NumberOfLeafJawPairs;
    const std::string& mlcType = beam->MultiLeafCollimatorType;
    if (mlcType.empty())
    {
      vtkDebugMacro("GetControlPointMultiLeafCollimatorPositions: MLC type undefined");
      return nullptr;
    }
    if (beam->ControlPointSequenceVector.size() && (controlPointIndex < beam->ControlPointSequenceVector.size()))
    {
      vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);
      const std::string& mlcPositionsType = controlPoint.MultiLeafCollimatorType;
      size_t positions = controlPoint.LeafPositions.size();
      size_t boundaries = beam->MultiLeafCollimator.LeafPositionBoundary.size();

      if ((mlcPositionsType == mlcType) && pairs && positions && boundaries &&
        ((pairs * 2) == positions) && ((pairs + 1) == boundaries))
      {
        pairBoundaries = beam->MultiLeafCollimator.LeafPositionBoundary;
        leafPositions = controlPoint.LeafPositions;
        return mlcType.c_str();
      }
      else
      {
        vtkErrorMacro("GetControlPointMultiLeafCollimatorPositions: " \
         "Different kinds of MLC between control point data and beam limiting device type, " \
         "or different number of leaf pairs and positions");
      }
    }
    else
    {
      vtkErrorMacro("GetControlPointMultiLeafCollimatorPositions: " \
       "No control point sequence data for current beam: " << beam->Name);
    }
  }
  else
  {
    vtkErrorMacro("GetControlPointMultiLeafCollimatorPositions: " \
      "Unable to find beam of number" << beamNumber);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkSlicerDicomRtReader::GetBeamControlPointScanSpotParameters( unsigned int beamNumber, 
  unsigned int controlPointIndex, std::vector<float>& positionMap, 
  std::vector<float>& metersetWeights)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);

    const std::string& scanMode = beam->ScanMode;
    if (!scanMode.compare( 0, strlen("MODULATED") - 1, "MODULATED"))
    {
      vtkWarningMacro("GetControlPointScanSpotParameters: ScanMode of the beam isn't MODULATED");
      return false;
    }
    const unsigned int& positions = controlPoint.NumberOfScanSpotPositions;
    size_t mapSize = controlPoint.ScanSpotPositionMap.size();
    size_t weightSize = controlPoint.ScanSpotMetersetWeights.size();

    if (positions && mapSize && weightSize &&
      (positions * 2 == mapSize) && (positions == weightSize))
    {
      positionMap = controlPoint.ScanSpotPositionMap;
      metersetWeights = controlPoint.ScanSpotMetersetWeights;
      return true;
    }
    else
    {
      vtkWarningMacro("GetControlPointScanSpotParameters: " \
       "Different number of scan spot positions in map or weights");
    }
  }
  else if (!beam)
  {
    vtkErrorMacro("GetControlPointScanSpotParameters: " \
      "Unable to find beam of number" << beamNumber);
  }
  else
  {
    vtkErrorMacro("GetControlPointScanSpotParameters: " \
     "No control point sequence data for current beam: " << beam->Name);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSlicerDicomRtReader::GetBeamControlPointScanningSpotSize( unsigned int beamNumber, 
  unsigned int controlPointIndex, std::array< float, 2 >& ScanSpotSize)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam && (controlPointIndex < beam->ControlPointSequenceVector.size()))
  {
    vtkInternal::ControlPointEntry& controlPoint = beam->ControlPointSequenceVector.at(controlPointIndex);

    const std::string& scanMode = beam->ScanMode;
    if (scanMode == "MODULATED" || scanMode == "MODULATED_SPEC")
    {
      ScanSpotSize = controlPoint.ScanningSpotSize;
      return true;
    }
    else
    {
      vtkWarningMacro("GetControlPointScanSpotParameters: ScanMode of the beam isn't MODULATED");
      return false;
    }
  }
  else if (!beam)
  {
    vtkErrorMacro("GetControlPointScanSpotParameters: " \
      "Unable to find beam of number" << beamNumber);
  }
  else
  {
    vtkErrorMacro("GetControlPointScanSpotParameters: " \
     "No control point sequence data for current beam: " << beam->Name);
  }
  return false;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceToMultiLeafCollimatorDistance(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam)
  {
     return beam->MultiLeafCollimator.SourceIsoDistance;
  }
  else
  {
    vtkErrorMacro("GetBeamSourceToMultiLeafCollimatorDistance: " \
      "Unable to find beam of number" << beamNumber);
  }
  return 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamIsocenterToMultiLeafCollimatorDistance(unsigned int beamNumber)
{
  return this->LoadRTIonPlanSuccessful ? this->GetBeamSourceToMultiLeafCollimatorDistance(beamNumber) : 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceToJawsDistanceX(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam)
  {
     return beam->SourceIsoToJawsDistance[0];
  }
  else
  {
    vtkErrorMacro("GetBeamSourceToJawsXDistance: " \
      "Unable to find beam of number" << beamNumber);
  }
  return 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamIsocenterToJawsDistanceX(unsigned int beamNumber)
{
  return this->LoadRTIonPlanSuccessful ? this->GetBeamSourceToJawsDistanceX(beamNumber) : 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceToJawsDistanceY(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam = this->Internal->FindBeamByNumber(beamNumber);
  if (beam)
  {
     return beam->SourceIsoToJawsDistance[1];
  }
  else
  {
    vtkErrorMacro("GetBeamSourceToJawsYDistance: " \
      "Unable to find beam of number" << beamNumber);
  }
  return 0.0;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamIsocenterToJawsDistanceY(unsigned int beamNumber)
{
  return this->LoadRTIonPlanSuccessful ? this->GetBeamSourceToJawsDistanceY(beamNumber) : 0.0;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfChannels()
{
  return this->Internal->ChannelSequenceVector.size();
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetChannelNumberOfControlPoints(unsigned int channelNumber)
{
  vtkInternal::ChannelEntry* channel=this->Internal->FindChannelByNumber(channelNumber);
  if (!channel)
  {
    vtkErrorMacro("GetChannelNumberOfControlPoints: Unable to find channel of number" << channelNumber);
    return 0;
  }
  return channel->NumberOfControlPoints;
}

//----------------------------------------------------------------------------
bool vtkSlicerDicomRtReader::GetChannelControlPoint(
  unsigned int channelNumber, unsigned int controlPointNumber, double controlPointPosition[3] )
{
  vtkInternal::ChannelEntry* channel=this->Internal->FindChannelByNumber(channelNumber);
  if (!channel)
  {
    vtkErrorMacro("GetChannelControlPoint: Unable to find channel of number" << channelNumber);
    return false;
  }

  if (controlPointNumber >= channel->ControlPointVector.size())
  {
    vtkErrorMacro("GetChannelControlPoint: Invalid control point index (" << controlPointNumber << ") for channel of number" << channelNumber);
    return false;
  }

  controlPointPosition[0] = channel->ControlPointVector[controlPointNumber][0];
  controlPointPosition[1] = channel->ControlPointVector[controlPointNumber][1];
  controlPointPosition[2] = channel->ControlPointVector[controlPointNumber][2];
  return true;
}
