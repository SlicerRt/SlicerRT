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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Contours includes
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkMRMLContourNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkVolumesOrientedResampleUtility.h"

// Subject Hierarchy includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkLookupTable.h>
#include <vtkObjectFactory.h>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLScalarVolumeNode.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContoursModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic::vtkSlicerContoursModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic::~vtkSlicerContoursModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeAboutToBeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::ProcessMRMLSceneEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLSceneEvents(caller, event, callData);

  vtkMRMLScene* scene = this->GetMRMLScene();
  vtkMRMLNode* node = reinterpret_cast<vtkMRMLNode*>(callData);
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("ProcessMRMLSceneEvents: Invalid MRML scene or caller node!");
    return;
  }

  if (scene->IsBatchProcessing())
  {
    return;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (contourNode)
  {
    scene->StartState(vtkMRMLScene::BatchProcessState);

    vtkMRMLContourNode::ContourRepresentationType type = contourNode->GetActiveRepresentationType();
    switch(type)
    {
    case vtkMRMLContourNode::RibbonModel:
      {
        vtkMRMLNode* aNode = scene->GetNodeByID(contourNode->GetRibbonModelNodeId());

        // Remove associated model node
        vtkMRMLHierarchyNode* modelHierarchyNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, aNode->GetID());
        if (modelHierarchyNode)
        {
          scene->RemoveNode(modelHierarchyNode);
        }

        scene->RemoveNode(aNode);
        contourNode->SetAndObserveRibbonModelNodeId(NULL);
        break;
      }
    case vtkMRMLContourNode::IndexedLabelmap:
      {
        vtkMRMLNode* aNode = scene->GetNodeByID(contourNode->GetIndexedLabelmapVolumeNodeId());
        scene->RemoveNode(aNode);
        contourNode->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
        break;
      }
    case vtkMRMLContourNode::ClosedSurfaceModel:
      {
        vtkMRMLNode* aNode = scene->GetNodeByID(contourNode->GetClosedSurfaceModelNodeId());

        // Remove associated model node
        vtkMRMLHierarchyNode* modelHierarchyNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, aNode->GetID());
        if (modelHierarchyNode)
        {
          scene->RemoveNode(modelHierarchyNode);
        }

        scene->RemoveNode(aNode);
        contourNode->SetAndObserveClosedSurfaceModelNodeId(NULL);
        break;
      }
    default:
      vtkErrorMacro("Unknown representation type.");
    }

    // Get associated subject hierarchy node
    vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode =
      vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(contourNode, scene);

    // No need to remove color and subject hierarchy node if the scene is closing and there is no associated subject hierarchy node any more
    if (!scene->IsClosing() || contourSubjectHierarchyNode)
    {
      // Remove color table entry
      vtkMRMLColorTableNode* colorNode = NULL;
      int structureColorIndex = -1;
      contourNode->GetColor(structureColorIndex, colorNode, scene);
      if (colorNode)
      {
        if (structureColorIndex != SlicerRtCommon::COLOR_INDEX_INVALID)
        {
          // Set color entry to invalid
          colorNode->SetColor(structureColorIndex, SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
            SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3]);
          colorNode->SetColorName(structureColorIndex, SlicerRtCommon::COLOR_NAME_REMOVED);
        }
        else
        {
          vtkErrorMacro("OnMRMLSceneNodeRemoved: There was no associated color for contour '" << contourNode->GetName() << "' before removing!");
        }
      }
      else
      {
        vtkErrorMacro("OnMRMLSceneNodeRemoved: There is no color node for the removed contour '" << contourNode->GetName() << "'!");
      }

      // Remove subject hierarchy node
      if (contourSubjectHierarchyNode)
      {
        scene->RemoveNode(contourSubjectHierarchyNode);
      }
    }

    this->Modified();
    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLContourNode"))
  {
    // Create empty ribbon model
    this->CreateEmptyRibbonModelForContour(node);

    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneEndImport()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndImport: Invalid MRML scene!");
    return;
  }

  vtkSmartPointer<vtkCollection> contourNodes = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLContourNode") );
  vtkObject* nextObject = NULL;
  for (contourNodes->InitTraversal(); (nextObject = contourNodes->GetNextItemAsObject()); )
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(nextObject);
    if (contourNode)
    {
      contourNode->UpdateRepresentations();
    }
  }
  
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::CreateEmptyRibbonModelForContour(vtkMRMLNode* node)
{
  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("CreateEmptyRibbonModelForContour: Invalid MRML scene!");
    return;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (!contourNode)
  {
    vtkErrorMacro("CreateEmptyRibbonModelForContour: Argument node is not a contour node!");
    return;
  }

  // If a scene is being loaded then don't create empty model, because the node will be updated
  // to the proper representation when importing has finished. \sa vtkMRMLContourNode::ReadXMLAttributes
  if (contourNode->RibbonModelNodeId && !contourNode->RibbonModelNode)
  {
    return;
  }

  vtkSmartPointer<vtkPolyData> emptyPolyData = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkMRMLModelDisplayNode> emptyRibbonModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  mrmlScene->AddNode(emptyRibbonModelDisplayNode);

  vtkSmartPointer<vtkMRMLModelNode> emptyRibbonModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  mrmlScene->AddNode(emptyRibbonModelNode);
  emptyRibbonModelNode->SetAndObserveDisplayNodeID(emptyRibbonModelDisplayNode->GetID());
  emptyRibbonModelNode->SetAndObservePolyData(emptyPolyData);

  std::string emptyRibbonModelName(contourNode->GetName());
  emptyRibbonModelName.append(SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX);
  emptyRibbonModelNode->SetName(emptyRibbonModelName.c_str());

  contourNode->SetAndObserveRibbonModelNodeId(emptyRibbonModelNode->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::PaintLabelmapForeground(vtkMRMLScalarVolumeNode* volumeNode, unsigned char newColor)
{
  if (!volumeNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::PaintLabelmapForeground: volumeNode argument is null!" << std::endl;
    return;
  }
  if (newColor <= SlicerRtCommon::COLOR_INDEX_INVALID)
  {
    vtkErrorWithObjectMacro(volumeNode, "PaintLabelmapForeground: Invalid color index given! Color index must be greater than the invalid color index (" << SlicerRtCommon::COLOR_INDEX_INVALID << ")");
    return;
  }

  vtkImageData* imageData = volumeNode->GetImageData();
  if (!imageData || imageData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkErrorWithObjectMacro(volumeNode, "PaintLabelmapForeground: Invalid image data! Scalar type has to be unsigned char instead of '" << (imageData?imageData->GetScalarTypeAsString():"None") << "'");
    return;
  }

  unsigned char* imagePtr = (unsigned char*)imageData->GetScalarPointer();
  for (long i=0; i<imageData->GetNumberOfPoints(); ++i)
  {
    if ( (*imagePtr) != 0 )
    {
      (*imagePtr) = newColor;
    }
    ++imagePtr;
  }
  imageData->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(vtkMRMLNode* node, std::vector<vtkMRMLContourNode*>& contours)
{
  contours.clear();

  if (!node)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode: Invalid input node!" << std::endl;
    return;
  }

  // Create list of selected contour nodes
  if (node->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
    if (contourNode)
    {
      contours.push_back(contourNode);
    }
  }
  else if ( node->IsA("vtkMRMLSubjectHierarchyNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLSubjectHierarchyNode::SafeDownCast(node)->GetAssociatedChildrenNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      vtkWarningWithObjectMacro(node, "GetContourNodesFromSelectedNode: Selected contour hierarchy node has no children contour nodes!");
      return;
    }

    // Collect contour nodes in the hierarchy and determine whether their active representation types are the same
    for (int i=0; i<childContourNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(childContourNodes->GetItemAsObject(i));
      if (contourNode)
      {
        contours.push_back(contourNode);
      }
    }
  }
  else
  {
    vtkErrorWithObjectMacro(node, "GetContourNodesFromSelectedNode: Invalid node type for contour!");
  }
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType vtkSlicerContoursModuleLogic::GetRepresentationTypeOfContours(std::vector<vtkMRMLContourNode*>& contours)
{
  bool sameRepresentationTypes = true;
  vtkMRMLContourNode::ContourRepresentationType representationType = vtkMRMLContourNode::None;

  for (std::vector<vtkMRMLContourNode*>::iterator it = contours.begin(); it != contours.end(); ++it)
  {
    if (representationType == vtkMRMLContourNode::None)
    {
      representationType = (*it)->GetActiveRepresentationType();
    }
    else if ((*it)->GetActiveRepresentationType() == vtkMRMLContourNode::None) // Sanity check
    {
      vtkWarningWithObjectMacro((*it), "getRepresentationTypeOfSelectedContours: Invalid representation type (None) found for the contour node '" << (*it)->GetName() << "'!")
    }
    else if (representationType != (*it)->GetActiveRepresentationType())
    {
      sameRepresentationTypes = false;
    }
  }

  if (sameRepresentationTypes)
  {
    return representationType;
  }
  else
  {
    return vtkMRMLContourNode::None;
  }
}

//-----------------------------------------------------------------------------
const char* vtkSlicerContoursModuleLogic::GetRasterizationReferenceVolumeOfContours(std::vector<vtkMRMLContourNode*>& contours, bool &sameReferenceVolumeInContours)
{
  sameReferenceVolumeInContours = true;
  std::string rasterizationReferenceVolumeNodeId("");

  for (std::vector<vtkMRMLContourNode*>::iterator it = contours.begin(); it != contours.end(); ++it)
  {
    if (rasterizationReferenceVolumeNodeId.empty())
    {
      rasterizationReferenceVolumeNodeId = std::string((*it)->GetRasterizationReferenceVolumeNodeId());
    }
    else if ( ((*it)->GetRasterizationReferenceVolumeNodeId() == NULL && !rasterizationReferenceVolumeNodeId.empty())
           || ((*it)->GetRasterizationReferenceVolumeNodeId() != NULL && STRCASECMP(rasterizationReferenceVolumeNodeId.c_str(), (*it)->GetRasterizationReferenceVolumeNodeId())) )
    {
      sameReferenceVolumeInContours = false;
    }
  }

  if (sameReferenceVolumeInContours)
  {
    return rasterizationReferenceVolumeNodeId.c_str();
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerContoursModuleLogic::GetReferencedVolumeByDicomForContour(vtkMRMLContourNode* contour)
{
  if (!contour)
  {
    return NULL;
  }

  // Get referenced series UID for contour
  vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(contour);
  const char* referencedSeriesUid = contourSubjectHierarchyNode->GetAttribute(
    SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str() );
  if (!referencedSeriesUid)
  {
    vtkWarningWithObjectMacro(contourSubjectHierarchyNode, "No referenced series UID found for contour '" << contourSubjectHierarchyNode->GetName() << "'");
    return NULL;
  }

  // Get referenced volume subject hierarchy node by found UID
  vtkMRMLSubjectHierarchyNode* referencedSeriesNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    contour->GetScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, referencedSeriesUid);
  if (!referencedSeriesNode)
  {
    return NULL;
  }

  // Get and return referenced volume
  return vtkMRMLScalarVolumeNode::SafeDownCast(referencedSeriesNode->GetAssociatedDataNode());
}

//-----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerContoursModuleLogic::GetReferencedVolumeByDicomForContours(std::vector<vtkMRMLContourNode*>& contours)
{
  if (contours.empty())
  {
    return NULL;
  }

  // Get series hierarchy node from first contour
  vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(*contours.begin());
  if (!contourSubjectHierarchyNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetReferencedSeriesForContours: Failed to find subject hierarchy node for contour '"
      << ((*contours.begin())==NULL ? "NULL" : (*contours.begin())->GetName()) << "'!" << std::endl;
    return NULL;
  }
  vtkMRMLSubjectHierarchyNode* contourHierarchySeriesNode = contourSubjectHierarchyNode->GetAncestorAtLevel(
    vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES);
  if (!contourHierarchySeriesNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetReferencedSeriesForContours: Failed to find series hierarchy node for contour '"
      << ((*contours.begin())==NULL ? "NULL" : (*contours.begin())->GetName()) << "'!" << std::endl;
    return NULL;
  }

  // Traverse contour nodes from parent
  std::string commonReferencedSeriesUid("");
  std::vector<vtkMRMLHierarchyNode*> contourHierarchyNodes = contourHierarchySeriesNode->GetChildrenNodes();
  for (std::vector<vtkMRMLHierarchyNode*>::iterator contourIt=contourHierarchyNodes.begin(); contourIt!=contourHierarchyNodes.end(); ++contourIt)
  {
    vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(*contourIt);
    if ( contourSubjectHierarchyNode->GetAssociatedDataNode()
      && contourSubjectHierarchyNode->GetAssociatedDataNode()->IsA("vtkMRMLColorTableNode") )
    {
      continue;
    }

    // Get referenced series UID for contour
    // Note: Do not use function for getting reference for one contour due to performance reasons (comparing the UID strings are
    //   much faster than getting the subject hierarchy node by UID many times)
    const char* referencedSeriesUid = contourSubjectHierarchyNode->GetAttribute(
      SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str() );

    if (!referencedSeriesUid)
    {
      vtkWarningWithObjectMacro(contourHierarchySeriesNode, "No referenced series UID found for contour '" << contourSubjectHierarchyNode->GetName() << "'");
    }
    // Set a candidate common reference UID in the first loop
    else if (commonReferencedSeriesUid.empty())
    {
      commonReferencedSeriesUid = std::string(referencedSeriesUid);
    }
    // Return NULL if referenced UIDs are different
    else if (STRCASECMP(referencedSeriesUid, commonReferencedSeriesUid.c_str()))
    {
      return NULL;
    }
  }

  // Return NULL if no referenced UID has been found for any of the contours
  if (commonReferencedSeriesUid.empty())
  {
    return NULL;
  }

  // Common referenced series UID found, get corresponding subject hierarchy node
  vtkMRMLSubjectHierarchyNode* referencedSeriesNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    contourHierarchySeriesNode->GetScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, commonReferencedSeriesUid.c_str());
  if (!referencedSeriesNode)
  {
    return NULL;
  }

  // Get and return referenced volume
  return vtkMRMLScalarVolumeNode::SafeDownCast(referencedSeriesNode->GetAssociatedDataNode());
}

//-----------------------------------------------------------------------------
bool vtkSlicerContoursModuleLogic::ContoursContainRepresentation(std::vector<vtkMRMLContourNode*>& contours, vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain/*=true*/)
{
  if (contours.size() == 0)
  {
    return false;
  }

  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = contours.begin(); contourIt != contours.end(); ++contourIt)
  {
    if (allMustContain && !(*contourIt)->RepresentationExists(representationType))
    {
      // At least one misses the requested representation
      return false;
    }
    else if (!allMustContain && (*contourIt)->RepresentationExists(representationType))
    {
      // At least one has the requested representation
      return true;
    }
  }

  if (allMustContain)
  {
    // All contours have the requested representation
    return true;
  }
  else
  {
    // None of the contours have the requested representation
    return false;
  }
}

//-----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::GetIndexedLabelmapWithGivenGeometry(vtkMRMLContourNode* contour, vtkMRMLScalarVolumeNode* referenceVolumeNode, vtkMRMLScalarVolumeNode* outputIndexedLabelmap)
{
  if (!contour)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetIndexedLabelmapWithGivenGeometry: Invalid contour argument!" << std::endl;
    return;
  }
  if (!referenceVolumeNode || !outputIndexedLabelmap)
  {
    vtkErrorWithObjectMacro(contour, "GetIndexedLabelmapWithGivenGeometry: Invalid reference volume or output labelmap argument!");
    return;
  }

  // Return the contained indexed labelmap representation directly if its lattice matches the reference volume
  vtkMRMLScalarVolumeNode* inputIndexedLabelmap = contour->GetIndexedLabelmapVolumeNode(); // Also does conversion if necessary
  if (SlicerRtCommon::DoVolumeLatticesMatch(inputIndexedLabelmap, referenceVolumeNode))
  {
    outputIndexedLabelmap->Copy(inputIndexedLabelmap);
    outputIndexedLabelmap->CopyOrientation(inputIndexedLabelmap); // Make sure the IJK to RAS matrix is copied too (the Copy function does not do this in some cases)
    return;
  }

  // Resampling is needed
  vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode(inputIndexedLabelmap, referenceVolumeNode, outputIndexedLabelmap);
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode* vtkSlicerContoursModuleLogic::CreateContourFromRepresentation(vtkMRMLDisplayableNode* representationNode)
{
  if (!representationNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::CreateContourFromRepresentation: Input node is NULL" << std::endl;
    return NULL;
  }

  vtkMRMLScene* mrmlScene = representationNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorWithObjectMacro(representationNode, "CreateContourFromRepresentation: Input node is not in a MRML scene!");
    return NULL;
  }

  // Create contour node if dropped node is volume or model
  if (representationNode->IsA("vtkMRMLModelNode") || representationNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkSmartPointer<vtkMRMLContourNode> newContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
    newContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->AddNode(newContourNode));
    std::string contourName = std::string(representationNode->GetName()) + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
    contourName = mrmlScene->GenerateUniqueName(contourName);
    newContourNode->SetName(contourName.c_str());

    if (representationNode->IsA("vtkMRMLScalarVolumeNode"))
    {
      newContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(representationNode->GetID());

      // Set the labelmap itself as reference thus indicating there was no conversion from model representation
      newContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(representationNode->GetID());
    }
    else
    {
      newContourNode->SetAndObserveClosedSurfaceModelNodeId(representationNode->GetID());
      newContourNode->SetDecimationTargetReductionFactor(0.0);
    }
    newContourNode->HideFromEditorsOff();
    newContourNode->Modified();

    return newContourNode.GetPointer();
  }
  else
  {
    vtkErrorWithObjectMacro(representationNode, "CreateContourFromRepresentation: Invalid node type to add!");
    return NULL;
  }

  return NULL;
}
