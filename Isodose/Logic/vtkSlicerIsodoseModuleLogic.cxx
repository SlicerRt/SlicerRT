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

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLSegmentationDisplayNode.h>

// DicomRtImportExport includes
#include <vtkPlanarContourToClosedSurfaceConversionRule.h>

// MRMLLogic includes
#include <vtkMRMLColorLogic.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkDecimatePro.h>
#include <vtkGeneralTransform.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageMarchingCubes.h>
#include <vtkImageReslice.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include "vtksys/SystemTools.hxx"

// STD includes
#include <tuple>

//----------------------------------------------------------------------------
const char* DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME = "Isodose_ColorTable.ctbl";
const char* DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME = "Isodose_ColorTable_Default";
const char* RELATIVE_ISODOSE_COLOR_TABLE_NODE_NAME = "Isodose_ColorTable_Relative";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_MODEL_NODE_NAME_PREFIX = "IsodoseLevel_";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX = "IsodoseParameterSet_";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_HIERARCHY_NAME_POSTFIX = "_IsodoseSurfaces";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_RELATIVE_ROOT_HIERARCHY_NAME_POSTFIX = "_RelativeIsodoseSurfaces";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX = "_IsodoseColorTable";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerIsodoseModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::vtkSlicerIsodoseModuleLogic() = default;

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::~vtkSlicerIsodoseModuleLogic() = default;

//----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());

  // Load (or create) default isodose color table
  vtkMRMLColorTableNode* isodoseColorTableNode = nullptr;
  if ( (isodoseColorTableNode = this->LoadDefaultIsodoseColorTable()) == nullptr )
  {
    isodoseColorTableNode = vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable(newScene);
  }
  // Create dose color table if load/create succeeded
  if (isodoseColorTableNode)
  {
    vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(newScene);
    vtkSlicerIsodoseModuleLogic::CreateRelativeDoseColorTable(newScene);
  }
  else
  {
    vtkErrorMacro("SetMRMLSceneInternal: Failed to create isodose color table node");
  }
}

//-----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLIsodoseNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLIsodoseNode>::New());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLIsodoseNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLIsodoseNode"))
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkSlicerIsodoseModuleLogic::GetIsodoseFolderItemID(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("GetIsodoseFolderItemID: Invalid scene");
    return 0;
  }
  vtkMRMLScalarVolumeNode* doseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!doseVolumeNode)
  {
    // If input is not a volume then check if it's a parameter node and get the dose volume from that
    vtkMRMLIsodoseNode* parameterNode = vtkMRMLIsodoseNode::SafeDownCast(node);
    if (!parameterNode)
    {
      vtkErrorMacro("GetIsodoseFolderItemID: Input node must be a dose volume node or an isodose parameter node");
      return 0;
    }
    doseVolumeNode = parameterNode->GetDoseVolumeNode();
  }
  if (!doseVolumeNode)
  {
    vtkErrorMacro("GetIsodoseFolderItemID: Failed to get dose volume");
    return 0;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("GetIsodoseFolderItemID: Failed to access subject hierarchy node");
    return 0;
  }

  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (!doseShItemID)
  {
    vtkErrorMacro("GetIsodoseFolderItemID: Failed to find subject hierarchy item for dose volume");
    return 0;
  }
  if (shNode->GetNumberOfItemChildren(doseShItemID) == 0)
  {
    // No isodose folder yet
    return 0;
  }

  std::vector<vtkIdType> doseChildItemIDs;
  shNode->GetItemChildren(doseShItemID, doseChildItemIDs, false);
  for (vtkIdType childItemID : doseChildItemIDs)
  {
    std::string childItemName = shNode->GetItemName(childItemID);
    std::string childItemNamePostfix = childItemName.substr(childItemName.size() - ISODOSE_ROOT_HIERARCHY_NAME_POSTFIX.size());
    if (!childItemNamePostfix.compare(ISODOSE_ROOT_HIERARCHY_NAME_POSTFIX))
    {
      return childItemID;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable: Invalid MRML scene");
    return nullptr;
  }

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> defaultIsodoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME) );
  if (defaultIsodoseColorTableNodes->GetNumberOfItems() > 0)
  {
    if (defaultIsodoseColorTableNodes->GetNumberOfItems() != 1)
    {
      vtkWarningWithObjectMacro(scene, "GetDefaultIsodoseColorTable: Multiple default isodose color table nodes found");
    }

    vtkMRMLColorTableNode* isodoseColorTableNode = vtkMRMLColorTableNode::SafeDownCast(defaultIsodoseColorTableNodes->GetItemAsObject(0));
    return isodoseColorTableNode;
  }

  // Create default isodose color table if does not yet exist
  vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  colorTableNode->SetName(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME);
  colorTableNode->SetTypeToUser();
  colorTableNode->SetSingletonTag(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME);
  //colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);

  colorTableNode->NamesInitialisedOn();
  colorTableNode->SetNumberOfColors(7);
  colorTableNode->GetLookupTable()->SetTableRange(0,6);
  colorTableNode->AddColor("5", 0, 1, 0, 0.2);
  colorTableNode->AddColor("10", 0.5, 1, 0, 0.2);
  colorTableNode->AddColor("15", 1, 1, 0, 0.2);
  colorTableNode->AddColor("20", 1, 0.66, 0, 0.2);
  colorTableNode->AddColor("25", 1, 0.33, 0, 0.2);
  colorTableNode->AddColor("30", 1, 0, 0, 0.2);
  colorTableNode->AddColor("35", 221./255., 18./255., 123./255., 0.2);
  colorTableNode->SaveWithSceneOff();
  
  scene->AddNode(colorTableNode);
  return colorTableNode;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::GetRelativeIsodoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::GetRelativeIsodoseColorTable: Invalid MRML scene");
    return nullptr;
  }

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> relativeIsodoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(RELATIVE_ISODOSE_COLOR_TABLE_NODE_NAME) );
  if (relativeIsodoseColorTableNodes->GetNumberOfItems() > 0)
  {
    if (relativeIsodoseColorTableNodes->GetNumberOfItems() != 1)
    {
      vtkWarningWithObjectMacro(scene, "GetRelativeIsodoseColorTable: Multiple default isodose color table nodes found");
    }

    vtkMRMLColorTableNode* isodoseColorTableNode = vtkMRMLColorTableNode::SafeDownCast(relativeIsodoseColorTableNodes->GetItemAsObject(0));
    return isodoseColorTableNode;
  }

  // Create default isodose color table if does not yet exist
  vtkNew<vtkMRMLColorTableNode> colorTableNode;
  colorTableNode->SetName(RELATIVE_ISODOSE_COLOR_TABLE_NODE_NAME);
  colorTableNode->SetTypeToUser();
  colorTableNode->SetSingletonTag(RELATIVE_ISODOSE_COLOR_TABLE_NODE_NAME);
  //colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);

  colorTableNode->NamesInitialisedOn();
  colorTableNode->SetNumberOfColors(5);
  colorTableNode->GetLookupTable()->SetTableRange(0,4);

  colorTableNode->AddColor("80", 108./255., 0., 208./255., 0.2);
  colorTableNode->AddColor("90", 0., 147./255., 221./255., 0.2);
  colorTableNode->AddColor("100", 1., 1., 0., 0.2);
  colorTableNode->AddColor("105", 1., 0.5, 0., 0.2);
  colorTableNode->AddColor("110", 1., 0., 0., 0.2);
  colorTableNode->SaveWithSceneOff();
  
  scene->AddNode(colorTableNode);
  return colorTableNode;
}

//---------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::LoadDefaultIsodoseColorTable()
{
  // Load default color table file
  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string colorTableFilePath = moduleShareDirectory + "/" + DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME;
  vtkMRMLColorTableNode* colorTableNode = nullptr;

  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile(
      colorTableFilePath.c_str(), DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME );

    // Create temporary lookup table storing the color data while the type of the loaded color table is set to user
    // (workaround for bug #409)
    vtkSmartPointer<vtkLookupTable> tempLookupTable = vtkSmartPointer<vtkLookupTable>::New();
    tempLookupTable->DeepCopy(loadedColorNode->GetLookupTable());

    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
    colorTableNode->SetName(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME);
    colorTableNode->SetSingletonTag(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME);
    //colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
    colorTableNode->NamesInitialisedOn();
    colorTableNode->SetNumberOfColors(tempLookupTable->GetNumberOfColors());
    colorTableNode->SetLookupTable(tempLookupTable);
    colorTableNode->SaveWithSceneOff();
  }
  else
  {
    if (!moduleShareDirectory.empty())
    {
      // Only log warning if the application exists (no warning when running automatic tests)
      vtkWarningMacro("LoadDefaultIsodoseColorTable: Default isodose color table file '" << colorTableFilePath << "' cannot be found");
    }
    // If file is not found, then create it programmatically
    colorTableNode = this->GetDefaultIsodoseColorTable(this->GetMRMLScene());
  }

  if (!colorTableNode)
  {
    vtkErrorMacro("LoadDefaultIsodoseColorTable: Failed to load or create default isodose color table");
  }

  return colorTableNode;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable: Invalid MRML scene");
    return nullptr;
  }

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> defaultDoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(vtkSlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME) );
  if (defaultDoseColorTableNodes->GetNumberOfItems() > 0)
  {
    if (defaultDoseColorTableNodes->GetNumberOfItems() != 1)
    {
      vtkWarningWithObjectMacro(scene, "CreateDefaultDoseColorTable: Multiple default dose color table nodes found");
    }

    vtkMRMLColorTableNode* doseColorTable = vtkMRMLColorTableNode::SafeDownCast(defaultDoseColorTableNodes->GetItemAsObject(0));
    return doseColorTable;
  }

  // Create default dose color table if does not yet exist
  vtkMRMLColorTableNode* defaultIsodoseColorTable = vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable(scene);
  if (!defaultIsodoseColorTable)
  {
    vtkErrorWithObjectMacro(scene, "CreateDefaultDoseColorTable: Unable to access default isodose color table");
    return nullptr;
  }

  vtkSmartPointer<vtkMRMLColorTableNode> defaultDoseColorTable = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  defaultDoseColorTable->SetName(vtkSlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME);
  defaultDoseColorTable->SetTypeToFile();
  defaultDoseColorTable->SetSingletonTag(vtkSlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME);
  //defaultDoseColorTable->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
  defaultDoseColorTable->SetNumberOfColors(256);
  defaultDoseColorTable->SaveWithSceneOff();

  // Create dose color table by stretching the isodose color table
  vtkSlicerRtCommon::StretchDiscreteColorTable(defaultIsodoseColorTable, defaultDoseColorTable);

  scene->AddNode(defaultDoseColorTable);
  return defaultDoseColorTable;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::CreateRelativeDoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::CreateRelativeDoseColorTable: Invalid MRML scene");
    return nullptr;
  }

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> relativeDoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(vtkSlicerRtCommon::RELATIVE_DOSE_COLOR_TABLE_NAME) );
  if (relativeDoseColorTableNodes->GetNumberOfItems() > 0)
  {
    if (relativeDoseColorTableNodes->GetNumberOfItems() != 1)
    {
      vtkWarningWithObjectMacro(scene, "CreateRelativeDoseColorTable: Multiple relative dose color table nodes found");
    }

    vtkMRMLColorTableNode* doseColorTable = vtkMRMLColorTableNode::SafeDownCast(relativeDoseColorTableNodes->GetItemAsObject(0));
    return doseColorTable;
  }

  // Create relative dose color table if does not yet exist
  vtkMRMLColorTableNode* relativeIsodoseColorTable = vtkSlicerIsodoseModuleLogic::GetRelativeIsodoseColorTable(scene);
  if (!relativeIsodoseColorTable)
  {
    vtkErrorWithObjectMacro(scene, "CreateRelativeDoseColorTable: Unable to access relative isodose color table");
    return nullptr;
  }

  vtkSmartPointer<vtkMRMLColorTableNode> relativeDoseColorTable = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  relativeDoseColorTable->SetName(vtkSlicerRtCommon::RELATIVE_DOSE_COLOR_TABLE_NAME);
  relativeDoseColorTable->SetTypeToFile();
  relativeDoseColorTable->SetSingletonTag(vtkSlicerRtCommon::RELATIVE_DOSE_COLOR_TABLE_NAME);
  //relativeDoseColorTable->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
  relativeDoseColorTable->SetNumberOfColors(256);
  relativeDoseColorTable->SaveWithSceneOff();

  // Create dose color table by stretching the isodose color table
  vtkSlicerRtCommon::StretchDiscreteColorTable(relativeIsodoseColorTable, relativeDoseColorTable);

  scene->AddNode(relativeDoseColorTable);
  return relativeDoseColorTable;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::SetupColorTableNodeForDoseVolumeNode(vtkMRMLScalarVolumeNode* doseVolumeNode)
{
  if (!doseVolumeNode)
  {
    vtkErrorMacro("SetupColorTableNodeForDoseVolumeNode: Invalid dose volume");
    return nullptr;
  }

  // Look for existing associated isodose color table node and return if found
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    doseVolumeNode->GetNodeReference(vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE) );
  if (colorTableNode)
  {
    return colorTableNode;
  }

  // Create isodose color table node by cloning the default node
  vtkMRMLColorTableNode* defaultIsodoseColorTableNode = this->GetDefaultIsodoseColorTable(this->GetMRMLScene());
  if (!defaultIsodoseColorTableNode)
  {
    vtkErrorMacro("SetupColorTableNodeForDoseVolumeNode: Failed to get default isodose color table node");
    return nullptr;
  }

  std::string colorTableNodeName(doseVolumeNode->GetName());
  colorTableNodeName.append(ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX);
  colorTableNode = this->GetMRMLApplicationLogic()->GetColorLogic()->CopyNode(defaultIsodoseColorTableNode, colorTableNodeName.c_str());
  this->GetMRMLScene()->AddNode(colorTableNode);
  colorTableNode->SetSingletonOff();
  //colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
  colorTableNode->Delete(); // Release ownership to scene only

  doseVolumeNode->SetNodeReferenceID(vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE, colorTableNode->GetID());

  return colorTableNode;
}

//------------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetNumberOfIsodoseLevels(vtkMRMLIsodoseNode* parameterNode, int newNumberOfColors)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("SetNumberOfIsodoseLevels: Invalid scene or parameter set node");
    return;
  }

  vtkMRMLColorTableNode* colorTableNode = parameterNode->GetColorTableNode();  
  if (!colorTableNode || newNumberOfColors < 1)
  {
    return;
  }

  int currentNumberOfColors = colorTableNode->GetNumberOfColors();
  colorTableNode->SetNumberOfColors(newNumberOfColors);
  colorTableNode->GetLookupTable()->SetTableRange(0, newNumberOfColors-1);

  vtkMRMLIsodoseNode::DoseUnitsType doseUnits = parameterNode->GetDoseUnits();

  bool relativeFlag = false;
  if (parameterNode->GetRelativeRepresentationFlag() 
    && (doseUnits == vtkMRMLIsodoseNode::Gy
    || doseUnits == vtkMRMLIsodoseNode::Unknown))
  {
    relativeFlag = true;
  }
  else if (doseUnits == vtkMRMLIsodoseNode::Relative)
  {
    relativeFlag = true;
  }

  if (!relativeFlag) // absolute values
  {
    // Set the default colors in case the number of colors was less than that in the default table
    for (int colorIndex=currentNumberOfColors; colorIndex<newNumberOfColors; ++colorIndex)
    {
      switch (colorIndex)
      {
      case 0:
        colorTableNode->SetColor(colorIndex, "5", 0, 1, 0, 0.2);
        break;
      case 1:
        colorTableNode->SetColor(colorIndex, "10", 0.5, 1, 0, 0.2);
        break;
      case 2:
        colorTableNode->SetColor(colorIndex, "15", 1, 1, 0, 0.2);
        break;
      case 3:
        colorTableNode->SetColor(colorIndex, "20", 1, 0.66, 0, 0.2);
        break;
      case 4:
        colorTableNode->SetColor(colorIndex, "25", 1, 0.33, 0, 0.2);
        break;
      case 5:
        colorTableNode->SetColor(colorIndex, "30", 1, 0, 0, 0.2);
        break;
      case 6:
        colorTableNode->SetColor(colorIndex, "35", 221./255., 18./255., 123./255., 0.2);
        break;
      }
    }
    // Add colors with index 7 and higher with default gray color
    for (int colorIndex=7; colorIndex<newNumberOfColors; ++colorIndex)
    {
      colorTableNode->SetColor(colorIndex,
        vtkSlicerRtCommon::COLOR_VALUE_INVALID[0], vtkSlicerRtCommon::COLOR_VALUE_INVALID[1], vtkSlicerRtCommon::COLOR_VALUE_INVALID[2], 0.2);
    }
  }
  else // relative
  {
    for (int colorIndex=currentNumberOfColors; colorIndex<newNumberOfColors; ++colorIndex)
    {
      switch (colorIndex)
      {
      case 0:
        colorTableNode->SetColor(colorIndex, "80", 108./255., 0., 208./255., 0.2);
        break;
      case 1:
        colorTableNode->SetColor(colorIndex, "90", 0., 147./255., 221./255., 0.2);
        break;
      case 2:
        colorTableNode->SetColor(colorIndex, "100", 1., 1., 0., 0.2);
        break;
      case 3:
        colorTableNode->SetColor(colorIndex, "105", 1., 0.5, 0., 0.2);
        break;
      case 4:
        colorTableNode->SetColor(colorIndex, "110", 1., 0., 0., 0.2);
        break;
      }
    }
    // Add colors with index 5 and higher with default gray color
    for (int colorIndex=5; colorIndex<newNumberOfColors; ++colorIndex)
    {
      colorTableNode->SetColor(colorIndex,
        vtkSlicerRtCommon::COLOR_VALUE_INVALID[0], vtkSlicerRtCommon::COLOR_VALUE_INVALID[1], vtkSlicerRtCommon::COLOR_VALUE_INVALID[2], 0.2);
    }
  }

  // Something messes up the category, it needs to be set back to SlicerRT
  //colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::CreateIsodoseSurfaces(vtkMRMLIsodoseNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene || !parameterNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid scene or parameter set node");
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to access subject hierarchy node");
    return;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode || !doseVolumeNode->GetImageData())
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid dose volume");
    return;
  }

  scene->StartState(vtkMRMLScene::BatchProcessState); 

  // Get subject hierarchy item for the dose volume
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (!doseShItemID)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to get subject hierarchy item for dose volume '" << doseVolumeNode->GetName() << "'");
    return;
  }

  // Check if that absolute of relative values
  bool relativeFlag = false;
  vtkMRMLIsodoseNode::DoseUnitsType doseUnits = parameterNode->GetDoseUnits();
  if (parameterNode->GetRelativeRepresentationFlag() 
    && (doseUnits == vtkMRMLIsodoseNode::Gy
    || doseUnits == vtkMRMLIsodoseNode::Unknown))
  {
    relativeFlag = true;
  }
  else if (doseUnits == vtkMRMLIsodoseNode::Relative)
  {
    relativeFlag = true;
  }
  std::string isodoseName = relativeFlag ? 
    vtkSlicerIsodoseModuleLogic::ISODOSE_RELATIVE_ROOT_HIERARCHY_NAME_POSTFIX :
    vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_HIERARCHY_NAME_POSTFIX;

  // Segmentation node data
  vtkIdType segmentationShItemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;

  // Setup isodose subject hierarchy folder if isodose representation is not segmentation node
  std::string isodoseFolderName = std::string(doseVolumeNode->GetName()) + isodoseName;

  vtkNew<vtkMRMLSegmentationNode> segmentationNode;
  std::string segmentationNodeName = scene->GenerateUniqueName(isodoseFolderName.c_str());
  segmentationNode->SetName(segmentationNodeName.c_str());
  scene->AddNode(segmentationNode);

  // Set master representation to closed surface
  segmentationNode->SetMasterRepresentationToClosedSurface();

  double doseVolumeSpacing[3] = { 0.0, 0.0, 0.0 };

  // Get image geometry from previously loaded volume if found
  // Segmentation node checks added nodes and sets the geometry parameter in case the referenced volume is loaded later
  segmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(doseVolumeNode);
  doseVolumeNode->GetSpacing(doseVolumeSpacing);

  // Set up subject hierarchy node for segmentation
  segmentationShItemID = shNode->CreateItem(shNode->GetSceneItemID(), segmentationNode);
  shNode->SetItemUID(segmentationShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), "");
  shNode->SetItemAttribute(segmentationShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME, "");
  shNode->SetItemAttribute(segmentationShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName(), "" );

  // Setup segmentation display and storage
  vtkNew<vtkMRMLSegmentationDisplayNode> segmentationDisplayNode;
  scene->AddNode(segmentationDisplayNode);
  segmentationNode->SetAndObserveDisplayNodeID(segmentationDisplayNode->GetID());
  segmentationDisplayNode->SetBackfaceCulling(0);

  // Get color table
  vtkMRMLColorTableNode* colorTableNode = parameterNode->GetColorTableNode();
  if (!colorTableNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to get isodose color table node for dose volume " << doseVolumeNode->GetName());
    return;
  }

  // Check that range is valid for dose and relative dose
  // Set dose unit name
  std::string doseUnitName = "Gy";
  switch (doseUnits)
  {
  case vtkMRMLIsodoseNode::Gy:
    break;
  case vtkMRMLIsodoseNode::Relative:
    doseUnitName = "%";
    break;
  case vtkMRMLIsodoseNode::Unknown:
  default:
    doseUnitName = "MU";
    break;
  }

  // force percentage dose units for relative isodose representation
  if (relativeFlag)
  {
    doseUnitName = "%";
  }
  
  // Progress
  int progressStepCount = colorTableNode->GetNumberOfColors() + 1 /* reslice step */;
  int currentProgressStep = 0;

  // Reslice dose volume
  vtkNew<vtkMatrix4x4> inputIJK2RASMatrix;
  doseVolumeNode->GetIJKToRASMatrix(inputIJK2RASMatrix);
  vtkNew<vtkMatrix4x4> inputRAS2IJKMatrix;
  doseVolumeNode->GetRASToIJKMatrix(inputRAS2IJKMatrix); 

  vtkNew<vtkTransform> outputIJK2IJKResliceTransform; 
  outputIJK2IJKResliceTransform->Identity();
  outputIJK2IJKResliceTransform->PostMultiply();
  outputIJK2IJKResliceTransform->SetMatrix(inputIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = doseVolumeNode->GetParentTransformNode();
  vtkNew<vtkMatrix4x4> inputRAS2RASMatrix;
  if (inputVolumeNodeTransformNode)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(inputRAS2RASMatrix);  
    outputIJK2IJKResliceTransform->Concatenate(inputRAS2RASMatrix);
  }
  
  outputIJK2IJKResliceTransform->Concatenate(inputRAS2IJKMatrix);
  outputIJK2IJKResliceTransform->Inverse();

  int dimensions[3] = {0, 0, 0};
  doseVolumeNode->GetImageData()->GetDimensions(dimensions);
  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputData(doseVolumeNode->GetImageData());
  reslice->SetOutputOrigin(0, 0, 0);
  reslice->SetOutputSpacing(1, 1, 1);
  reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  reslice->SetResliceTransform(outputIJK2IJKResliceTransform);
  reslice->Update();

  // Report progress
  ++currentProgressStep;
  double progress = (double)(currentProgressStep) / (double)progressStepCount;
  this->InvokeEvent(vtkSlicerRtCommon::ProgressUpdated, (void*)&progress);

  // reference value for relative representation
  double referenceValue = parameterNode->GetReferenceDoseValue();

  std::vector< std::tuple< std::string, std::array< double, 6>, vtkSmartPointer<vtkPolyData> > > singleBorderData;

  // Create isodose surfaces
  for (int i = 0; i < colorTableNode->GetNumberOfColors(); i++)
  {
    std::array< double, 6 > val;
    const char* strIsoLevel = colorTableNode->GetColorName(i);
    double isoLevel = vtkVariant(strIsoLevel).ToDouble();
    // change isoLevel value for relative representation
    if (relativeFlag)
    {
      if (doseUnits != vtkMRMLIsodoseNode::Relative)
      {
        isoLevel = isoLevel * referenceValue / 100.;
      }
    }
    colorTableNode->GetColor(i, val.data());
    
    vtkNew<vtkImageMarchingCubes> marchingCubes;
    marchingCubes->SetInputData(reslice->GetOutput());
    marchingCubes->SetNumberOfContours(1); 
    marchingCubes->SetValue(0, isoLevel);
    marchingCubes->ComputeScalarsOff();
    marchingCubes->ComputeGradientsOff();
    marchingCubes->ComputeNormalsOff();
    marchingCubes->Update();

    vtkPolyData* isoPolyData = marchingCubes->GetOutput();
    if (isoPolyData && isoPolyData->GetNumberOfPoints() > 0)
    {
      vtkNew<vtkTriangleFilter> triangleFilter;
      triangleFilter->SetInputData(marchingCubes->GetOutput());
      triangleFilter->Update();

      vtkNew<vtkDecimatePro> decimate;
      decimate->SetInputData(triangleFilter->GetOutput());
      decimate->SetTargetReduction(0.6);
      decimate->SetFeatureAngle(60);
      decimate->SplittingOff();
      decimate->PreserveTopologyOn();
      decimate->SetMaximumError(1);
      decimate->Update();

      vtkNew<vtkWindowedSincPolyDataFilter> smootherSinc;
      smootherSinc->SetPassBand(0.1);
      smootherSinc->SetInputData(decimate->GetOutput());
      smootherSinc->SetNumberOfIterations(2);
      smootherSinc->FeatureEdgeSmoothingOff();
      smootherSinc->BoundarySmoothingOff();
      smootherSinc->Update();

      vtkNew<vtkPolyDataNormals> normals;
      normals->SetInputData(smootherSinc->GetOutput());
      normals->ComputePointNormalsOn();
      normals->SetFeatureAngle(60);
      normals->Update();

      vtkNew<vtkTransform> inputIJKToRASTransform;
      inputIJKToRASTransform->Identity();
      inputIJKToRASTransform->SetMatrix(inputIJK2RASMatrix);

      vtkNew<vtkTransformPolyDataFilter> transformPolyData;
      transformPolyData->SetInputData(normals->GetOutput());
      transformPolyData->SetTransform(inputIJKToRASTransform);
      transformPolyData->Update();

      singleBorderData.push_back({ std::string(strIsoLevel), val, transformPolyData->GetOutput()});
    }

    // Report progress
    ++currentProgressStep;
    progress = (double)(currentProgressStep) / (double)progressStepCount;
    this->InvokeEvent(vtkSlicerRtCommon::ProgressUpdated, (void*)&progress);
  } // For all isodose levels

  // Update dose color table based on isodose
  this->UpdateDoseColorTableFromIsodose(parameterNode);

  scene->EndState(vtkMRMLScene::BatchProcessState);

  switch (parameterNode->GetBorderMode())
  {
  case vtkMRMLIsodoseNode::Double:
    {
      for (size_t i = 1; i < singleBorderData.size(); ++i)
      {
        vtkNew<vtkAppendPolyData> append;
        std::string prevLevelString;
        vtkSmartPointer<vtkPolyData> prevPolyData;
        std::tie( prevLevelString, std::ignore, prevPolyData) = singleBorderData[i - 1];

        std::string currLevelString;
        std::array< double, 6 > currColor;
        vtkSmartPointer<vtkPolyData> currPolyData;
        std::tie( currLevelString, currColor, currPolyData) = singleBorderData[i];
        const double* currVal = currColor.data();

        append->AddInputData(prevPolyData);
        append->AddInputData(currPolyData);
        append->Update();

        // Add segment for double border isolevels
        std::string name = prevLevelString + "-" + currLevelString;
        vtkNew<vtkSegment> segment;
        segment->SetName(name.c_str());
        segment->SetColor(currVal[0], currVal[1], currVal[2]);
        segment->AddRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(), append->GetOutput());
        segmentationNode->GetSegmentation()->AddSegment(segment);

        // Add DICOM ROI number as tag to the segment
        segment->SetTag(vtkSlicerRtCommon::DICOMRTIMPORT_ROI_NUMBER_SEGMENT_TAG_NAME, std::to_string(i).c_str());
      }
    }
    break;
  case vtkMRMLIsodoseNode::Single:
    {
      // Fill segmentation node data with single border mode
      int i = 0;
      for (const auto& tupleValue : singleBorderData)
      {
        std::string isoLevelString;
        std::array< double, 6 > colorValue;
        vtkSmartPointer<vtkPolyData> polyData;
        std::tie( isoLevelString, colorValue, polyData) = tupleValue;
        const double* val = colorValue.data();
    
        // Add segment for current structure
        vtkNew<vtkSegment> segment;
        segment->SetName(isoLevelString.c_str());
        segment->SetColor(val[0], val[1], val[2]);
        segment->AddRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(), polyData);
        segmentationNode->GetSegmentation()->AddSegment(segment);

        // Add DICOM ROI number as tag to the segment
        segment->SetTag(vtkSlicerRtCommon::DICOMRTIMPORT_ROI_NUMBER_SEGMENT_TAG_NAME, std::to_string(i).c_str());
        ++i;
      } // For all isodose levels
    }
    break;
  }
  // Set fill opacity and parent
//  segmentationDisplayNode->SetAllSegmentsOpacity2DFill(0.);
//  segmentationDisplayNode->SetAllSegmentsVisibility(true);
  // dose volume as a parent in subject hierarchy
  shNode->SetItemParent( segmentationShItemID, doseShItemID);
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::UpdateDoseColorTableFromIsodose(vtkMRMLIsodoseNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene || !parameterNode)
  {
    vtkErrorMacro("UpdateDoseColorTableFromIsodose: Invalid scene or parameter set node");
    return;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode || !doseVolumeNode->GetImageData())
  {
    vtkErrorMacro("UpdateDoseColorTableFromIsodose: Invalid dose volume");
    return;
  }
  vtkMRMLScalarVolumeDisplayNode* doseVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(
    doseVolumeNode->GetDisplayNode() );
  if (!doseVolumeDisplayNode)
  {
    vtkErrorMacro("UpdateDoseColorTableFromIsodose: Failed to get display node for dose volume" << doseVolumeNode->GetName());
    return;
  }

  // Look for associated isodose color table
  vtkMRMLColorTableNode* isodoseColorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    doseVolumeNode->GetNodeReference(vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE) );
  if (!isodoseColorTableNode)
  {
    vtkDebugMacro("UpdateDoseColorTableFromIsodose: No isodose color table for dose volume " << doseVolumeNode->GetName());
    return;
  }

  // Get dose color table associated to isodose color table
  vtkMRMLColorTableNode* doseColorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    isodoseColorTableNode->GetNodeReference(vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE) );
  if (!doseColorTableNode)
  {
    doseColorTableNode = vtkMRMLColorTableNode::New();
    std::string colorTableNodeName(doseVolumeNode->GetName());
    colorTableNodeName.append("_DoseColorTable");
    doseColorTableNode->SetName(colorTableNodeName.c_str());
    doseColorTableNode->SetTypeToUser();
    //doseColorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
    doseColorTableNode->SetNumberOfColors(256);
    scene->AddNode(doseColorTableNode);
    doseColorTableNode->Delete(); // Release ownership to scene only

    isodoseColorTableNode->SetNodeReferenceID(vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE, doseColorTableNode->GetID());
  }

  // Create dose color table by stretching the isodose color table
  vtkSlicerRtCommon::StretchDiscreteColorTable(isodoseColorTableNode, doseColorTableNode);
  doseVolumeDisplayNode->SetAndObserveColorNodeID(doseColorTableNode->GetID());

  // Set window/level to match the isodose levels
  int minDoseInDefaultIsodoseLevels = vtkVariant(isodoseColorTableNode->GetColorName(0)).ToInt();
  int maxDoseInDefaultIsodoseLevels = vtkVariant(isodoseColorTableNode->GetColorName(isodoseColorTableNode->GetNumberOfColors()-1)).ToInt();

  doseVolumeDisplayNode->SetWindowLevelMinMax(minDoseInDefaultIsodoseLevels, maxDoseInDefaultIsodoseLevels);
  doseVolumeDisplayNode->AutoWindowLevelOn();

  // Get dose grid scaling
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
  {
    vtkErrorMacro("UpdateDoseColorTableFromIsodose: Failed to access subject hierarchy node");
    return;
  }
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (!doseShItemID)
  {
    vtkErrorMacro("UpdateDoseColorTableFromIsodose: Failed to get subject hierarchy item for dose volume '" << doseVolumeNode->GetName() << "'");
    return;
  }
  vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(doseShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  std::string doseUnitValueInStudy = shNode->GetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME);
  double doseUnitValue = vtkVariant(doseUnitValueInStudy).ToDouble();

  // Set display threshold
  doseVolumeDisplayNode->AutoThresholdOff();
  doseVolumeDisplayNode->SetLowerThreshold(0.5 * doseUnitValue);
  doseVolumeDisplayNode->SetApplyThreshold(1);
}
