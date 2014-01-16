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

// ModuleTemplate includes
#include "vtkSlicerDicomRtReader.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkRibbonFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>
#include <vtkMath.h>

// STD includes
#include <vector>

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
vtkSlicerDicomRtReader::RoiEntry::RoiEntry()
{
  this->Number = 0;
  this->SliceThickness = 0.0;
  this->DisplayColor[0] = 1.0;
  this->DisplayColor[1] = 0.0;
  this->DisplayColor[2] = 0.0;
  this->PolyData = NULL;
}

vtkSlicerDicomRtReader::RoiEntry::~RoiEntry()
{
  this->SetPolyData(NULL);
}

vtkSlicerDicomRtReader::RoiEntry::RoiEntry(const RoiEntry& src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor[0] = src.DisplayColor[0];
  this->DisplayColor[1] = src.DisplayColor[1];
  this->DisplayColor[2] = src.DisplayColor[2];
  this->PolyData = NULL;
  this->SetPolyData(src.PolyData);
  this->SliceThickness = src.SliceThickness;
  this->ReferencedSeriesUid = src.ReferencedSeriesUid;
  this->ReferencedFrameOfReferenceUid = src.ReferencedFrameOfReferenceUid;
  this->ContourIndexToSopInstanceUidMap = src.ContourIndexToSopInstanceUidMap;
}

vtkSlicerDicomRtReader::RoiEntry& vtkSlicerDicomRtReader::RoiEntry::operator=(const RoiEntry &src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor[0] = src.DisplayColor[0];
  this->DisplayColor[1] = src.DisplayColor[1];
  this->DisplayColor[2] = src.DisplayColor[2];
  this->SetPolyData(src.PolyData);
  this->SliceThickness = src.SliceThickness;
  this->ReferencedSeriesUid = src.ReferencedSeriesUid;
  this->ReferencedFrameOfReferenceUid = src.ReferencedFrameOfReferenceUid;
  this->ContourIndexToSopInstanceUidMap = src.ContourIndexToSopInstanceUidMap;

  return (*this);
}

void vtkSlicerDicomRtReader::RoiEntry::SetPolyData(vtkPolyData* roiPolyData)
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
//----------------------------------------------------------------------------

const std::string vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_DATABASE_FILENAME = "/ctkDICOM.sql";
const std::string vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_CONNECTION_NAME = "SlicerRt";

vtkStandardNewMacro(vtkSlicerDicomRtReader);

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkSlicerDicomRtReader()
{
  this->FileName = NULL;

  this->RoiSequenceVector.clear();

  this->SetPixelSpacing(0.0,0.0);
  this->DoseUnits = NULL;
  this->DoseGridScaling = NULL;

  this->SOPInstanceUID = NULL;

  this->ImageType = NULL;
  this->RTImageLabel = NULL;
  this->ReferencedRTPlanSOPInstanceUID = NULL;
  this->ReferencedBeamNumber = -1;
  this->SetRTImagePosition(0.0,0.0);
  this->RTImageSID = 0.0;
  this->WindowCenter = 0.0;
  this->WindowWidth = 0.0;

  this->PatientName = NULL;
  this->PatientId = NULL;
  this->PatientSex = NULL;
  this->PatientBirthDate = NULL;
  this->StudyInstanceUid = NULL;
  this->StudyDescription = NULL;
  this->StudyDate = NULL;
  this->StudyTime = NULL;
  this->SeriesInstanceUid = NULL;
  this->SeriesDescription = NULL;
  this->SeriesModality = NULL;

  this->DatabaseFile = NULL;

  this->LoadRTStructureSetSuccessful = false;
  this->LoadRTDoseSuccessful = false;
  this->LoadRTPlanSuccessful = false;
  this->LoadRTImageSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::~vtkSlicerDicomRtReader()
{
  this->RoiSequenceVector.clear();
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
          this->LoadRTDose(dataset);
        }
        else if (sopClass == UID_RTImageStorage)
        {
          this->LoadRTImage(dataset);
        }
        else if (sopClass == UID_RTPlanStorage)
        {
          this->LoadRTPlan(dataset);  
        }
        else if (sopClass == UID_RTStructureSetStorage)
        {
          this->LoadRTStructureSet(dataset);
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
void vtkSlicerDicomRtReader::LoadRTImage(DcmDataset* dataset)
{
  this->LoadRTImageSuccessful = false;

  DRTImageIOD rtImageObject;
  if (rtImageObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to read RT Image object!");
    return;
  }

  vtkDebugMacro("LoadRTImage: Load RT Image object");

  // ImageType
  OFString imageType("");
  if (rtImageObject.getImageType(imageType).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get Image Type for RT Image object");
    return; // mandatory DICOM value
  }
  this->SetImageType(imageType.c_str());

  // RTImageLabel
  OFString rtImagelabel("");
  if (rtImageObject.getRTImageLabel(rtImagelabel).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get RT Image Label for RT Image object");
    return; // mandatory DICOM value
  }
  this->SetRTImageLabel(imageType.c_str());

  // RTImagePlane (confirm it is NORMAL)
  OFString rtImagePlane("");
  if (rtImageObject.getRTImagePlane(rtImagePlane).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get RT Image Plane for RT Image object");
    return; // mandatory DICOM value
  }
  if (rtImagePlane.compare("NORMAL"))
  {
    vtkErrorMacro("LoadRTImage: Only value 'NORMAL' is supported for RTImagePlane tag for RT Image objects!");
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
      vtkErrorMacro("LoadRTImage: Referenced RT Plan SOP class has to be RTPlanStorage!");
    }
    else
    {
      // Read Referenced RT Plan SOP instance UID
      OFString referencedSOPInstanceUID("");
      currentReferencedRtPlanSequenceObject.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
      this->SetReferencedRTPlanSOPInstanceUID(referencedSOPInstanceUID.c_str());
    }

    if (rtReferencedRtPlanSequenceObject.getNumberOfItems() > 1)
    {
      vtkErrorMacro("LoadRTImage: Referenced RT Plan sequence object can contain one item! It contains " << rtReferencedRtPlanSequenceObject.getNumberOfItems());
    }
  }

  // ReferencedBeamNumber
  Sint32 referencedBeamNumber = -1;
  if (rtImageObject.getReferencedBeamNumber(referencedBeamNumber).good())
  {
    this->ReferencedBeamNumber = (int)referencedBeamNumber;
  }
  else if (rtReferencedRtPlanSequenceObject.getNumberOfItems() == 1)
  {
    // Type 3
    vtkDebugMacro("LoadRTImage: Unable to get referenced beam number in referenced RT Plan for RT image!");
  }

  // XRayImageReceptorTranslation
  OFVector<Float64> xRayImageReceptorTranslation;
  if (rtImageObject.getXRayImageReceptorTranslation(xRayImageReceptorTranslation).good())
  {
    if (xRayImageReceptorTranslation.size() == 3)
    {
      if ( xRayImageReceptorTranslation[0] != 0.0
        || xRayImageReceptorTranslation[1] != 0.0
        || xRayImageReceptorTranslation[2] != 0.0 )
      {
        vtkErrorMacro("LoadRTImage: Non-zero XRayImageReceptorTranslation vectors are not supported!");
        return;
      }
    }
    else
    {
      vtkErrorMacro("LoadRTImage: XRayImageReceptorTranslation tag should contain a vector of 3 elements (it has " << xRayImageReceptorTranslation.size() << "!");
    }
  }

  // XRayImageReceptorAngle
  Float64 xRayImageReceptorAngle = 0.0;
  if (rtImageObject.getXRayImageReceptorAngle(xRayImageReceptorAngle).good())
  {
    if (xRayImageReceptorAngle != 0.0)
    {
      vtkErrorMacro("LoadRTImage: Non-zero XRayImageReceptorAngle values are not supported!");
      return;
    }
  }

  // ImagePlanePixelSpacing
  OFVector<Float64> imagePlanePixelSpacing;
  if (rtImageObject.getImagePlanePixelSpacing(imagePlanePixelSpacing).good())
  {
    if (imagePlanePixelSpacing.size() == 2)
    {
      //this->SetImagePlanePixelSpacing(imagePlanePixelSpacing[0], imagePlanePixelSpacing[1]);
    }
    else
    {
      vtkErrorMacro("LoadRTImage: ImagePlanePixelSpacing tag should contain a vector of 2 elements (it has " << imagePlanePixelSpacing.size() << "!");
    }
  }

  // RTImagePosition
  OFVector<Float64> rtImagePosition;
  if (rtImageObject.getRTImagePosition(rtImagePosition).good())
  {
    if (rtImagePosition.size() == 2)
    {
      this->SetRTImagePosition(rtImagePosition[0], rtImagePosition[1]);
    }
    else
    {
      vtkErrorMacro("LoadRTImage: RTImagePosition tag should contain a vector of 2 elements (it has " << rtImagePosition.size() << ")!");
    }
  }

  // RTImageOrientation
  OFVector<Float64> rtImageOrientation;
  if (rtImageObject.getRTImageOrientation(rtImageOrientation).good())
  {
    if (rtImageOrientation.size() > 0)
    {
      vtkErrorMacro("LoadRTImage: RTImageOrientation is specified but not supported yet!");
    }
  }

  // GantryAngle
  Float64 gantryAngle = 0.0;
  if (rtImageObject.getGantryAngle(gantryAngle).good())
  {
    this->SetGantryAngle(gantryAngle);
  }

  // GantryPitchAngle
  Float32 gantryPitchAngle = 0.0;
  if (rtImageObject.getGantryPitchAngle(gantryPitchAngle).good())
  {
    if (gantryPitchAngle != 0.0)
    {
      vtkErrorMacro("LoadRTImage: Non-zero GantryPitchAngle tag values are not supported yet!");
      return;
    }
  }

  // BeamLimitingDeviceAngle
  Float64 beamLimitingDeviceAngle = 0.0;
  if (rtImageObject.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle).good())
  {
    this->SetBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
  }

  // PatientSupportAngle
  Float64 patientSupportAngle = 0.0;
  if (rtImageObject.getPatientSupportAngle(patientSupportAngle).good())
  {
    this->SetPatientSupportAngle(patientSupportAngle);
  }

  // RadiationMachineSAD
  Float64 radiationMachineSAD = 0.0;
  if (rtImageObject.getRadiationMachineSAD(radiationMachineSAD).good())
  {
    this->SetRadiationMachineSAD(radiationMachineSAD);
  }

  // RadiationMachineSSD
  Float64 radiationMachineSSD = 0.0;
  if (rtImageObject.getRadiationMachineSSD(radiationMachineSSD).good())
  {
    //this->SetRadiationMachineSSD(radiationMachineSSD);
  }

  // RTImageSID
  Float64 rtImageSID = 0.0;
  if (rtImageObject.getRTImageSID(rtImageSID).good())
  {
    this->SetRTImageSID(rtImageSID);
  }

  // SourceToReferenceObjectDistance
  Float64 sourceToReferenceObjectDistance = 0.0;
  if (rtImageObject.getSourceToReferenceObjectDistance(sourceToReferenceObjectDistance).good())
  {
    //this->SetSourceToReferenceObjectDistance(sourceToReferenceObjectDistance);
  }

  // WindowCenter
  Float64 windowCenter = 0.0;
  if (rtImageObject.getWindowCenter(windowCenter).good())
  {
    this->SetWindowCenter(windowCenter);
  }

  // WindowWidth
  Float64 windowWidth = 0.0;
  if (rtImageObject.getWindowWidth(windowWidth).good())
  {
    this->SetWindowWidth(windowWidth);
  }

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtImageObject);

  this->LoadRTImageSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTPlan(DcmDataset* dataset)
{
  this->LoadRTPlanSuccessful = false; 

  DRTPlanIOD rtPlanObject;
  if (rtPlanObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTPlan: Failed to read RT Plan object!");
    return;
  }

  vtkDebugMacro("LoadRTPlan: Load RT Plan object");

  DRTBeamSequence &rtPlaneBeamSequenceObject = rtPlanObject.getBeamSequence();
  if (rtPlaneBeamSequenceObject.gotoFirstItem().good())
  {
    do
    {
      DRTBeamSequence::Item &currentBeamSequenceObject = rtPlaneBeamSequenceObject.getCurrentItem();  
      if (!currentBeamSequenceObject.isValid())
      {
        vtkDebugMacro("LoadRTPlan: Found an invalid beam sequence in dataset");
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
      currentBeamSequenceObject.getBeamNumber(beamNumber);        
      beamEntry.Number = beamNumber;

      Float64 sourceAxisDistance = 0.0;
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
            OFVector<Float64> isocenterPositionDataLps;
            controlPointItem.getIsocenterPosition(isocenterPositionDataLps);

            // Convert from DICOM LPS -> Slicer RAS
            beamEntry.IsocenterPositionRas[0] = -isocenterPositionDataLps[0];
            beamEntry.IsocenterPositionRas[1] = -isocenterPositionDataLps[1];
            beamEntry.IsocenterPositionRas[2] = isocenterPositionDataLps[2];

            Float64 gantryAngle = 0.0;
            controlPointItem.getGantryAngle(gantryAngle);
            beamEntry.GantryAngle = gantryAngle;

            Float64 patientSupportAngle = 0.0;
            controlPointItem.getPatientSupportAngle(patientSupportAngle);
            beamEntry.PatientSupportAngle = patientSupportAngle;

            Float64 beamLimitingDeviceAngle = 0.0;
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

                  OFVector<Float64> leafJawPositions;
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
                      vtkDebugMacro("LoadRTPlan: No jaw position found in collimator entry");
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
                      vtkDebugMacro("LoadRTPlan: No jaw position found in collimator entry");
                    }
                  }
                  else if ( !rtBeamLimitingDeviceType.compare("MLCX") || !rtBeamLimitingDeviceType.compare("MLCY") )
                  {
                    vtkWarningMacro("LoadRTPlan: Multi-leaf collimator entry found. This collimator type is not yet supported!");
                  }
                  else
                  {
                    vtkErrorMacro("LoadRTPlan: Unsupported collimator type: " << rtBeamLimitingDeviceType);
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
    vtkErrorMacro("LoadRTPlan: No beams found in RT plan!");
    return;
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtPlanObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadRTPlan: Failed to get SOP instance UID for RT plan!");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtPlanObject);

  this->LoadRTPlanSuccessful = true;
}

//----------------------------------------------------------------------------
DRTRTReferencedSeriesSequence* vtkSlicerDicomRtReader::GetReferencedSeriesSequence(DRTStructureSetIOD &rtStructureSetObject)
{
  DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject.getReferencedFrameOfReferenceSequence();
  if (!rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: No referenced frame of reference sequence object item is available");
    return NULL;
  }

  DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
  if (!currentReferencedFrameOfReferenceSequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: Frame of reference sequence object item is invalid");
    return NULL;
  }

  DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
  if (!rtReferencedStudySequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: No referenced study sequence object item is available");
    return NULL;
  }

  DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
  if (!rtReferencedStudySequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: Referenced study sequence object item is invalid");
    return NULL;
  }

  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
  if (!rtReferencedSeriesSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: No referenced series sequence object item is available");
    return NULL;
  }

  return &rtReferencedSeriesSequenceObject;
}

//----------------------------------------------------------------------------
OFString vtkSlicerDicomRtReader::GetReferencedSeriesInstanceUID(DRTStructureSetIOD rtStructureSetObject)
{
  OFString invalidUid("");
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequenceObject = this->GetReferencedSeriesSequence(rtStructureSetObject);
  if (!rtReferencedSeriesSequenceObject || !rtReferencedSeriesSequenceObject->gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesInstanceUID: No referenced series sequence object item is available");
    return invalidUid;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedSeriesInstanceUID: Referenced series sequence object item is invalid");
    return invalidUid;
  }

  OFString referencedSeriesInstanceUID("");
  rtReferencedSeriesSequenceItem.getSeriesInstanceUID(referencedSeriesInstanceUID);
  return referencedSeriesInstanceUID;
}

//----------------------------------------------------------------------------
DRTContourImageSequence* vtkSlicerDicomRtReader::GetReferencedFrameOfReferenceContourImageSequence(DRTStructureSetIOD &rtStructureSetObject)
{
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequenceObject = this->GetReferencedSeriesSequence(rtStructureSetObject);
  if (!rtReferencedSeriesSequenceObject || !rtReferencedSeriesSequenceObject->gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedFrameOfReferenceContourImageSequence: No referenced series sequence object item is available");
    return NULL;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedFrameOfReferenceContourImageSequence: Referenced series sequence object item is invalid");
    return NULL;
  }

  DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
  if (!rtContourImageSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedFrameOfReferenceContourImageSequence: No contour image sequence object item is available");
    return NULL;
  }

  return &rtContourImageSequenceObject;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetSliceThickness(OFString sopInstanceUID)
{
  double defaultSliceThickness = 2.0;

  // Get DICOM image filename from SOP instance UID
  ctkDICOMDatabase dicomDatabase;
  dicomDatabase.openDatabase(this->DatabaseFile, DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());

  // Get filename for instance
  QString fileName = dicomDatabase.fileForInstance(sopInstanceUID.c_str());
  if ( fileName.isEmpty() )
  {
    vtkErrorMacro("GetSliceThickness: No referenced image file is found, default slice thickness (" << defaultSliceThickness << ") is used for contour import");
    dicomDatabase.closeDatabase();
    return defaultSliceThickness;
  }

  // Get slice thickness from file
  double sliceThickness = dicomDatabase.fileValue(fileName, DCM_SliceThickness.getGroup(), DCM_SliceThickness.getElement()).toDouble();
  dicomDatabase.closeDatabase();

  return sliceThickness;
}

//----------------------------------------------------------------------------
// Variables for estimating the distance between contour planes.
// This is not a reliable solution, as it assumes that the plane normals are (0,0,1) and
// the distance between all planes are equal.
double vtkSlicerDicomRtReader::GetDistanceBetweenContourPlanes(DRTContourSequence &rtContourSequenceObject)
{
  double invalidResult = -1.0;
  if (!rtContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetDistanceBetweenContourPlanes: Contour sequence object is invalid");
    return invalidResult;
  }

  double firstContourPlanePosition = 0.0;
  double secondContourPlanePosition = 0.0;
  int contourPlaneIndex = 0;
  do
  {
    DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
    if ( !contourItem.isValid())
    {
      continue;
    }

    OFString numberofpoints("");
    contourItem.getNumberOfContourPoints(numberofpoints);    

    std::stringstream ss;
    ss << numberofpoints;
    int number;
    ss >> number;
    if (number<3)
    {
      continue;
    }

    OFVector<Float64>  contourData_LPS;
    contourItem.getContourData(contourData_LPS);

    double firstContourPointZcoordinate = contourData_LPS[2];
    switch (contourPlaneIndex)
    {
    case 0:
      // First contour
      firstContourPlanePosition=firstContourPointZcoordinate;
      break;
    case 1:
      // Second contour
      secondContourPlanePosition=firstContourPointZcoordinate;
      break;
    default:
      // We ignore all the subsequent contour plane positions
      // distance is just estimated based on the first two
      break;
    }
    contourPlaneIndex++;

  }
  while (rtContourSequenceObject.gotoNextItem().good() && contourPlaneIndex<2);

  if (contourPlaneIndex < 2)
  {
    vtkErrorMacro("GetDistanceBetweenContourPlanes: Less than two contours found!");
    return invalidResult;
  }

  // There were at least contour planes, therefore we have a valid distance estimation
  double distanceBetweenContourPlanes = fabs(firstContourPlanePosition-secondContourPlanePosition);
  return distanceBetweenContourPlanes;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTStructureSet(DcmDataset* dataset)
{
  this->LoadRTStructureSetSuccessful = false;

  DRTStructureSetIOD rtStructureSetObject;
  if (rtStructureSetObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTStructureSet: Could not load strucure set object from dataset");
    return;
  }

  vtkDebugMacro("LoadRTStructureSet: RT Structure Set object");

  // Read ROI name, description, and number into the ROI contour sequence vector (StructureSetROISequence)
  DRTStructureSetROISequence &rtStructureSetROISequenceObject = rtStructureSetObject.getStructureSetROISequence();
  if (!rtStructureSetROISequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("LoadRTStructureSet: No structure sets were found");
    return;
  }
  do
  {
    DRTStructureSetROISequence::Item &currentROISequenceObject = rtStructureSetROISequenceObject.getCurrentItem();
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
    roiEntry.ReferencedFrameOfReferenceUid = referencedFrameOfReferenceUid.c_str();

    Sint32 roiNumber = -1;
    currentROISequenceObject.getROINumber(roiNumber);
    roiEntry.Number=roiNumber;

    // Save to vector          
    this->RoiSequenceVector.push_back(roiEntry);
  }
  while (rtStructureSetROISequenceObject.gotoNextItem().good());

  // Get referenced anatomical image
  OFString referencedSeriesInstanceUID = this->GetReferencedSeriesInstanceUID(rtStructureSetObject);

  // Get the slice thickness from the referenced anatomical image
  OFString firstReferencedSOPInstanceUID("");
  DRTContourImageSequence* rtContourImageSequenceObject = this->GetReferencedFrameOfReferenceContourImageSequence(rtStructureSetObject);
  if (rtContourImageSequenceObject && rtContourImageSequenceObject->gotoFirstItem().good())
  {
    DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject->getCurrentItem();
    if (rtContourImageSequenceItem.isValid())
    {
      rtContourImageSequenceItem.getReferencedSOPInstanceUID(firstReferencedSOPInstanceUID);
    }
    else
    {
      vtkErrorMacro("LoadRTStructureSet: Contour image sequence object item in referenced frame of reference sequence is invalid");
    }
  }
  else
  {
    vtkErrorMacro("LoadRTStructureSet: No items in contour image sequence object item in referenced frame of reference sequence!");
  }

  double sliceThickness = this->GetSliceThickness(firstReferencedSOPInstanceUID);

  DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject.getROIContourSequence();
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("LoadRTStructureSet: No ROIContourSequence found!");
    return;
  }

  // Used for connection from one planar contour ROI to the corresponding anatomical volume slice instance
  std::map<int, std::string> contourToSliceInstanceUidMap;

  // Read ROIs (ROIContourSequence)
  do 
  {
    DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
    if (!currentRoiObject.isValid())
    {
      continue;
    }

    // Create containers for vtkPolyData
    vtkSmartPointer<vtkPoints> tempPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> tempCellArray = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType pointId = 0;

    // Get ROI entry created for the referenced ROI
    Sint32 referencedRoiNumber = -1;
    currentRoiObject.getReferencedROINumber(referencedRoiNumber);
    RoiEntry* roiEntry = this->FindRoiByNumber(referencedRoiNumber);
    if (roiEntry == NULL)
    {
      vtkErrorMacro("LoadRTStructureSet: ROI with number " << referencedRoiNumber << " is not found!");      
      continue;
    } 

    // Get contour sequence
    DRTContourSequence &rtContourSequenceObject = currentRoiObject.getContourSequence();
    if (!rtContourSequenceObject.gotoFirstItem().good())
    {
      vtkErrorMacro("LoadRTStructureSet: Contour sequence for ROI named '"
        << roiEntry->Name << "' with number " << referencedRoiNumber << " is empty!");
      continue;
    }

    // Read contour data
    do
    {
      DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
      if (!contourItem.isValid())
      {
        continue;
      }

      OFString numberOfPointsString("");
      contourItem.getNumberOfContourPoints(numberOfPointsString);
      std::stringstream ss;
      ss << numberOfPointsString;
      int numberOfPoints;
      ss >> numberOfPoints;

      OFVector<Float64> contourData_LPS;
      contourItem.getContourData(contourData_LPS);

      unsigned int contourIndex = tempCellArray->InsertNextCell(numberOfPoints+1);
      for (int k=0; k<numberOfPoints; k++)
      {
        // Convert from DICOM LPS -> Slicer RAS
        tempPoints->InsertPoint(pointId, -contourData_LPS[3*k], -contourData_LPS[3*k+1], contourData_LPS[3*k+2]);
        tempCellArray->InsertCellPoint(pointId);
        pointId++;
      }

      // Close the contour
      tempCellArray->InsertCellPoint(pointId-numberOfPoints);

      // Add map to the referenced slice instance UID
      // This is not a mandatory field so no error logged if not found. The reason why it is still read and stored is that it references the contours individually
      DRTContourImageSequence &rtContourImageSequenceObject = contourItem.getContourImageSequence();
      if (rtContourImageSequenceObject.gotoFirstItem().good())
      {
        DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
        if (rtContourImageSequenceItem.isValid())
        {
          OFString referencedSOPInstanceUID("");
          rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
          contourToSliceInstanceUidMap[contourIndex] = referencedSOPInstanceUID.c_str();

          // Check if multiple SOP instance UIDs are referenced
          if (rtContourImageSequenceObject.getNumberOfItems() > 1)
          {
            vtkWarningMacro("LoadRTStructureSet: Contour in ROI " << roiEntry->Number << ": " << roiEntry->Name << " contains multiple referenced instances. This is not yet supported!");
          }
        }
        else
        {
          vtkErrorMacro("LoadRTStructureSet: Contour image sequence object item is invalid");
        }
      }
    }
    while (rtContourSequenceObject.gotoNextItem().good());

    // Read slice reference UIDs from referenced frame of reference sequence if it was not included in the ROIContourSequence above
    if (contourToSliceInstanceUidMap.empty())
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
            OFString currentReferencedSOPInstanceUID("");
            rtContourImageSequenceItem.getReferencedSOPInstanceUID(currentReferencedSOPInstanceUID);
            contourToSliceInstanceUidMap[currentSliceNumber] = currentReferencedSOPInstanceUID.c_str();
          }
          else
          {
            vtkErrorMacro("LoadRTStructureSet: Contour image sequence object item in referenced frame of reference sequence is invalid");
          }
          currentSliceNumber--;
        }
        while (rtContourImageSequenceObject->gotoNextItem().good());
      }
      else
      {
        vtkErrorMacro("LoadRTStructureSet: No items in contour image sequence object item in referenced frame of reference sequence!");
      }
    }

    // Save just loaded contour data into ROI entry
    vtkSmartPointer<vtkPolyData> tempPolyData = vtkSmartPointer<vtkPolyData>::New();
    tempPolyData->SetPoints(tempPoints);
    if (tempPoints->GetNumberOfPoints() == 1)
    {
      // Point ROI
      tempPolyData->SetVerts(tempCellArray);
    }
    else if (tempPoints->GetNumberOfPoints() > 1)
    {
      // Contour ROI
      tempPolyData->SetLines(tempCellArray);
    }
    roiEntry->SetPolyData(tempPolyData);

    // Get structure color
    Sint32 roiDisplayColor = -1;
    for (int j=0; j<3; j++)
    {
      currentRoiObject.getROIDisplayColor(roiDisplayColor,j);
      roiEntry->DisplayColor[j] = roiDisplayColor/255.0;
    }

    // Set referenced series UID
    roiEntry->ReferencedSeriesUid = (std::string)referencedSeriesInstanceUID.c_str();

    // Set slice thickness
    roiEntry->SliceThickness = sliceThickness;

    // Set referenced SOP instance UIDs
    roiEntry->ContourIndexToSopInstanceUidMap = contourToSliceInstanceUidMap;
  }
  while (rtROIContourSequenceObject.gotoNextItem().good());

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtStructureSetObject);

  this->LoadRTStructureSetSuccessful = true;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfRois()
{
  return this->RoiSequenceVector.size();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiName(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiName: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return (this->RoiSequenceVector[internalIndex].Name.empty() ? SlicerRtCommon::DICOMRTIMPORT_NO_NAME : this->RoiSequenceVector[internalIndex].Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetRoiDisplayColor(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiDisplayColor: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].DisplayColor;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetRoiPolyData(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiPolyData: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].PolyData;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiReferencedSeriesUid(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiName: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].ReferencedSeriesUid.c_str();
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->BeamSequenceVector.size();
}

//----------------------------------------------------------------------------
unsigned int vtkSlicerDicomRtReader::GetBeamNumberForIndex(unsigned int index)
{
  return this->BeamSequenceVector[index].Number;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamName(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }
  return (beam->Name.empty() ? SlicerRtCommon::DICOMRTIMPORT_NO_NAME : beam->Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamIsocenterPositionRas(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->IsocenterPositionRas;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceAxisDistance(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
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
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
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
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
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
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
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
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
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

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTDose(DcmDataset* dataset)
{
  this->LoadRTDoseSuccessful = false;

  DRTDoseIOD rtDoseObject;
  if (rtDoseObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to read RT Dose dataset!");
    return;
  }

  vtkDebugMacro("LoadRTDose: Load RT Dose object");

  OFString doseGridScaling("");
  if (rtDoseObject.getDoseGridScaling(doseGridScaling).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to get Dose Grid Scaling for dose object");
    return; // mandatory DICOM value
  }
  else if (doseGridScaling.empty())
  {
    vtkWarningMacro("LoadRTDose: Dose grid scaling tag is missing or empty! Using default dose grid scaling 0.0001.");
    doseGridScaling = "0.0001";
  }

  this->SetDoseGridScaling(doseGridScaling.c_str());

  OFString doseUnits("");
  if (rtDoseObject.getDoseUnits(doseUnits).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to get Dose Units for dose object");
    return; // mandatory DICOM value
  }
  this->SetDoseUnits(doseUnits.c_str());

  OFVector<Float64> pixelSpacingOFVector;
  if (rtDoseObject.getPixelSpacing(pixelSpacingOFVector).bad() || pixelSpacingOFVector.size() < 2)
  {
    vtkErrorMacro("LoadRTDose: Failed to get Pixel Spacing for dose object");
    return; // mandatory DICOM value
  }
  this->SetPixelSpacing(pixelSpacingOFVector[0], pixelSpacingOFVector[1]);
  vtkDebugMacro("Pixel Spacing: (" << pixelSpacingOFVector[0] << ", " << pixelSpacingOFVector[1] << ")");

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtDoseObject);

  this->LoadRTDoseSuccessful = true;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::BeamEntry* vtkSlicerDicomRtReader::FindBeamByNumber(unsigned int beamNumber)
{
  for (unsigned int i=0; i<this->BeamSequenceVector.size(); i++)
  {
    if (this->BeamSequenceVector[i].Number == beamNumber)
    {
      return &this->BeamSequenceVector[i];
    }
  }

  // Not found
  vtkErrorMacro("FindBeamByNumber: Beam cannot be found for number " << beamNumber);
  return NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::RoiEntry* vtkSlicerDicomRtReader::FindRoiByNumber(unsigned int roiNumber)
{
  for (unsigned int i=0; i<this->RoiSequenceVector.size(); i++)
  {
    if (this->RoiSequenceVector[i].Number == roiNumber)
    {
      return &this->RoiSequenceVector[i];
    }
  }

  // Not found
  vtkErrorMacro("FindBeamByNumber: ROI cannot be found for number " << roiNumber);
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::CreateRibbonModelForRoi(unsigned int internalIndex, vtkPolyData* ribbonModelPolyData)
{
  if (ribbonModelPolyData == NULL)
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Input ribbon model poly data is NULL!");
    return;
  }

  // Get ROI entry
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Cannot get ROI with internal index: " << internalIndex);
    return;
  }

  vtkPolyData* roiPolyData = this->RoiSequenceVector[internalIndex].PolyData;
  if (!roiPolyData || roiPolyData->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Invalid ROI with internal index: " << internalIndex);
    return;
  }
  else if (roiPolyData->GetNumberOfPoints() == 1)
  {
    vtkWarningMacro("CreateRibbonModelForRoi: Point ROI does not need to be ribbonized with internal index: " << internalIndex);
    ribbonModelPolyData->DeepCopy(roiPolyData);
    return;
  }

  // Get image orientation for the contour planes from the referenced slice orientations
  ctkDICOMDatabase dicomDatabase;
  dicomDatabase.openDatabase(this->DatabaseFile, DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());

  std::map<int,std::string>* contourIndexToSopInstanceUidMap = &(this->RoiSequenceVector[internalIndex].ContourIndexToSopInstanceUidMap);
  std::map<int,std::string>::iterator sliceInstanceUidIt;
  QString imageOrientation;
  for (sliceInstanceUidIt = contourIndexToSopInstanceUidMap->begin(); sliceInstanceUidIt != contourIndexToSopInstanceUidMap->end(); ++sliceInstanceUidIt)
  {
    // Get file name for referenced slice instance from the stored SOP instance UID
    QString fileName = dicomDatabase.fileForInstance(sliceInstanceUidIt->second.c_str());
    if (fileName.isEmpty())
    {
      vtkErrorMacro("CreateRibbonModelForRoi: No referenced image file is found for ROI contour slice number " << sliceInstanceUidIt->first);
      continue;
    }

    // Get image orientation string from the referenced slice
    QString currentImageOrientation = dicomDatabase.fileValue(fileName, DCM_ImageOrientationPatient.getGroup(), DCM_ImageOrientationPatient.getElement());

    // Check if the currently read orientation matches the orientation in the already loaded slices
    if (imageOrientation.isEmpty())
    {
      imageOrientation = currentImageOrientation;
    }
    else if (imageOrientation.compare(currentImageOrientation))
    {
      vtkWarningMacro("CreateRibbonModelForRoi: Image orientation in instance '" << fileName.toLatin1().constData() << "' differs from that in the first instance ("
        << currentImageOrientation.toLatin1().constData() << " != " << imageOrientation.toLatin1().constData() << ")! This is not supported yet, so the first orientation will be used for the whole contour.");
      break;
    }
  }

  dicomDatabase.closeDatabase();

  // Compute normal vector from the read image orientation
  QStringList imageOrientationComponentsString = imageOrientation.split("\\");
  if (imageOrientationComponentsString.size() != 6)
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Invalid image orientation for ROI: " << imageOrientation.toLatin1().constData());
    return;
  }

  double imageOrientationVectorRasX[3] = {0.0, 0.0, 0.0};
  double imageOrientationVectorRasY[3] = {0.0, 0.0, 0.0};
  double imageOrientationVectorRasZ[3] = {0.0, 0.0, 0.0};
  for (unsigned int componentIndex=0; componentIndex<3; ++componentIndex)
  {
    // Apply LPS to RAS conversion
    double lpsToRasConversionMultiplier = (componentIndex<2 ? (-1.0) : 1.0);
    imageOrientationVectorRasX[componentIndex] = lpsToRasConversionMultiplier * imageOrientationComponentsString[componentIndex].toDouble();
    imageOrientationVectorRasY[componentIndex] = lpsToRasConversionMultiplier * imageOrientationComponentsString[componentIndex+3].toDouble();
  }

  vtkMath::Cross(imageOrientationVectorRasX, imageOrientationVectorRasY, imageOrientationVectorRasZ);

  // Remove coincident points (if there are multiple contour points at the same position then the ribbon filter fails)
  vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInput(roiPolyData);

  // Convert to ribbon using vtkRibbonFilter
  vtkSmartPointer<vtkRibbonFilter> ribbonFilter = vtkSmartPointer<vtkRibbonFilter>::New();
  ribbonFilter->SetInputConnection(cleaner->GetOutputPort());
  ribbonFilter->SetDefaultNormal(imageOrientationVectorRasZ);
  ribbonFilter->UseDefaultNormalOn();
  ribbonFilter->SetWidth(this->RoiSequenceVector[internalIndex].SliceThickness / 2.0);
  ribbonFilter->SetAngle(90.0);
  ribbonFilter->Update();

  vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalFilter->SetInputConnection(ribbonFilter->GetOutputPort());
  normalFilter->ConsistencyOn();
  normalFilter->Update();

  ribbonModelPolyData->DeepCopy(normalFilter->GetOutput());
}
