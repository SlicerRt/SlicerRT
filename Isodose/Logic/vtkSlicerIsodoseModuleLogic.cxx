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
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>

// MRMLLogic includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageMarchingCubes.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageReslice.h>
#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkGeneralTransform.h>
#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkObjectFactory.h>
#include "vtksys/SystemTools.hxx"

//----------------------------------------------------------------------------
const char* vtkSlicerIsodoseModuleLogic::DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME = "Isodose_ColorTable.ctbl";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_MODEL_NODE_NAME_PREFIX = "IsodoseLevel_";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX = "IsodoseParameterSet_";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = "_IsodoseModels";
const std::string vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_SUBJECT_HIERARCHY_NODE_NAME_POSTFIX = "_IsodoseSurfaces";

static const char* ISODOSE_ROOT_MODEL_HIERARCHY_REFERENCE_ROLE = "isodoseRootModelHierarchyRef";
static const char* ISODOSE_ROOT_MODEL_HIERARCHY_DISPLAY_REFERENCE_ROLE = "isodoseRootModelHierarchyDisplayRef";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerIsodoseModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::vtkSlicerIsodoseModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::~vtkSlicerIsodoseModuleLogic()
{
}

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
  vtkMRMLColorTableNode* isodoseColorTableNode = NULL;
  if ( (isodoseColorTableNode = this->LoadDefaultIsodoseColorTable()) == NULL )
  {
    isodoseColorTableNode = vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable(newScene);
  }
  // Create dose color table if load/create succeeded
  if (isodoseColorTableNode)
  {
    vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(newScene);
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
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLIsodoseNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
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
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
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
vtkMRMLModelHierarchyNode* vtkSlicerIsodoseModuleLogic::GetRootModelHierarchyNode(vtkMRMLIsodoseNode* parameterNode)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("GetRootModelHierarchyNode: Invalid scene or parameter set node!");
    return NULL;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode || !doseVolumeNode->GetImageData())
  {
    vtkErrorMacro("GetRootModelHierarchyNode: Invalid dose volume!");
    return NULL;
  }

  return vtkMRMLModelHierarchyNode::SafeDownCast( doseVolumeNode->GetNodeReference(ISODOSE_ROOT_MODEL_HIERARCHY_REFERENCE_ROLE) );
}

//------------------------------------------------------------------------------
std::string vtkSlicerIsodoseModuleLogic::GetIsodoseColorTableNodeName()
{
  return vtksys::SystemTools::GetFilenameWithoutExtension(vtkSlicerIsodoseModuleLogic::DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME);
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable: Invalid MRML scene!");
    return NULL;
  }

  std::string colorTableNodeName = vtkSlicerIsodoseModuleLogic::GetIsodoseColorTableNodeName();

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> defaultIsodoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(colorTableNodeName.c_str()) );
  if (defaultIsodoseColorTableNodes->GetNumberOfItems() > 0)
  {
    if (defaultIsodoseColorTableNodes->GetNumberOfItems() != 1)
    {
      vtkWarningWithObjectMacro(scene, "CreateDefaultIsodoseColorTable: Multiple default isodose color table nodes found!");
    }

    vtkMRMLColorTableNode* isodoseColorTableNode = vtkMRMLColorTableNode::SafeDownCast(defaultIsodoseColorTableNodes->GetItemAsObject(0));
    return isodoseColorTableNode;
  }

  // Create default isodose color table if does not yet exist
  vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  colorTableNode->SetName(colorTableNodeName.c_str());
  colorTableNode->SetTypeToUser();
  colorTableNode->SetSingletonTag(colorTableNodeName.c_str());
  colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);

  colorTableNode->NamesInitialisedOn();
  colorTableNode->SetNumberOfColors(6);
  colorTableNode->GetLookupTable()->SetTableRange(0,5);
  colorTableNode->AddColor("5", 0, 1, 0, 0.2);
  colorTableNode->AddColor("10", 0.5, 1, 0, 0.2);
  colorTableNode->AddColor("15", 1, 1, 0, 0.2);
  colorTableNode->AddColor("20", 1, 0.66, 0, 0.2);
  colorTableNode->AddColor("25", 1, 0.33, 0, 0.2);
  colorTableNode->AddColor("30", 1, 0, 0, 0.2);
  colorTableNode->SaveWithSceneOff();
  
  scene->AddNode(colorTableNode);
  return colorTableNode;
}

//---------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::LoadDefaultIsodoseColorTable()
{
  // Load default color table file
  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string colorTableFilePath = moduleShareDirectory + "/" + vtkSlicerIsodoseModuleLogic::DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME;
  vtkMRMLColorTableNode* colorTableNode = NULL;

  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    std::string colorTableNodeName = vtkSlicerIsodoseModuleLogic::GetIsodoseColorTableNodeName();
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile(
      colorTableFilePath.c_str(), colorTableNodeName.c_str() );

    // Create temporary lookup table storing the color data while the type of the loaded color table is set to user
    // (workaround for bug #409)
    vtkSmartPointer<vtkLookupTable> tempLookupTable = vtkSmartPointer<vtkLookupTable>::New();
    tempLookupTable->DeepCopy(loadedColorNode->GetLookupTable());

    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
    colorTableNode->SetName(colorTableNodeName.c_str());

    colorTableNode->SetTypeToUser();
    colorTableNode->NamesInitialisedOn();
    colorTableNode->SetNumberOfColors(tempLookupTable->GetNumberOfColors());
    colorTableNode->SetLookupTable(tempLookupTable);

    colorTableNode->SaveWithSceneOff();
    colorTableNode->SetSingletonTag(colorTableNode->GetName());
    colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  }
  else
  {
    if (!moduleShareDirectory.empty())
    {
      // Only log warning if the application exists (no warning when running automatic tests)
      vtkWarningMacro("LoadDefaultIsodoseColorTable: Default isodose color table file '" << colorTableFilePath << "' cannot be found!");
    }
    // If file is not found, then create it programmatically
    colorTableNode = this->CreateDefaultIsodoseColorTable(this->GetMRMLScene());
  }

  if (!colorTableNode)
  {
    vtkErrorMacro("LoadDefaultIsodoseColorTable: Failed to load or create default isodose color table!");
  }

  return colorTableNode;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkGenericWarningMacro("vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable: Invalid MRML scene!");
    return NULL;
  }

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> defaultDoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    scene->GetNodesByName(SlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME) );
  if (defaultDoseColorTableNodes->GetNumberOfItems() > 0)
  {
    if (defaultDoseColorTableNodes->GetNumberOfItems() != 1)
    {
      vtkWarningWithObjectMacro(scene, "CreateDefaultDoseColorTable: Multiple default dose color table nodes found!");
    }

    vtkMRMLColorTableNode* doseColorTable = vtkMRMLColorTableNode::SafeDownCast(defaultDoseColorTableNodes->GetItemAsObject(0));
    return doseColorTable;
  }

  // Create default dose color table if does not yet exist
  vtkMRMLColorTableNode* defaultIsodoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable(scene);
  if (!defaultIsodoseColorTable)
  {
    vtkErrorWithObjectMacro(scene, "CreateDefaultDoseColorTable: Unable to access default isodose color table!");
    return NULL;
  }

  vtkSmartPointer<vtkMRMLColorTableNode> defaultDoseColorTable = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  defaultDoseColorTable->SetName(SlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME);
  defaultDoseColorTable->SetTypeToUser();
  defaultDoseColorTable->SetSingletonTag(SlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME);
  defaultDoseColorTable->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  defaultDoseColorTable->SetNumberOfColors(256);
  defaultDoseColorTable->SaveWithSceneOff();

  // Create dose color table by stretching the isodose color table
  SlicerRtCommon::StretchDiscreteColorTable(defaultIsodoseColorTable, defaultDoseColorTable);

  scene->AddNode(defaultDoseColorTable);
  return defaultDoseColorTable;
}

//------------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetNumberOfIsodoseLevels(vtkMRMLIsodoseNode* parameterNode, int newNumberOfColors)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("SetNumberOfIsodoseLevels: Invalid scene or parameter set node!");
    return;
  }

  vtkMRMLColorTableNode* colorTableNode = parameterNode->GetColorTableNode();  
  if (!colorTableNode || newNumberOfColors < 1)
  {
    return;
  }

  // Set the default colors in case the number of colors was less than that in the default table
  colorTableNode->SetNumberOfColors(6);
  colorTableNode->SetColor(0, "5", 0, 1, 0, 0.2);
  colorTableNode->SetColor(1, "10", 0.5, 1, 0, 0.2);
  colorTableNode->SetColor(2, "15", 1, 1, 0, 0.2);
  colorTableNode->SetColor(3, "20", 1, 0.66, 0, 0.2);
  colorTableNode->SetColor(4, "25", 1, 0.33, 0, 0.2);
  colorTableNode->SetColor(5, "30", 1, 0, 0, 0.2);

  colorTableNode->SetNumberOfColors(newNumberOfColors);
  colorTableNode->GetLookupTable()->SetTableRange(0, newNumberOfColors-1);
  for (int colorIndex=6; colorIndex<newNumberOfColors; ++colorIndex)
  {
    colorTableNode->SetColor(colorIndex, SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1], SlicerRtCommon::COLOR_VALUE_INVALID[2], 0.2);
  }

  // Something messes up the category, it needs to be set back to SlicerRT
  colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::CreateIsodoseSurfaces(vtkMRMLIsodoseNode* parameterNode)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid scene or parameter set node!");
    return;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode || !doseVolumeNode->GetImageData())
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid dose volume!");
    return;
  }

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Get subject hierarchy node for the dose volume
  vtkMRMLSubjectHierarchyNode* doseVolumeSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(doseVolumeNode);
  if (!doseVolumeSubjectHierarchyNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to get subject hierarchy node for dose volume '" << doseVolumeNode->GetName() << "'");
  }

  // Model hierarchy node for the loaded structure set
  vtkMRMLModelHierarchyNode* rootModelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast( doseVolumeNode->GetNodeReference(ISODOSE_ROOT_MODEL_HIERARCHY_REFERENCE_ROLE) );
  if (!rootModelHierarchyNode)
  {
    rootModelHierarchyNode = vtkMRMLModelHierarchyNode::New();
    this->GetMRMLScene()->AddNode(rootModelHierarchyNode);
    rootModelHierarchyNode->Delete();
  }
  else 
  {
    std::vector< vtkMRMLHierarchyNode *> children = rootModelHierarchyNode->GetChildrenNodes(); 
    for (unsigned int i=0; i<children.size(); i++)
    {
      vtkMRMLHierarchyNode *child = children[i];
      vtkMRMLModelNode* mnode = vtkMRMLModelNode::SafeDownCast(child->GetAssociatedNode());
      this->GetMRMLScene()->RemoveNode(mnode);
      this->GetMRMLScene()->RemoveNode(child);
    }
    this->GetMRMLScene()->RemoveNode(rootModelHierarchyNode);
    rootModelHierarchyNode = vtkMRMLModelHierarchyNode::New();
    this->GetMRMLScene()->AddNode(rootModelHierarchyNode);
    rootModelHierarchyNode->Delete();    
  }
  std::string modelHierarchyNodeName = std::string(doseVolumeNode->GetName()) + vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  rootModelHierarchyNode->SetName(modelHierarchyNodeName.c_str());
  doseVolumeNode->SetNodeReferenceID(ISODOSE_ROOT_MODEL_HIERARCHY_REFERENCE_ROLE, rootModelHierarchyNode->GetID());
 
  // Create display node for the model hierarchy node
  vtkMRMLModelDisplayNode* rootModelHierarchyDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast( doseVolumeNode->GetNodeReference(ISODOSE_ROOT_MODEL_HIERARCHY_DISPLAY_REFERENCE_ROLE) );
  if (rootModelHierarchyDisplayNode)
  {
    rootModelHierarchyDisplayNode = vtkMRMLModelDisplayNode::New();
    this->GetMRMLScene()->AddNode(rootModelHierarchyDisplayNode);
    rootModelHierarchyDisplayNode->Delete();
  }
  else
  {
    this->GetMRMLScene()->RemoveNode(rootModelHierarchyDisplayNode);
    rootModelHierarchyDisplayNode = vtkMRMLModelDisplayNode::New();
    this->GetMRMLScene()->AddNode(rootModelHierarchyDisplayNode);
    rootModelHierarchyDisplayNode->Delete();
  }
  rootModelHierarchyDisplayNode->SetName(modelHierarchyNodeName.c_str());
  rootModelHierarchyDisplayNode->SetVisibility(1);
  rootModelHierarchyNode->SetAndObserveDisplayNodeID( rootModelHierarchyDisplayNode->GetID() );
  doseVolumeNode->SetNodeReferenceID(ISODOSE_ROOT_MODEL_HIERARCHY_DISPLAY_REFERENCE_ROLE, rootModelHierarchyDisplayNode->GetID() );

  // Subject hierarchy node for the isodose surfaces
  if (doseVolumeSubjectHierarchyNode->GetNumberOfChildrenNodes() >= 1)
  {
    doseVolumeSubjectHierarchyNode->RemoveAllHierarchyChildrenNodes();
  }
  std::string isodoseShNodeName = std::string(doseVolumeNode->GetName()) + vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_SUBJECT_HIERARCHY_NODE_NAME_POSTFIX;
  vtkMRMLSubjectHierarchyNode* subjectHierarchyRootNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    this->GetMRMLScene(), doseVolumeSubjectHierarchyNode, vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelFolder(),
    isodoseShNodeName.c_str());

  // Get color table
  vtkMRMLColorTableNode* colorTableNode = parameterNode->GetColorTableNode();

  // Progress
  int stepCount = 1 /* reslice step */ + colorTableNode->GetNumberOfColors();
  int currentStep = 0;

  // Reslice dose volume
  vtkSmartPointer<vtkMatrix4x4> inputIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetIJKToRASMatrix(inputIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> inputRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetRASToIJKMatrix(inputRAS2IJKMatrix); 

  vtkSmartPointer<vtkTransform> outputIJK2IJKResliceTransform = vtkSmartPointer<vtkTransform>::New(); 
  outputIJK2IJKResliceTransform->Identity();
  outputIJK2IJKResliceTransform->PostMultiply();
  outputIJK2IJKResliceTransform->SetMatrix(inputIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = doseVolumeNode->GetParentTransformNode();
  vtkSmartPointer<vtkMatrix4x4> inputRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(inputRAS2RASMatrix);  
    outputIJK2IJKResliceTransform->Concatenate(inputRAS2RASMatrix);
  }
  
  outputIJK2IJKResliceTransform->Concatenate(inputRAS2IJKMatrix);
  outputIJK2IJKResliceTransform->Inverse();

  int dimensions[3] = {0, 0, 0};
  doseVolumeNode->GetImageData()->GetDimensions(dimensions);
  vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInputData(doseVolumeNode->GetImageData());
  reslice->SetOutputOrigin(0, 0, 0);
  reslice->SetOutputSpacing(1, 1, 1);
  reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  reslice->SetResliceTransform(outputIJK2IJKResliceTransform);
  reslice->Update();
  vtkSmartPointer<vtkImageData> reslicedDoseVolumeImage = reslice->GetOutput(); 

  // Report progress
  ++currentStep;
  double progress = (double)(currentStep) / (double)stepCount;
  this->InvokeEvent(SlicerRtCommon::ProgressUpdated, (void*)&progress);

  // Create isodose surfaces
  for (int i = 0; i < colorTableNode->GetNumberOfColors(); i++)
  {
    double val[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    const char* strIsoLevel = colorTableNode->GetColorName(i);
    double isoLevel = vtkVariant(strIsoLevel).ToDouble();
    colorTableNode->GetColor(i, val);

    vtkSmartPointer<vtkImageMarchingCubes> marchingCubes = vtkSmartPointer<vtkImageMarchingCubes>::New();
    marchingCubes->SetInputData(reslicedDoseVolumeImage);
    marchingCubes->SetNumberOfContours(1); 
    marchingCubes->SetValue(0, isoLevel);
    marchingCubes->ComputeScalarsOff();
    marchingCubes->ComputeGradientsOff();
    marchingCubes->ComputeNormalsOff();
    marchingCubes->Update();

    vtkSmartPointer<vtkPolyData> isoPolyData= marchingCubes->GetOutput();
    if (isoPolyData->GetNumberOfPoints() >= 1)
    {
      vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
      triangleFilter->SetInputData(marchingCubes->GetOutput());
      triangleFilter->Update();

      vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
      decimate->SetInputData(triangleFilter->GetOutput());
      decimate->SetTargetReduction(0.6);
      decimate->SetFeatureAngle(60);
      decimate->SplittingOff();
      decimate->PreserveTopologyOn();
      decimate->SetMaximumError(1);
      decimate->Update();

      vtkSmartPointer<vtkWindowedSincPolyDataFilter> smootherSinc = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
      smootherSinc->SetPassBand(0.1);
      smootherSinc->SetInputData(decimate->GetOutput() );
      smootherSinc->SetNumberOfIterations(2);
      smootherSinc->FeatureEdgeSmoothingOff();
      smootherSinc->BoundarySmoothingOff();
      smootherSinc->Update();

      vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
      normals->SetInputData(smootherSinc->GetOutput());
      normals->ComputePointNormalsOn();
      normals->SetFeatureAngle(60);
      normals->Update();

      vtkSmartPointer<vtkTransform> inputIJKToRASTransform = vtkSmartPointer<vtkTransform>::New();
      inputIJKToRASTransform->Identity();
      inputIJKToRASTransform->SetMatrix(inputIJK2RASMatrix);

      vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
      transformPolyData->SetInputData(normals->GetOutput());
      transformPolyData->SetTransform(inputIJKToRASTransform);
      transformPolyData->Update();
  
      vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
      displayNode->SliceIntersectionVisibilityOn();  
      displayNode->VisibilityOn(); 
      displayNode->SetColor(val[0], val[1], val[2]);
      displayNode->SetOpacity(val[3]);
    
      // Disable backface culling to make the back side of the model visible as well
      displayNode->SetBackfaceCulling(0);

      std::string doseUnitName("");
      vtkMRMLSubjectHierarchyNode* doseSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(doseVolumeNode);
      if (doseSubjectHierarchyNode)
      {
        const char* doseUnitattributeValue = doseSubjectHierarchyNode->GetAttributeFromAncestor(
          SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
        doseUnitName = std::string(doseUnitattributeValue ? doseUnitattributeValue : "");
      }

      vtkSmartPointer<vtkMRMLModelNode> isodoseModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      isodoseModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(isodoseModelNode));
      std::string isodoseModelNodeName = vtkSlicerIsodoseModuleLogic::ISODOSE_MODEL_NODE_NAME_PREFIX + strIsoLevel + doseUnitName;
      isodoseModelNode->SetName(isodoseModelNodeName.c_str());
      isodoseModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      isodoseModelNode->SetAndObservePolyData(transformPolyData->GetOutput());
      isodoseModelNode->SetSelectable(1);
      isodoseModelNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ISODOSE_MODEL_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

      // Put the new node in the model hierarchy
      vtkSmartPointer<vtkMRMLModelHierarchyNode> isodoseModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      this->GetMRMLScene()->AddNode(isodoseModelHierarchyNode);
      std::string modelHierarchyNodeName = std::string(isodoseModelNodeName) + SlicerRtCommon::DICOMRTIMPORT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
      isodoseModelHierarchyNode->SetName(modelHierarchyNodeName.c_str());
      isodoseModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      isodoseModelHierarchyNode->SetModelNodeID(isodoseModelNode->GetID());
      isodoseModelHierarchyNode->HideFromEditorsOn();

      // Put the new node in the subject hierarchy
      std::string isodoseSHNodeName = strIsoLevel + doseUnitName;
      vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
        this->GetMRMLScene(), subjectHierarchyRootNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
        isodoseSHNodeName.c_str(), isodoseModelNode);
    }

    // Report progress
    ++currentStep;
    progress = (double)(currentStep) / (double)stepCount;
    this->InvokeEvent(SlicerRtCommon::ProgressUpdated, (void*)&progress);
  }

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
}
