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

// SlicerRT includes
#include "vtkSlicerRtCommon.h"
#include "vtkSlicerDicomRtImportExportModuleLogic.h"

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

// DICOMLib includes
#include "vtkSlicerDICOMLoadable.h"

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
void vtkSlicerDicomSroImportExportModuleLogic::ExamineForLoad(vtkStringArray* fileList, vtkCollection* loadables)
{
  if (!fileList || !loadables)
  {
    return;
  }
  loadables->RemoveAllItems();

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

    // The file is a loadable RT object, create and set up loadable
    vtkNew<vtkSlicerDICOMLoadable> loadable;
    loadable->SetName(name.c_str());
    loadable->AddFile(fileName.c_str());
    loadable->SetConfidence(confidence);
    loadable->SetSelected(true);
    std::vector<OFString>::iterator uidIt;
    loadables->AddItem(loadable);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportExportModuleLogic::LoadDicomSro(vtkSlicerDICOMLoadable* loadable)
{
  if (!loadable || loadable->GetFiles()->GetNumberOfValues() < 1 || loadable->GetConfidence() == 0.0)
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

  const char* firstFileName = loadable->GetFiles()->GetValue(0).c_str();

  vtkDebugMacro("Loading series '" << loadable->GetName() << "' from file '" << firstFileName << "'");

  vtkNew<vtkSlicerDicomSroReader> sroReader;
  sroReader->SetFileName(firstFileName);
  sroReader->Update();

  vtkMRMLLinearTransformNode* loadedLinearTransformNode = nullptr;
  vtkMRMLGridTransformNode* loadedGridTransformNode = nullptr;
  vtkMRMLTransformNode* loadedTransformNode = nullptr;

  // Spatial registration
  if (sroReader->GetLoadSpatialRegistrationSuccessful())
  {
    loadedLinearTransformNode = this->LoadSpatialRegistration(sroReader, loadable);
    if (loadedLinearTransformNode)
    {
      loadedTransformNode = loadedLinearTransformNode;
    }
  }
  // Deformable spatial registration
  else if (sroReader->GetLoadDeformableSpatialRegistrationSuccessful())
  {
    loadedGridTransformNode = this->LoadDeformableSpatialRegistration(sroReader, loadable);
    if (loadedGridTransformNode)
    {
      loadedTransformNode = loadedGridTransformNode;
    }
  }
  else
  {
    vtkErrorMacro("Failed to load DICOM registration object from file " << (firstFileName ? firstFileName : "(null)"));
    return false;
  }

  // Setup subject hierarchy entry
  vtkIdType seriesItemID = shNode->CreateItem(shNode->GetSceneItemID(), loadedTransformNode);
  if (sroReader->GetSeriesInstanceUid())
  {
    shNode->SetItemUID(seriesItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), sroReader->GetSeriesInstanceUid());
  }
  else
  {
    vtkErrorMacro("LoadDicomSro: series instance UID not found for transform " << loadedTransformNode->GetName());
  }

  // Add loaded transform under study of transformed image
  vtkSlicerDicomRtImportExportModuleLogic::InsertSeriesInSubjectHierarchy(sroReader, this->GetMRMLScene());

  // Apply loaded transform to corresponding image
  vtkIdType studyItemId = shNode->GetItemAncestorAtLevel(seriesItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  for (int i=0; i<sroReader->GetNumberOfReferencedSeriesUids(); ++i)
  {
    // Sanity check
    if (i > 1)
    {
      vtkWarningMacro("LoadDicomSro: Unexpected number of referenced series from SRO transform: " << sroReader->GetNumberOfReferencedSeriesUids());
      break;
    }

    std::string seriesUid = sroReader->GetReferencedSeriesUid(i);

    // Set DICOM reference to SRO transform item
    shNode->SetItemAttribute(seriesItemID,
      vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName(), seriesUid);

    // If the referenced item is a volume, and it is in another study than the transform, then apply the transform
    //TODO: May change if decision is made on https://github.com/SlicerRt/SlicerRT/issues/108
    vtkIdType refSeriesItemId = shNode->GetItemByUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), seriesUid.c_str());
    vtkIdType refStudyItemId = shNode->GetItemAncestorAtLevel(refSeriesItemId, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (refStudyItemId != studyItemId)
    {
      // Study of moving image
      vtkMRMLScalarVolumeNode* refVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(shNode->GetItemDataNode(refSeriesItemId));
      if (refVolumeNode)
      {
        // Apply transform on moving image
        refVolumeNode->SetAndObserveTransformNodeID(loadedTransformNode->GetID());
        // Set node reference indicating moving image
        loadedTransformNode->AddNodeReferenceID(vtkMRMLTransformNode::GetMovingNodeReferenceRole(), refVolumeNode->GetID());
      }
    }
    else
    {
      // Study of fixed image
      vtkMRMLScalarVolumeNode* refVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(shNode->GetItemDataNode(refSeriesItemId));
      if (refVolumeNode)
      {
        // Set node reference indicating fixed image
        loadedTransformNode->AddNodeReferenceID(vtkMRMLTransformNode::GetFixedNodeReferenceRole(), refVolumeNode->GetID());
      }
    }
  }

  // Spatial fiducials
  //TODO: Function not implemented yet (also take into account for return value)
  //if (spatialRegistrationReader->GetLoadSpatialFiducialsSuccessful())
  //{
  //  loadSuccessful = this->LoadSpatialFiducials(spatialRegistrationReader, loadable);
  //}

  return (loadedTransformNode != nullptr);
}

//---------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerDicomSroImportExportModuleLogic::LoadSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkSlicerDICOMLoadable* loadable)
{
  const char* seriesName = loadable->GetName();

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

//---------------------------------------------------------------------------
vtkMRMLGridTransformNode* vtkSlicerDicomSroImportExportModuleLogic::LoadDeformableSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkSlicerDICOMLoadable* loadable)
{
  const char* seriesName = loadable->GetName();

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

// TODO: not implemented yet...
//---------------------------------------------------------------------------
bool vtkSlicerDicomSroImportExportModuleLogic::LoadSpatialFiducials(vtkSlicerDicomSroReader* regReader, vtkSlicerDICOMLoadable* loadable)
{
  UNUSED_VARIABLE(regReader);
  UNUSED_VARIABLE(loadable);
  return false;
}
