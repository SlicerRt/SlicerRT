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
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkRibbonFilter.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>
#include <vtkPlane.h>

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

namespace
{
  template<class T>
  vtkVector3<T> operator -(const vtkVector3<T>& a, const vtkVector3<T>& b)
  {
    vtkVector3<T> result;
    result.SetX(a.GetX() - b.GetX());
    result.SetY(a.GetY() - b.GetY());
    result.SetZ(a.GetZ() - b.GetZ());
    return result;
  }

  template<class T>
  bool operator ==(const vtkVector3<T>& a, const vtkVector3<T>& b)
  {
    return a.X() == b.X() && a.Y() == b.Y() && a.Z() == b.Z();
  }

  template<class T>
  bool operator !=(const vtkVector3<T>& a, const vtkVector3<T>& b)
  {
    return !(a==b);
  }

  template<class T>
  vtkTypeFloat64 Vector3Magnitude(const vtkVector3<T>& aVector)
  {
    return sqrt( pow(aVector.GetX(),2) + pow(aVector.GetY(),2) + pow(aVector.GetZ(),2) );
  }

  bool AreSame(double a, double b)
  {
    return fabs(a - b) < EPSILON;
  }
}

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
  OFVector<vtkTypeFloat64> xRayImageReceptorTranslation;
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
  vtkTypeFloat64 xRayImageReceptorAngle = 0.0;
  if (rtImageObject.getXRayImageReceptorAngle(xRayImageReceptorAngle).good())
  {
    if (xRayImageReceptorAngle != 0.0)
    {
      vtkErrorMacro("LoadRTImage: Non-zero XRayImageReceptorAngle values are not supported!");
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
      vtkErrorMacro("LoadRTImage: ImagePlanePixelSpacing tag should contain a vector of 2 elements (it has " << imagePlanePixelSpacing.size() << "!");
    }
  }

  // RTImagePosition
  OFVector<vtkTypeFloat64> rtImagePosition;
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
  OFVector<vtkTypeFloat64> rtImageOrientation;
  if (rtImageObject.getRTImageOrientation(rtImageOrientation).good())
  {
    if (rtImageOrientation.size() > 0)
    {
      vtkErrorMacro("LoadRTImage: RTImageOrientation is specified but not supported yet!");
    }
  }

  // GantryAngle
  vtkTypeFloat64 gantryAngle = 0.0;
  if (rtImageObject.getGantryAngle(gantryAngle).good())
  {
    this->SetGantryAngle(gantryAngle);
  }

  // GantryPitchAngle
  vtkTypeFloat32 gantryPitchAngle = 0.0;
  if (rtImageObject.getGantryPitchAngle(gantryPitchAngle).good())
  {
    if (gantryPitchAngle != 0.0)
    {
      vtkErrorMacro("LoadRTImage: Non-zero GantryPitchAngle tag values are not supported yet!");
      return;
    }
  }

  // BeamLimitingDeviceAngle
  vtkTypeFloat64 beamLimitingDeviceAngle = 0.0;
  if (rtImageObject.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle).good())
  {
    this->SetBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
  }

  // PatientSupportAngle
  vtkTypeFloat64 patientSupportAngle = 0.0;
  if (rtImageObject.getPatientSupportAngle(patientSupportAngle).good())
  {
    this->SetPatientSupportAngle(patientSupportAngle);
  }

  // RadiationMachineSAD
  vtkTypeFloat64 radiationMachineSAD = 0.0;
  if (rtImageObject.getRadiationMachineSAD(radiationMachineSAD).good())
  {
    this->SetRadiationMachineSAD(radiationMachineSAD);
  }

  // RadiationMachineSSD
  vtkTypeFloat64 radiationMachineSSD = 0.0;
  if (rtImageObject.getRadiationMachineSSD(radiationMachineSSD).good())
  {
    //this->SetRadiationMachineSSD(radiationMachineSSD);
  }

  // RTImageSID
  vtkTypeFloat64 rtImageSID = 0.0;
  if (rtImageObject.getRTImageSID(rtImageSID).good())
  {
    this->SetRTImageSID(rtImageSID);
  }

  // SourceToReferenceObjectDistance
  vtkTypeFloat64 sourceToReferenceObjectDistance = 0.0;
  if (rtImageObject.getSourceToReferenceObjectDistance(sourceToReferenceObjectDistance).good())
  {
    //this->SetSourceToReferenceObjectDistance(sourceToReferenceObjectDistance);
  }

  // WindowCenter
  vtkTypeFloat64 windowCenter = 0.0;
  if (rtImageObject.getWindowCenter(windowCenter).good())
  {
    this->SetWindowCenter(windowCenter);
  }

  // WindowWidth
  vtkTypeFloat64 windowWidth = 0.0;
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
double vtkSlicerDicomRtReader::GetSliceThickness( DRTContourSequence& rtContourSequence )
{
  double defaultSliceThickness(2.0);
  double sliceThickness(-1);
  bool foundSliceThickness(false);

  // Get DICOM image filename from SOP instance UID
  ctkDICOMDatabase* dicomDatabase = new ctkDICOMDatabase();
  dicomDatabase->openDatabase(this->DatabaseFile, DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());

  OFString currentReferencedSOPInstanceUID("");
  OFString previousReferencedSOPInstanceUID("");
  vtkVector3<float> previousImagePositionPatient(0,0,0);
  double currentImagePositionPatient[3] = {0,0,0};
  double currentOrientationPatient[6] = {0,0,0,0,0,0};

  if( rtContourSequence.gotoFirstItem().bad() )
  {
    vtkErrorMacro("Unable to go to first item in contour sequence. Returning default slice thickness of " << defaultSliceThickness);
    return defaultSliceThickness;
  }

  std::map<int, std::string> slices;
  do
  {
    // For each slice
    DRTContourSequence::Item& rtContourSequenceItem = rtContourSequence.getCurrentItem();

    DRTContourImageSequence& rtContourImageSequence = rtContourSequenceItem.getContourImageSequence();
    if( rtContourImageSequence.gotoFirstItem().bad())
    {
      continue;
    }
    do 
    {
      DRTContourImageSequence::Item& rtContourImageSequenceItem = rtContourImageSequence.getCurrentItem();
      if (rtContourImageSequenceItem.isValid())
      {
        rtContourImageSequenceItem.getReferencedSOPInstanceUID(currentReferencedSOPInstanceUID);
        slices[-1] = std::string(currentReferencedSOPInstanceUID.c_str());
      }
    }
    while (rtContourImageSequence.gotoNextItem().good());
  }
  while(rtContourSequence.gotoNextItem().good());

  if( !this->OrderSliceSOPInstanceUID(*dicomDatabase, slices) )
  {
    vtkErrorMacro("Error while re-ordering DICOM files. Unable to parse slice thickness in order. Returning default of " << defaultSliceThickness);
    return defaultSliceThickness;
  }

  for( std::map<int, std::string>::iterator it = slices.begin(); it != slices.end(); ++it )
  {
    // Get filename for instance
    QString fileName = dicomDatabase->fileForInstance(it->second.c_str());
    if ( fileName.isEmpty() )
    {
      vtkWarningMacro("GetSliceThickness: No referenced image file is found, continuing to examine sequence.");
      continue;
    }
    std::string posPatientStringCurrent = std::string(dicomDatabase->fileValue(fileName, DCM_ImagePositionPatient.getGroup(), DCM_ImagePositionPatient.getElement()).toLatin1());

    // Get slice thickness from file
    QString thicknessString = dicomDatabase->fileValue(fileName, DCM_SliceThickness.getGroup(), DCM_SliceThickness.getElement());
    bool conversionSuccessful(false);
    double thisSliceThickness = thicknessString.toDouble(&conversionSuccessful);
    if( conversionSuccessful && foundSliceThickness && thisSliceThickness != sliceThickness)
    {
      vtkErrorMacro("GetSliceThickness: Varying slice thickness in referenced image series. Returning default of " << defaultSliceThickness << ".");
      return defaultSliceThickness;
    }
    else if( !conversionSuccessful && !previousReferencedSOPInstanceUID.empty() )
    {
      // Compute slice thickness between this slice and the previous image
      std::string posPatientStringCurrent = std::string(dicomDatabase->fileValue(fileName, DCM_ImagePositionPatient.getGroup(), DCM_ImagePositionPatient.getElement()).toLatin1());
      if( posPatientStringCurrent.empty() )
      {
        vtkErrorMacro("No ImagePositionPatient in DICOM database for file " << std::string(fileName.toLatin1()) << ". This is a serious error!");
        continue;
      }
      std::string orientationPatientStringCurrent = std::string(dicomDatabase->fileValue(fileName, DCM_ImageOrientationPatient.getGroup(), DCM_ImageOrientationPatient.getElement()).toLatin1());
      if( orientationPatientStringCurrent.empty() )
      {
        vtkErrorMacro("No ImageOrientationPatient in DICOM database for file " << std::string(fileName.toLatin1()) << ". This is a serious error!");
        continue;
      }
      int rc = sscanf(posPatientStringCurrent.c_str(), "%lf\\%lf\\%lf", &currentImagePositionPatient[0], &currentImagePositionPatient[1], &currentImagePositionPatient[2]);
      rc = sscanf(orientationPatientStringCurrent.c_str(), "%lf\\%lf\\%lf\\%lf\\%lf\\%lf", &currentOrientationPatient[0], &currentOrientationPatient[1], &currentOrientationPatient[2],
        &currentOrientationPatient[3], &currentOrientationPatient[4], &currentOrientationPatient[5]);

      // k vector is cross product of i and j vector, which are orientation read above
      vtkVector3<float> currentIVector(currentOrientationPatient[0], currentOrientationPatient[1], currentOrientationPatient[2]);
      vtkVector3<float> currentJVector(currentOrientationPatient[3], currentOrientationPatient[4], currentOrientationPatient[5]);
      vtkVector3<float> currentKVector = currentIVector.Cross(currentJVector);

      // projection of distance vector along k vector
      vtkVector3<float> originDifferenceVector = vtkVector3<float>(currentImagePositionPatient[0], currentImagePositionPatient[1], currentImagePositionPatient[2]) - previousImagePositionPatient;
      thisSliceThickness = abs(originDifferenceVector.Dot(currentKVector));
      if( foundSliceThickness && thisSliceThickness != sliceThickness )
      {
        vtkErrorMacro("GetSliceThickness: Varying slice thickness in referenced image series. Returning default of " << defaultSliceThickness << ".");
        return defaultSliceThickness;
      }   
    }
    // either we found it in the file or we calculated it, but we've got some value (or we've skipped this image)
    // if we've calculated a varying slice thickness, we've already exited because that's a problem
    // we might have a k vector pointing away from the previous slice, so just take the absolute value
    sliceThickness = thisSliceThickness;
    foundSliceThickness = true;

    previousImagePositionPatient.Set(currentImagePositionPatient[0], currentImagePositionPatient[1], currentImagePositionPatient[2]);
    previousReferencedSOPInstanceUID = currentReferencedSOPInstanceUID;
    currentReferencedSOPInstanceUID = OFString("");
  }

  if( !foundSliceThickness )
  {
    // This can only be hit if there is no DCM slice thickness tag and no DCM image position patient or orientation tags
    vtkErrorMacro("Unable to calculate slice thickness in an image series. Image series is missing key DICOM tags. Returning default slice thickness " << defaultSliceThickness <<".");
    return defaultSliceThickness;
  }

  dicomDatabase->closeDatabase();
  delete dicomDatabase;
  QSqlDatabase::removeDatabase(DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());
  QSqlDatabase::removeDatabase(QString(DICOMRTREADER_DICOM_CONNECTION_NAME.c_str()) + "TagCache");

  return sliceThickness;
}

//----------------------------------------------------------------------------
// Variables for calculating the distance between contour planes.
double vtkSlicerDicomRtReader::GetDistanceBetweenContourPlanes(DRTROIContourSequence &rtROIContourSequenceObject)
{
  double defaultDistanceBetweenContourPlanes(1.0);

  double distanceBetweenContourPlanes(-1.0);
  bool foundDistance(false);
  vtkPlane* previousContourPlane = vtkPlane::New();
  bool previousSet(false);
  vtkPlane* currentContourPlane = vtkPlane::New();

  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("vtkSlicerDicomRtReader::GetDistanceBetweenContourPlanes: No structure sets were found. Returning default of " << defaultDistanceBetweenContourPlanes);
    return defaultDistanceBetweenContourPlanes;
  }
  // Iterate over each contour in the set until you find one that gives you a result
  do 
  {
    DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
    if (!currentRoiObject.isValid())
    {
      continue;
    }
    // Get contour sequence
    DRTContourSequence &rtContourSequenceObject = currentRoiObject.getContourSequence();
    if (!rtContourSequenceObject.gotoFirstItem().good())
    {
      vtkErrorMacro("LoadRTStructureSet: Contour sequence for ROI is empty!");
      continue;
    }

    if( rtContourSequenceObject.isEmpty() )
    {
      vtkErrorMacro("Empty contour. Skipping.");
      continue;
    }
    if( rtContourSequenceObject.getNumberOfItems() < 2 )
    {
      vtkErrorMacro("Unable to calculate distance between contour planes, less than two planes detected. Skipping.");
      continue;
    }
    if (!rtContourSequenceObject.gotoFirstItem().good())
    {
      vtkErrorMacro("GetDistanceBetweenContourPlanes: Contour sequence object is invalid. Skipping.");
      continue;
    }

    // Iterate over each plane in the contour
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
      if (number < 3)
      {
        Sint32 contourNumber(-1);
        contourItem.getContourNumber(contourNumber);
        vtkWarningMacro("Contour does not contain enough points to extract a planar equation. Skipping contour number: " << contourNumber << ".");
        previousContourPlane->SetNormal(0.0, 0.0, 0.0);
        previousContourPlane->SetOrigin(0.0, 0.0, 0.0);
        previousSet = false;
        continue;
      }

      OFVector<vtkTypeFloat64>  contourData_LPS;
      contourItem.getContourData(contourData_LPS);
      vtkVector3<vtkTypeFloat64> firstPlanePoint;
      vtkVector3<vtkTypeFloat64> secondPlanePoint;
      vtkVector3<vtkTypeFloat64> thirdPlanePoint;

      vtkVector3<vtkTypeFloat64> currentPlaneIVector;
      vtkVector3<vtkTypeFloat64> currentPlaneJVector;
      vtkVector3<vtkTypeFloat64> currentPlaneKVector;

      for( int i = 0; i < contourData_LPS.size(); i+=3 )
      {
        if( i+8 >= contourData_LPS.size() )
        {
          break;
        }
        firstPlanePoint.Set(contourData_LPS[i], contourData_LPS[i+1], contourData_LPS[i+2]);
        secondPlanePoint.Set(contourData_LPS[i+3], contourData_LPS[i+4], contourData_LPS[i+5]);
        thirdPlanePoint.Set(contourData_LPS[i+6], contourData_LPS[i+7], contourData_LPS[i+8]);

        currentPlaneIVector = secondPlanePoint - firstPlanePoint;
        currentPlaneJVector = thirdPlanePoint - firstPlanePoint;
        currentPlaneKVector = currentPlaneIVector.Cross(currentPlaneJVector);

        if( !(currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0) )
        {
          break;
        }
      }

      if( currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0 )
      {
        vtkErrorMacro("All points in contour produce co-linear vectors. Unable to determine equation of the plane. Returning default of " << defaultDistanceBetweenContourPlanes);
        return defaultDistanceBetweenContourPlanes;
      }

      currentPlaneKVector.Normalize();
      currentContourPlane->SetNormal(currentPlaneKVector.GetX(), currentPlaneKVector.GetY(), currentPlaneKVector.GetZ());
      currentContourPlane->SetOrigin(firstPlanePoint.GetX(), firstPlanePoint.GetY(), firstPlanePoint.GetZ());

      if( previousSet )
      {
        // Previous contour plane was valid, let er rip
        double thisDistanceBetweenPlanes = currentContourPlane->DistanceToPlane(previousContourPlane->GetOrigin(), currentContourPlane->GetNormal(), currentContourPlane->GetOrigin());
        if( foundDistance && !AreSame(thisDistanceBetweenPlanes, distanceBetweenContourPlanes) )
        {
          vtkErrorMacro("Contours do not have consistent plane spacing. Unable to compute distance between planes. Returning default of " << defaultDistanceBetweenContourPlanes);
          distanceBetweenContourPlanes = defaultDistanceBetweenContourPlanes;
          break;
        }
        else if ( !foundDistance )
        {
          distanceBetweenContourPlanes = thisDistanceBetweenPlanes;
          foundDistance = true;
        }
      }

      previousSet = true;
      previousContourPlane->SetNormal(currentContourPlane->GetNormal());
      previousContourPlane->SetOrigin(currentContourPlane->GetOrigin());
    } 
    while (rtContourSequenceObject.gotoNextItem().good());

    if( foundDistance )
    {
      break;
    }
  }
  while(rtROIContourSequenceObject.gotoNextItem().good());

  currentContourPlane->Delete();
  previousContourPlane->Delete();

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

  DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject.getROIContourSequence();
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("LoadRTStructureSet: No ROIContourSequence found!");
    return;
  }
  double sliceThickness = this->GetDistanceBetweenContourPlanes(rtROIContourSequenceObject);

  // Reset the ROI contour sequence to the start
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
    std::vector<vtkSmartPointer<vtkPlane> > tempPlanes;
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

      OFVector<vtkTypeFloat64> contourData_LPS;
      contourItem.getContourData(contourData_LPS);

      // Create a vtk plane for later access
      vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
      this->CreatePlaneFromContourData(contourData_LPS, plane);
      tempPlanes.push_back(plane);

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

    // Now that I have all planes, lets order them
    std::map<double, vtkSmartPointer<vtkPlane> > orderedPlanes;
    SlicerRtCommon::OrderPlanesAlongNormal(tempPlanes, orderedPlanes);
    roiEntry->OrderedContourPlanes = orderedPlanes;

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
    vtkErrorMacro("GetRoiReferencedSeriesUid: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].ReferencedSeriesUid.c_str();
}

//---------------------------------------------------------------------------
std::map<double, vtkSmartPointer<vtkPlane> > vtkSlicerDicomRtReader::GetRoiOrderedContourPlanes( unsigned int internalIndex )
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiOrderedContourPlanes: Cannot get ROI with internal index: " << internalIndex);
    std::map<double, vtkSmartPointer<vtkPlane> > noPlanes;
    return noPlanes;
  }
  return this->RoiSequenceVector[internalIndex].OrderedContourPlanes;
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

  OFVector<vtkTypeFloat64> pixelSpacingOFVector;
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
  ctkDICOMDatabase* dicomDatabase = new ctkDICOMDatabase();
  dicomDatabase->openDatabase(this->DatabaseFile, DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());

  std::map<int,std::string>* contourIndexToSopInstanceUidMap = &(this->RoiSequenceVector[internalIndex].ContourIndexToSopInstanceUidMap);
  std::map<int,std::string>::iterator sliceInstanceUidIt;
  QString imageOrientation;
  for (sliceInstanceUidIt = contourIndexToSopInstanceUidMap->begin(); sliceInstanceUidIt != contourIndexToSopInstanceUidMap->end(); ++sliceInstanceUidIt)
  {
    // Get file name for referenced slice instance from the stored SOP instance UID
    QString fileName = dicomDatabase->fileForInstance(sliceInstanceUidIt->second.c_str());
    if (fileName.isEmpty())
    {
      vtkErrorMacro("CreateRibbonModelForRoi: No referenced image file is found for ROI contour slice number " << sliceInstanceUidIt->first);
      continue;
    }

    // Get image orientation string from the referenced slice
    QString currentImageOrientation = dicomDatabase->fileValue(fileName, DCM_ImageOrientationPatient.getGroup(), DCM_ImageOrientationPatient.getElement());

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

  dicomDatabase->closeDatabase();
  delete dicomDatabase;
  QSqlDatabase::removeDatabase(DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());
  QSqlDatabase::removeDatabase(QString(DICOMRTREADER_DICOM_CONNECTION_NAME.c_str()) + "TagCache");

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

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtReader::OrderSliceSOPInstanceUID( ctkDICOMDatabase& openDatabase, std::map<int, std::string>& slices )
{
  if( !openDatabase.isOpen() )
  {
    return false;
  }
  double imagePositionPatient[3] = {0,0,0};
  double orientationPatient[6] = {0,0,0,0,0,0};
  vtkVector3<vtkTypeFloat64> patientPosition;
  vtkVector3<vtkTypeFloat64> patientOrientationNormal;
  vtkVector3<vtkTypeFloat64> prevPatientOrientationNormal(0,0,0);

  std::vector<vtkSmartPointer<vtkPlane> > planeList;
  std::map<std::string, vtkSmartPointer<vtkPlane> > planeToIdMap;

  for( std::map<int, std::string>::iterator it = slices.begin(); it != slices.end(); ++it )
  {
    // Get filename for instance
    QString fileName = openDatabase.fileForInstance(it->second.c_str());
    if ( fileName.isEmpty() )
    {
      vtkWarningMacro("GetSliceThickness: No referenced image file is found, continuing to examine sequence.");
      continue;
    }
    std::string posPatientStringCurrent = std::string(openDatabase.fileValue(fileName, DCM_ImagePositionPatient.getGroup(), DCM_ImagePositionPatient.getElement()).toLatin1());
    if( posPatientStringCurrent.empty() )
    {
      continue;
    }
    std::string orientationPatientStringCurrent = std::string(openDatabase.fileValue(fileName, DCM_ImageOrientationPatient.getGroup(), DCM_ImageOrientationPatient.getElement()).toLatin1());
    if( orientationPatientStringCurrent.empty() )
    {
      continue;
    }
    int rc = sscanf(posPatientStringCurrent.c_str(), "%lf\\%lf\\%lf", &imagePositionPatient[0], &imagePositionPatient[1], &imagePositionPatient[2]);
    rc = sscanf(orientationPatientStringCurrent.c_str(), "%lf\\%lf\\%lf\\%lf\\%lf\\%lf", &orientationPatient[0], &orientationPatient[1], &orientationPatient[2],
      &orientationPatient[3], &orientationPatient[4], &orientationPatient[5]);

    // Calculate the slice normal
    vtkVector3<vtkTypeFloat64> iVec(orientationPatient[0], orientationPatient[1], orientationPatient[2]);
    vtkVector3<vtkTypeFloat64> jVec(orientationPatient[3], orientationPatient[4], orientationPatient[5]);
    patientOrientationNormal = iVec.Cross(jVec);
    patientOrientationNormal.Normalize();

    if( prevPatientOrientationNormal.X() == 0 && prevPatientOrientationNormal.Y() == 0 && prevPatientOrientationNormal.Z() == 0 )
    {
      prevPatientOrientationNormal.Set(patientOrientationNormal[0], patientOrientationNormal[1], patientOrientationNormal[2]);
    }
    else if( patientOrientationNormal != prevPatientOrientationNormal )
    {
      vtkErrorMacro("Normals are not the same. Slices are not co-planar. Algorithm requires this.");
      return false;
    }

    vtkVector3<vtkTypeFloat64> patientOrigin(imagePositionPatient[0], imagePositionPatient[1], imagePositionPatient[2]);

    // Create a plane representation for this slice
    vtkSmartPointer<vtkPlane> slicePlane = vtkSmartPointer<vtkPlane>::New();
    slicePlane->SetOrigin(imagePositionPatient[0], imagePositionPatient[1], imagePositionPatient[2]);
    slicePlane->SetNormal(patientOrientationNormal[0], patientOrientationNormal[1], patientOrientationNormal[2]);
    planeList.push_back(slicePlane);
    planeToIdMap[it->second] = slicePlane;
  }

  std::map<double, vtkSmartPointer<vtkPlane> > orderedPlanes;
  SlicerRtCommon::OrderPlanesAlongNormal(planeList, orderedPlanes);

  // Now that we have the sorted planes, rebuild the ID to sorted plane order map as requested
  int i = 0;
  for( std::map<double, vtkSmartPointer<vtkPlane> >::iterator it = orderedPlanes.begin(); it != orderedPlanes.end(); ++it )
  {
    std::string thisPlaneId;
    // find string for this plane
    for( std::map<std::string, vtkSmartPointer<vtkPlane> >::iterator lookupIt = planeToIdMap.begin(); lookupIt != planeToIdMap.end(); ++lookupIt )
    {
      if( lookupIt->second == it->second )
      {
        thisPlaneId = lookupIt->first;
        break;
      }
    }

    slices[i] = thisPlaneId;
  }

  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtReader::CreatePlaneFromContourData( OFVector<vtkTypeFloat64>& contourData_LPS, vtkPlane* aPlane )
{
  if( aPlane == NULL )
  {
    vtkErrorMacro("Null plane sent to vtkSlicerDicomRtReader::CreatePlaneFromContourData. Can't continue.");
    return false;
  }
  vtkVector3<vtkTypeFloat64> firstPlanePoint;
  vtkVector3<vtkTypeFloat64> secondPlanePoint;
  vtkVector3<vtkTypeFloat64> thirdPlanePoint;

  vtkVector3<vtkTypeFloat64> currentPlaneIVector;
  vtkVector3<vtkTypeFloat64> currentPlaneJVector;
  vtkVector3<vtkTypeFloat64> currentPlaneKVector;

  for( int i = 0; i < contourData_LPS.size(); i+=3 )
  {
    if( i+8 >= contourData_LPS.size() )
    {
      break;
    }
    firstPlanePoint.Set(contourData_LPS[i], contourData_LPS[i+1], contourData_LPS[i+2]);
    secondPlanePoint.Set(contourData_LPS[i+3], contourData_LPS[i+4], contourData_LPS[i+5]);
    thirdPlanePoint.Set(contourData_LPS[i+6], contourData_LPS[i+7], contourData_LPS[i+8]);

    currentPlaneIVector = secondPlanePoint - firstPlanePoint;
    currentPlaneJVector = thirdPlanePoint - firstPlanePoint;
    currentPlaneKVector = currentPlaneIVector.Cross(currentPlaneJVector);

    if( !(currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0) )
    {
      break;
    }
  }

  if( currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0 )
  {
    vtkErrorMacro("All points in contour produce co-linear vectors. Unable to determine equation of the plane.");
    return false;
  }

  currentPlaneKVector.Normalize();
  aPlane->SetNormal(currentPlaneKVector.GetX(), currentPlaneKVector.GetY(), currentPlaneKVector.GetZ());
  aPlane->SetOrigin(firstPlanePoint.GetX(), firstPlanePoint.GetY(), firstPlanePoint.GetZ());

  return true;
}
