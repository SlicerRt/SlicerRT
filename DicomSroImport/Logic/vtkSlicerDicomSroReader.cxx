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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Module includes
#include "vtkSlicerDicomSroReader.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

// STD includes
#include <cassert>

// DCMTK includes
#include <dcmtk/config/osconfig.h>    /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofconapp.h>
#include <dcmtk/dcmdata/dctk.h>

// Qt includes

// CTK includes
#include <ctkDICOMDatabase.h>

//----------------------------------------------------------------------------

const std::string vtkSlicerDicomSroReader::DICOMSROREADER_DICOM_DATABASE_FILENAME = "/ctkDICOM.sql";
const std::string vtkSlicerDicomSroReader::DICOMSROREADER_DICOM_CONNECTION_NAME = "Slicer";

vtkStandardNewMacro(vtkSlicerDicomSroReader);

//----------------------------------------------------------------------------
vtkSlicerDicomSroReader::vtkSlicerDicomSroReader()
{
  this->FileName = NULL;

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

  this->RegistrationMatrix = vtkMatrix4x4::New();

  this->LoadSpatialRegistrationSuccessful = false;
  this->LoadSpatialFiducialsSuccessful = false;
  this->LoadDeformableSpatialRegistrationSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerDicomSroReader::~vtkSlicerDicomSroReader()
{
  this->RegistrationMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::Update()
{

  this->RegistrationMatrix->Identity();

  if ((this->FileName != NULL) && (strlen(this->FileName) > 0))
  {
    // Set DICOM database file name
    QSettings settings;
    QString databaseDirectory = settings.value("DatabaseDirectory").toString();
    QString databaseFile = databaseDirectory + DICOMSROREADER_DICOM_DATABASE_FILENAME.c_str();
    this->SetDatabaseFile(databaseFile.toLatin1().constData());

    // Load DICOM file or dataset
    DcmFileFormat fileformat;

    OFCondition result;
    result = fileformat.loadFile(this->FileName, EXS_Unknown);
    if (result.good())
    {
      DcmDataset *dataset = fileformat.getDataset();

      // Check SOP Class UID for one of the supported RT objects
      OFString sopClass;
      if (dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() && !sopClass.empty())
      {
        if (sopClass == UID_SpatialRegistrationStorage)
        {
          this->LoadSpatialRegistration(dataset);
        }
        else if (sopClass == UID_SpatialFiducialsStorage)
        {
          this->LoadSpatialFiducials(dataset);
        }
        else if (sopClass == UID_DeformableSpatialRegistrationStorage)
        {
          this->LoadDeformableSpatialRegistration(dataset);  
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
void vtkSlicerDicomSroReader::LoadSpatialRegistration(DcmDataset* dataset)
{
  this->LoadSpatialRegistrationSuccessful = false; 
  
  DcmSequenceOfItems *registrationSequence = NULL;
  OFCondition result = dataset->findAndGetSequence(DCM_RegistrationSequence, registrationSequence);
  if (result.good())
  {
    OFString tmpString, dummyString;
    vtkDebugMacro("LoadSpatialRegistration: Load Spatial Registration object");

    unsigned long numOfRegistrationSequenceItems = registrationSequence->card();
    for(unsigned long i=0; i<numOfRegistrationSequenceItems; i++)
    {

      DcmItem *currentRegistrationSequenceItem = NULL;
      currentRegistrationSequenceItem  = registrationSequence->getItem(i);  
      
      if (currentRegistrationSequenceItem->isEmpty())
      {
        vtkDebugMacro("LoadSpatialRegistration: Found an invalid sequence in dataset");
        break;
      }

      DcmSequenceOfItems *matrixRegistrationSequence = NULL;
      OFCondition result = currentRegistrationSequenceItem->findAndGetSequence(DCM_MatrixRegistrationSequence, matrixRegistrationSequence);
      if (result.good())
      {
        unsigned long numOfMatrixRegistrationSequenceItems = matrixRegistrationSequence->card();
        for (unsigned long j=0; j<numOfMatrixRegistrationSequenceItems; j++)
        {
          DcmItem *currentmatrixRegistrationSequenceItem = NULL;
          currentmatrixRegistrationSequenceItem = matrixRegistrationSequence->getItem(j);
          
          if (currentmatrixRegistrationSequenceItem->isEmpty())
          {
            vtkDebugMacro("LoadSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }
          DcmSequenceOfItems *matrixSequence = NULL;
          OFCondition result = currentmatrixRegistrationSequenceItem->findAndGetSequence(DCM_MatrixSequence, matrixSequence);
          if (result.good())
          {
            unsigned long numOfMatrixSequenceItems = matrixSequence->card();
            for (unsigned long k=0; k<numOfMatrixSequenceItems; k++)
            {
              DcmItem *currentmatrixSequenceItem = NULL;
              currentmatrixSequenceItem = matrixSequence->getItem(k);
              
              if (currentmatrixRegistrationSequenceItem->isEmpty())
              {
                vtkDebugMacro("LoadSpatialRegistration: Found an invalid sequence in dataset");
                break;
              }
              OFString  matrixType;
              OFCondition result = currentmatrixSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrixType, matrixType);
              if (!result.good())
              {
                continue;
              }
              const Float64* matrixData = new Float64[16];
              unsigned long count = 0;
              result = currentmatrixSequenceItem->findAndGetFloat64Array(DCM_FrameOfReferenceTransformationMatrix, matrixData, &count, OFTrue);
              vtkSmartPointer<vtkMatrix4x4> tempMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
              OFString  matrixString;
              //unsigned long pos = 0;
              for (unsigned long n=0; n<16; n++)
              {
                result = currentmatrixSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
                tempMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
              }
              vtkSmartPointer<vtkMatrix4x4> tempMatrix2 = vtkSmartPointer<vtkMatrix4x4>::New();
              vtkMatrix4x4::Multiply4x4(this->RegistrationMatrix, tempMatrix, tempMatrix2);
              this->RegistrationMatrix->DeepCopy(tempMatrix2);
            } //
          }
        } // 
      }
    } // numOfRegistrationSequenceItems 
  }

  this->LoadSpatialRegistrationSuccessful = true;
}

// TODO: not implemented yet...
//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::LoadSpatialFiducials(DcmDataset* dataset)
{
  this->LoadSpatialFiducialsSuccessful = false; 
}

// TODO: not implemented yet...
//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::LoadDeformableSpatialRegistration(DcmDataset* dataset)
{
  this->LoadDeformableSpatialRegistrationSuccessful = false; 
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkSlicerDicomSroReader::GetRegistrationMatrix()
{
  return this->RegistrationMatrix;
}
