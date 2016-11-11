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

// ModuleTemplate includes
#include "vtkSlicerDicomRtReader.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkCellArray.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// STD includes
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
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QVariant>
#include <QStringList>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

// CTK includes
#include <ctkDICOMDatabase.h>

//----------------------------------------------------------------------------
const std::string vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_DATABASE_FILENAME = "/ctkDICOM.sql";
const std::string vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_CONNECTION_NAME = "SlicerRt";

vtkStandardNewMacro(vtkSlicerDicomRtReader);

//----------------------------------------------------------------------------
class vtkSlicerDicomRtReader::vtkInternal
{
public:
  vtkInternal(vtkSlicerDicomRtReader* external);
  ~vtkInternal();

public:
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
    double DisplayColor[3];
    vtkPolyData* PolyData;
    std::string ReferencedSeriesUID;
    std::string ReferencedFrameOfReferenceUID;
    std::map<int,std::string> ContourIndexToSOPInstanceUIDMap;
  };

  /// List of loaded contour ROIs from structure set
  std::vector<RoiEntry> RoiSequenceVector;

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

  /// List of loaded contour ROIs from structure set
  std::vector<BeamEntry> BeamSequenceVector;

public:
  /// Load RT Dose
  void LoadRTDose(DcmDataset* dataset);

  /// Load RT Plan 
  void LoadRTPlan(DcmDataset* dataset);

  /// Load RT Structure Set
  void LoadRTStructureSet(DcmDataset* dataset);
  /// Load contours from a structure sequence
  void LoadContoursFromRoiSequence(DRTStructureSetROISequence* roiSequence);
  /// Load individual contour from RT Structure Set
  vtkSlicerDicomRtReader::vtkInternal::RoiEntry* LoadContour(DRTROIContourSequence::Item &roiObject, DRTStructureSetIOD* rtStructureSetObject);

  /// Load RT Image
  void LoadRTImage(DcmDataset* dataset);

public:
  /// Find and return a beam entry according to its beam number
  BeamEntry* FindBeamByNumber(unsigned int beamNumber);

  /// Find and return a ROI entry according to its ROI number
  RoiEntry* FindRoiByNumber(unsigned int roiNumber);

  /// Get frame of reference for an SOP instance
  DRTRTReferencedSeriesSequence* GetReferencedSeriesSequence(DRTStructureSetIOD* rtStructureSetObject);

  /// Get referenced series instance UID for the structure set (0020,000E)
  OFString GetReferencedSeriesInstanceUID(DRTStructureSetIOD* rtStructureSetObject);

  /// Get contour image sequence object in the referenced frame of reference sequence for a structure set
  DRTContourImageSequence* GetReferencedFrameOfReferenceContourImageSequence(DRTStructureSetIOD* rtStructureSetObject);

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
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::~vtkInternal()
{
  this->RoiSequenceVector.clear();
  this->BeamSequenceVector.clear();
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry::RoiEntry()
{
  this->Number = 0;
  this->DisplayColor[0] = 1.0;
  this->DisplayColor[1] = 0.0;
  this->DisplayColor[2] = 0.0;
  this->PolyData = NULL;
}

vtkSlicerDicomRtReader::vtkInternal::RoiEntry::~RoiEntry()
{
  this->SetPolyData(NULL);
}

vtkSlicerDicomRtReader::vtkInternal::RoiEntry::RoiEntry(const RoiEntry& src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor[0] = src.DisplayColor[0];
  this->DisplayColor[1] = src.DisplayColor[1];
  this->DisplayColor[2] = src.DisplayColor[2];
  this->PolyData = NULL;
  this->SetPolyData(src.PolyData);
  this->ReferencedSeriesUID = src.ReferencedSeriesUID;
  this->ReferencedFrameOfReferenceUID = src.ReferencedFrameOfReferenceUID;
  this->ContourIndexToSOPInstanceUIDMap = src.ContourIndexToSOPInstanceUIDMap;
}

vtkSlicerDicomRtReader::vtkInternal::RoiEntry& vtkSlicerDicomRtReader::vtkInternal::RoiEntry::operator=(const RoiEntry &src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor[0] = src.DisplayColor[0];
  this->DisplayColor[1] = src.DisplayColor[1];
  this->DisplayColor[2] = src.DisplayColor[2];
  this->SetPolyData(src.PolyData);
  this->ReferencedSeriesUID = src.ReferencedSeriesUID;
  this->ReferencedFrameOfReferenceUID = src.ReferencedFrameOfReferenceUID;
  this->ContourIndexToSOPInstanceUIDMap = src.ContourIndexToSOPInstanceUIDMap;

  return (*this);
}

void vtkSlicerDicomRtReader::vtkInternal::RoiEntry::SetPolyData(vtkPolyData* roiPolyData)
{
  if (roiPolyData == this->PolyData)
  {
    // not changed
    return;
  }
  if (this->PolyData != NULL)
  {
    this->PolyData->UnRegister(NULL);
  }

  this->PolyData = roiPolyData;

  if (this->PolyData != NULL)
  {
    this->PolyData->Register(NULL);
  }
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
  return NULL;
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
  vtkErrorWithObjectMacro(this->External, "FindBeamByNumber: ROI cannot be found for number " << roiNumber);
  return NULL;
}

//----------------------------------------------------------------------------
DRTRTReferencedSeriesSequence* vtkSlicerDicomRtReader::vtkInternal::GetReferencedSeriesSequence(DRTStructureSetIOD* rtStructureSetObject)
{
  DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject->getReferencedFrameOfReferenceSequence();
  if (!rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: No referenced frame of reference sequence object item is available");
    return NULL;
  }

  DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
  if (!currentReferencedFrameOfReferenceSequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: Frame of reference sequence object item is invalid");
    return NULL;
  }

  DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
  if (!rtReferencedStudySequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: No referenced study sequence object item is available");
    return NULL;
  }

  DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
  if (!rtReferencedStudySequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: Referenced study sequence object item is invalid");
    return NULL;
  }

  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
  if (!rtReferencedSeriesSequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesSequence: No referenced series sequence object item is available");
    return NULL;
  }

  return &rtReferencedSeriesSequenceObject;
}

//----------------------------------------------------------------------------
OFString vtkSlicerDicomRtReader::vtkInternal::GetReferencedSeriesInstanceUID(DRTStructureSetIOD* rtStructureSetObject)
{
  OFString invalidUid("");
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequenceObject = this->GetReferencedSeriesSequence(rtStructureSetObject);
  if (!rtReferencedSeriesSequenceObject || !rtReferencedSeriesSequenceObject->gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedSeriesInstanceUID: No referenced series sequence object item is available");
    return invalidUid;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject->getCurrentItem();
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
DRTContourImageSequence* vtkSlicerDicomRtReader::vtkInternal::GetReferencedFrameOfReferenceContourImageSequence(DRTStructureSetIOD* rtStructureSetObject)
{
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequenceObject = this->GetReferencedSeriesSequence(rtStructureSetObject);
  if (!rtReferencedSeriesSequenceObject || !rtReferencedSeriesSequenceObject->gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedFrameOfReferenceContourImageSequence: No referenced series sequence object item is available");
    return NULL;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedFrameOfReferenceContourImageSequence: Referenced series sequence object item is invalid");
    return NULL;
  }

  DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
  if (!rtContourImageSequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "GetReferencedFrameOfReferenceContourImageSequence: No contour image sequence object item is available");
    return NULL;
  }

  return &rtContourImageSequenceObject;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTDose(DcmDataset* dataset)
{
  this->External->LoadRTDoseSuccessful = false;

  DRTDoseIOD rtDoseObject;
  if (rtDoseObject.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to read RT Dose dataset!");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTDose: Load RT Dose object");

  OFString doseGridScaling("");
  if (rtDoseObject.getDoseGridScaling(doseGridScaling).bad())
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
  if (rtDoseObject.getDoseUnits(doseUnits).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to get Dose Units for dose object");
    return; // mandatory DICOM value
  }
  this->External->SetDoseUnits(doseUnits.c_str());

  OFVector<vtkTypeFloat64> pixelSpacingOFVector;
  if (rtDoseObject.getPixelSpacing(pixelSpacingOFVector).bad() || pixelSpacingOFVector.size() < 2)
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
  DRTReferencedRTPlanSequence &referencedRTPlanSequence = rtDoseObject.getReferencedRTPlanSequence();
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

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtDoseObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTDose: Failed to get SOP instance UID for RT dose!");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->External->GetAndStoreHierarchyInformation(&rtDoseObject);

  this->External->LoadRTDoseSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTPlan(DcmDataset* dataset)
{
  this->External->LoadRTPlanSuccessful = false; 

  DRTPlanIOD rtPlanObject;
  if (rtPlanObject.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Failed to read RT Plan object!");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTPlan: Load RT Plan object");

  DRTBeamSequence &rtPlaneBeamSequenceObject = rtPlanObject.getBeamSequence();
  if (rtPlaneBeamSequenceObject.gotoFirstItem().good())
  {
    do
    {
      DRTBeamSequence::Item &currentBeamSequenceObject = rtPlaneBeamSequenceObject.getCurrentItem();  
      if (!currentBeamSequenceObject.isValid())
      {
        vtkDebugWithObjectMacro(this->External, "LoadRTPlan: Found an invalid beam sequence in dataset");
        continue;
      }

      // Read item into the BeamSequenceVector
      BeamEntry beamEntry;

      OFString beamName("");
      currentBeamSequenceObject.getBeamName(beamName);
      beamEntry.Name=beamName.c_str();

      OFString beamDescription("");
      currentBeamSequenceObject.getBeamDescription(beamDescription);
      beamEntry.Description=beamDescription.c_str();

      OFString beamType("");
      currentBeamSequenceObject.getBeamType(beamType);
      beamEntry.Type=beamType.c_str();

      Sint32 beamNumber = -1;
      currentBeamSequenceObject.getBeamNumber( beamNumber );        
      beamEntry.Number = beamNumber;

      vtkTypeFloat64 sourceAxisDistance = 0.0;
      currentBeamSequenceObject.getSourceAxisDistance(sourceAxisDistance);
      beamEntry.SourceAxisDistance = sourceAxisDistance;

      DRTControlPointSequence &rtControlPointSequenceObject = currentBeamSequenceObject.getControlPointSequence();
      if (rtControlPointSequenceObject.gotoFirstItem().good())
      {
        // do // TODO: comment out for now since only first control point is loaded (as isocenter)
        {
          DRTControlPointSequence::Item &controlPointItem = rtControlPointSequenceObject.getCurrentItem();
          if (controlPointItem.isValid())
          {
            OFVector<vtkTypeFloat64> isocenterPositionDataLps;
            controlPointItem.getIsocenterPosition(isocenterPositionDataLps);

            // Convert from DICOM LPS -> Slicer RAS
            beamEntry.IsocenterPositionRas[0] = -isocenterPositionDataLps[0];
            beamEntry.IsocenterPositionRas[1] = -isocenterPositionDataLps[1];
            beamEntry.IsocenterPositionRas[2] = isocenterPositionDataLps[2];

            vtkTypeFloat64 gantryAngle = 0.0;
            controlPointItem.getGantryAngle(gantryAngle);
            beamEntry.GantryAngle = gantryAngle;

            vtkTypeFloat64 patientSupportAngle = 0.0;
            controlPointItem.getPatientSupportAngle(patientSupportAngle);
            beamEntry.PatientSupportAngle = patientSupportAngle;

            vtkTypeFloat64 beamLimitingDeviceAngle = 0.0;
            controlPointItem.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
            beamEntry.BeamLimitingDeviceAngle = beamLimitingDeviceAngle;

            DRTBeamLimitingDevicePositionSequence &currentCollimatorPositionSequenceObject =
              controlPointItem.getBeamLimitingDevicePositionSequence();
            if (currentCollimatorPositionSequenceObject.gotoFirstItem().good())
            {
              do 
              {
                DRTBeamLimitingDevicePositionSequence::Item &collimatorPositionItem =
                  currentCollimatorPositionSequenceObject.getCurrentItem();
                if (collimatorPositionItem.isValid())
                {
                  OFString rtBeamLimitingDeviceType("");
                  collimatorPositionItem.getRTBeamLimitingDeviceType(rtBeamLimitingDeviceType);

                  OFVector<vtkTypeFloat64> leafJawPositions;
                  OFCondition getJawPositionsCondition = collimatorPositionItem.getLeafJawPositions(leafJawPositions);

                  if ( !rtBeamLimitingDeviceType.compare("ASYMX") || !rtBeamLimitingDeviceType.compare("X") )
                  {
                    if (getJawPositionsCondition.good())
                    {
                      beamEntry.LeafJawPositions[0][0] = leafJawPositions[0];
                      beamEntry.LeafJawPositions[0][1] = leafJawPositions[1];
                    }
                    else
                    {
                      vtkDebugWithObjectMacro(this->External, "LoadRTPlan: No jaw position found in collimator entry");
                    }
                  }
                  else if ( !rtBeamLimitingDeviceType.compare("ASYMY") || !rtBeamLimitingDeviceType.compare("Y") )
                  {
                    if (getJawPositionsCondition.good())
                    {
                      beamEntry.LeafJawPositions[1][0] = leafJawPositions[0];
                      beamEntry.LeafJawPositions[1][1] = leafJawPositions[1];
                    }
                    else
                    {
                      vtkDebugWithObjectMacro(this->External, "LoadRTPlan: No jaw position found in collimator entry");
                    }
                  }
                  else if ( !rtBeamLimitingDeviceType.compare("MLCX") || !rtBeamLimitingDeviceType.compare("MLCY") )
                  {
                    vtkWarningWithObjectMacro(this->External, "LoadRTPlan: Multi-leaf collimator entry found. This collimator type is not yet supported!");
                  }
                  else
                  {
                    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Unsupported collimator type: " << rtBeamLimitingDeviceType);
                  }
                }
              }
              while (currentCollimatorPositionSequenceObject.gotoNextItem().good());
            }
          } // endif controlPointItem.isValid()
        }
        // while (rtControlPointSequenceObject.gotoNextItem().good());
      }

      this->BeamSequenceVector.push_back(beamEntry);
    }
    while (rtPlaneBeamSequenceObject.gotoNextItem().good());
  }
  else
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: No beams found in RT plan!");
    return;
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtPlanObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTPlan: Failed to get SOP instance UID for RT plan!");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Referenced structure set UID
  DRTReferencedStructureSetSequence &referencedStructureSetSequence = rtPlanObject.getReferencedStructureSetSequence();
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
  DRTReferencedDoseSequence &referencedDoseSequence = rtPlanObject.getReferencedDoseSequence();
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
  this->External->SetRTPlanReferencedDoseSOPInstanceUIDs(serializedDoseUidList.size() > 0 ? serializedDoseUidList.c_str() : NULL);

  // Get and store patient, study and series information
  this->External->GetAndStoreHierarchyInformation(&rtPlanObject);

  this->External->LoadRTPlanSuccessful = true;
}


//----------------------------------------------------------------------------
// vtkSlicerDicomRtReader methods

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadRTStructureSet(DcmDataset* dataset)
{
  this->External->LoadRTStructureSetSuccessful = false;

  DRTStructureSetIOD* rtStructureSetObject = new DRTStructureSetIOD();
  if (rtStructureSetObject->read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTStructureSet: Could not load strucure set object from dataset");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTStructureSet: RT Structure Set object");

  // Read ROI name, description, and number into the ROI contour sequence vector (StructureSetROISequence)
  DRTStructureSetROISequence* rtStructureSetROISequenceObject = new DRTStructureSetROISequence(rtStructureSetObject->getStructureSetROISequence());
  this->LoadContoursFromRoiSequence(rtStructureSetROISequenceObject);

  // Get referenced anatomical image
  OFString referencedSeriesInstanceUID = this->GetReferencedSeriesInstanceUID(rtStructureSetObject);

  // Get ROI contour sequence
  DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject->getROIContourSequence();
  // Reset the ROI contour sequence to the start
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTStructureSet: No ROIContourSequence found!");
    return;
  }

  // Read ROIs, iterate over ROI contour sequence
  do 
  {
    DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
    RoiEntry* currentRoiEntry = this->LoadContour(currentRoiObject, rtStructureSetObject);
    if (currentRoiEntry)
    {
      // Set referenced series UID
      currentRoiEntry->ReferencedSeriesUID = (std::string)referencedSeriesInstanceUID.c_str();
    }
  }
  while (rtROIContourSequenceObject.gotoNextItem().good());

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtStructureSetObject->getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTStructureSet: Failed to get SOP instance UID for RT structure set!");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->External->GetAndStoreHierarchyInformation(rtStructureSetObject);

  this->External->LoadRTStructureSetSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::vtkInternal::LoadContoursFromRoiSequence(DRTStructureSetROISequence* rtStructureSetROISequenceObject)
{
  if (!rtStructureSetROISequenceObject->gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadContoursFromRoiSequence: No structure sets were found");
    return;
  }
  do
  {
    DRTStructureSetROISequence::Item &currentROISequenceObject = rtStructureSetROISequenceObject->getCurrentItem();
    if (!currentROISequenceObject.isValid())
    {
      continue;
    }

    RoiEntry roiEntry;

    OFString roiName("");
    currentROISequenceObject.getROIName(roiName);
    roiEntry.Name = roiName.c_str();

    OFString roiDescription("");
    currentROISequenceObject.getROIDescription(roiDescription);
    roiEntry.Description = roiDescription.c_str();                   

    OFString referencedFrameOfReferenceUid("");
    currentROISequenceObject.getReferencedFrameOfReferenceUID(referencedFrameOfReferenceUid);
    roiEntry.ReferencedFrameOfReferenceUID = referencedFrameOfReferenceUid.c_str();

    Sint32 roiNumber = -1;
    currentROISequenceObject.getROINumber(roiNumber);
    roiEntry.Number=roiNumber;

    // Save to vector          
    this->RoiSequenceVector.push_back(roiEntry);
  }
  while (rtStructureSetROISequenceObject->gotoNextItem().good());
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkInternal::RoiEntry* vtkSlicerDicomRtReader::vtkInternal::LoadContour(
  DRTROIContourSequence::Item &roiObject, DRTStructureSetIOD* rtStructureSetObject)
{
  if (!roiObject.isValid())
  {
    return NULL;
  }

  // Used for connection from one planar contour ROI to the corresponding anatomical volume slice instance
  std::map<int, std::string> contourToSliceInstanceUIDMap;
  std::set<std::string> referencedSopInstanceUids;

  // Get ROI entry created for the referenced ROI
  Sint32 referencedRoiNumber = -1;
  roiObject.getReferencedROINumber(referencedRoiNumber);
  RoiEntry* roiEntry = this->FindRoiByNumber(referencedRoiNumber);
  if (roiEntry == NULL)
  {
    vtkErrorWithObjectMacro(this->External, "LoadContour: ROI with number " << referencedRoiNumber << " is not found!");      
    return NULL;
  } 

  // Get contour sequence
  DRTContourSequence &rtContourSequenceObject = roiObject.getContourSequence();
  if (!rtContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadContour: Contour sequence for ROI named '"
      << roiEntry->Name << "' with number " << referencedRoiNumber << " is empty!");
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
    DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
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
    DRTContourImageSequence &rtContourImageSequenceObject = contourItem.getContourImageSequence();
    if (rtContourImageSequenceObject.gotoFirstItem().good())
    {
      DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
      if (rtContourImageSequenceItem.isValid())
      {
        OFString referencedSOPInstanceUID("");
        rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
        contourToSliceInstanceUIDMap[contourIndex] = referencedSOPInstanceUID.c_str();
        referencedSopInstanceUids.insert(referencedSOPInstanceUID.c_str());

        // Check if multiple SOP instance UIDs are referenced
        if (rtContourImageSequenceObject.getNumberOfItems() > 1)
        {
          vtkWarningWithObjectMacro(this->External, "LoadContour: Contour in ROI " << roiEntry->Number << ": " << roiEntry->Name << " contains multiple referenced instances. This is not yet supported!");
        }
      }
      else
      {
        vtkErrorWithObjectMacro(this->External, "LoadContour: Contour image sequence object item is invalid");
      }
    }
  }
  while (rtContourSequenceObject.gotoNextItem().good());

  // Read slice reference UIDs from referenced frame of reference sequence if it was not included in the ROIContourSequence above
  if (contourToSliceInstanceUIDMap.empty())
  {
    DRTContourImageSequence* rtContourImageSequenceObject = this->GetReferencedFrameOfReferenceContourImageSequence(rtStructureSetObject);
    if (rtContourImageSequenceObject && rtContourImageSequenceObject->gotoFirstItem().good())
    {
      int currentSliceNumber = -1; // Use negative keys to indicate that the slice instances cannot be directly mapped to the ROI planar contours
      do 
      {
        DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject->getCurrentItem();
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
      while (rtContourImageSequenceObject->gotoNextItem().good());
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadContour: No items in contour image sequence object item in referenced frame of reference sequence!");
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
    roiObject.getROIDisplayColor(roiDisplayColor,j);
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

  DRTImageIOD rtImageObject;
  if (rtImageObject.read(*dataset).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to read RT Image object!");
    return;
  }

  vtkDebugWithObjectMacro(this->External, "LoadRTImage: Load RT Image object");

  // ImageType
  OFString imageType("");
  if (rtImageObject.getImageType(imageType).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get Image Type for RT Image object");
    return; // mandatory DICOM value
  }
  this->External->SetImageType(imageType.c_str());

  // RTImageLabel
  OFString rtImagelabel("");
  if (rtImageObject.getRTImageLabel(rtImagelabel).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get RT Image Label for RT Image object");
    return; // mandatory DICOM value
  }
  this->External->SetRTImageLabel(imageType.c_str());

  // RTImagePlane (confirm it is NORMAL)
  OFString rtImagePlane("");
  if (rtImageObject.getRTImagePlane(rtImagePlane).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get RT Image Plane for RT Image object");
    return; // mandatory DICOM value
  }
  if (rtImagePlane.compare("NORMAL"))
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Only value 'NORMAL' is supported for RTImagePlane tag for RT Image objects!");
    return;
  }

  // ReferencedRTPlanSequence
  DRTReferencedRTPlanSequenceInRTImageModule &rtReferencedRtPlanSequenceObject = rtImageObject.getReferencedRTPlanSequence();
  if (rtReferencedRtPlanSequenceObject.gotoFirstItem().good())
  {
    DRTReferencedRTPlanSequenceInRTImageModule::Item &currentReferencedRtPlanSequenceObject = rtReferencedRtPlanSequenceObject.getCurrentItem();

    OFString referencedSOPClassUID("");
    currentReferencedRtPlanSequenceObject.getReferencedSOPClassUID(referencedSOPClassUID);
    if (referencedSOPClassUID.compare(UID_RTPlanStorage))
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Referenced RT Plan SOP class has to be RTPlanStorage!");
    }
    else
    {
      // Read Referenced RT Plan SOP instance UID
      OFString referencedSOPInstanceUID("");
      currentReferencedRtPlanSequenceObject.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
      this->External->SetRTImageReferencedRTPlanSOPInstanceUID(referencedSOPInstanceUID.c_str());
    }

    if (rtReferencedRtPlanSequenceObject.getNumberOfItems() > 1)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Referenced RT Plan sequence object can contain one item! It contains " << rtReferencedRtPlanSequenceObject.getNumberOfItems());
    }
  }

  // ReferencedBeamNumber
  Sint32 referencedBeamNumber = -1;
  if (rtImageObject.getReferencedBeamNumber(referencedBeamNumber).good())
  {
    this->External->ReferencedBeamNumber = (int)referencedBeamNumber;
  }
  else if (rtReferencedRtPlanSequenceObject.getNumberOfItems() == 1)
  {
    // Type 3
    vtkDebugWithObjectMacro(this->External, "LoadRTImage: Unable to get referenced beam number in referenced RT Plan for RT image!");
  }

  // XRayImageReceptorTranslation
  OFVector<vtkTypeFloat64> xRayImageReceptorTranslation;
  if (rtImageObject.getXRayImageReceptorTranslation(xRayImageReceptorTranslation).good())
  {
    if (xRayImageReceptorTranslation.size() == 3)
    {
      if ( xRayImageReceptorTranslation[0] != 0.0
        || xRayImageReceptorTranslation[1] != 0.0
        || xRayImageReceptorTranslation[2] != 0.0 )
      {
        vtkErrorWithObjectMacro(this->External, "LoadRTImage: Non-zero XRayImageReceptorTranslation vectors are not supported!");
        return;
      }
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: XRayImageReceptorTranslation tag should contain a vector of 3 elements (it has " << xRayImageReceptorTranslation.size() << "!");
    }
  }

  // XRayImageReceptorAngle
  vtkTypeFloat64 xRayImageReceptorAngle = 0.0;
  if (rtImageObject.getXRayImageReceptorAngle(xRayImageReceptorAngle).good())
  {
    if (xRayImageReceptorAngle != 0.0)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Non-zero XRayImageReceptorAngle spacingValues are not supported!");
      return;
    }
  }

  // ImagePlanePixelSpacing
  OFVector<vtkTypeFloat64> imagePlanePixelSpacing;
  if (rtImageObject.getImagePlanePixelSpacing(imagePlanePixelSpacing).good())
  {
    if (imagePlanePixelSpacing.size() == 2)
    {
      //this->SetImagePlanePixelSpacing(imagePlanePixelSpacing[0], imagePlanePixelSpacing[1]);
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: ImagePlanePixelSpacing tag should contain a vector of 2 elements (it has " << imagePlanePixelSpacing.size() << "!");
    }
  }

  // RTImagePosition
  OFVector<vtkTypeFloat64> rtImagePosition;
  if (rtImageObject.getRTImagePosition(rtImagePosition).good())
  {
    if (rtImagePosition.size() == 2)
    {
      this->External->SetRTImagePosition(rtImagePosition[0], rtImagePosition[1]);
    }
    else
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: RTImagePosition tag should contain a vector of 2 elements (it has " << rtImagePosition.size() << ")!");
    }
  }

  // RTImageOrientation
  OFVector<vtkTypeFloat64> rtImageOrientation;
  if (rtImageObject.getRTImageOrientation(rtImageOrientation).good())
  {
    if (rtImageOrientation.size() > 0)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: RTImageOrientation is specified but not supported yet!");
    }
  }

  // GantryAngle
  vtkTypeFloat64 gantryAngle = 0.0;
  if (rtImageObject.getGantryAngle(gantryAngle).good())
  {
    this->External->SetGantryAngle(gantryAngle);
  }

  // GantryPitchAngle
  vtkTypeFloat32 gantryPitchAngle = 0.0;
  if (rtImageObject.getGantryPitchAngle(gantryPitchAngle).good())
  {
    if (gantryPitchAngle != 0.0)
    {
      vtkErrorWithObjectMacro(this->External, "LoadRTImage: Non-zero GantryPitchAngle tag spacingValues are not supported yet!");
      return;
    }
  }

  // BeamLimitingDeviceAngle
  vtkTypeFloat64 beamLimitingDeviceAngle = 0.0;
  if (rtImageObject.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle).good())
  {
    this->External->SetBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
  }

  // PatientSupportAngle
  vtkTypeFloat64 patientSupportAngle = 0.0;
  if (rtImageObject.getPatientSupportAngle(patientSupportAngle).good())
  {
    this->External->SetPatientSupportAngle(patientSupportAngle);
  }

  // RadiationMachineSAD
  vtkTypeFloat64 radiationMachineSAD = 0.0;
  if (rtImageObject.getRadiationMachineSAD(radiationMachineSAD).good())
  {
    this->External->SetRadiationMachineSAD(radiationMachineSAD);
  }

  // RadiationMachineSSD
  vtkTypeFloat64 radiationMachineSSD = 0.0;
  if (rtImageObject.getRadiationMachineSSD(radiationMachineSSD).good())
  {
    //this->External->SetRadiationMachineSSD(radiationMachineSSD);
  }

  // RTImageSID
  vtkTypeFloat64 rtImageSID = 0.0;
  if (rtImageObject.getRTImageSID(rtImageSID).good())
  {
    this->External->SetRTImageSID(rtImageSID);
  }

  // SourceToReferenceObjectDistance
  vtkTypeFloat64 sourceToReferenceObjectDistance = 0.0;
  if (rtImageObject.getSourceToReferenceObjectDistance(sourceToReferenceObjectDistance).good())
  {
    //this->External->SetSourceToReferenceObjectDistance(sourceToReferenceObjectDistance);
  }

  // WindowCenter
  vtkTypeFloat64 windowCenter = 0.0;
  if (rtImageObject.getWindowCenter(windowCenter).good())
  {
    this->External->SetWindowCenter(windowCenter);
  }

  // WindowWidth
  vtkTypeFloat64 windowWidth = 0.0;
  if (rtImageObject.getWindowWidth(windowWidth).good())
  {
    this->External->SetWindowWidth(windowWidth);
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtImageObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorWithObjectMacro(this->External, "LoadRTImage: Failed to get SOP instance UID for RT image!");
    return; // mandatory DICOM value
  }
  this->External->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->External->GetAndStoreHierarchyInformation(&rtImageObject);

  this->External->LoadRTImageSuccessful = true;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkSlicerDicomRtReader()
{
  this->Internal = new vtkInternal(this);

  this->FileName = NULL;

  this->RTStructureSetReferencedSOPInstanceUIDs = NULL;

  this->SetPixelSpacing(0.0,0.0);
  this->DoseUnits = NULL;
  this->DoseGridScaling = NULL;
  this->RTDoseReferencedRTPlanSOPInstanceUID = NULL;

  this->SOPInstanceUID = NULL;

  this->RTPlanReferencedStructureSetSOPInstanceUID = NULL;
  this->RTPlanReferencedDoseSOPInstanceUIDs = NULL;

  this->ImageType = NULL;
  this->RTImageLabel = NULL;
  this->RTImageReferencedRTPlanSOPInstanceUID = NULL;
  this->ReferencedBeamNumber = -1;
  this->SetRTImagePosition(0.0,0.0);
  this->RTImageSID = 0.0;
  this->WindowCenter = 0.0;
  this->WindowWidth = 0.0;

  this->PatientName = NULL;
  this->PatientId = NULL;
  this->PatientSex = NULL;
  this->PatientBirthDate = NULL;
  this->PatientComments = NULL;
  this->StudyInstanceUid = NULL;
  this->StudyId = NULL;
  this->StudyDescription = NULL;
  this->StudyDate = NULL;
  this->StudyTime = NULL;
  this->SeriesInstanceUid = NULL;
  this->SeriesDescription = NULL;
  this->SeriesModality = NULL;
  this->SeriesNumber = NULL;

  this->DatabaseFile = NULL;

  this->LoadRTStructureSetSuccessful = false;
  this->LoadRTDoseSuccessful = false;
  this->LoadRTPlanSuccessful = false;
  this->LoadRTImageSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::~vtkSlicerDicomRtReader()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::Update()
{
  if ((this->FileName != NULL) && (strlen(this->FileName) > 0))
  {
    // Set DICOM database file name
    QSettings settings;
    QString databaseDirectory = settings.value("DatabaseDirectory").toString();
    QString databaseFile = databaseDirectory + DICOMRTREADER_DICOM_DATABASE_FILENAME.c_str();
    this->SetDatabaseFile(databaseFile.toLatin1().constData());

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
          //result = dumpRTIonPlan(out, *dataset);
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
    return NULL;
  }
  return (this->Internal->RoiSequenceVector[internalIndex].Name.empty() ? SlicerRtCommon::DICOMRTIMPORT_NO_NAME : this->Internal->RoiSequenceVector[internalIndex].Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetRoiDisplayColor(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiDisplayColor: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->Internal->RoiSequenceVector[internalIndex].DisplayColor;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetRoiPolyData(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiPolyData: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->Internal->RoiSequenceVector[internalIndex].PolyData;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiReferencedSeriesUid(unsigned int internalIndex)
{
  if (internalIndex >= this->Internal->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiReferencedSeriesUid: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->Internal->RoiSequenceVector[internalIndex].ReferencedSeriesUID.c_str();
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->Internal->BeamSequenceVector.size();
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
  if (beam==NULL)
  {
    return NULL;
  }
  return (beam->Name.empty() ? SlicerRtCommon::DICOMRTIMPORT_NO_NAME : beam->Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamIsocenterPositionRas(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->IsocenterPositionRas;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceAxisDistance(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamSourceAxisDistance: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->SourceAxisDistance;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamGantryAngle(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamGantryAngle: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->GantryAngle;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamPatientSupportAngle(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamPatientSupportAngle: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->PatientSupportAngle;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamBeamLimitingDeviceAngle(unsigned int beamNumber)
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamBeamLimitingDeviceAngle: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->BeamLimitingDeviceAngle;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::GetBeamLeafJawPositions(unsigned int beamNumber, double jawPositions[2][2])
{
  vtkInternal::BeamEntry* beam=this->Internal->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    jawPositions[0][0]=jawPositions[0][1]=jawPositions[1][0]=jawPositions[1][1]=0.0;
    return;
  }  
  jawPositions[0][0]=beam->LeafJawPositions[0][0];
  jawPositions[0][1]=beam->LeafJawPositions[0][1];
  jawPositions[1][0]=beam->LeafJawPositions[1][0];
  jawPositions[1][1]=beam->LeafJawPositions[1][1];
}
