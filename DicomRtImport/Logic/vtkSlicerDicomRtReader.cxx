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
#include <vtkNew.h>
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
#include <dcmtk/dcmrt/drtstruct.h>
#include <dcmtk/dcmrt/drttreat.h>
#include <dcmtk/dcmrt/drtionpl.h>
#include <dcmtk/dcmrt/drtiontr.h>

// Qt includes
#include <QSqlQuery>
#include <QSqlDatabase>
//#include <QSqlRecord>
//#include <QSqlError>
#include <QVariant>
#include <QStringList>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

// CTK includes
#include <ctkDICOMDatabase.h>

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::ROIStructureSetEntry::ROIStructureSetEntry()
{
  Number=0;
  DisplayColor[0]=1.0;
  DisplayColor[1]=0.0;
  DisplayColor[2]=0.0;
  PolyData=NULL;
}

vtkSlicerDicomRtReader::ROIStructureSetEntry::~ROIStructureSetEntry()
{
  SetPolyData(NULL);
}

vtkSlicerDicomRtReader::ROIStructureSetEntry::ROIStructureSetEntry(const ROIStructureSetEntry& src)
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

vtkSlicerDicomRtReader::ROIStructureSetEntry& vtkSlicerDicomRtReader::ROIStructureSetEntry::operator=(const ROIStructureSetEntry &src)
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

void vtkSlicerDicomRtReader::ROIStructureSetEntry::SetPolyData(vtkPolyData* roiPolyData)
{
  if (roiPolyData==PolyData)
  {
    // not changed
    return;
  }
  if (PolyData!=NULL)
  {
    PolyData->UnRegister(NULL);
  }
  PolyData=roiPolyData;
  if (PolyData!=NULL)
  {
    PolyData->Register(NULL);
  }
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtReader);

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkSlicerDicomRtReader()
{
  this->FileName = NULL;

  this->ROIContourSequencePolyData = NULL;
  this->SetPixelSpacing(0,0);
  this->DoseUnits = NULL;
  this->DoseGridScaling = NULL;

  this->LoadRTStructureSetSuccessful = false;
  this->LoadRTDoseSuccessful = false;
  this->LoadRTPlanSuccessful = false;
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
      //OFLOG_FATAL(drtdumpLogger, OFFIS_CONSOLE_APPLICATION << ": error (" << result.text()
      //    << ") reading file: " << ifname);
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
        BeamSequenceEntry beamEntry;

        OFString beamName;
        currentBeamSequenceObject.getBeamName(beamName);
        beamEntry.Name=beamName.c_str();

        OFString beamDescription;
        currentBeamSequenceObject.getBeamDescription(beamDescription);
        beamEntry.Description=beamDescription.c_str();

        Sint32 beamNumber;
        currentBeamSequenceObject.getBeamNumber(beamNumber);        
        beamEntry.Number = beamNumber;

        this->BeamSequenceVector.push_back(beamEntry);

        DRTControlPointSequence &rtControlPointSequenceObject = currentBeamSequenceObject.getControlPointSequence();
        if (rtControlPointSequenceObject.gotoFirstItem().good())
        {
          // do // comment out for now since only first control point (as isocenter)
          {
            DRTControlPointSequence::Item &controlPointItem = rtControlPointSequenceObject.getCurrentItem();
            if ( controlPointItem.isValid())
            {
              OFVector<Float64>  IsocenterPositionData_LPS;
              controlPointItem.getIsocenterPosition(IsocenterPositionData_LPS);
              // convert from DICOM LPS -> Slicer RAS
              beamEntry.IsocenterPosition[0] = -IsocenterPositionData_LPS[0];
              beamEntry.IsocenterPosition[1] = -IsocenterPositionData_LPS[1];
              beamEntry.IsocenterPosition[2] = IsocenterPositionData_LPS[2];
            }
          }
          // while (rtControlPointSequenceObject.gotoNextItem().good());
        }

      }
      while (rtPlaneBeamSequenceObject.gotoNextItem().good());
    }
  }

  this->LoadRTPlanSuccessful = true;
}

//----------------------------------------------------------------------------
OFString GetReferencedFrameOfReferenceSOPInstanceUID(DRTStructureSetIOD &rtStructureSetObject)
{
  OFString invalidUid;
  DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject.getReferencedFrameOfReferenceSequence();
  if (!rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
  {
    std::cerr << "No referenced frame of reference sequence object item is available" << std::endl;
    return invalidUid;
  }
  //do
  //{
  DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
  if (!currentReferencedFrameOfReferenceSequenceItem.isValid())
  {
    std::cerr << "Frame of reference sequence object item is invalid" << std::endl;
    return invalidUid;
  }
  // Sint32 ROINumber;
  DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
  if (!rtReferencedStudySequenceObject.gotoFirstItem().good())
  {
    std::cerr << "No referenced study sequence object item is available" << std::endl;
    return invalidUid;
  }
  //do
  //{
  DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
  if (!rtReferencedStudySequenceItem.isValid())
  {
    std::cerr << "Referenced study sequence object item is invalid" << std::endl;
    return invalidUid;
  }
  //rtReferencedStudySequenceItem.getReferencedSOPInstanceUID(ReferencedSOPInstanceUID);
  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
  if (!rtReferencedSeriesSequenceObject.gotoFirstItem().good())
  {
    std::cerr << "No referenced series sequence object item is available" << std::endl;
    return invalidUid;
  }
  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject.getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    std::cerr << "Referenced series sequence object item is invalid" << std::endl;
    return invalidUid;
  }
  DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
  if (!rtContourImageSequenceObject.gotoFirstItem().good())
  {
    std::cerr << "No contour image sequence object item is available" << std::endl;
    return invalidUid;
  }
  DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
  if (!rtContourImageSequenceItem.isValid())
  {
    std::cerr << "Contour image sequence object item is invalid" << std::endl;
    return invalidUid;
  }
  OFString resultUid;
  rtContourImageSequenceItem.getReferencedSOPInstanceUID(resultUid);
  return resultUid;
}

//----------------------------------------------------------------------------
double GetSliceThickness(OFString referencedSOPInstanceUID)
{
  double defaultSliceThickness = 2.0;

  // Get DICOM image filename from SOP instance UID
  ctkDICOMDatabase dicomDatabase;
  QSettings settings;
  QString databaseDirectory = settings.value("DatabaseDirectory").toString();
  dicomDatabase.openDatabase(databaseDirectory + "/ctkDICOM.sql", "SlicerRt");
  QString referencedFilename=dicomDatabase.fileForInstance(referencedSOPInstanceUID.c_str());
  dicomDatabase.closeDatabase();
  if ( referencedFilename.isEmpty() ) //isNull?
  {
    std::cerr << "No referenced image file is found, default slice thickness is used for contour import" << std::endl;
    return defaultSliceThickness;
  }

  // Load DICOM file
  DcmFileFormat fileformat;
  OFCondition result;
  result = fileformat.loadFile( referencedFilename.toStdString().c_str(), EXS_Unknown);
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
  double sliceThickness = atof(sliceThicknessString.c_str());
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
double GetDistanceBetweenContourPlanes(DRTContourSequence &rtContourSequenceObject)
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
    int number = atoi(numberofpoints.c_str());
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
    ROIStructureSetEntry roiEntry;

    OFString roiName;
    currentROISequenceObject.getROIName(roiName);
    roiEntry.Name=roiName.c_str();

    OFString roiDescription;
    currentROISequenceObject.getROIDescription(roiDescription);
    roiEntry.Description=roiDescription.c_str();                   

    Sint32 roiNumber;
    currentROISequenceObject.getROINumber(roiNumber);
    roiEntry.Number=roiNumber;
    // cout << "roi number:" << ROINumber << " roi name:" << ROIName << " roi description:" << ROIDescription << OFendl;

    // save to vector          
    this->ROIContourSequenceVector.push_back(roiEntry);
  }
  while (rtStructureSetROISequenceObject.gotoNextItem().good());

  // Get the slice thickness from the referenced image
  OFString referencedSOPInstanceUID=GetReferencedFrameOfReferenceSOPInstanceUID(rtStructureSetObject);
  double sliceThickness = GetSliceThickness(referencedSOPInstanceUID);

  Sint32 referenceROINumber;
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

    currentRoiObject.getReferencedROINumber(referenceROINumber);
    //cout << "refence roi number:" << referenceROINumber << OFendl;
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
        int number = atoi(numberofpoints.c_str());

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

    } // if gotofirstitem

    // Save it into ROI vector
    ROIStructureSetEntry* referenceROI=FindROIByNumber(referenceROINumber);
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

      // Remove coincident points (if there are multiple
      // contour points at the same position then the
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

  this->LoadRTStructureSetSuccessful = true;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfROIs()
{
  return this->ROIContourSequenceVector.size();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetROINameByROINumber(unsigned int ROINumber)
{
  ROIStructureSetEntry* roi=FindROIByNumber(ROINumber);
  if (roi==NULL)
  {
    return NULL;
  }  
  return roi->Name.c_str();
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetROIByROINumber(unsigned int ROINumber)
{
  ROIStructureSetEntry* roi=FindROIByNumber(ROINumber);
  if (roi==NULL)
  {
    return NULL;
  }
  return roi->PolyData;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetROIDisplayColorByROINumber(unsigned int ROINumber)
{
  ROIStructureSetEntry* roi=FindROIByNumber(ROINumber);
  if (roi==NULL)
  {
    return NULL;
  }
  return roi->DisplayColor;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetROIName(unsigned int number)
{
  if (number >= this->ROIContourSequenceVector.size())
  {
    vtkWarningMacro("Cannot get roi with number: " << number);
    return NULL;
  }
  return this->ROIContourSequenceVector[number].Name.c_str();
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetROI(unsigned int number)
{
  if (number >= this->ROIContourSequenceVector.size())
  {
    vtkWarningMacro("Cannot get roi with number: " << number);
    return NULL;
  }
  return this->ROIContourSequenceVector[number].PolyData;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetROIDisplayColor(unsigned int number)
{
  if (number >= this->ROIContourSequenceVector.size())
  {
    vtkWarningMacro("Cannot get roi with number: " << number);
    return NULL;
  }
  return this->ROIContourSequenceVector[number].DisplayColor;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->BeamSequenceVector.size();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamName(unsigned int BeamNumber)
{
  BeamSequenceEntry* beam=FindBeamByNumber(BeamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->Name.c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamIsocenterPosition(unsigned int BeamNumber)
{
  BeamSequenceEntry* beam=FindBeamByNumber(BeamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->IsocenterPosition;
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
  OFString doseGridScaling=0;
  if (rtDoseObject.getDoseGridScaling(doseGridScaling).bad())
  {
    cerr << "Error: Failed to get Dose Grid Scaling for dose object" << OFendl;
    return; // mandatory DICOM value
  }
  this->SetDoseGridScaling(doseGridScaling.c_str());

  OFString doseUnits=0;
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
  cout << "Pixel Spacing: (" << pixelSpacingOFVector[0] << ", " << pixelSpacingOFVector[1] << ")" << OFendl;

  this->SetPixelSpacing(pixelSpacingOFVector[0], pixelSpacingOFVector[1]);

  this->LoadRTDoseSuccessful = true;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::BeamSequenceEntry* vtkSlicerDicomRtReader::FindBeamByNumber(unsigned int beamNumber)
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
vtkSlicerDicomRtReader::ROIStructureSetEntry* vtkSlicerDicomRtReader::FindROIByNumber(unsigned int roiNumber)
{
  for (unsigned int i=0; i<this->ROIContourSequenceVector.size(); i++)
  {
    if (this->ROIContourSequenceVector[i].Number == roiNumber)
    {
      return &this->ROIContourSequenceVector[i];
    }
  }
  // not found
  return NULL;
}
