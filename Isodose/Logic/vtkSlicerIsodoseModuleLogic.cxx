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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>

// MRMLLogic includes
#include "vtkMRMLColorLogic.h"
#include "vtkMRMLApplicationLogic.h"

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
#include "vtksys/SystemTools.hxx"

// STD includes
#include <cassert>

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
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLIsodoseNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneEndClose()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->IsodoseNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLIsodoseNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->IsodoseNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLIsodoseNode"))
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
    return false;
  }

  vtkMRMLNode* doseVolumeNode = this->GetMRMLScene()->GetNodeByID(this->IsodoseNode->GetDoseVolumeNodeId());
  return SlicerRtCommon::IsDoseVolumeNode(doseVolumeNode);
}

//------------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable()
{
  vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string nodeName = vtksys::SystemTools::GetFilenameWithoutExtension(SlicerRtCommon::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME);
  nodeName = this->GetMRMLScene()->GenerateUniqueName(nodeName);
  colorTableNode->SetName(nodeName.c_str());
  colorTableNode->SetTypeToUser();
  colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  colorTableNode->HideFromEditorsOff();
  colorTableNode->SaveWithSceneOff();
  colorTableNode->SetNumberOfColors(6);
  colorTableNode->GetLookupTable()->SetTableRange(0,5);
  colorTableNode->AddColor("5", 0, 1, 0, 0.2);
  colorTableNode->AddColor("10", 0.5, 1, 0, 0.2);
  colorTableNode->AddColor("15", 1, 1, 0, 0.2);
  colorTableNode->AddColor("20", 1, 0.66, 0, 0.2);
  colorTableNode->AddColor("25", 1, 0.33, 0, 0.2);
  colorTableNode->AddColor("30", 1, 0, 0, 0.2);
  
  this->GetMRMLScene()->AddNode(colorTableNode);
  this->SetDefaultIsodoseColorTableNodeId(colorTableNode->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::LoadDefaultIsodoseColorTable()
{
  // Load default color table file
  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string colorTableFilePath = moduleShareDirectory + "/" + SlicerRtCommon::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME;
  vtkMRMLColorTableNode* colorTableNode = NULL;
  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile( colorTableFilePath.c_str(),
      vtksys::SystemTools::GetFilenameWithoutExtension(SlicerRtCommon::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME).c_str() );

    // Create temporary lookup table storing the color data while the type of the loaded color table is set to user
    // (workaround for bug #409)
    vtkSmartPointer<vtkLookupTable> tempLookupTable = vtkSmartPointer<vtkLookupTable>::New();
    tempLookupTable->DeepCopy(loadedColorNode->GetLookupTable());

    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
    colorTableNode->SetTypeToUser();
    colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
    colorTableNode->HideFromEditorsOff();
    colorTableNode->SaveWithSceneOff();
    colorTableNode->SetNumberOfColors(tempLookupTable->GetNumberOfColors());
    colorTableNode->SetLookupTable(tempLookupTable);
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
  if (!this->IsodoseNode->GetColorTableNodeId() || newNumberOfColors < 1)
  {
    return;
  }
  vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseNode->GetColorTableNodeId()));  

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

  double origin[3] = {0, 0, 0};
  double spacing[3] = {1, 1, 1};
  int dimensions[3] = {0, 0, 0};

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseNode->GetDoseVolumeNodeId()));
  if (!doseVolumeNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Invalid dose volume!");
    return;
  }

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Get patient hierarchy node for the dose volume
  vtkMRMLHierarchyNode* doseVolumePatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(doseVolumeNode);
  if (!doseVolumePatientHierarchyNode)
  {
    vtkErrorMacro("CreateIsodoseSurfaces: Failed to get patient hierarchy node for dose volume '" << doseVolumeNode->GetName() << "'");
  }

  // Hierarchy node for the loaded structure sets
  vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode = vtkMRMLModelHierarchyNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetIsodoseNode()->GetIsodoseSurfaceModelsParentHierarchyNodeId()));

  modelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  modelHierarchyRootNode->AllowMultipleChildrenOn();
  modelHierarchyRootNode->HideFromEditorsOff();
  std::string hierarchyNodeName = std::string(doseVolumeNode->GetName()) + SlicerRtCommon::ISODOSE_ISODOSE_SURFACES_HIERARCHY_NODE_NAME_POSTFIX
    + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  hierarchyNodeName = this->GetMRMLScene()->GenerateUniqueName(hierarchyNodeName);
  modelHierarchyRootNode->SetName(hierarchyNodeName.c_str());
  modelHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  modelHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  if (doseVolumePatientHierarchyNode)
  {
    modelHierarchyRootNode->SetParentNodeID(doseVolumePatientHierarchyNode->GetID());
  }
  this->GetMRMLScene()->AddNode(modelHierarchyRootNode);

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  hierarchyNodeName.append("Display");
  modelDisplayNode->SetName(hierarchyNodeName.c_str());
  modelDisplayNode->SetVisibility(1);
  this->GetMRMLScene()->AddNode(modelDisplayNode);
  modelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );

  // Get color table
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseNode->GetColorTableNodeId()));  
  vtkSmartPointer<vtkLookupTable> lookupTable = colorTableNode->GetLookupTable();

  // Reslice dose volume
  vtkSmartPointer<vtkMatrix4x4> inputIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetIJKToRASMatrix(inputIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> referenceRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetRASToIJKMatrix(referenceRAS2IJKMatrix); //TODO: confusing matrix names: input <-> reference ?

  vtkSmartPointer<vtkTransform> outputResliceTransform = vtkSmartPointer<vtkTransform>::New(); //TODO: bad transform name. From which coordinate system to which one?
  outputResliceTransform->Identity();
  outputResliceTransform->PostMultiply();
  outputResliceTransform->SetMatrix(inputIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = doseVolumeNode->GetParentTransformNode();
  vtkSmartPointer<vtkMatrix4x4> inputRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(inputRAS2RASMatrix);  
    outputResliceTransform->Concatenate(inputRAS2RASMatrix);
  }
  
  outputResliceTransform->Concatenate(referenceRAS2IJKMatrix);
  outputResliceTransform->Inverse();

  doseVolumeNode->GetOrigin(origin); 
  doseVolumeNode->GetSpacing(spacing);
  doseVolumeNode->GetImageData()->GetDimensions(dimensions);
  vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInput(doseVolumeNode->GetImageData());
  reslice->SetOutputOrigin(0, 0, 0);
  reslice->SetOutputSpacing(1, 1, 1);
  reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  reslice->SetResliceTransform(outputResliceTransform);
  reslice->Update();
  vtkSmartPointer<vtkImageData> tempImage = reslice->GetOutput(); //TODO: bad variable name

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
    marchingCubes->SetInput(tempImage);
    marchingCubes->SetNumberOfContours(1); 
    marchingCubes->SetValue(0, isoLevel);
    marchingCubes->ComputeScalarsOff();
    marchingCubes->ComputeGradientsOff();
    marchingCubes->ComputeNormalsOff();
    marchingCubes->Update();

    vtkSmartPointer<vtkPolyData> isoPolyData= marchingCubes->GetOutput();
    if(isoPolyData->GetNumberOfPoints() >= 1)
    {
      vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
      triangleFilter->SetInput(marchingCubes->GetOutput());
      triangleFilter->Update();

      vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
      decimate->SetInput(triangleFilter->GetOutput());
      decimate->SetTargetReduction(0.6);
      decimate->SetFeatureAngle(60);
      decimate->SplittingOff();
      decimate->PreserveTopologyOn();
      decimate->SetMaximumError(1);
      decimate->Update();

      vtkSmartPointer<vtkWindowedSincPolyDataFilter> smootherSinc = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
      smootherSinc->SetPassBand(0.1);
      smootherSinc->SetInput(decimate->GetOutput() );
      smootherSinc->SetNumberOfIterations(2);
      smootherSinc->FeatureEdgeSmoothingOff();
      smootherSinc->BoundarySmoothingOff();
      smootherSinc->Update();

      vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
      normals->SetInput(smootherSinc->GetOutput());
      normals->ComputePointNormalsOn();
      normals->SetFeatureAngle(60);
      normals->Update();

      vtkSmartPointer<vtkTransform> inputIJKToRASTransform = vtkSmartPointer<vtkTransform>::New();
      inputIJKToRASTransform->Identity();
      inputIJKToRASTransform->SetMatrix(inputIJK2RASMatrix);

      vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
      transformPolyData->SetInput(normals->GetOutput());
      transformPolyData->SetTransform(inputIJKToRASTransform);
      transformPolyData->Update();
  
      vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
      displayNode->SliceIntersectionVisibilityOn();  
      displayNode->VisibilityOn(); 
      displayNode->SetColor(val[0], val[1], val[2]);
      displayNode->SetOpacity(val[3]);
    
      // Disable backface culling to make the back side of the contour visible as well
      displayNode->SetBackfaceCulling(0);

      const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());
      if (!doseUnitName)
      {
        doseUnitName = "";
      }

      vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(modelNode));
      std::string isodoseModelNodeName = SlicerRtCommon::ISODOSE_MODEL_NODE_NAME_PREFIX + strIsoLevel + doseUnitName;
      isodoseModelNodeName = this->GetMRMLScene()->GenerateUniqueName(isodoseModelNodeName);
      modelNode->SetName(isodoseModelNodeName.c_str());
      modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      modelNode->SetAndObservePolyData(transformPolyData->GetOutput());
      modelNode->SetHideFromEditors(0);
      modelNode->SetSelectable(1);

      // Put the new node in the hierarchy
      vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      this->GetMRMLScene()->AddNode(modelHierarchyNode);
      std::string modelHierarchyNodeName = std::string(isodoseModelNodeName) + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
      modelHierarchyNode->SetName(modelHierarchyNodeName.c_str());
      modelHierarchyNode->SetParentNodeID( modelHierarchyRootNode->GetID() );
      modelHierarchyNode->SetModelNodeID( modelNode->GetID() );
      modelHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
        SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
      modelHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
    }
  }

  this->IsodoseNode->SetAndObserveIsodoseSurfaceModelsParentHierarchyNodeId(modelHierarchyRootNode->GetID());
    
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
}
