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

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkVersion.h>

// DCMTK includes
#include <dcmtk/config/osconfig.h>    /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofconapp.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmiod/modpatient.h>
#include <dcmtk/dcmiod/modgeneralstudy.h>
#include <dcmtk/dcmiod/modgeneralseries.h>
#include <dcmtk/dcmiod/modsopcommon.h>

// Qt includes
#include <QSettings>

vtkStandardNewMacro(vtkSlicerDicomSroReader);

//----------------------------------------------------------------------------
class vtkSlicerDicomSroReader::vtkInternal
{
public:
  vtkInternal(vtkSlicerDicomSroReader* external);
  ~vtkInternal();

  /// Utility function to load referenced series information
  void LoadReferencedSeriesUIDs(DcmDataset*);

public:
  vtkSlicerDicomSroReader* External;

  std::vector<std::string> ReferencedSeriesUids;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//----------------------------------------------------------------------------
vtkSlicerDicomSroReader::vtkInternal::vtkInternal(vtkSlicerDicomSroReader* external)
  : External(external)
{
  this->ReferencedSeriesUids.clear();
}

//----------------------------------------------------------------------------
vtkSlicerDicomSroReader::vtkInternal::~vtkInternal()
{
  this->ReferencedSeriesUids.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::vtkInternal::LoadReferencedSeriesUIDs(DcmDataset* dataset)
{
  this->ReferencedSeriesUids.clear();

  DcmSequenceOfItems* referencedSeriesSequence = nullptr;
  OFCondition refSeqQueryResult = dataset->findAndGetSequence(DCM_ReferencedSeriesSequence, referencedSeriesSequence);
  if (!refSeqQueryResult.good())
  {
    vtkErrorWithObjectMacro(this->External, "LoadReferencedSeriesUIDs: Unable to find referenced series sequence");
    return;
  }

  unsigned long numOfReferencedSequenceItems = referencedSeriesSequence->card();
  for (unsigned long i = 0; i < numOfReferencedSequenceItems; i++)
  {
    DcmItem* currentReferencedSequenceItem = nullptr;
    currentReferencedSequenceItem = referencedSeriesSequence->getItem(i);
    if (currentReferencedSequenceItem->isEmpty())
    {
      vtkWarningWithObjectMacro(this->External, "LoadReferencedSeriesUIDs: Found an invalid sequence in dataset");
      break;
    }

    OFString seriesInstanceUidStr;
    OFCondition uidQueryResult = currentReferencedSequenceItem->findAndGetOFString(DCM_SeriesInstanceUID, seriesInstanceUidStr, 0);
    if (uidQueryResult.good())
    {
      this->ReferencedSeriesUids.push_back(seriesInstanceUidStr.c_str());
    }
  }
}


//----------------------------------------------------------------------------
// vtkSlicerDicomSroReader methods

//----------------------------------------------------------------------------
vtkSlicerDicomSroReader::vtkSlicerDicomSroReader()
{
  this->Internal = new vtkInternal(this);

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
  if (this->Internal)
  {
    delete this->Internal;
    this->Internal = nullptr;
  }

  if (this->SpatialRegistrationMatrix)
  {
    this->SpatialRegistrationMatrix->Delete();
    this->SpatialRegistrationMatrix = nullptr;
  }
  if (this->PostDeformationRegistrationMatrix)
  {
    this->PostDeformationRegistrationMatrix->Delete();
    this->PostDeformationRegistrationMatrix = nullptr;
  }
  if (this->DeformableRegistrationGrid)
  {
    this->DeformableRegistrationGrid->Delete();
    this->DeformableRegistrationGrid = nullptr;
  }
  if (this->DeformableRegistrationGridOrientationMatrix)
  {
    this->DeformableRegistrationGridOrientationMatrix->Delete();
    this->DeformableRegistrationGridOrientationMatrix = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkSlicerDicomSroReader::GetNumberOfReferencedSeriesUids()
{
  return this->Internal->ReferencedSeriesUids.size();
}

//----------------------------------------------------------------------------
std::string vtkSlicerDicomSroReader::GetReferencedSeriesUid(int index)
{
  if (index < 0 || index >= this->Internal->ReferencedSeriesUids.size())
  {
    vtkErrorMacro("GetReferencedSeriesUid: Invalid index");
    return "";
  }

  return this->Internal->ReferencedSeriesUids[index];
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::Update()
{
  this->SpatialRegistrationMatrix->Identity();
  this->PostDeformationRegistrationMatrix->Identity();
  this->DeformableRegistrationGridOrientationMatrix->Identity();

  if ((this->FileName != nullptr) && (strlen(this->FileName) > 0))
  {
    // Set DICOM database file name
    //TODO: Get rid of Qt code
    QSettings settings;
    QString databaseDirectory = settings.value("DatabaseDirectory").toString();
    QString databaseFile = databaseDirectory + DICOMREADER_DICOM_DATABASE_FILENAME.c_str();
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
        else if (sopClass == UID_DeformableSpatialRegistrationStorage)
        {
          this->LoadDeformableSpatialRegistration(dataset);  
        }
        else if (sopClass == UID_SpatialFiducialsStorage)
        {
          this->LoadSpatialFiducials(dataset);
        }
        else
        {
          vtkWarningMacro("vtkSlicerDicomSroReader::Update: unsupported SOPClassUID (" << sopClass);
        }
      } 
      else 
      {
        vtkWarningMacro("vtkSlicerDicomSroReader::Update: SOPClassUID (0008,0016) missing or empty in file");
      }
    } 
    else 
    {
      vtkErrorMacro("vtkSlicerDicomSroReader::Update: error reading file");
    }
  } 
  else 
  {
    vtkErrorMacro("vtkSlicerDicomSroReader::Update: invalid filename: <empty string>");
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

  // Load registration matrices
  DcmSequenceOfItems *registrationSequence = nullptr;
  OFCondition regSeqQueryResult = dataset->findAndGetSequence(DCM_RegistrationSequence, registrationSequence);
  if (regSeqQueryResult.good())
  {
    OFString tmpString, dummyString;
    vtkDebugMacro("LoadSpatialRegistration: Load Spatial Registration object");

    unsigned long numOfRegistrationSequenceItems = registrationSequence->card();
    for (unsigned long i=0; i<numOfRegistrationSequenceItems; i++)
    {
      DcmItem *currentRegistrationSequenceItem = nullptr;
      currentRegistrationSequenceItem  = registrationSequence->getItem(i);  
      
      if (currentRegistrationSequenceItem->isEmpty())
      {
        vtkWarningMacro("LoadSpatialRegistration: Found an invalid sequence in dataset");
        break;
      }

      DcmSequenceOfItems *matrixRegistrationSequence = nullptr;
      OFCondition regMtxQueryResult = currentRegistrationSequenceItem->findAndGetSequence(DCM_MatrixRegistrationSequence, matrixRegistrationSequence);
      if (regMtxQueryResult.good())
      {
        unsigned long numOfMatrixRegistrationSequenceItems = matrixRegistrationSequence->card();
        for (unsigned long j=0; j<numOfMatrixRegistrationSequenceItems; j++)
        {
          DcmItem *currentmatrixRegistrationSequenceItem = nullptr;
          currentmatrixRegistrationSequenceItem = matrixRegistrationSequence->getItem(j);
          
          if (currentmatrixRegistrationSequenceItem->isEmpty())
          {
            vtkWarningMacro("LoadSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }
          DcmSequenceOfItems *matrixSequence = nullptr;
          OFCondition mtxQueryResult = currentmatrixRegistrationSequenceItem->findAndGetSequence(DCM_MatrixSequence, matrixSequence);
          if (mtxQueryResult.good())
          {
            unsigned long numOfMatrixSequenceItems = matrixSequence->card();
            for (unsigned long k=0; k<numOfMatrixSequenceItems; k++)
            {
              DcmItem *currentmatrixSequenceItem = nullptr;
              currentmatrixSequenceItem = matrixSequence->getItem(k);
              
              if (currentmatrixRegistrationSequenceItem->isEmpty())
              {
                vtkWarningMacro("LoadSpatialRegistration: Found an invalid sequence in dataset");
                break;
              }
              OFString matrixType;
              OFCondition forTypeQueryResult = currentmatrixSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrixType, matrixType);
              if (!forTypeQueryResult.good())
              {
                continue;
              }

              vtkSmartPointer<vtkMatrix4x4> tempMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
              OFString matrixString;
              for (unsigned long n=0; n<16; n++)
              {
                currentmatrixSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
                tempMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
              }

              vtkSmartPointer<vtkMatrix4x4> tempMatrix2 = vtkSmartPointer<vtkMatrix4x4>::New();
              vtkMatrix4x4::Multiply4x4(this->SpatialRegistrationMatrix, tempMatrix, tempMatrix2);
              this->SpatialRegistrationMatrix->DeepCopy(tempMatrix2);
            } // for each matrix
          }
        } // for each matrix sequence
      }
    } // for each registration item 

    // Change to RAS system from DICOM LPS system
    vtkMatrix4x4::Multiply4x4(invMatrix, this->SpatialRegistrationMatrix, this->SpatialRegistrationMatrix);
    vtkMatrix4x4::Multiply4x4(this->SpatialRegistrationMatrix, forMatrix, this->SpatialRegistrationMatrix);

    this->LoadSpatialRegistrationSuccessful = true;
  } // Load registration sequence

  // Get SOP instance UID
  IODSOPCommonModule sop;
  OFString sopInstanceUid("");
  if (sop.read(*dataset).bad() || sop.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadSpatialRegistration: Failed to get SOP instance UID for spatial registration object");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  IODPatientModule patient;
  IODGeneralStudyModule study;
  IODGeneralSeriesModule series;
  if (patient.read(*dataset).good() && study.read(*dataset).good() && series.read(*dataset).good())
  {
    this->GetAndStorePatientInformation(&patient);
    this->GetAndStoreStudyInformation(&study);
    this->GetAndStoreSeriesInformation(&series);
  }

  // Load referenced series information
  this->Internal->LoadReferencedSeriesUIDs(dataset);
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
  DcmSequenceOfItems *registrationSequence = nullptr;
  OFCondition regSeqQueryResult = dataset->findAndGetSequence(DCM_DeformableRegistrationSequence, registrationSequence);
  if (regSeqQueryResult.good())
  {
    OFString tmpString, dummyString;
    vtkDebugMacro("LoadDeformableSpatialRegistration: Load Deformable Spatial Registration object");

    unsigned long numOfDeformableRegistrationSequenceItems = registrationSequence->card();
    for(unsigned long i=0; i<numOfDeformableRegistrationSequenceItems; i++)
    {
      DcmItem *currentDeformableRegistrationSequenceItem = nullptr;
      currentDeformableRegistrationSequenceItem  = registrationSequence->getItem(i);  
      
      if (currentDeformableRegistrationSequenceItem->isEmpty())
      {
        vtkWarningMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
        break;
      }

      // Pre-Deformation matrix registration sequence
      vtkSmartPointer<vtkMatrix4x4> preDeformationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      DcmSequenceOfItems *preDeformationMatrixRegistrationSequence = nullptr;
      OFCondition regQueryResult = currentDeformableRegistrationSequenceItem->findAndGetSequence(DCM_PreDeformationMatrixRegistrationSequence, preDeformationMatrixRegistrationSequence);
      if (regQueryResult.good())
      {
        unsigned long numOfPreDeformationMMatrixRegistrationSequenceItems = preDeformationMatrixRegistrationSequence->card();
        for (unsigned long j=0; j<numOfPreDeformationMMatrixRegistrationSequenceItems; j++)
        {
          DcmItem *preDeformationMatrixRegistrationSequenceItem = nullptr;
          preDeformationMatrixRegistrationSequenceItem = preDeformationMatrixRegistrationSequence->getItem(j);
          
          if (preDeformationMatrixRegistrationSequenceItem->isEmpty())
          {
            vtkWarningMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          OFString matrixType;
          OFCondition forTypeQueryResult = preDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrixType, matrixType);
          if (!forTypeQueryResult.good())
          {
            continue;
          }
          const Float64* matrixData = new Float64[16];
          unsigned long count = 0;
          preDeformationMatrixRegistrationSequenceItem->findAndGetFloat64Array(DCM_FrameOfReferenceTransformationMatrix, matrixData, &count, OFTrue);

          OFString matrixString;
          for (unsigned long n=0; n<16; n++)
          {
            preDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
            preDeformationMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
          }
        } // numOfMatrixRegistrationSequenceItems
      } // if 

      // Post Deformation matrix registration sequence
      DcmSequenceOfItems *postDeformationMatrixRegistrationSequence = nullptr;
      regSeqQueryResult = currentDeformableRegistrationSequenceItem->findAndGetSequence(DCM_PostDeformationMatrixRegistrationSequence, postDeformationMatrixRegistrationSequence);
      if (regSeqQueryResult.good())
      {
        unsigned long numOfPostDeformationMMatrixRegistrationSequenceItems = postDeformationMatrixRegistrationSequence->card();
        for (unsigned long j=0; j<numOfPostDeformationMMatrixRegistrationSequenceItems; j++)
        {
          DcmItem *postDeformationMatrixRegistrationSequenceItem = nullptr;
          postDeformationMatrixRegistrationSequenceItem = postDeformationMatrixRegistrationSequence->getItem(j);
          
          if (postDeformationMatrixRegistrationSequenceItem->isEmpty())
          {
            vtkWarningMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
            break;
          }

          OFString matrixType;
          OFCondition forTypeQueryResult = postDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrixType, matrixType);
          if (!forTypeQueryResult.good())
          {
            continue;
          }
          const Float64* matrixData = new Float64[16];
          unsigned long count = 0;
          postDeformationMatrixRegistrationSequenceItem->findAndGetFloat64Array(DCM_FrameOfReferenceTransformationMatrix, matrixData, &count, OFTrue);

          vtkSmartPointer<vtkMatrix4x4> postDeformationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
          OFString  matrixString;
          for (unsigned long n=0; n<16; n++)
          {
            postDeformationMatrixRegistrationSequenceItem->findAndGetOFString(DCM_FrameOfReferenceTransformationMatrix, matrixString, n);
            postDeformationMatrix->SetElement((int)(n/4), n%4, atof(matrixString.c_str()));
          }
          this->PostDeformationRegistrationMatrix->DeepCopy(postDeformationMatrix);
        } // numOfMatrixRegistrationSequenceItems
      } // if 

      // Change to RAS system from DICOM LPS system
      vtkMatrix4x4::Multiply4x4(invMatrix, this->PostDeformationRegistrationMatrix, this->PostDeformationRegistrationMatrix);
      vtkMatrix4x4::Multiply4x4(this->PostDeformationRegistrationMatrix, forMatrix, this->PostDeformationRegistrationMatrix);

      // Deformable registration grid sequence
      DcmSequenceOfItems *deformableRegistrationGridSequence = nullptr;
      regSeqQueryResult = currentDeformableRegistrationSequenceItem->findAndGetSequence(DCM_DeformableRegistrationGridSequence, deformableRegistrationGridSequence);
      if (regSeqQueryResult.good())
      {
        unsigned long numOfDeformableRegistrationGridSequenceItems = deformableRegistrationGridSequence->card();
        for (unsigned long j=0; j<numOfDeformableRegistrationGridSequenceItems; j++)
        {
          DcmItem *deformableRegistrationGridSequenceItem = nullptr;
          deformableRegistrationGridSequenceItem = deformableRegistrationGridSequence->getItem(j);
          
          if (deformableRegistrationGridSequenceItem->isEmpty())
          {
            vtkWarningMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
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
            vtkWarningMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
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
            vtkWarningMacro("LoadDeformableSpatialRegistration: Found an invalid sequence in dataset");
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
          OFString tmpStrX;
          OFString tmpStrY;
          OFString tmpStrZ;
          this->DeformableRegistrationGrid->SetOrigin(imagePositionPatient[0], imagePositionPatient[1], imagePositionPatient[2]);
          this->DeformableRegistrationGrid->SetSpacing(gridSpacingX, gridSpacingY, gridSpacingZ);
          this->DeformableRegistrationGrid->SetExtent(0,gridDimX-1,0,gridDimY-1,0,gridDimZ-1);
          this->DeformableRegistrationGrid->AllocateScalars(VTK_DOUBLE, 3);

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
  } // Load registration sequence

  // Get SOP instance UID
  IODSOPCommonModule sop;
  OFString sopInstanceUid("");
  if (sop.read(*dataset).bad() || sop.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadDeformableSpatialRegistration: Failed to get SOP instance UID for spatial registration object");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  IODPatientModule patient;
  IODGeneralStudyModule study;
  IODGeneralSeriesModule series;
  if (patient.read(*dataset).good() && study.read(*dataset).good() && series.read(*dataset).good())
  {
    this->GetAndStorePatientInformation(&patient);
    this->GetAndStoreStudyInformation(&study);
    this->GetAndStoreSeriesInformation(&series);
  }

  // Load referenced series information
  this->Internal->LoadReferencedSeriesUIDs(dataset);

}

// TODO: not implemented yet...
//----------------------------------------------------------------------------
void vtkSlicerDicomSroReader::LoadSpatialFiducials(DcmDataset* dataset)
{
  UNUSED_VARIABLE(dataset);
  this->LoadSpatialFiducialsSuccessful = false;
  vtkErrorMacro("LoadSpatialFiducials: Not yet implemented");
}
