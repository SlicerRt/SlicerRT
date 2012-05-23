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
  if (ROIContourSequenceVector.size() > 0)
  {
    for(unsigned int i=0; i< ROIContourSequenceVector.size();i++)
    {
      delete this->ROIContourSequenceVector[i]->ROIName;
      this->ROIContourSequenceVector[i]->ROIName=NULL;
      if (this->ROIContourSequenceVector[i]->ROIPolyData!=NULL)
      {
        this->ROIContourSequenceVector[i]->ROIPolyData->Delete();
        this->ROIContourSequenceVector[i]->ROIPolyData=NULL;
      }
      delete this->ROIContourSequenceVector[i];
      this->ROIContourSequenceVector[i]=NULL;
    }
  }

  if (BeamSequenceVector.size() > 0)
  {
    for(unsigned int i=0; i < BeamSequenceVector.size();i++)
    {
      delete this->BeamSequenceVector[i]->BeamName;
      this->BeamSequenceVector[i]->BeamName=NULL;
      delete this->BeamSequenceVector[i];
      this->BeamSequenceVector[i]=NULL;
    }
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
  std::cerr << "here!" << std::endl;
  if (result.good())
  {
    OFString tmpString, dummyString;
    cout << "RT Plan object" << OFendl << OFendl;

    DRTBeamSequence &rtPlaneBeamSequenceObject = rtPlanObject.getBeamSequence();
    if (rtPlaneBeamSequenceObject.gotoFirstItem().good())
    {
      do
      {
        DRTBeamSequence::Item &currentBeamSequenceObject = rtPlaneBeamSequenceObject.getCurrentItem();  
        if (currentBeamSequenceObject.isValid())
        {
          OFString BeamName;
          OFString BeamDescription;
          currentBeamSequenceObject.getBeamName(BeamName);
          currentBeamSequenceObject.getBeamDescription(BeamDescription);

          Sint32 BeamNumber;
          currentBeamSequenceObject.getBeamNumber(BeamNumber);

          // add into vector
          BeamSequenceEntry* tempEntry = new BeamSequenceEntry();
          tempEntry->BeamName = new char[BeamName.size()+1];
          strcpy(tempEntry->BeamName, BeamName.c_str());
          tempEntry->BeamNumber = BeamNumber;
          BeamSequenceVector.push_back(tempEntry);

          DRTControlPointSequence &rtControlPointSequenceObject = currentBeamSequenceObject.getControlPointSequence();
          if (rtControlPointSequenceObject.gotoFirstItem().good())
          {
            // do // comment out for now since only first control point has isocenter
            {
              DRTControlPointSequence::Item &controlPointItem = rtControlPointSequenceObject.getCurrentItem();

              if ( controlPointItem.isValid())
              {
                OFVector<Float64>  IsocenterPositionData_LPS;
                controlPointItem.getIsocenterPosition(IsocenterPositionData_LPS);
                // convert from DICOM LPS -> Slicer RAS
                tempEntry->BeamIsocenterPosition[0] = -IsocenterPositionData_LPS[0];
                tempEntry->BeamIsocenterPosition[1] = -IsocenterPositionData_LPS[1];
                tempEntry->BeamIsocenterPosition[2] = IsocenterPositionData_LPS[2];
              }
            }
            // while (rtControlPointSequenceObject.gotoNextItem().good());
          }
        }
      }
      while (rtPlaneBeamSequenceObject.gotoNextItem().good());
    }
  }

  this->LoadRTPlanSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTStructureSet(DcmDataset* dataset)
{
  this->LoadRTStructureSetSuccessful = false;
  double SliceThickness = 1.1;

  DRTStructureSetIOD rtStructureSetObject;
  OFCondition result = rtStructureSetObject.read(*dataset);
  if (result.good())
  {
    OFString tmpString, dummyString;
    cout << "RT Structure Set object" << OFendl << OFendl;

    DRTStructureSetROISequence &rtStructureSetROISequenceObject = rtStructureSetObject.getStructureSetROISequence();
    if (rtStructureSetROISequenceObject.gotoFirstItem().good())
    {
      do
      {
        DRTStructureSetROISequence::Item &currentROISequenceObject = rtStructureSetROISequenceObject.getCurrentItem();
        if (currentROISequenceObject.isValid())
        {
          OFString ROIName;
          OFString ROIDescription;
          currentROISequenceObject.getROIName(ROIName);
          currentROISequenceObject.getROIDescription(ROIDescription);

          Sint32 ROINumber;
          currentROISequenceObject.getROINumber(ROINumber);
          // cout << "roi number:" << ROINumber << " roi name:" << ROIName << " roi description:" << ROIDescription << OFendl;
          // add into vector
          ROIStructureSetEntry* tempEntry = new ROIStructureSetEntry();
          tempEntry->ROIName = new char[ROIName.size()+1];
          strcpy(tempEntry->ROIName, ROIName.c_str());
          tempEntry->ROINumber = ROINumber;
          ROIContourSequenceVector.push_back(tempEntry);
        }
      }
      while (rtStructureSetROISequenceObject.gotoNextItem().good());
    }
    // cout << OFendl;

    OFString ReferencedSOPInstanceUID;
    DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject.getReferencedFrameOfReferenceSequence();
    if (rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
    {
      //do
      //{
        DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
        if (currentReferencedFrameOfReferenceSequenceItem.isValid())
        {
          // Sint32 ROINumber;
          DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
          if (rtReferencedStudySequenceObject.gotoFirstItem().good())
          {
            //do
            //{
              DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
              if (rtReferencedStudySequenceItem.isValid())
              {
                 //rtReferencedStudySequenceItem.getReferencedSOPInstanceUID(ReferencedSOPInstanceUID);
                 DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
                 if (rtReferencedSeriesSequenceObject.gotoFirstItem().good())
                 {
                   DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject.getCurrentItem();
                   if (rtReferencedSeriesSequenceItem.isValid())
                   {
                     DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
                     if (rtContourImageSequenceObject.gotoFirstItem().good())
                     {
                       DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
                       if (rtContourImageSequenceItem.isValid())
                       {
                         rtContourImageSequenceItem.getReferencedSOPInstanceUID(ReferencedSOPInstanceUID);
                       }
                     }
                   }
                 }
              }
            //}
            //while (rtReferencedStudySequenceObject.gotoNextItem().good());
          }
          
        }
      //}
      //while (rtReferencedFrameOfReferenceSequenceObject.gotoNextItem().good());
    }
    
    ctkDICOMDatabase dicomDatabase;
    QSettings settings;
    QString databaseDirectory = settings.value("DatabaseDirectory").toString();
    dicomDatabase.openDatabase(databaseDirectory + "/ctkDICOM.sql", "SLICER");
    QString query("SELECT Filename FROM Images WHERE SOPInstanceUID="); 
    QString uid(ReferencedSOPInstanceUID.c_str());
    QString quote("\"");
    QStringList resultStringList = dicomDatabase.runQuery(query + quote + uid + quote);
    dicomDatabase.closeDatabase();
    
    if ( !resultStringList.isEmpty() )
    {
      if ( !resultStringList.first().isNull() && !resultStringList.first().isEmpty() )
      {
        // load DICOM file or dataset
        DcmFileFormat fileformat;
    
        OFCondition result;
        result = fileformat.loadFile( resultStringList.first().toStdString().c_str(), EXS_Unknown);
        if (result.good())
        {
          DcmDataset *dataset = fileformat.getDataset();
          // from here use dicom toolkit to read slice thickness; wangk 2012/04/10
          OFString sliceThicknessString;
          if (dataset->findAndGetOFString(DCM_SliceThickness, sliceThicknessString).good())
          {
            SliceThickness = atof(sliceThicknessString.c_str());
            if (SliceThickness <= 0.0 || SliceThickness > 20.0)
            {
              SliceThickness = 1.1;
            }
          }
          else
          {
          }
        }
      }
    }
    
    Sint32 referenceROINumber;
    DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject.getROIContourSequence();
    if (rtROIContourSequenceObject.gotoFirstItem().good())
    {
      do 
      {
        DRTROIContourSequence::Item &currentROIObject = rtROIContourSequenceObject.getCurrentItem();

        if (currentROIObject.isValid())
        {
          // create vtkPolyData
          vtkSmartPointer<vtkPoints> tempPoints = vtkSmartPointer<vtkPoints>::New();
          vtkSmartPointer<vtkCellArray> tempCellArray = vtkSmartPointer<vtkCellArray>::New();
          vtkIdType pointId=0;

          currentROIObject.getReferencedROINumber(referenceROINumber);
          //cout << "refence roi number:" << referenceROINumber << OFendl;
          DRTContourSequence &rtContourSequenceObject = currentROIObject.getContourSequence();
          
          // Variables for estimating the distance between contour planes.
          // This is just a temporary solution, as it assumes that the plane normals are (0,0,1) and
          // the distance between all planes are equal.
          // TODO: Determine contour thickness from the thickness of the slice where it was drawn at (https://www.assembla.com/spaces/sparkit/tickets/49)
          double firstContourPlanePosition=0.0;
          double secondContourPlanePosition=0.0;
          int contourPlaneIndex=0;

          if (rtContourSequenceObject.gotoFirstItem().good())
          {
            do
            {
              DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();

              if ( contourItem.isValid())
              {
                OFString contourNumber;
                contourItem.getContourNumber(contourNumber);

                OFString numberofpoints;
                contourItem.getNumberOfContourPoints(numberofpoints);
                //cout << "\t contour number:" << contourNumber.c_str() << " numberOf points: "<< numberofpoints.c_str() << OFendl;
                int number = atoi(numberofpoints.c_str());

                OFVector<Float64>  contourData_LPS;
                contourItem.getContourData(contourData_LPS);

                if (number>=3)
                {
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
                }                

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
            }
            while (rtContourSequenceObject.gotoNextItem().good());
          } // if gotofirstitem

          vtkSmartPointer<vtkPolyData> tempPolyData = vtkSmartPointer<vtkPolyData>::New();
          tempPolyData->SetPoints(tempPoints);
          vtkPolyData* roiPolyData = NULL;
          if (tempPoints->GetNumberOfPoints() == 1)
          {
            tempPolyData->SetVerts(tempCellArray);
            roiPolyData = tempPolyData;
            roiPolyData->Register(this);
          }
          else if (tempPoints->GetNumberOfPoints() > 1)
          {
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
            ribbonFilter->SetWidth(SliceThickness); // take the slice thickness from dicom file 
                                       // assumption is that the slice thickness is constant (not varying)
            // ribbonFilter->SetWidth(1.1); // a reasonable default value that often works well
            if (contourPlaneIndex>=1)
            {
              // there were at least contour planes, therefore we have a valid distance estimation
              double distanceBetweenContourPlanes=fabs(firstContourPlanePosition-secondContourPlanePosition);
              // If the distance between the contour planes is too large then probably the contours should not be connected, so just keep using the default
              // TODO: this is totally heuristic, the actual thickness should be read from the referred slice thickness (as noted above)
              if (distanceBetweenContourPlanes<10.0/* mm*/)
              {
                ribbonFilter->SetWidth(distanceBetweenContourPlanes/2.0);
              }
            }
            ribbonFilter->SetAngle(90.0);
            ribbonFilter->UseDefaultNormalOn();
            ribbonFilter->Update();

            vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
            normalFilter->SetInputConnection(ribbonFilter->GetOutputPort());
            normalFilter->ConsistencyOn();
            normalFilter->Update();

            roiPolyData = normalFilter->GetOutput();
            roiPolyData->Register(this);
          }

          for (unsigned int i=0; i<this->ROIContourSequenceVector.size();i++)
          {
            if (referenceROINumber == this->ROIContourSequenceVector[i]->ROINumber)
            {
              // the ownership of the roiPolyData is now passed to the ROI contour sequence vector
              this->ROIContourSequenceVector[i]->ROIPolyData = roiPolyData;
              roiPolyData=NULL;

              Sint32 ROIDisplayColor;
              for (int j=0; j<3; j++)
              {
                currentROIObject.getROIDisplayColor(ROIDisplayColor,j);
                this->ROIContourSequenceVector[i]->ROIDisplayColor[j] = ROIDisplayColor/255.0;
              }
            }
          }

        } // if valid
      }
      while (rtROIContourSequenceObject.gotoNextItem().good());
    } // if gotofirstitem

    //cout << OFendl;

    this->LoadRTStructureSetSuccessful = true;
  }
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfROIs()
{
  return this->ROIContourSequenceVector.size();
}

//----------------------------------------------------------------------------
char* vtkSlicerDicomRtReader::GetROIName(int ROINumber)
{
  int id=-1;
  for (unsigned int i=0; i<this->ROIContourSequenceVector.size(); i++)
  {
    if (this->ROIContourSequenceVector[i]->ROINumber == ROINumber)
    {
      id = i;
    }
  }
  if ( id == -1)
  {
    return NULL;
  }
  return this->ROIContourSequenceVector[id]->ROIName;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetROI(int ROINumber)
{
  int id=-1;
  for (unsigned int i=0; i<this->ROIContourSequenceVector.size(); i++)
  {
    if (this->ROIContourSequenceVector[i]->ROINumber == ROINumber)
    {
      id = i;
    }
  }
  if ( id == -1)
  {
    return NULL;
  }
  return this->ROIContourSequenceVector[id]->ROIPolyData;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetROIDisplayColor(int ROINumber)
{
  int id=-1;
  for (unsigned int i=0; i<this->ROIContourSequenceVector.size(); i++)
  {
    if (this->ROIContourSequenceVector[i]->ROINumber == ROINumber)
    {
      id = i;
    }
  }
  if ( id == -1)
  {
    return NULL;
  }
  return this->ROIContourSequenceVector[id]->ROIDisplayColor;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->BeamSequenceVector.size();
}

//----------------------------------------------------------------------------
char* vtkSlicerDicomRtReader::GetBeamName(int BeamNumber)
{
  int id=-1;
  for (unsigned int i=0; i<this->BeamSequenceVector.size(); i++)
  {
    if (this->BeamSequenceVector[i]->BeamNumber == BeamNumber)
    {
      id = i;
    }
  }
  if ( id == -1)
  {
    return NULL;
  }
  return this->BeamSequenceVector[id]->BeamName;
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamIsocenterPosition(int BeamNumber)
{
  int id=-1;
  for (unsigned int i=0; i<this->BeamSequenceVector.size(); i++)
  {
    if (this->BeamSequenceVector[i]->BeamNumber == BeamNumber)
    {
      id = i;
    }
  }
  if ( id == -1)
  {
    return NULL;
  }
  return this->BeamSequenceVector[id]->BeamIsocenterPosition;
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
