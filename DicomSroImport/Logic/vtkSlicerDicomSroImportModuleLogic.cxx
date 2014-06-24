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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


// DicomSroImport includes
#include "vtkSlicerDicomSroImportModuleLogic.h"
#include "vtkSlicerDicomSroReader.h"
#include "vtkDICOMImportInfo.h"

// Slicer Logic includes
#include "vtkSlicerVolumesLogic.h"

// DCMTK includes
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofcond.h>
#include <dcmtk/ofstd/ofstring.h>
#include <dcmtk/ofstd/ofstd.h>        /* for class OFStandard */

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLGridTransformNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkOrientedGridTransform.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomSroImportModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomSroImportModuleLogic::vtkSlicerDicomSroImportModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerDicomSroImportModuleLogic::~vtkSlicerDicomSroImportModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomSroImportModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerDicomSroImportModuleLogic::RegisterNodes()
{
}

//---------------------------------------------------------------------------
void vtkSlicerDicomSroImportModuleLogic::Examine(vtkDICOMImportInfo *importInfo)
{
  importInfo->RemoveAllLoadables();

  for (int fileListIndex=0; fileListIndex<importInfo->GetNumberOfFileLists(); fileListIndex++)
  {
    vtkStringArray *fileList=importInfo->GetFileList(fileListIndex);
    for (int fileIndex=0; fileIndex<fileList->GetNumberOfValues(); fileIndex++)
    {
      DcmFileFormat fileformat;

      vtkStdString fileName=fileList->GetValue(fileIndex);
      OFCondition result;
      result = fileformat.loadFile(fileName.c_str(), EXS_Unknown);
      if (!result.good())
      {
        continue; // Failed to parse this file, skip it
      }
      DcmDataset *dataset = fileformat.getDataset();
      // Check SOP Class UID for one of the supported RT objects
      OFString sopClass;
      if (!dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() || sopClass.empty())
      {
        continue; // Failed to parse this file, skip it
      }    
      
      // DICOM parsing is successful, now check if the object is loadable 
      std::string name;
      std::string tooltip;
      std::string warning;
      bool selected=true;
      double confidence=0.9; // Almost sure, it's not 1.0 to allow user modules to override this importer

      OFString seriesNumber;
      dataset->findAndGetOFString(DCM_SeriesNumber, seriesNumber);
      if (!seriesNumber.empty())
      {
        name+=std::string(seriesNumber.c_str())+": ";
      }

      if (sopClass == UID_SpatialRegistrationStorage)
      {
        name+="SpatialReg";
        OFString instanceNumber;
        dataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
        OFString seriesDescription;
        dataset->findAndGetOFString(DCM_SeriesDescription, seriesDescription);
        if (!seriesDescription.empty())
        {
          name+=std::string(": ")+seriesDescription.c_str(); 
        }
        if (!instanceNumber.empty())
        {
          name+=std::string(" [")+instanceNumber.c_str()+"]"; 
        }
      }
      else if (sopClass == UID_SpatialFiducialsStorage)
      {
        name+="SpatialFid";
        OFString instanceNumber;
        dataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
        OFString seriesDescription;
        dataset->findAndGetOFString(DCM_SeriesDescription, seriesDescription);
        if (!seriesDescription.empty())
        {
          name+=std::string(": ")+seriesDescription.c_str(); 
        }
        if (!instanceNumber.empty())
        {
          name+=std::string(" [")+instanceNumber.c_str()+"]"; 
        }
      }
      else if (sopClass == UID_DeformableSpatialRegistrationStorage)
      {
        name+="DeformableReg";
        OFString instanceNumber;
        dataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
        OFString seriesDescription;
        dataset->findAndGetOFString(DCM_SeriesDescription, seriesDescription);
        if (!seriesDescription.empty())
        {
          name+=std::string(": ")+seriesDescription.c_str(); 
        }
        if (!instanceNumber.empty())
        {
          name+=std::string(" [")+instanceNumber.c_str()+"]"; 
        }
      }
      else
      {
        continue; // not an registration object
      }

      // The object is stored in a single file
      vtkSmartPointer<vtkStringArray> loadableFileList=vtkSmartPointer<vtkStringArray>::New();
      loadableFileList->InsertNextValue(fileName);
     
      importInfo->InsertNextLoadable(loadableFileList, name.c_str(), tooltip.c_str(), warning.c_str(), selected, confidence);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportModuleLogic::LoadDicomSro(vtkDICOMImportInfo *loadInfo)
{
  bool loadSuccessful = false;
  if (!loadInfo || !loadInfo->GetLoadableFiles(0) || loadInfo->GetLoadableFiles(0)->GetNumberOfValues() < 1)
  {
    vtkErrorMacro("LoadDICOMSpatialRegistration: Unable to load DICOM REG data due to invalid loadable information.");
    return loadSuccessful;
  }

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  vtkSmartPointer<vtkSlicerDicomSroReader> spatialRegistrationReader = vtkSmartPointer<vtkSlicerDicomSroReader>::New();
  spatialRegistrationReader->SetFileName(firstFileNameStr.c_str());
  spatialRegistrationReader->Update();

  // Spatial registration
  if (spatialRegistrationReader->GetLoadSpatialRegistrationSuccessful())
  {
    loadSuccessful = this->LoadSpatialRegistration(spatialRegistrationReader, loadInfo);
  }

  // Spatial fiducials
  if (spatialRegistrationReader->GetLoadSpatialFiducialsSuccessful())
  {
    loadSuccessful = this->LoadSpatialFiducials(spatialRegistrationReader, loadInfo);
  }

  // Deformable spatial registration
  if (spatialRegistrationReader->GetLoadDeformableSpatialRegistrationSuccessful())
  {
    loadSuccessful = this->LoadDeformableSpatialRegistration(spatialRegistrationReader, loadInfo);
  }

  return loadSuccessful;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportModuleLogic::LoadSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo *loadInfo)
{
  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  vtkSmartPointer<vtkMatrix4x4> regMatrix = NULL;
  regMatrix = regReader->GetSpatialRegistrationMatrix();

  // Add transform node
  vtkSmartPointer<vtkMRMLLinearTransformNode> spatialTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  spatialTransformNode->SetScene(this->GetMRMLScene());
  spatialTransformNode->SetDisableModifiedEvent(1);
  std::string spatialTransformNodeName;
  spatialTransformNodeName = std::string(seriesName);
  spatialTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(spatialTransformNodeName+"_SpatialRegistration");
  spatialTransformNode->SetName(spatialTransformNodeName.c_str());
  spatialTransformNode->HideFromEditorsOff();
  spatialTransformNode->SetMatrixTransformToParent(regMatrix);
  spatialTransformNode->SetDisableModifiedEvent(0);

  this->GetMRMLScene()->AddNode(spatialTransformNode);

  return true;
}

// TODO: not implemented yet...
//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportModuleLogic::LoadSpatialFiducials(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo *loadInfo)
{
  return false;
}

// TODO: since currently vtkMRMLGridTransformNode cannot handle orientation, this method
//       generate two outputs, one is GridTransformNode without orientation, 
//       the other is a vectorVolumeNode with orientation.
//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportModuleLogic::LoadDeformableSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo *loadInfo)
{
  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  // Post deformation node
  vtkMatrix4x4* postDeformationMatrix = NULL;
  postDeformationMatrix = regReader->GetPostDeformationRegistrationMatrix();

  // Add post deformation transform node
  vtkSmartPointer<vtkMRMLLinearTransformNode> spatialPostTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  spatialPostTransformNode->SetScene(this->GetMRMLScene());
  spatialPostTransformNode->SetDisableModifiedEvent(1);
  std::string spatialPostTransformNodeName;
  spatialPostTransformNodeName = std::string(seriesName);
  spatialPostTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(spatialPostTransformNodeName+"_PostDeformationMatrix");
  spatialPostTransformNode->SetName(spatialPostTransformNodeName.c_str());
  spatialPostTransformNode->HideFromEditorsOff();
  spatialPostTransformNode->SetAndObserveMatrixTransformToParent(postDeformationMatrix);
  spatialPostTransformNode->SetDisableModifiedEvent(0);
  this->GetMRMLScene()->AddNode(spatialPostTransformNode);

  // Deformable grid vector image
  vtkImageData* deformableRegistrationGrid = NULL;
  deformableRegistrationGrid = regReader->GetDeformableRegistrationGrid();

  // vtkOrientedGridTransform
  vtkSmartPointer<vtkOrientedGridTransform> gridTransform = vtkSmartPointer<vtkOrientedGridTransform>::New();
#if (VTK_MAJOR_VERSION <= 5)
  gridTransform->SetDisplacementGrid(deformableRegistrationGrid);
#else
  gridTransform->SetDisplacementGridData(deformableRegistrationGrid);
#endif
  gridTransform->SetDisplacementScale(1);
  gridTransform->SetDisplacementShift(0);
  gridTransform->SetInterpolationModeToLinear();

  // Post deformation node
  vtkMatrix4x4* gridOrientationMatrix = NULL;
  gridOrientationMatrix = regReader->GetDeformableRegistrationGridOrientationMatrix();

  gridTransform->SetGridDirectionMatrix(gridOrientationMatrix);

  // Add grid transform node
  vtkSmartPointer<vtkMRMLGridTransformNode> deformableRegistrationGridTransformNode = vtkSmartPointer<vtkMRMLGridTransformNode>::New();
  deformableRegistrationGridTransformNode->SetScene(this->GetMRMLScene());
  deformableRegistrationGridTransformNode->SetDisableModifiedEvent(1);
  std::string deformableRegistrationGridTransformNodeName;
  deformableRegistrationGridTransformNodeName = std::string(seriesName);
  deformableRegistrationGridTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(deformableRegistrationGridTransformNodeName+"_DeformableRegistrationGrid");
  deformableRegistrationGridTransformNode->SetName(deformableRegistrationGridTransformNodeName.c_str());
  deformableRegistrationGridTransformNode->HideFromEditorsOff();
#ifdef TRANSFORM_NODE_MATRIX_COPY_REQUIRED
  deformableRegistrationGridTransformNode->SetAndObserveTransformToParent(gridTransform); 
#else
  deformableRegistrationGridTransformNode->SetAndObserveWarpTransformToParent(gridTransform, true); 
#endif
  deformableRegistrationGridTransformNode->SetDisableModifiedEvent(0);
  this->GetMRMLScene()->AddNode(deformableRegistrationGridTransformNode);

  return true;
}
