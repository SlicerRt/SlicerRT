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
const char* vtkSlicerIsodoseModuleLogic::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME = "Isodose_ColorTable.ctbl";
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
  this->IsodoseNode = NULL;
  this->DefaultIsodoseColorTableNodeId = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::~vtkSlicerIsodoseModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->IsodoseNode, NULL);
  this->SetDefaultIsodoseColorTableNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetAndObserveIsodoseNode(vtkMRMLIsodoseNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->IsodoseNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());

  // Load default isodose color table
  this->LoadDefaultIsodoseColorTable();
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

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLIsodoseNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
  if (node)
  {
    paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->IsodoseNode, paramNode);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerIsodoseModuleLogic::DoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->IsodoseNode)
  {
    vtkErrorMacro("DoseVolumeContainsDose: Invalid MRML scene or parameter set node!");
    return false;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = this->IsodoseNode->GetDoseVolumeNode();
  return SlicerRtCommon::IsDoseVolumeNode(doseVolumeNode);
}

//------------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable()
{
  vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string nodeName = vtksys::SystemTools::GetFilenameWithoutExtension(vtkSlicerIsodoseModuleLogic::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME);
  nodeName = this->GetMRMLScene()->GenerateUniqueName(nodeName);
  colorTableNode->SetName(nodeName.c_str());
  colorTableNode->SetTypeToUser();
  colorTableNode->SetSingletonTag(nodeName.c_str());
  colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  colorTableNode->HideFromEditorsOn();
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
  
  this->GetMRMLScene()->AddNode(colorTableNode);
  this->SetDefaultIsodoseColorTableNodeId(colorTableNode->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::LoadDefaultIsodoseColorTable()
{
  // Load default color table file
  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string colorTableFilePath = moduleShareDirectory + "/" + vtkSlicerIsodoseModuleLogic::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME;
  vtkMRMLColorTableNode* colorTableNode = NULL;
  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile( colorTableFilePath.c_str(),
      vtksys::SystemTools::GetFilenameWithoutExtension(vtkSlicerIsodoseModuleLogic::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME).c_str() );

    // Create temporary lookup table storing the color data while the type of the loaded color table is set to user
    // (workaround for bug #409)
    vtkSmartPointer<vtkLookupTable> tempLookupTable = vtkSmartPointer<vtkLookupTable>::New();
    tempLookupTable->DeepCopy(loadedColorNode->GetLookupTable());

    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
    colorTableNode->SetTypeToUser();
    colorTableNode->SetSingletonTag(colorTableNode->GetName());
    colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
    colorTableNode->HideFromEditorsOn();
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
      vtkWarningMacro("LoadDefaultIsodoseColorTable: Default isodose color table file '" << colorTableFilePath << "' cannot be found!");
    }
    // If file is not found, then create it programatically
    this->CreateDefaultIsodoseColorTable();
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DefaultIsodoseColorTableNodeId));
  }

  if (colorTableNode)
  {
    colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
    this->SetDefaultIsodoseColorTableNodeId(colorTableNode->GetID());
  }
  else
  {
    vtkErrorMacro("LoadDefaultIsodoseColorTable: Failed to load or create default isodose color table!");
  }
}

//------------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetNumberOfIsodoseLevels(int newNumberOfColors)
{
  vtkMRMLColorTableNode* colorTableNode = this->IsodoseNode->GetColorTableNode();  
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
void vtkSlicerIsodoseModuleLogic::CreateIsodoseSurfaces()
{
  if (!this->GetMRMLScene() || !this->IsodoseNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid scene or parameter set node!");
    return;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = this->IsodoseNode->GetDoseVolumeNode();
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
  vtkMRMLModelHierarchyNode* RootModelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast( doseVolumeNode->GetNodeReference(ISODOSE_ROOT_MODEL_HIERARCHY_REFERENCE_ROLE) );
  if (!RootModelHierarchyNode)
  {
    RootModelHierarchyNode = vtkMRMLModelHierarchyNode::New();
    this->GetMRMLScene()->AddNode(RootModelHierarchyNode);
    RootModelHierarchyNode->Delete();
  }
  else 
  {
    std::vector< vtkMRMLHierarchyNode *> children = RootModelHierarchyNode->GetChildrenNodes(); 
    for (unsigned int i=0; i<children.size(); i++)
    {
      vtkMRMLHierarchyNode *child = children[i];
      vtkMRMLModelNode* mnode = vtkMRMLModelNode::SafeDownCast(child->GetAssociatedNode());
      this->GetMRMLScene()->RemoveNode(mnode);
      this->GetMRMLScene()->RemoveNode(child);
    }
    this->GetMRMLScene()->RemoveNode(RootModelHierarchyNode);
    RootModelHierarchyNode = vtkMRMLModelHierarchyNode::New();
    this->GetMRMLScene()->AddNode(RootModelHierarchyNode);
    RootModelHierarchyNode->Delete();    
  }
  RootModelHierarchyNode->AllowMultipleChildrenOn();
  RootModelHierarchyNode->HideFromEditorsOff();
  std::string modelHierarchyNodeName = std::string(doseVolumeNode->GetName()) + vtkSlicerIsodoseModuleLogic::ISODOSE_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  modelHierarchyNodeName = this->GetMRMLScene()->GenerateUniqueName(modelHierarchyNodeName);
  RootModelHierarchyNode->SetName(modelHierarchyNodeName.c_str());
  doseVolumeNode->SetNodeReferenceID(ISODOSE_ROOT_MODEL_HIERARCHY_REFERENCE_ROLE, RootModelHierarchyNode->GetID());
 
  // Create display node for the model hierarchy node
  vtkMRMLModelDisplayNode* RootModelHierarchyDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast( doseVolumeNode->GetNodeReference(ISODOSE_ROOT_MODEL_HIERARCHY_DISPLAY_REFERENCE_ROLE) );
  if (RootModelHierarchyDisplayNode)
  {
    RootModelHierarchyDisplayNode = vtkMRMLModelDisplayNode::New();
    this->GetMRMLScene()->AddNode(RootModelHierarchyDisplayNode);
    RootModelHierarchyDisplayNode->Delete();
  }
  else
  {
    this->GetMRMLScene()->RemoveNode(RootModelHierarchyDisplayNode);
    RootModelHierarchyDisplayNode = vtkMRMLModelDisplayNode::New();
    this->GetMRMLScene()->AddNode(RootModelHierarchyDisplayNode);
    RootModelHierarchyDisplayNode->Delete();
  }
  RootModelHierarchyDisplayNode->SetName(modelHierarchyNodeName.c_str());
  RootModelHierarchyDisplayNode->SetVisibility(1);
  RootModelHierarchyNode->SetAndObserveDisplayNodeID( RootModelHierarchyDisplayNode->GetID() );
  doseVolumeNode->SetNodeReferenceID(ISODOSE_ROOT_MODEL_HIERARCHY_DISPLAY_REFERENCE_ROLE, RootModelHierarchyDisplayNode->GetID() );

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
  vtkMRMLColorTableNode* colorTableNode = this->IsodoseNode->GetColorTableNode();

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
#if (VTK_MAJOR_VERSION <= 5)
  reslice->SetInput(doseVolumeNode->GetImageData());
#else
  reslice->SetInputData(doseVolumeNode->GetImageData());
#endif
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

    std::stringstream ss;
    ss << strIsoLevel;
    double doubleValue;
    ss >> doubleValue;
    double isoLevel = doubleValue;
    colorTableNode->GetColor(i, val);

    vtkSmartPointer<vtkImageMarchingCubes> marchingCubes = vtkSmartPointer<vtkImageMarchingCubes>::New();
#if (VTK_MAJOR_VERSION <= 5)
    marchingCubes->SetInput(reslicedDoseVolumeImage);
#else
    marchingCubes->SetInputData(reslicedDoseVolumeImage);
#endif
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
#if (VTK_MAJOR_VERSION <= 5)
      triangleFilter->SetInput(marchingCubes->GetOutput());
#else
      triangleFilter->SetInputData(marchingCubes->GetOutput());
#endif
      triangleFilter->Update();

      vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
#if (VTK_MAJOR_VERSION <= 5)
      decimate->SetInput(triangleFilter->GetOutput());
#else
      decimate->SetInputData(triangleFilter->GetOutput());
#endif
      decimate->SetTargetReduction(0.6);
      decimate->SetFeatureAngle(60);
      decimate->SplittingOff();
      decimate->PreserveTopologyOn();
      decimate->SetMaximumError(1);
      decimate->Update();

      vtkSmartPointer<vtkWindowedSincPolyDataFilter> smootherSinc = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
      smootherSinc->SetPassBand(0.1);
#if (VTK_MAJOR_VERSION <= 5)
      smootherSinc->SetInput(decimate->GetOutput() );
#else
      smootherSinc->SetInputData(decimate->GetOutput() );
#endif
      smootherSinc->SetNumberOfIterations(2);
      smootherSinc->FeatureEdgeSmoothingOff();
      smootherSinc->BoundarySmoothingOff();
      smootherSinc->Update();

      vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
#if (VTK_MAJOR_VERSION <= 5)
      normals->SetInput(smootherSinc->GetOutput());
#else
      normals->SetInputData(smootherSinc->GetOutput());
#endif
      normals->ComputePointNormalsOn();
      normals->SetFeatureAngle(60);
      normals->Update();

      vtkSmartPointer<vtkTransform> inputIJKToRASTransform = vtkSmartPointer<vtkTransform>::New();
      inputIJKToRASTransform->Identity();
      inputIJKToRASTransform->SetMatrix(inputIJK2RASMatrix);

      vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
#if (VTK_MAJOR_VERSION <= 5)
      transformPolyData->SetInput(normals->GetOutput());
#else
      transformPolyData->SetInputData(normals->GetOutput());
#endif

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
      isodoseModelNodeName = this->GetMRMLScene()->GenerateUniqueName(isodoseModelNodeName);
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
      isodoseModelHierarchyNode->SetParentNodeID( RootModelHierarchyNode->GetID() );
      isodoseModelHierarchyNode->SetModelNodeID( isodoseModelNode->GetID() );
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

  //this->IsodoseNode->SetAndObserveIsodoseSurfaceModelsParentHierarchyNode(ModelHierarchyRootNode);
    
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
}
