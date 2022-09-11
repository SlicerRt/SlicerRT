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

// Colors includes
#include "vtkSlicerColorLogic.h"

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
#include <vtkMRMLColorLegendDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>

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
#include <vtkAppendPolyData.h>
#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkFloatArray.h>

#include "vtksys/SystemTools.hxx"

//----------------------------------------------------------------------------
const char* DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME = "Isodose_ColorTable.ctbl";
const char* DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME = "Isodose_ColorTable_Default";
const char* DEFAULT_ISODOSE_COLOR_TABLECOPY_NODE_NAME = "Isodose_ColorTable_DefaultCopy";
const char* RELATIVE_ISODOSE_COLOR_TABLE_NODE_NAME = "Isodose_ColorTable_Relative";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_MODEL_NODE_NAME_POSTFIX = "_IsodoseLevels";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX = "_IsodoseColorTable";

std::string vtkSlicerIsodoseModuleLogic::IsodoseColorNodeCopyUniqueName = DEFAULT_ISODOSE_COLOR_TABLECOPY_NODE_NAME;

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
  // Create a copy of default isodose color table. The copy can be edited.
  if (isodoseColorTableNode)
  {
    // Create a copy of isodose color table with unique name
    std::string uniqueName = this->GetMRMLScene()->GenerateUniqueName(vtkSlicerIsodoseModuleLogic::IsodoseColorNodeCopyUniqueName.c_str());
    // Get "Colors" module logic and copy a isodose color node
    vtkMRMLColorTableNode* colorNode = vtkMRMLColorLogic::CopyNode(isodoseColorTableNode, uniqueName.c_str());
    if (colorNode)
    {
      colorNode->SetSingletonTag(DEFAULT_ISODOSE_COLOR_TABLECOPY_NODE_NAME); // set tag that it's a copy
      if (!this->GetMRMLScene()->AddNode(colorNode))
      {
        vtkErrorMacro("SetMRMLSceneInternal: Failed to add copy of default isodose color node to scene");
      }
      colorNode->Delete();
    }
    // Create dose color table for relative and absolute doses
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

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable: Invalid MRML scene");
    return nullptr;
  }

  // Check if copy with unique name of default isodose color table node already exists
  // Get all the nodes with name template starting with Isodose_ColorTable_Default
  vtkSmartPointer<vtkCollection> defaultIsodoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(vtkSlicerIsodoseModuleLogic::IsodoseColorNodeCopyUniqueName.c_str()) );
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
  vtkNew<vtkMRMLColorTableNode> colorTableNode;
  colorTableNode->SetName(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME);
  colorTableNode->SetTypeToUser();
  colorTableNode->SetSingletonTag(DEFAULT_ISODOSE_COLOR_TABLE_NODE_NAME);
  //colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);

  colorTableNode->NamesInitialisedOn();
  colorTableNode->SetNumberOfColors(7);
  colorTableNode->GetLookupTable()->SetTableRange(0,6);
  colorTableNode->AddColor("5", 0., 1., 0., 1.);
  colorTableNode->AddColor("10", 0.5, 1., 0., 1.);
  colorTableNode->AddColor("15", 1., 1., 0., 1.);
  colorTableNode->AddColor("20", 1., 0.66, 0., 1.);
  colorTableNode->AddColor("25", 1., 0.33, 0., 1.);
  colorTableNode->AddColor("30", 1., 0., 0., 1.);
  colorTableNode->AddColor("35", 221./255., 18./255., 123./255., 1.);
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

  colorTableNode->AddColor("80", 108./255., 0., 208./255., 1.);
  colorTableNode->AddColor("90", 0., 147./255., 221./255., 1.);
  colorTableNode->AddColor("100", 1., 1., 0., 1.);
  colorTableNode->AddColor("105", 1., 0.5, 0., 1.);
  colorTableNode->AddColor("110", 1., 0., 0., 1.);
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
    vtkSmartPointer<vtkLookupTable> tempLookupTable = vtkSmartPointer<vtkLookupTable>::Take(loadedColorNode->CreateLookupTableCopy());

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
  colorTableNode = vtkMRMLColorLogic::CopyNode(defaultIsodoseColorTableNode, colorTableNodeName.c_str());
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
        colorTableNode->SetColor(colorIndex, "5", 0., 1., 0., 1.);
        break;
      case 1:
        colorTableNode->SetColor(colorIndex, "10", 0.5, 1., 0., 1.);
        break;
      case 2:
        colorTableNode->SetColor(colorIndex, "15", 1., 1., 0., 1.);
        break;
      case 3:
        colorTableNode->SetColor(colorIndex, "20", 1., 0.66, 0., 1.);
        break;
      case 4:
        colorTableNode->SetColor(colorIndex, "25", 1., 0.33, 0., 1.);
        break;
      case 5:
        colorTableNode->SetColor(colorIndex, "30", 1., 0., 0., 1.);
        break;
      case 6:
        colorTableNode->SetColor(colorIndex, "35", 221./255., 18./255., 123./255., 1.);
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
        colorTableNode->SetColor(colorIndex, "80", 108./255., 0., 208./255., 1.);
        break;
      case 1:
        colorTableNode->SetColor(colorIndex, "90", 0., 147./255., 221./255., 1.);
        break;
      case 2:
        colorTableNode->SetColor(colorIndex, "100", 1., 1., 0., 1.);
        break;
      case 3:
        colorTableNode->SetColor(colorIndex, "105", 1., 0.5, 0., 1.);
        break;
      case 4:
        colorTableNode->SetColor(colorIndex, "110", 1., 0., 0., 1.);
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
bool vtkSlicerIsodoseModuleLogic::CreateIsodoseSurfaces(vtkMRMLIsodoseNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene || !parameterNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid scene or parameter set node");
    return false;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to access subject hierarchy node");
    return false;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode || !doseVolumeNode->GetImageData())
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid dose volume");
    return false;
  }

  scene->StartState(vtkMRMLScene::BatchProcessState);

  // Get subject hierarchy item for the dose volume
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (!doseShItemID)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to get subject hierarchy item for dose volume '" << doseVolumeNode->GetName() << "'");
    return false;
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

  // Get color table
  vtkMRMLColorTableNode* colorTableNode = parameterNode->GetColorTableNode();
  if (!colorTableNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to get isodose color table node for dose volume " << doseVolumeNode->GetName());
    return false;
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
  if (inputVolumeNodeTransformNode!=nullptr)
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
  vtkSmartPointer<vtkImageData> reslicedDoseVolumeImage = reslice->GetOutput();

  // Report progress
  ++currentProgressStep;
  double progress = (double)(currentProgressStep) / (double)progressStepCount;
  this->InvokeEvent(vtkSlicerRtCommon::ProgressUpdated, (void*)&progress);

  // reference value for relative representation
  double referenceValue = parameterNode->GetReferenceDoseValue();

  // Create isodose surfaces
  vtkNew<vtkAppendPolyData> append;
  vtkNew<vtkFloatArray> colors;
  colors->SetNumberOfComponents(1);
  colors->SetName("isolevels");

  bool res = true;
  for (int i = 0; i < colorTableNode->GetNumberOfColors(); i++)
  {
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

    vtkNew<vtkImageMarchingCubes> marchingCubes;
    marchingCubes->SetInputData(reslicedDoseVolumeImage);
    marchingCubes->SetNumberOfContours(1);
    marchingCubes->SetValue(0, isoLevel);
    marchingCubes->ComputeScalarsOff();
    marchingCubes->ComputeGradientsOff();
    marchingCubes->ComputeNormalsOff();
    marchingCubes->Update();

    vtkSmartPointer<vtkPolyData> isoPolyData = marchingCubes->GetOutput();
    if (isoPolyData->GetNumberOfPoints() >= 1)
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
      smootherSinc->SetInputData(decimate->GetOutput() );
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

      vtkPolyData* isoSurface = transformPolyData->GetOutput();
      for (vtkIdType i = 0; i < isoSurface->GetNumberOfPoints(); ++i)
      {
        colors->InsertNextTuple1(static_cast<float>(isoLevel));
      }
      append->AddInputData(isoSurface);
    }

    // Report progress
    ++currentProgressStep;
    progress = (double)(currentProgressStep) / (double)progressStepCount;
    this->InvokeEvent(vtkSlicerRtCommon::ProgressUpdated, (void*)&progress);
  } // For all isodose levels

  // update appended isosurfaces
  append->Update();

  // create or update display node
  vtkPolyData* isoSurfaces = append->GetOutput();
  if (isoSurfaces)
  {
    isoSurfaces->GetPointData()->SetScalars(colors);

    vtkMRMLModelDisplayNode* displayNode = nullptr;

    vtkSmartPointer<vtkMRMLModelNode> isodoseModelNode;
    if (parameterNode->GetIsosurfacesModelNode())
    {
      isodoseModelNode = parameterNode->GetIsosurfacesModelNode();
      std::string name = std::string(doseVolumeNode->GetName()) + vtkSlicerIsodoseModuleLogic::ISODOSE_MODEL_NODE_NAME_POSTFIX;
      isodoseModelNode->SetName(name.c_str());
      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(isodoseModelNode->GetDisplayNode());
    }
    else
    {
      isodoseModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      std::string tempname = std::string(doseVolumeNode->GetName()) + vtkSlicerIsodoseModuleLogic::ISODOSE_MODEL_NODE_NAME_POSTFIX;
      std::string uniqueName = scene->GenerateUniqueName(tempname.c_str());
      isodoseModelNode->SetName(uniqueName.c_str());
      scene->AddNode(isodoseModelNode);
      // Set and observe isosurfaces node
      parameterNode->SetAndObserveIsosurfacesModelNode(isodoseModelNode);

      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(scene->AddNewNodeByClass("vtkMRMLModelDisplayNode"));
      isodoseModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
    }

    // Disable backface culling to make the back side of the model visible as well
    displayNode->SetBackfaceCulling(0);

    displayNode->Visibility2DOn();
    displayNode->VisibilityOn();
    displayNode->SetActiveScalarName("isolevels");
    displayNode->SetAutoScalarRange(true);
    displayNode->SetAndObserveColorNodeID(colorTableNode->GetID());
    displayNode->SetScalarVisibility(true);

    isodoseModelNode->SetSelectable(1);
    isodoseModelNode->SetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_ISODOSE_MODEL_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1"); // The attribute above distinguishes isodoses from regular models
    isodoseModelNode->SetAndObservePolyData(isoSurfaces);

    // Put the new node as a child of dose volume
    vtkIdType isodoseModelItemID = shNode->GetItemByDataNode(isodoseModelNode);
    if (isodoseModelItemID) // There is no automatic SH creation in automatic tests 
    {
      shNode->SetItemParent(isodoseModelItemID, doseShItemID);
    }

    // Update dose color table based on isodose
    this->UpdateDoseColorTableFromIsodose(parameterNode);
  }
  else
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to create isosurfaces for dose volume " << doseVolumeNode->GetName());
    res = false;
  }

  scene->EndState(vtkMRMLScene::BatchProcessState);
  return res;
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
  double minDoseInDefaultIsodoseLevels = vtkVariant(isodoseColorTableNode->GetColorName(0)).ToDouble();
  double maxDoseInDefaultIsodoseLevels = vtkVariant(isodoseColorTableNode->GetColorName(isodoseColorTableNode->GetNumberOfColors()-1)).ToDouble();

  // Check if the isodose levels are relative values
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
  if (relativeFlag)
  {
    // reference value for relative representation
    double referenceValue = parameterNode->GetReferenceDoseValue();

    // If the isodose levels are relative, we need to scale the min/max isodose level for dose volume scalars.
    minDoseInDefaultIsodoseLevels *= referenceValue / 100.0;
    maxDoseInDefaultIsodoseLevels *= referenceValue / 100.0;
  }

  doseVolumeDisplayNode->SetWindowLevelMinMax(minDoseInDefaultIsodoseLevels, maxDoseInDefaultIsodoseLevels);
  doseVolumeDisplayNode->AutoWindowLevelOff();

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

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetColorLegendDefaults(vtkMRMLIsodoseNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("SetColorLegendDefaults: Invalid parameter set node");
    return;
  }

  vtkMRMLModelNode* isodoseModelNode = parameterNode->GetIsosurfacesModelNode();
  if (!isodoseModelNode)
  {
    vtkErrorMacro("SetColorLegendDefaults: Invalid isodose surface model node");
    return;
  }

  vtkMRMLColorLegendDisplayNode* dispNode = vtkSlicerColorLogic::GetColorLegendDisplayNode(isodoseModelNode);
  if (!dispNode)
  {
    vtkErrorMacro("SetColorLegendDefaults: Failed to get color legend display node for isodose model node" << isodoseModelNode->GetName());
    return;
  }

  // Check that range is valid for dose and relative dose
  // Set dose unit name
  std::string doseUnits = "Gy";
  std::string title = "Isolevels, ";
  switch (parameterNode->GetDoseUnits())
  {
  case vtkMRMLIsodoseNode::Gy:
    break;
  case vtkMRMLIsodoseNode::Relative:
    doseUnits = "%";
    title = "Relative isolevels, ";
    break;
  case vtkMRMLIsodoseNode::Unknown:
  default:
    doseUnits = "MU";
    break;
  }

  // force percentage dose units for relative isodose representation
  if (parameterNode->GetRelativeRepresentationFlag())
  {
    doseUnits = "%";
    title = "Relative isolevels, ";
  }
  title += doseUnits;

  dispNode->SetTitleText(title);
  dispNode->SetSize( 0.08, 0.46);
  dispNode->SetPosition( 0.98, 0.5);
}
