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


// DicomSroImportExport includes
#include "vtkSlicerDicomSroImportExportModuleLogic.h"
#include "vtkSlicerDicomSroReader.h"
#include "vtkDICOMImportInfo.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// DCMTK includes
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofcond.h>
#include <dcmtk/ofstd/ofstring.h>
#include <dcmtk/ofstd/ofstd.h> // for class OFStandard

// MRML includes
#include <vtkMRMLGridTransformNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedGridTransform.h>
#include <vtkStringArray.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomSroImportExportModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomSroImportExportModuleLogic::vtkSlicerDicomSroImportExportModuleLogic() = default;

//----------------------------------------------------------------------------
vtkSlicerDicomSroImportExportModuleLogic::~vtkSlicerDicomSroImportExportModuleLogic() = default;

//----------------------------------------------------------------------------
void vtkSlicerDicomSroImportExportModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerDicomSroImportExportModuleLogic::RegisterNodes()
{
}

//---------------------------------------------------------------------------
void vtkSlicerDicomSroImportExportModuleLogic::Examine(vtkDICOMImportInfo *importInfo)
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
      vtkNew<vtkStringArray> loadableFileList;
      loadableFileList->InsertNextValue(fileName);
     
      importInfo->InsertNextLoadable(loadableFileList, name.c_str(), tooltip.c_str(), warning.c_str(), selected, confidence);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportExportModuleLogic::LoadDicomSro(vtkDICOMImportInfo *loadInfo)
{
  if (!loadInfo || !loadInfo->GetLoadableFiles(0) || loadInfo->GetLoadableFiles(0)->GetNumberOfValues() < 1)
  {
    vtkErrorMacro("LoadDicomSro: Unable to load DICOM REG data due to invalid loadable information");
    return false;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("LoadDicomSro: Failed to access subject hierarchy node");
    return false;
  }

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);

  vtkNew<vtkSlicerDicomSroReader> spatialRegistrationReader;
  spatialRegistrationReader->SetFileName(firstFileNameStr.c_str());
  spatialRegistrationReader->Update();

  vtkMRMLLinearTransformNode* loadedLinearTransformNode = nullptr;
  vtkMRMLGridTransformNode* loadedGridTransformNode = nullptr;
  vtkMRMLTransformNode* loadedTransformNode = nullptr;

  // Spatial registration
  if (spatialRegistrationReader->GetLoadSpatialRegistrationSuccessful())
  {
    loadedLinearTransformNode = this->LoadSpatialRegistration(spatialRegistrationReader, loadInfo);
    if (loadedLinearTransformNode)
    {
      loadedTransformNode = loadedLinearTransformNode;
    }
  }
  // Deformable spatial registration
  if (spatialRegistrationReader->GetLoadDeformableSpatialRegistrationSuccessful())
  {
    loadedGridTransformNode = this->LoadDeformableSpatialRegistration(spatialRegistrationReader, loadInfo);
    if (loadedGridTransformNode)
    {
      loadedTransformNode = loadedGridTransformNode;
    }
  }

  // Add loaded transform under study of transformed image
  //TODO:sajt

  // Apply loaded transform to corresponding image
  //TODO: May change if decision is made on https://github.com/SlicerRt/SlicerRT/issues/108
  //TODO:sajt

  // Spatial fiducials
  //TODO: Function not implemented yet (also take into account for return value)
  //if (spatialRegistrationReader->GetLoadSpatialFiducialsSuccessful())
  //{
  //  loadSuccessful = this->LoadSpatialFiducials(spatialRegistrationReader, loadInfo);
  //}

  return (loadedTransformNode != nullptr);
}

//---------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerDicomSroImportExportModuleLogic::LoadSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo *loadInfo)
{
  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  vtkMatrix4x4* regMatrix = nullptr;
  regMatrix = regReader->GetSpatialRegistrationMatrix();

  // Add transform node
  vtkNew<vtkMRMLLinearTransformNode> spatialTransformNode;
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

  return spatialTransformNode;
}

// TODO: not implemented yet...
//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportExportModuleLogic::LoadSpatialFiducials(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo *loadInfo)
{
  UNUSED_VARIABLE(regReader);
  UNUSED_VARIABLE(loadInfo);
  return false;
}

//---------------------------------------------------------------------------
vtkMRMLGridTransformNode* vtkSlicerDicomSroImportExportModuleLogic::LoadDeformableSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo *loadInfo)
{
  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  // Post deformation node
  vtkMatrix4x4* postDeformationMatrix = nullptr;
  postDeformationMatrix = regReader->GetPostDeformationRegistrationMatrix();

  // Add post deformation transform node
  vtkNew<vtkMRMLLinearTransformNode> spatialPostTransformNode;
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
  vtkImageData* deformableRegistrationGrid = nullptr;
  deformableRegistrationGrid = regReader->GetDeformableRegistrationGrid();

  // vtkOrientedGridTransform
  vtkNew<vtkOrientedGridTransform> gridTransform;
  gridTransform->SetDisplacementGridData(deformableRegistrationGrid);
  gridTransform->SetDisplacementScale(1);
  gridTransform->SetDisplacementShift(0);
  gridTransform->SetInterpolationModeToLinear();

  // Post deformation node
  vtkMatrix4x4* gridOrientationMatrix = nullptr;
  gridOrientationMatrix = regReader->GetDeformableRegistrationGridOrientationMatrix();

  gridTransform->SetGridDirectionMatrix(gridOrientationMatrix);

  // Add grid transform node
  vtkNew<vtkMRMLGridTransformNode> deformableRegistrationGridTransformNode;
  deformableRegistrationGridTransformNode->SetScene(this->GetMRMLScene());
  deformableRegistrationGridTransformNode->SetDisableModifiedEvent(1);
  std::string deformableRegistrationGridTransformNodeName;
  deformableRegistrationGridTransformNodeName = std::string(seriesName);
  deformableRegistrationGridTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(deformableRegistrationGridTransformNodeName+"_DeformableRegistrationGrid");
  deformableRegistrationGridTransformNode->SetName(deformableRegistrationGridTransformNodeName.c_str());
  deformableRegistrationGridTransformNode->HideFromEditorsOff();
  deformableRegistrationGridTransformNode->SetAndObserveTransformToParent(gridTransform); 
  deformableRegistrationGridTransformNode->SetDisableModifiedEvent(0);
  this->GetMRMLScene()->AddNode(deformableRegistrationGridTransformNode);

  return deformableRegistrationGridTransformNode;
}
