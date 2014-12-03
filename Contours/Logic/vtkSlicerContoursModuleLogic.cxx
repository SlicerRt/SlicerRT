/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourStorageNode.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// VTK includes
#include <vtkImageCast.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtksys/SystemTools.hxx>

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLContourModelDisplayNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelNode.h>
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
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourStorageNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourModelDisplayNode>::New());
  // TODO : 2d vis readdition
  //this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourLabelmapDisplayNode>::New());
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
  if (!scene)
  {
    vtkErrorMacro("ProcessMRMLSceneEvents: Invalid MRML scene or caller node!");
    return;
  }
  if (scene->IsBatchProcessing())
  {
    return;
  }

  vtkMRMLNode* node = reinterpret_cast<vtkMRMLNode*>(callData);
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (contourNode)
  {
    if (event == vtkMRMLScene::NodeAboutToBeRemovedEvent)
    {
      scene->StartState(vtkMRMLScene::BatchProcessState);

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

  // By using a smart pointer and ::Take the deallocation is handled when the function ends
  vtkSmartPointer<vtkCollection> contourNodes = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLContourNode") );
  vtkObject* nextObject = NULL;
  for (contourNodes->InitTraversal(); (nextObject = contourNodes->GetNextItemAsObject()); )
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(nextObject);
    if (contourNode)
    {
      contourNode->UpdateRepresentations();

      // Restore any color not handled during load
      // The color attribute is saved in the contour display node, but it doesn't seem to be restored on load
      vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(contourNode, this->GetMRMLScene());
      if (shNode)
      {
        vtkMRMLSubjectHierarchyNode* parentNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shNode->GetParentNode());
        if (parentNode)
        {
          vtkMRMLColorTableNode* colorTableNode(NULL);
          int structureIndex(-1);
          contourNode->GetColor(structureIndex, colorTableNode, this->GetMRMLScene());
          double color[4] = {0,0,0,1};
          if (colorTableNode)
          {
            colorTableNode->GetColor(structureIndex, color);

            if (contourNode->GetRibbonModelDisplayNode())
            {
              contourNode->GetRibbonModelDisplayNode()->SetColor(color);
            }

            if (contourNode->GetClosedSurfaceModelDisplayNode())
            {
              contourNode->GetClosedSurfaceModelDisplayNode()->SetColor(color);
            }
          }
        }
      }
    }
  }
  
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::PaintLabelmapRepresentationForeground(vtkMRMLContourNode* contourNode, unsigned char newColor)
{
  if (!contourNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::PaintLabelmapForeground: contourNode argument is null!" << std::endl;
    return;
  }
  if (newColor <= SlicerRtCommon::COLOR_INDEX_INVALID)
  {
    vtkErrorWithObjectMacro(contourNode, "PaintLabelmapForeground: Invalid color index given! Color index must be greater than the invalid color index (" << SlicerRtCommon::COLOR_INDEX_INVALID << ")");
    return;
  }

  vtkImageData* imageData = contourNode->GetLabelmapImageData();
  if (!imageData || imageData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkErrorWithObjectMacro(contourNode, "PaintLabelmapForeground: Invalid image data! Scalar type has to be unsigned char instead of '" << (imageData?imageData->GetScalarTypeAsString():"None") << "'");
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
  vtkMRMLContourNode::ContourRepresentationType representationType = vtkMRMLContourNode::None;

  for (std::vector<vtkMRMLContourNode*>::iterator it = contours.begin(); it != contours.end(); ++it)
  {
    // Determine which representations this contour has
    vtkMRMLContourNode::ContourRepresentationType thisRepresentationType(vtkMRMLContourNode::None);

    bool labelmapExists(false), ribbonModelExists(false), closedSurfaceModelExists(false);
    labelmapExists = (*it)->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap);
    ribbonModelExists = (*it)->HasRepresentation(vtkMRMLContourNode::RibbonModel);
    closedSurfaceModelExists = (*it)->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);
    int total = (labelmapExists ? 1 : 0) + (ribbonModelExists ? 1 : 0) + (closedSurfaceModelExists ? 1 : 0);
    if (total > 1)
    {
      return vtkMRMLContourNode::None;
    }
    else if ( total == 1 )
    {
      if (labelmapExists)
      {
        thisRepresentationType = vtkMRMLContourNode::IndexedLabelmap;
      }
      else if (ribbonModelExists)
      {
        thisRepresentationType = vtkMRMLContourNode::RibbonModel;
      }
      else if (closedSurfaceModelExists)
      {
        thisRepresentationType = vtkMRMLContourNode::ClosedSurfaceModel;
      }
    }
    else
    {
      vtkErrorWithObjectMacro( (*it), "Contour does not contain any representations!");
    }

    if (representationType == vtkMRMLContourNode::None)
    {
      representationType = thisRepresentationType;
    }
    else if (thisRepresentationType != representationType)
    {
      return vtkMRMLContourNode::None;
    }
  }

  return representationType;
}

//-----------------------------------------------------------------------------
const char* vtkSlicerContoursModuleLogic::GetRasterizationReferenceVolumeIdOfContours(std::vector<vtkMRMLContourNode*>& contours, bool &sameReferenceVolumeInContours)
{
  sameReferenceVolumeInContours = true;
  vtkMRMLScalarVolumeNode* rasterizationReferenceNode = (*contours.begin())->GetRasterizationReferenceVolumeNode();

  for (std::vector<vtkMRMLContourNode*>::iterator it = contours.begin(); it != contours.end(); ++it)
  {
    vtkMRMLScalarVolumeNode* thisContourRasterizationReferenceNode = (*it)->GetRasterizationReferenceVolumeNode();
    if (thisContourRasterizationReferenceNode != rasterizationReferenceNode)
    {
      sameReferenceVolumeInContours = false;
      return NULL;
    }
  }

  return rasterizationReferenceNode->GetID();
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
    contour->GetScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), referencedSeriesUid);
  if (!referencedSeriesNode)
  {
    // TODO: If there is no rasterization reference, can we try to guess a reasonable default (regular volume in the same study)
    //   Create separate utility function for this, see other TODOs
    return NULL;
  }

  // Get and return referenced volume
  return vtkMRMLScalarVolumeNode::SafeDownCast(referencedSeriesNode->GetAssociatedNode());
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
    vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries());
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
    if (!contourSubjectHierarchyNode)
    {
      continue;
    }

    // Get referenced series UID for contour
    // Note: Do not use function for getting reference for one contour due to performance reasons (comparing the UID strings are
    //   much faster than getting the subject hierarchy node by UID many times)
    // TODO: If there is no rasterization reference, can we try to guess a reasonable default (regular volume in the same study)
    //   Create separate utility function for this, see other TODOs
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
    contourHierarchySeriesNode->GetScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), commonReferencedSeriesUid.c_str());
  if (!referencedSeriesNode)
  {
    return NULL;
  }

  // Get and return referenced volume
  return vtkMRMLScalarVolumeNode::SafeDownCast(referencedSeriesNode->GetAssociatedNode());
}

//-----------------------------------------------------------------------------
bool vtkSlicerContoursModuleLogic::ContoursContainRepresentation(std::vector<vtkMRMLContourNode*> contours, vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain/*=true*/)
{
  if (contours.size() == 0)
  {
    return false;
  }

  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = contours.begin(); contourIt != contours.end(); ++contourIt)
  {
    if (allMustContain && !(*contourIt)->HasRepresentation(representationType))
    {
      // At least one misses the requested representation
      return false;
    }
    else if (!allMustContain && (*contourIt)->HasRepresentation(representationType))
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
void vtkSlicerContoursModuleLogic::GetIndexedLabelmapWithGivenGeometry(vtkMRMLContourNode* inputContour, vtkMRMLScalarVolumeNode* referenceVolumeNode, vtkMRMLContourNode* outputLabelmapContour)
{
  if (!inputContour)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetIndexedLabelmapWithGivenGeometry: Invalid contour argument!" << std::endl;
    return;
  }
  if (!referenceVolumeNode || !outputLabelmapContour)
  {
    vtkErrorWithObjectMacro(inputContour, "GetIndexedLabelmapWithGivenGeometry: Invalid reference volume or output labelmap argument!");
    return;
  }

  // Return the contained indexed labelmap representation directly if its lattice matches the reference volume
  if (vtkMRMLContourNode::DoVolumeLatticesMatch(inputContour, referenceVolumeNode))
  {
    outputLabelmapContour->Copy(inputContour);
    outputLabelmapContour->CopyOrientation(inputContour); // Make sure the IJK to RAS matrix is copied too (the Copy function does not do this in some cases)
    return;
  }

  // Resampling is needed
  vtkMRMLContourNode::ResampleInputContourNodeToReferenceVolumeNode(inputContour->GetScene(), inputContour, referenceVolumeNode, outputLabelmapContour);
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode* vtkSlicerContoursModuleLogic::CreateEmptyContourFromExistingContour(vtkMRMLContourNode* refContourNode, const std::string& contourNameNoSuffix)
{
  if (refContourNode == NULL)
  {
    std::cerr << "Null input sent to vtkSlicerContoursModuleLogic::CreateEmptyContour.";
    return NULL;
  }
  vtkMRMLScene* scene = refContourNode->GetScene();
  if (scene == NULL)
  {
    vtkErrorWithObjectMacro(refContourNode, "Invalid scene extracted from reference contour node: " << refContourNode->GetName());
  }
  std::string fullName = contourNameNoSuffix + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
  std::string uniqueName = scene->GenerateUniqueName(fullName);

  // Create contour node
  vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  contourNode = vtkMRMLContourNode::SafeDownCast(scene->AddNode(contourNode));
  contourNode->SetName(uniqueName.c_str());
  contourNode->SetCreatedFromIndexLabelmap(false);
  contourNode->HideFromEditorsOff();

  vtkSlicerContoursModuleLogic::CreateContourStorageNode(contourNode);

  if (refContourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
  {
    double dirs[3][3];
    refContourNode->GetIJKToRASDirections(dirs);
    contourNode->SetIJKToRASDirections(dirs);
    contourNode->SetOrigin(refContourNode->GetOrigin());
    contourNode->SetSpacing(refContourNode->GetSpacing());
  }
  
  if (refContourNode->GetRasterizationReferenceVolumeNode())
  {
    contourNode->SetAndObserveRasterizationReferenceVolumeNodeId(refContourNode->GetRasterizationReferenceVolumeNode()->GetID());
  }

  contourNode->SetRasterizationOversamplingFactor(refContourNode->GetRasterizationOversamplingFactor());
  contourNode->SetDecimationTargetReductionFactor(refContourNode->GetDecimationTargetReductionFactor());
  // I'm not sure about this last one... maybe it shouldn't be copied
  contourNode->SetMetaDataDictionary(refContourNode->GetMetaDataDictionary());

  return contourNode;
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode* vtkSlicerContoursModuleLogic::CreateContourFromRepresentation(vtkMRMLDisplayableNode* representationNode, const char* optionalName)
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

  // Cannot create contour node if invalid (not volume or model) representation node is given
  if (!representationNode->IsA("vtkMRMLModelNode") && !representationNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkErrorWithObjectMacro(representationNode, "CreateContourFromRepresentation: Invalid contour representation node type!");
    return NULL;
  }

  // Assemble contour name
  std::string contourName("");
  if (optionalName != NULL)
  {
    contourName = std::string(optionalName);
  }
  else
  {
    contourName = representationNode->GetName();
  }
  vtksys::SystemTools::ReplaceString(contourName, SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX.c_str(), "");
  vtksys::SystemTools::ReplaceString(contourName, SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX.c_str(), "");
  vtksys::SystemTools::ReplaceString(contourName, SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX.c_str(), "");
  std::string structureName(contourName);
  contourName.append(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX);
  contourName = mrmlScene->GenerateUniqueName(contourName);

  // Create contour node (subject hierarchy node automatically created)
  vtkSmartPointer<vtkMRMLContourNode> newContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  newContourNode->SetName(contourName.c_str());
  mrmlScene->AddNode(newContourNode);

  // Create storage node for contour node
  vtkSlicerContoursModuleLogic::CreateContourStorageNode(newContourNode);

  // Representation node is a volume
  if (representationNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkMRMLScalarVolumeNode* volNode = vtkMRMLScalarVolumeNode::SafeDownCast(representationNode);

    if (volNode->GetImageData()->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
      vtkWarningWithObjectMacro(volNode, "Input image data to contour creation is not of scalar type VTK_UNSIGNED_CHAR. Attempting conversion.");
      vtkSmartPointer<vtkImageCast> imageCast = vtkSmartPointer<vtkImageCast>::New();
      imageCast->SetOutputScalarTypeToUnsignedChar();
#if (VTK_MAJOR_VERSION <= 5)
      imageCast->SetInput(volNode->GetImageData());
#else
      imageCast->SetInputData(volNode->GetImageData());
#endif
      imageCast->Update();
      volNode->SetAndObserveImageData(imageCast->GetOutput());
    }

    newContourNode->SetMetaDataDictionary( volNode->GetMetaDataDictionary() );
    newContourNode->SetOrigin( volNode->GetOrigin() );
    newContourNode->SetSpacing( volNode->GetSpacing() );
    double dirs[3][3];
    volNode->GetIJKToRASDirections(dirs);
    newContourNode->SetIJKToRASDirections(dirs);
    newContourNode->SetAndObserveLabelmapImageData( volNode->GetImageData() );

    // Set rasterization reference if the representation is a labelmap node and has a saved reference node ID
    if ( volNode->GetAttribute("AssociatedNodeID")
      && mrmlScene->GetNodeByID(volNode->GetAttribute("AssociatedNodeID")) )
    {
      newContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(volNode->GetAttribute("AssociatedNodeID"));
    }

    newContourNode->SetCreatedFromIndexLabelmap(true);

    // Create display node
    // TODO : 2d vis readdition, creation 2d vis display node
  }
  // Representation node is a model
  else
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(representationNode);
    newContourNode->SetDecimationTargetReductionFactor(0.0);
    newContourNode->SetAndObserveClosedSurfacePolyData( modelNode->GetPolyData() );

    // Unset (just in case) CreatedFromIndexLabelmap flag
    newContourNode->SetCreatedFromIndexLabelmap(false);

    // Create display node
    newContourNode->CreateClosedSurfaceDisplayNode();
  }
  newContourNode->HideFromEditorsOff();
  newContourNode->Modified();

  // Setup automatically created subject hierarchy node for the contour
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(newContourNode);
  if (subjectHierarchyNode)
  {
    subjectHierarchyNode->SetLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries());
    subjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureName.c_str() );
  }

  return newContourNode.GetPointer();
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode* vtkSlicerContoursModuleLogic::LoadContourFromFile( const char* filename )
{
  if (this->GetMRMLScene() == 0 ||
    filename == 0)
  {
    return 0;
  }
  vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  vtkSmartPointer<vtkMRMLContourStorageNode> storageNode = vtkSmartPointer<vtkMRMLContourStorageNode>::New();
  storageNode->SetFileName(filename);

  // check to see which node can read this type of file
  if ( !storageNode->SupportedFileType(filename) )
  {
    vtkErrorMacro("Contour storage node unable to load contour file.");
    return NULL;
  }

  std::string baseName = vtksys::SystemTools::GetFilenameWithoutExtension(filename);
  std::string uname( this->GetMRMLScene()->GetUniqueNameByString(baseName.c_str()));
  contourNode->SetName(uname.c_str());
  std::string storageUName = uname + SlicerRtCommon::CONTOUR_STORAGE_NODE_POSTFIX;
  storageNode->SetName(storageUName.c_str());
  this->GetMRMLScene()->SaveStateForUndo();
  this->GetMRMLScene()->AddNode(storageNode.GetPointer());

  contourNode->SetScene(this->GetMRMLScene());
  contourNode->SetAndObserveStorageNodeID(storageNode->GetID());

  this->GetMRMLScene()->AddNode(contourNode);

  // now set up the reading
  vtkDebugMacro("AddModel: calling read on the storage node");
  int retval = storageNode->ReadData(contourNode);
  if (retval != 1)
  {
    vtkErrorMacro("AddModel: error reading " << filename);
    this->GetMRMLScene()->RemoveNode(contourNode);
    return NULL;
  }

  return contourNode.GetPointer();
}

//-----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerContoursModuleLogic::ExtractLabelmapFromContour( vtkMRMLContourNode* contourNode )
{
  vtkSmartPointer<vtkMRMLScalarVolumeNode> volNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  // Convert on demand
  volNode->SetAndObserveImageData(contourNode->GetLabelmapImageData());
  std::string volNodeName = std::string(contourNode->GetName()) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  volNode->SetName(volNodeName.c_str());
  volNode->SetLabelMap(1);
  volNode->SetOrigin(contourNode->GetOrigin());
  volNode->SetSpacing(contourNode->GetSpacing());
  double dirs[3][3];
  contourNode->GetIJKToRASDirections(dirs);
  volNode->SetIJKToRASDirections(dirs);
  volNode->SetMetaDataDictionary(contourNode->GetMetaDataDictionary());
  contourNode->GetScene()->AddNode(volNode);
  volNode->SetAndObserveTransformNodeID(contourNode->GetTransformNodeID());

  vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode> dispNode = vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode>::New();
  std::string dispNodeName = std::string(contourNode->GetName()) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX + SlicerRtCommon::CONTOUR_DISPLAY_NODE_SUFFIX;
  contourNode->GetScene()->AddNode(dispNode);
  volNode->SetAndObserveDisplayNodeID(dispNode->GetID());

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(contourNode);
  if (shNode)
  {
    vtkMRMLSubjectHierarchyNode* seriesNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shNode->GetParentNode());
    if (seriesNode)
    {
      vtkMRMLColorTableNode* ctNode = vtkMRMLColorTableNode::SafeDownCast(seriesNode->GetNodeReference(SlicerRtCommon::CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE.c_str()));
      if (ctNode)
      {
        dispNode->SetAndObserveColorNodeID(ctNode->GetID());
      }
      else
      {
        vtkErrorWithObjectMacro(seriesNode, "Unable to find color table node for contour set " << seriesNode->GetNameWithoutPostfix());
      }
    }
  }

  return volNode;
}

//-----------------------------------------------------------------------------
vtkMRMLContourStorageNode* vtkSlicerContoursModuleLogic::CreateContourStorageNode( vtkMRMLContourNode* contourNode )
{
  if (contourNode == NULL || contourNode->GetScene() == NULL)
  {
    std::cerr << "Unable to create a contour storage node. Null contour or null scene passed in." << std::endl;
    return NULL;
  }

  // Create contour storage node
  vtkSmartPointer<vtkMRMLContourStorageNode> storageNode = vtkSmartPointer<vtkMRMLContourStorageNode>::New();
  std::string storageNodeName = contourNode->GetName() + SlicerRtCommon::CONTOUR_STORAGE_NODE_POSTFIX;
  storageNodeName = contourNode->GetScene()->GenerateUniqueName(storageNodeName);
  storageNode->SetName(storageNodeName.c_str());
  storageNode->SetFileName(storageNodeName.c_str());
  contourNode->GetScene()->AddNode(storageNode);

  contourNode->AddAndObserveStorageNodeID(storageNode->GetID());

  return storageNode;
}
