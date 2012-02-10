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
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkRibbonFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkSmartPointer.h"

// STD includes
#include <cassert>
#include <vector>

// DCMTK includes
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/ofstd/ofconapp.h"

#include "dcmtk/dcmrt/drtdose.h"
#include "dcmtk/dcmrt/drtimage.h"
#include "dcmtk/dcmrt/drtplan.h"
#include "dcmtk/dcmrt/drtstruct.h"
#include "dcmtk/dcmrt/drttreat.h"
#include "dcmtk/dcmrt/drtionpl.h"
#include "dcmtk/dcmrt/drtiontr.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtReader);

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkSlicerDicomRtReader()
{
  this->FileName = NULL;

  this->ROIContourSequencePolyData = NULL;
  this->SetPixelSpacing(0,0);

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
      this->ROIContourSequenceVector[i]->ROIPolyData->Delete();
      delete this->ROIContourSequenceVector[i];
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
    /* load DICOM file or dataset */
    DcmFileFormat fileformat;

    OFCondition result;
    result = fileformat.loadFile(this->FileName, EXS_Unknown);
    if (result.good())
    {
      DcmDataset *dataset = fileformat.getDataset();
      /* check SOP Class UID for one of the supported RT objects */
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
          //result = dumpRTPlan(out, *dataset);
          cout << "RT Plan loading not implemented - not loaded" << OFendl;
          this->LoadRTPlanSuccessful = true; // Do the real loading here
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
void vtkSlicerDicomRtReader::LoadRTStructureSet(DcmDataset* dataset)
{
  this->LoadRTStructureSetSuccessful = false;

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
          cout << "roi number:" << ROINumber << " roi name:" << ROIName << " roi description:" << ROIDescription << OFendl;
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
    cout << OFendl;

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
					vtkSmartPointer<vtkPolyData> tempPolyData = vtkSmartPointer<vtkPolyData>::New();
					vtkSmartPointer<vtkPoints> tempPoints = vtkSmartPointer<vtkPoints>::New();
					vtkSmartPointer<vtkCellArray> tempCellArray = vtkSmartPointer<vtkCellArray>::New();
          vtkIdType pointId=0;

          currentROIObject.getReferencedROINumber(referenceROINumber);
          cout << "refence roi number:" << referenceROINumber << OFendl;
          DRTContourSequence &rtContourSequenceObject = currentROIObject.getContourSequence();
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
                cout << "\t contour number:" << contourNumber.c_str() << " numberOf points: "<< numberofpoints.c_str() << OFendl;
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
            }
            while (rtContourSequenceObject.gotoNextItem().good());
          } // if gotofirstitem

          tempPolyData->SetPoints(tempPoints);
          if (tempPoints->GetNumberOfPoints() == 1)
          {
            tempPolyData->SetVerts(tempCellArray);
          }
          else if (tempPoints->GetNumberOfPoints() > 1)
          {
            tempPolyData->SetLines(tempCellArray);
          }

          // convert to ribbon using vtkRibbonFilter
					vtkSmartPointer<vtkRibbonFilter> ribbonFilter = vtkSmartPointer<vtkRibbonFilter>::New();
					ribbonFilter->SetInput(tempPolyData);
					ribbonFilter->SetDefaultNormal(0,0,-1);
					ribbonFilter->SetWidth(1.1); // TODO: get this from the distance between the slices (https://www.assembla.com/spaces/sparkit/tickets/49)
					ribbonFilter->SetAngle(90.0);
					ribbonFilter->UseDefaultNormalOn();
					ribbonFilter->Update();

					vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
          normalFilter->SetInputConnection(ribbonFilter->GetOutputPort());
					normalFilter->ConsistencyOn();
					normalFilter->Update();

          for (unsigned int i=0; i<this->ROIContourSequenceVector.size();i++)
          {
            if (referenceROINumber == this->ROIContourSequenceVector[i]->ROINumber)
            {
              this->ROIContourSequenceVector[i]->ROIPolyData = normalFilter->GetOutput();
              this->ROIContourSequenceVector[i]->ROIPolyData->Register(0);

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

    cout << OFendl;

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

  OFVector<Float64> pixelSpacingOFVector;
  if (rtDoseObject.getPixelSpacing(pixelSpacingOFVector).bad() || pixelSpacingOFVector.size() < 2)
  {
    cerr << "Error: Failed to get Pixel Spacing for dose object" << OFendl;
    return;
  }
  cerr << "Pixel Spacing: (" << pixelSpacingOFVector[0] << ", " << pixelSpacingOFVector[1] << ")" << OFendl;

  this->SetPixelSpacing(pixelSpacingOFVector[0], pixelSpacingOFVector[1]);

  this->LoadRTDoseSuccessful = true;
}
