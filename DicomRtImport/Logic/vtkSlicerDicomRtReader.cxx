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

// MRML includes

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkRibbonFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>

// STD includes
#include <cassert>
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
  Number=0;
  DisplayColor[0]=1.0;
  DisplayColor[1]=0.0;
  DisplayColor[2]=0.0;
  PolyData=NULL;
}

vtkSlicerDicomRtReader::RoiEntry::~RoiEntry()
{
  SetPolyData(NULL);
}

vtkSlicerDicomRtReader::RoiEntry::RoiEntry(const RoiEntry& src)
{
  Number=src.Number;
  Name=src.Name;
  Description=src.Description;
  DisplayColor[0]=src.DisplayColor[0];
  DisplayColor[1]=src.DisplayColor[1];
  DisplayColor[2]=src.DisplayColor[2];
  PolyData=NULL;
  SetPolyData(src.PolyData);
}

vtkSlicerDicomRtReader::RoiEntry& vtkSlicerDicomRtReader::RoiEntry::operator=(const RoiEntry &src)
{
  Number=src.Number;
  Name=src.Name;
  Description=src.Description;
  DisplayColor[0]=src.DisplayColor[0];
  DisplayColor[1]=src.DisplayColor[1];
  DisplayColor[2]=src.DisplayColor[2];
  SetPolyData(src.PolyData);
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

  this->SetPixelSpacing(0,0);
  this->DoseUnits = NULL;
  this->DoseGridScaling = NULL;

  this->PatientName = NULL;
  this->PatientId = NULL;
  this->StudyInstanceUid = NULL;
  this->StudyDescription = NULL;
  this->SeriesInstanceUid = NULL;
  this->SeriesDescription = NULL;

  this->DatabaseFile = NULL;

  this->LoadRTStructureSetSuccessful = false;
  this->LoadRTDoseSuccessful = false;
  this->LoadRTPlanSuccessful = false;
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

    // load DICOM file or dataset
    DcmFileFormat fileformat;

    OFCondition result;
    result = fileformat.loadFile(this->FileName, EXS_Unknown);
    if (result.good())
    {
      DcmDataset *dataset = fileformat.getDataset();
      // check SOP Class UID for one of the supported RT objects
      OFString sopClass;
      if (dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() && !sopClass.empty())
      {
        if (sopClass == UID_RTDoseStorage)
        {
          this->LoadRTDose(dataset);
        }
        else if (sopClass == UID_RTImageStorage)
        {
          //result = dumpRTImage(out, *dataset);
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
void vtkSlicerDicomRtReader::LoadRTPlan(DcmDataset* dataset)
{
  this->LoadRTPlanSuccessful = false; 

  DRTPlanIOD rtPlanObject;
  OFCondition result = rtPlanObject.read(*dataset);
  if (result.good())
  {
    OFString tmpString, dummyString;
    std::cout << "Load RT Plan object" << std::endl;

    DRTBeamSequence &rtPlaneBeamSequenceObject = rtPlanObject.getBeamSequence();
    if (rtPlaneBeamSequenceObject.gotoFirstItem().good())
    {
      do
      {
        DRTBeamSequence::Item &currentBeamSequenceObject = rtPlaneBeamSequenceObject.getCurrentItem();  
        if (!currentBeamSequenceObject.isValid())
        {
          std::cout << "Found an invalid beam sequence in dataset" << std::endl;
          continue;
        }

        // Read item into the BeamSequenceVector
        BeamEntry beamEntry;

        OFString beamName;
        currentBeamSequenceObject.getBeamName(beamName);
        beamEntry.Name=beamName.c_str();

        OFString beamDescription;
        currentBeamSequenceObject.getBeamDescription(beamDescription);
        beamEntry.Description=beamDescription.c_str();

        OFString beamType;
        currentBeamSequenceObject.getBeamType(beamType);
        beamEntry.Type=beamType.c_str();

        Sint32 beamNumber;
        currentBeamSequenceObject.getBeamNumber(beamNumber);        
        beamEntry.Number = beamNumber;

        Float64 sourceAxisDistance;
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
              // convert from DICOM LPS -> Slicer RAS
              beamEntry.IsocenterPositionRas[0] = -isocenterPositionDataLps[0];
              beamEntry.IsocenterPositionRas[1] = -isocenterPositionDataLps[1];
              beamEntry.IsocenterPositionRas[2] = isocenterPositionDataLps[2];

              Float64 gantryAngle;
              controlPointItem.getGantryAngle(gantryAngle);
              beamEntry.GantryAngle = gantryAngle;

              Float64 patientSupportAngle;
              controlPointItem.getPatientSupportAngle(patientSupportAngle);
              beamEntry.PatientSupportAngle = patientSupportAngle;

              Float64 beamLimitingDeviceAngle;
              controlPointItem.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
              beamEntry.BeamLimitingDeviceAngle = beamLimitingDeviceAngle;

              unsigned int numberOfFoundCollimatorPositionItems = 0;
              DRTBeamLimitingDevicePositionSequence &currentCollimatorPositionSequenceObject
                = controlPointItem.getBeamLimitingDevicePositionSequence();
              if (currentCollimatorPositionSequenceObject.gotoFirstItem().good())
              {
                do 
                {
                  if (++numberOfFoundCollimatorPositionItems > 2)
                  {
                    std::cerr << "Unexpected number of collimator position items (we expect exactly 2)" << std::endl;
                    break;
                  }

                  DRTBeamLimitingDevicePositionSequence::Item &collimatorPositionItem
                    = currentCollimatorPositionSequenceObject.getCurrentItem();
                  if (collimatorPositionItem.isValid())
                  {
                    OFString rtBeamLimitingDeviceType;
                    collimatorPositionItem.getRTBeamLimitingDeviceType(rtBeamLimitingDeviceType);

                    OFVector<Float64> leafJawPositions;
                    collimatorPositionItem.getLeafJawPositions(leafJawPositions);

                    if ( !rtBeamLimitingDeviceType.compare("ASYMX") || !rtBeamLimitingDeviceType.compare("X") )
                    {
                      beamEntry.LeafJawPositions[0][0] = leafJawPositions[0];
                      beamEntry.LeafJawPositions[0][1] = leafJawPositions[1];
                    }
                    else if ( !rtBeamLimitingDeviceType.compare("ASYMY") || !rtBeamLimitingDeviceType.compare("Y") )
                    {
                      beamEntry.LeafJawPositions[1][0] = leafJawPositions[0];
                      beamEntry.LeafJawPositions[1][1] = leafJawPositions[1];
                    }
                    else
                    {
                      std::cerr << "Unsupported collimator type: " << rtBeamLimitingDeviceType << std::endl;
                    }
                  }
                }
                while (currentCollimatorPositionSequenceObject.gotoNextItem().good());
              }
            } //endif controlPointItem.isValid()
          }
          // while (rtControlPointSequenceObject.gotoNextItem().good());
        }

        this->BeamSequenceVector.push_back(beamEntry);
      }
      while (rtPlaneBeamSequenceObject.gotoNextItem().good());
    }
  }

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtPlanObject);

  this->LoadRTPlanSuccessful = true;
}

//----------------------------------------------------------------------------
OFString vtkSlicerDicomRtReader::GetReferencedFrameOfReferenceSOPInstanceUID(DRTStructureSetIOD &rtStructureSetObject)
{
  OFString invalidUid;
  DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject.getReferencedFrameOfReferenceSequence();
  if (!rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("No referenced frame of reference sequence object item is available");
    return invalidUid;
  }

  DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
  if (!currentReferencedFrameOfReferenceSequenceItem.isValid())
  {
    vtkErrorMacro("Frame of reference sequence object item is invalid");
    return invalidUid;
  }

  DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
  if (!rtReferencedStudySequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("No referenced study sequence object item is available");
    return invalidUid;
  }

  DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
  if (!rtReferencedStudySequenceItem.isValid())
  {
    vtkErrorMacro("Referenced study sequence object item is invalid");
    return invalidUid;
  }

  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
  if (!rtReferencedSeriesSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("No referenced series sequence object item is available");
    return invalidUid;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject.getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorMacro("Referenced series sequence object item is invalid");
    return invalidUid;
  }

  DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
  if (!rtContourImageSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("No contour image sequence object item is available");
    return invalidUid;
  }

  DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
  if (!rtContourImageSequenceItem.isValid())
  {
    vtkErrorMacro("Contour image sequence object item is invalid");
    return invalidUid;
  }

  OFString resultUid;
  rtContourImageSequenceItem.getReferencedSOPInstanceUID(resultUid);
  return resultUid;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetSliceThickness(OFString referencedSOPInstanceUID)
{
  double defaultSliceThickness = 2.0;

  // Get DICOM image filename from SOP instance UID
  ctkDICOMDatabase dicomDatabase;
  dicomDatabase.openDatabase(this->DatabaseFile, DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());
  QString referencedFilename=dicomDatabase.fileForInstance(referencedSOPInstanceUID.c_str());
  dicomDatabase.closeDatabase();
  if ( referencedFilename.isEmpty() ) //TODO: isNull?
  {
    std::cerr << "No referenced image file is found, default slice thickness is used for contour import" << std::endl;
    return defaultSliceThickness;
  }

  // Load DICOM file
  DcmFileFormat fileformat;
  OFCondition result;
  result = fileformat.loadFile(referencedFilename.toStdString().c_str(), EXS_Unknown);
  if (!result.good())
  {
    std::cerr << "Could not load image file" << std::endl;
    return defaultSliceThickness;
  }
  DcmDataset *dataset = fileformat.getDataset();

  // Use the slice thickness defined in the DICOM file
  OFString sliceThicknessString;
  if (!dataset->findAndGetOFString(DCM_SliceThickness, sliceThicknessString).good())
  {
    std::cerr << "Could not find slice thickness tag in image file" << std::endl;
    return defaultSliceThickness;
  }

  std::stringstream ss;
  ss << sliceThicknessString;
  double doubleValue;
  ss >> doubleValue;
  double sliceThickness = doubleValue;
  if (sliceThickness <= 0.0 || sliceThickness > 20.0)
  {
    std::cerr << "Slice thickness field value is invalid: " << sliceThicknessString << std::endl;
    return defaultSliceThickness;
  }

  return sliceThickness;
}

//----------------------------------------------------------------------------
// Variables for estimating the distance between contour planes.
// This is not a reliable solution, as it assumes that the plane normals are (0,0,1) and
// the distance between all planes are equal.
double vtkSlicerDicomRtReader::GetDistanceBetweenContourPlanes(DRTContourSequence &rtContourSequenceObject)
{
  double invalidResult=-1.0;
  if (!rtContourSequenceObject.gotoFirstItem().good())
  {
    std::cerr << "Contour sequence object is invalid" << std::endl;
    return invalidResult;
  }

  double firstContourPlanePosition=0.0;
  double secondContourPlanePosition=0.0;
  int contourPlaneIndex=0;
  do
  {
    DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
    if ( !contourItem.isValid())
    {
      continue;
    }

    OFString numberofpoints;
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

    double firstContourPointZcoordinate=contourData_LPS[2];
    switch (contourPlaneIndex)
    {
    case 0:
      // first contour
      firstContourPlanePosition=firstContourPointZcoordinate;
      break;
    case 1:
      // second contour
      secondContourPlanePosition=firstContourPointZcoordinate;
      break;
    default:
      // we ignore all the subsequent contour plane positions
      // distance is just estimated based on the first two
      break;
    }
    contourPlaneIndex++;

  } while (rtContourSequenceObject.gotoNextItem().good() && contourPlaneIndex<2);

  if (contourPlaneIndex<2)
  {
    std::cerr << "Not found two contours" << std::endl;
    return invalidResult;
  }

  // there were at least contour planes, therefore we have a valid distance estimation
  double distanceBetweenContourPlanes=fabs(firstContourPlanePosition-secondContourPlanePosition);
  return distanceBetweenContourPlanes;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTStructureSet(DcmDataset* dataset)
{
  this->LoadRTStructureSetSuccessful = false;

  DRTStructureSetIOD rtStructureSetObject;
  OFCondition result = rtStructureSetObject.read(*dataset);
  if (!result.good())
  {
    std::cerr << "Could not load strucure set object from dataset" << std::endl;
    return;
  }

  std::cout << "RT Structure Set object" << std::endl;

  // Read ROI name, description, and number into the ROI contour sequence vector
  DRTStructureSetROISequence &rtStructureSetROISequenceObject = rtStructureSetObject.getStructureSetROISequence();
  if (!rtStructureSetROISequenceObject.gotoFirstItem().good())
  {
    std::cerr << "No structure sets were found" << std::endl;
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

    OFString roiName;
    currentROISequenceObject.getROIName(roiName);
    roiEntry.Name=roiName.c_str();

    OFString roiDescription;
    currentROISequenceObject.getROIDescription(roiDescription);
    roiEntry.Description=roiDescription.c_str();                   

    Sint32 roiNumber;
    currentROISequenceObject.getROINumber(roiNumber);
    roiEntry.Number=roiNumber;
    // cout << "roi number:" << roiNumber << " roi name:" << ROIName << " roi description:" << ROIDescription << OFendl;

    // save to vector          
    this->RoiSequenceVector.push_back(roiEntry);
  }
  while (rtStructureSetROISequenceObject.gotoNextItem().good());

  // Get the slice thickness from the referenced image
  OFString referencedSOPInstanceUID=GetReferencedFrameOfReferenceSOPInstanceUID(rtStructureSetObject);
  double sliceThickness = GetSliceThickness(referencedSOPInstanceUID);

  Sint32 referenceRoiNumber;
  DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject.getROIContourSequence();
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    return;
  }

  do 
  {
    DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
    if (!currentRoiObject.isValid())
    {
      continue;
    }

    // create vtkPolyData
    vtkSmartPointer<vtkPoints> tempPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> tempCellArray = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType pointId=0;

    currentRoiObject.getReferencedROINumber(referenceRoiNumber);
    DRTContourSequence &rtContourSequenceObject = currentRoiObject.getContourSequence();

    if (rtContourSequenceObject.gotoFirstItem().good())
    {
      do
      {
        DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();

        if ( !contourItem.isValid())
        {
          continue;
        }
        OFString contourNumber;
        contourItem.getContourNumber(contourNumber);

        OFString numberofpoints;
        contourItem.getNumberOfContourPoints(numberofpoints);
        //cout << "\t contour number:" << contourNumber.c_str() << " numberOf points: "<< numberofpoints.c_str() << OFendl;
        std::stringstream ss;
        ss << numberofpoints;
        int number;
        ss >> number;

        OFVector<Float64>  contourData_LPS;
        contourItem.getContourData(contourData_LPS);

        tempCellArray->InsertNextCell(number+1);
        for (int k=0; k<number; k++)
        {
          // convert from DICOM LPS -> Slicer RAS
          tempPoints->InsertPoint(pointId, -contourData_LPS[3*k], -contourData_LPS[3*k+1], contourData_LPS[3*k+2]);
          tempCellArray->InsertCellPoint(pointId);
          pointId++;
        }

        // to close the contour
        tempCellArray->InsertCellPoint(pointId-number);

      }
      while (rtContourSequenceObject.gotoNextItem().good());

    } // if gotoFirstItem

    // Save it into ROI vector
    RoiEntry* referenceROI = this->FindRoiByNumber(referenceRoiNumber);
    if (referenceROI==NULL)
    {
      std::cerr << "Reference ROI is not found" << std::endl;      
      continue;
    } 

    if (tempPoints->GetNumberOfPoints() == 1)
    {
      // Point ROI
      vtkSmartPointer<vtkPolyData> tempPolyData = vtkSmartPointer<vtkPolyData>::New();
      tempPolyData->SetPoints(tempPoints);
      tempPolyData->SetVerts(tempCellArray);
      referenceROI->SetPolyData(tempPolyData);
    }
    else if (tempPoints->GetNumberOfPoints() > 1)
    {
      // Contour ROI
      vtkSmartPointer<vtkPolyData> tempPolyData = vtkSmartPointer<vtkPolyData>::New();
      tempPolyData->SetPoints(tempPoints);
      tempPolyData->SetLines(tempCellArray);

      // Remove coincident points (if there are multiple contour points at the same position then the
      // ribbon filter fails)
      vtkSmartPointer<vtkCleanPolyData> cleaner=vtkSmartPointer<vtkCleanPolyData>::New();
      cleaner->SetInput(tempPolyData);

      // convert to ribbon using vtkRibbonFilter
      vtkSmartPointer<vtkRibbonFilter> ribbonFilter = vtkSmartPointer<vtkRibbonFilter>::New();
      ribbonFilter->SetInputConnection(cleaner->GetOutputPort());
      ribbonFilter->SetDefaultNormal(0,0,-1);
      ribbonFilter->SetWidth(sliceThickness/2.0);
      ribbonFilter->SetAngle(90.0);
      ribbonFilter->UseDefaultNormalOn();
      ribbonFilter->Update();

      vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
      normalFilter->SetInputConnection(ribbonFilter->GetOutputPort());
      normalFilter->ConsistencyOn();
      normalFilter->Update();

      referenceROI->SetPolyData(normalFilter->GetOutput());
    }

    Sint32 roiDisplayColor;
    for (int j=0; j<3; j++)
    {
      currentRoiObject.getROIDisplayColor(roiDisplayColor,j);
      referenceROI->DisplayColor[j] = roiDisplayColor/255.0;
    }    
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
const char* vtkSlicerDicomRtReader::GetRoiNameByRoiNumber(unsigned int roiNumber)
{
  RoiEntry* roi = this->FindRoiByNumber(roiNumber);
  if (roi==NULL)
  {
    return NULL;
  }  
  return roi->Name.c_str();
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetRoiPolyDataByRoiNumber(unsigned int roiNumber)
{
  RoiEntry* roi = this->FindRoiByNumber(roiNumber);
  if (roi==NULL)
  {
    return NULL;
  }
  return roi->PolyData;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetRoiDisplayColorByRoiNumber(unsigned int roiNumber)
{
  RoiEntry* roi = this->FindRoiByNumber(roiNumber);
  if (roi==NULL)
  {
    return NULL;
  }
  return roi->DisplayColor;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiName(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkWarningMacro("Cannot get roi with number: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].Name.c_str();
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetRoiPolyData(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkWarningMacro("Cannot get roi with number: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].PolyData;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetRoiDisplayColor(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkWarningMacro("Cannot get roi with number: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].DisplayColor;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->BeamSequenceVector.size();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamName(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->Name.c_str();
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
    return NULL;
  }  
  return beam->SourceAxisDistance;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamGantryAngle(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->GantryAngle;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamPatientSupportAngle(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->PatientSupportAngle;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamBeamLimitingDeviceAngle(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
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
  OFCondition result = rtDoseObject.read(*dataset);
  if (result.bad())
  {
    cerr << "Error: Failed to read RT Dose dataset!" << OFendl;
  }

  cout << "RT Dose object" << OFendl << OFendl;
  OFString doseGridScaling;
  if (rtDoseObject.getDoseGridScaling(doseGridScaling).bad())
  {
    cerr << "Error: Failed to get Dose Grid Scaling for dose object" << OFendl;
    return; // mandatory DICOM value
  }
  this->SetDoseGridScaling(doseGridScaling.c_str());

  OFString doseUnits;
  if (rtDoseObject.getDoseUnits(doseUnits).bad())
  {
    cerr << "Error: Failed to get Dose Units for dose object" << OFendl;
    return; // mandatory DICOM value
  }
  this->SetDoseUnits(doseUnits.c_str());

  OFVector<Float64> pixelSpacingOFVector;
  if (rtDoseObject.getPixelSpacing(pixelSpacingOFVector).bad() || pixelSpacingOFVector.size() < 2)
  {
    cerr << "Error: Failed to get Pixel Spacing for dose object" << OFendl;
    return; // mandatory DICOM value
  }
  this->SetPixelSpacing(pixelSpacingOFVector[0], pixelSpacingOFVector[1]);
  cout << "Pixel Spacing: (" << pixelSpacingOFVector[0] << ", " << pixelSpacingOFVector[1] << ")" << OFendl;

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
  // not found
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
  // not found
  return NULL;
}
