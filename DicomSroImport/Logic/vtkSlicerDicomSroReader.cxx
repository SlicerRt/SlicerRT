/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


// Module includes
#include "vtkSlicerDicomSroReader.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkVersion.h>

// SlicerRT includes
#include "SlicerRtCommon.h"

// DCMTK includes
#include <dcmtk/config/osconfig.h>    /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofconapp.h>
#include <dcmtk/dcmdata/dctk.h>

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

  this->SpatialRegistrationMatrix = vtkMatrix4x4::New();

  this->PostDeformationRegistrationMatrix = vtkMatrix4x4::New();
  this->DeformableRegistrationGrid = vtkImageData::New();
  this->DeformableRegistrationGridOrientationMatrix = vtkMatrix4x4::New();

  this->LoadSpatialRegistrationSuccessful = false;
  this->LoadSpatialFiducialsSuccessful = false;
  this->LoadDeformableSpatialRegistrationSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerDicomSroReader::~vtkSlicerDicomSroReader()
{
  this->SpatialRegistrationMatrix->Delete();
  this->PostDeformationRegistrationMatrix->Delete();
  this->DeformableRegistrationGrid->Delete();
  this->DeformableRegistrationGridOrientationMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::Update()
{

  this->SpatialRegistrationMatrix->Identity();
  this->PostDeformationRegistrationMatrix->Identity();
  this->DeformableRegistrationGridOrientationMatrix->Identity();

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
          vtkDebugMacro("vtkSlicerDicomSroReader::Update: unsupported SOPClassUID (" << sopClass);
        }
      } 
      else 
      {
        vtkDebugMacro("vtkSlicerDicomSroReader::Update: SOPClassUID (0008,0016) missing or empty in file");
      }
    } 
    else 
    {
      vtkDebugMacro("vtkSlicerDicomSroReader::Update: error reading file");
    }
  } 
  else 
  {
    vtkDebugMacro("vtkSlicerDicomSroReader::Update: invalid filename: <empty string>");
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::LoadSpatialRegistration(DcmDataset* dataset)
{
  this->LoadSpatialRegistrationSuccessful = false; 
  
  vtkSmartPointer<vtkMatrix4x4> invMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  invMatrix->Identity();
  invMatrix->SetElement(0,0,-1);
  invMatrix->SetElement(1,1,-1);
  vtkSmartPointer<vtkMatrix4x4> forMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  forMatrix->Identity();
  forMatrix->SetElement(0,0,-1);
  forMatrix->SetElement(1,1,-1);

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

              vtkSmartPointer<vtkMatrix4x4> tempMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
              OFString  matrixString;
              for (unsigned long n=0; n<16; n++)
              {
                result = currentmatrixSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
                tempMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
              }

              vtkSmartPointer<vtkMatrix4x4> tempMatrix2 = vtkSmartPointer<vtkMatrix4x4>::New();
              vtkMatrix4x4::Multiply4x4(this->SpatialRegistrationMatrix, tempMatrix, tempMatrix2);
              this->SpatialRegistrationMatrix->DeepCopy(tempMatrix2);
            } //
          }
        } // 
      }
    } // numOfRegistrationSequenceItems 
  }

  // Change to RAS system from DICOM LPS system
  vtkMatrix4x4::Multiply4x4(invMatrix, this->SpatialRegistrationMatrix, this->SpatialRegistrationMatrix);
  vtkMatrix4x4::Multiply4x4(this->SpatialRegistrationMatrix, forMatrix, this->SpatialRegistrationMatrix);

  this->LoadSpatialRegistrationSuccessful = true;
}

// TODO: not implemented yet...
//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::LoadSpatialFiducials(DcmDataset* dataset)
{
  UNUSED_VARIABLE(dataset);
  this->LoadSpatialFiducialsSuccessful = false; 
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::LoadDeformableSpatialRegistration(DcmDataset* dataset)
{
  this->LoadDeformableSpatialRegistrationSuccessful = false; 
  
  vtkSmartPointer<vtkMatrix4x4> invMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  invMatrix->Identity();
  invMatrix->SetElement(0,0,-1);
  invMatrix->SetElement(1,1,-1);
  vtkSmartPointer<vtkMatrix4x4> forMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  forMatrix->Identity();
  forMatrix->SetElement(0,0,-1);
  forMatrix->SetElement(1,1,-1);

  // Deformable registration sequence
  DcmSequenceOfItems *registrationSequence = NULL;
  OFCondition result = dataset->findAndGetSequence(DCM_DeformableRegistrationSequence, registrationSequence);
  if (result.good())
  {
    OFString tmpString, dummyString;
    vtkDebugMacro("LoadDeformableSpatialRegistration: Load Deformable Spatial Registration object");

    unsigned long numOfDeformableRegistrationSequenceItems = registrationSequence->card();
    for(unsigned long i=0; i<numOfDeformableRegistrationSequenceItems; i++)
    {

      DcmItem *currentDeformableRegistrationSequenceItem = NULL;
      currentDeformableRegistrationSequenceItem  = registrationSequence->getItem(i);  
      
      if (currentDeformableRegistrationSequenceItem->isEmpty())
      {
        vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
        break;
      }

      // Pre-Deformation matrix registration sequence
      vtkSmartPointer<vtkMatrix4x4> preDeformationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      DcmSequenceOfItems *preDeformationMatrixRegistrationSequence = NULL;
      OFCondition result = currentDeformableRegistrationSequenceItem->findAndGetSequence(DCM_PreDeformationMatrixRegistrationSequence, preDeformationMatrixRegistrationSequence);
      if (result.good())
      {
        unsigned long numOfPreDeformationMMatrixRegistrationSequenceItems = preDeformationMatrixRegistrationSequence->card();
        for (unsigned long j=0; j<numOfPreDeformationMMatrixRegistrationSequenceItems; j++)
        {
          DcmItem *preDeformationMatrixRegistrationSequenceItem = NULL;
          preDeformationMatrixRegistrationSequenceItem = preDeformationMatrixRegistrationSequence->getItem(j);
          
          if (preDeformationMatrixRegistrationSequenceItem->isEmpty())
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          OFString  matrixType;
          OFCondition result = preDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrixType, matrixType);
          if (!result.good())
          {
            continue;
          }
          const Float64* matrixData = new Float64[16];
          unsigned long count = 0;
          result = preDeformationMatrixRegistrationSequenceItem->findAndGetFloat64Array(DCM_FrameOfReferenceTransformationMatrix, matrixData, &count, OFTrue);

          OFString  matrixString;
          for (unsigned long n=0; n<16; n++)
          {
            result = preDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
            preDeformationMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
          }
        } // numOfMatrixRegistrationSequenceItems
      } // if 

      // Post Deformation matrix registration sequence
      DcmSequenceOfItems *postDeformationMatrixRegistrationSequence = NULL;
      result = currentDeformableRegistrationSequenceItem->findAndGetSequence(DCM_PostDeformationMatrixRegistrationSequence, postDeformationMatrixRegistrationSequence);
      if (result.good())
      {
        unsigned long numOfPostDeformationMMatrixRegistrationSequenceItems = postDeformationMatrixRegistrationSequence->card();
        for (unsigned long j=0; j<numOfPostDeformationMMatrixRegistrationSequenceItems; j++)
        {
          DcmItem *postDeformationMatrixRegistrationSequenceItem = NULL;
          postDeformationMatrixRegistrationSequenceItem = postDeformationMatrixRegistrationSequence->getItem(j);
          
          if (postDeformationMatrixRegistrationSequenceItem->isEmpty())
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          OFString  matrixType;
          OFCondition result = postDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrixType, matrixType);
          if (!result.good())
          {
            continue;
          }
          const Float64* matrixData = new Float64[16];
          unsigned long count = 0;
          result = postDeformationMatrixRegistrationSequenceItem->findAndGetFloat64Array(DCM_FrameOfReferenceTransformationMatrix, matrixData, &count, OFTrue);

          vtkSmartPointer<vtkMatrix4x4> postDeformationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
          OFString  matrixString;
          for (unsigned long n=0; n<16; n++)
          {
            result = postDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
            postDeformationMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
          }
          this->PostDeformationRegistrationMatrix->DeepCopy(postDeformationMatrix);
        } // numOfMatrixRegistrationSequenceItems
      } // if 

      // Change to RAS system from DICOM LPS system
      vtkMatrix4x4::Multiply4x4(invMatrix, this->PostDeformationRegistrationMatrix, this->PostDeformationRegistrationMatrix);
      vtkMatrix4x4::Multiply4x4(this->PostDeformationRegistrationMatrix, forMatrix, this->PostDeformationRegistrationMatrix);

      // Deformable registration grid sequence
      DcmSequenceOfItems *deformableRegistrationGridSequence = NULL;
      result = currentDeformableRegistrationSequenceItem->findAndGetSequence(DCM_DeformableRegistrationGridSequence, deformableRegistrationGridSequence);
      if (result.good())
      {
        unsigned long numOfDeformableRegistrationGridSequenceItems = deformableRegistrationGridSequence->card();
        for (unsigned long j=0; j<numOfDeformableRegistrationGridSequenceItems; j++)
        {
          DcmItem *deformableRegistrationGridSequenceItem = NULL;
          deformableRegistrationGridSequenceItem = deformableRegistrationGridSequence->getItem(j);
          
          if (deformableRegistrationGridSequenceItem->isEmpty())
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          // Image orientation patient
          double imageOrientationPatient[6];
          OFString tmpGridOrientationX0;
          OFString tmpGridOrientationX1;
          OFString tmpGridOrientationX2;
          OFString tmpGridOrientationY0;
          OFString tmpGridOrientationY1;
          OFString tmpGridOrientationY2;
          if (deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImageOrientationPatient, tmpGridOrientationX0, 0).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImageOrientationPatient, tmpGridOrientationX1, 1).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImageOrientationPatient, tmpGridOrientationX2, 2).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImageOrientationPatient, tmpGridOrientationY0, 3).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImageOrientationPatient, tmpGridOrientationY1, 4).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImageOrientationPatient, tmpGridOrientationY2, 5).good())
          {
            imageOrientationPatient[0] = atof(tmpGridOrientationX0.c_str());
            imageOrientationPatient[1] = atof(tmpGridOrientationX1.c_str());
            imageOrientationPatient[2] = atof(tmpGridOrientationX2.c_str());
            imageOrientationPatient[3] = atof(tmpGridOrientationY0.c_str());
            imageOrientationPatient[4] = atof(tmpGridOrientationY1.c_str());
            imageOrientationPatient[5] = atof(tmpGridOrientationY2.c_str());
          }
          else
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          // Image position patient
          double imagePositionPatient[3];
          OFString tmpGridPositionX;
          OFString tmpGridPositionY;
          OFString tmpGridPositionZ;
          if (deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImagePositionPatient, tmpGridPositionX, 0).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImagePositionPatient, tmpGridPositionY, 1).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_ImagePositionPatient, tmpGridPositionZ, 2).good())
          {
            imagePositionPatient[0] = atof(tmpGridPositionX.c_str());
            imagePositionPatient[1] = atof(tmpGridPositionY.c_str());
            imagePositionPatient[2] = atof(tmpGridPositionZ.c_str());
          }
          else
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          this->DeformableRegistrationGridOrientationMatrix->SetElement(0, 0, imageOrientationPatient[0]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(0, 1, imageOrientationPatient[1]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(0, 2, imageOrientationPatient[2]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(1, 0, imageOrientationPatient[3]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(1, 1, imageOrientationPatient[4]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(1, 2, imageOrientationPatient[5]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(2, 0, 0);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(2, 1, 0);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(2, 2, 1);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(0, 3, imagePositionPatient[0]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(1, 3, imagePositionPatient[1]);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(2, 3, imagePositionPatient[2]);

          unsigned int gridDimX, gridDimY, gridDimZ;
          double gridSpacingX, gridSpacingY, gridSpacingZ;

          // Grid dimension
          OFString tmpGridDimX;
          OFString tmpGridDimY;
          OFString tmpGridDimZ;
          if (deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_GridDimensions, tmpGridDimX, 0).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_GridDimensions, tmpGridDimY, 1).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_GridDimensions, tmpGridDimZ, 2).good())
          {
              gridDimX = static_cast<int>(atoi(tmpGridDimX.c_str()));
              gridDimY = static_cast<int>(atoi(tmpGridDimY.c_str()));
              gridDimZ = static_cast<int>(atoi(tmpGridDimZ.c_str()));
          }
          else
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          // Grid spacing
          OFString tmpGridSpacingX;
          OFString tmpGridSpacingY;
          OFString tmpGridSpacingZ;
          if (deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_GridResolution, tmpGridSpacingX, 0).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_GridResolution, tmpGridSpacingY, 1).good() &&
              deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_GridResolution, tmpGridSpacingZ, 2).good())
          {
            gridSpacingX = atof(tmpGridSpacingX.c_str());
            gridSpacingY = atof(tmpGridSpacingY.c_str());
            gridSpacingZ = atof(tmpGridSpacingZ.c_str());
          }
          else
          {
            vtkDebugMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          // Concatenate pre-matrix with orientation matrix to get the final orientation matrix
          vtkMatrix4x4::Multiply4x4(preDeformationMatrix, this->DeformableRegistrationGridOrientationMatrix, this->DeformableRegistrationGridOrientationMatrix);
          vtkMatrix4x4::Multiply4x4(invMatrix, this->DeformableRegistrationGridOrientationMatrix, this->DeformableRegistrationGridOrientationMatrix);

          // Get the offset from final orientation matrix to set the origin
          imagePositionPatient[0] = this->DeformableRegistrationGridOrientationMatrix->GetElement(0,3);
          imagePositionPatient[1] = this->DeformableRegistrationGridOrientationMatrix->GetElement(1,3);
          imagePositionPatient[2] = this->DeformableRegistrationGridOrientationMatrix->GetElement(2,3);

          this->DeformableRegistrationGridOrientationMatrix->SetElement(0,3,0);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(1,3,0);
          this->DeformableRegistrationGridOrientationMatrix->SetElement(2,3,0);

          // Grid vector
          OFString  tmpStrX;
          OFString  tmpStrY;
          OFString  tmpStrZ;
          unsigned long numOfVector = gridDimX * gridDimY * gridDimZ;
          this->DeformableRegistrationGrid->SetOrigin(imagePositionPatient[0], imagePositionPatient[1], imagePositionPatient[2]);
          this->DeformableRegistrationGrid->SetSpacing(gridSpacingX, gridSpacingY, gridSpacingZ);
          this->DeformableRegistrationGrid->SetExtent(0,gridDimX-1,0,gridDimY-1,0,gridDimZ-1);
#if (VTK_MAJOR_VERSION <= 5)
          this->DeformableRegistrationGrid->SetScalarTypeToDouble();
          this->DeformableRegistrationGrid->SetNumberOfScalarComponents(3);
	      this->DeformableRegistrationGrid->AllocateScalars();
#else
	      this->DeformableRegistrationGrid->AllocateScalars(VTK_DOUBLE, 3);
#endif
          unsigned int n = 0;
          for (unsigned int k=0; k<gridDimZ; k++)
          {
            for (unsigned int j=0; j<gridDimY; j++)
            {
              for (unsigned int i=0; i<gridDimX; i++)
              {
                n = i + j*gridDimX + k*gridDimX*gridDimY ;
                if (deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_VectorGridData, tmpStrX, 3*n).good() &&
                    deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_VectorGridData, tmpStrY, 3*n + 1).good() &&
                    deformableRegistrationGridSequenceItem->findAndGetOFString(DCM_VectorGridData, tmpStrZ, 3*n + 2).good())
                {
                  this->DeformableRegistrationGrid->SetScalarComponentFromDouble(i,j,k,0,-atof(tmpStrX.c_str()));
                  this->DeformableRegistrationGrid->SetScalarComponentFromDouble(i,j,k,1,-atof(tmpStrY.c_str()));
                  this->DeformableRegistrationGrid->SetScalarComponentFromDouble(i,j,k,2,atof(tmpStrZ.c_str()));
                }
                else
                {
                  continue;
                }
              }
            }
          }
        } // numOfMatrixRegistrationSequenceItems
      } // if 

    } // numOfRegistrationSequenceItems 
    this->LoadDeformableSpatialRegistrationSuccessful = true; 
  }
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkSlicerDicomSroReader::GetSpatialRegistrationMatrix()
{
  return this->SpatialRegistrationMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkSlicerDicomSroReader::GetPostDeformationRegistrationMatrix()
{
  return this->PostDeformationRegistrationMatrix;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSlicerDicomSroReader::GetDeformableRegistrationGrid()
{
  return this->DeformableRegistrationGrid;
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkSlicerDicomSroReader::GetDeformableRegistrationGridOrientationMatrix()
{
  return this->DeformableRegistrationGridOrientationMatrix;
}

