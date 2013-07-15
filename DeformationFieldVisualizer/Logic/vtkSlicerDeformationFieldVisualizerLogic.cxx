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
  
  This file was originally developed by Franklin King, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// DeformationFieldVisualizer includes
#include "vtkMRMLDeformationFieldVisualizerNode.h"
#include "vtkSlicerDeformationFieldVisualizerLogic.h"
#include "vtkDeformationFieldVisualizerGlyph3D.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkVectorNorm.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkGeneralTransform.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>

// Glyph VTK includes
#include <vtkArrowSource.h>
#include <vtkConeSource.h>
#include <vtkSphereSource.h>

// Grid VTK includes
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkLine.h>
#include <vtkWarpVector.h>
#include <vtkExtractVOI.h>
#include <vtkImageResample.h>

// Block VTK includes
#include <vtkGeometryFilter.h>
#include <vtkVectorDot.h>
#include <vtkPolyDataNormals.h>

// Contour VTK includes
#include <vtkMarchingCubes.h>
#include <vtkDecimatePro.h>

// Glyph Slice VTK includes
#include <vtkGlyphSource2D.h>
#include <vtkRibbonFilter.h>
#include <vtkPlane.h>

// Grid Slice VTK includes
#include <vtkAppendPolyData.h>

// STD includes
#include <cassert>
#include <math.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDeformationFieldVisualizerLogic);

//----------------------------------------------------------------------------
vtkSlicerDeformationFieldVisualizerLogic::vtkSlicerDeformationFieldVisualizerLogic()
{
  this->DeformationFieldVisualizerNode = NULL;
  this->deformationField = vtkImageData::New();
}

//----------------------------------------------------------------------------
vtkSlicerDeformationFieldVisualizerLogic::~vtkSlicerDeformationFieldVisualizerLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->DeformationFieldVisualizerNode, NULL);
  this->deformationField->Delete();
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::SetAndObserveDeformationFieldVisualizerNode(vtkMRMLDeformationFieldVisualizerNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DeformationFieldVisualizerNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  assert(scene != 0);
  
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVectorVolumeNode") ||
    node->IsA("vtkMRMLLinearTransformNode") || 
    node->IsA("vtkMRMLGridTransformNode") || 
    node->IsA("vtkMRMLBSplineTransformNode") || 
    node->IsA("vtkMRMLDeformationFieldVisualizerNode"))
{
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVectorVolumeNode") ||
    node->IsA("vtkMRMLLinearTransformNode") || 
    node->IsA("vtkMRMLGridTransformNode") || 
    node->IsA("vtkMRMLBSplineTransformNode") || 
    node->IsA("vtkMRMLDeformationFieldVisualizerNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneEndImport()
{
  //Select parameter node if it exists
  vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode> paramNode = NULL;
  vtkSmartPointer<vtkMRMLNode> node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDeformationFieldVisualizerNode");
  if (node)
  {
    paramNode = vtkMRMLDeformationFieldVisualizerNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DeformationFieldVisualizerNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::GenerateDeformationField()
{
  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNode = vtkMRMLTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()));
  if (inputVolumeNode == NULL)
  {
    vtkErrorMacro("Failed to convert input: input node is invalid");
    return;
  }
  
  vtkSmartPointer<vtkMRMLVolumeNode> referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetReferenceVolumeNodeID()));
  if (referenceVolumeNode == NULL)
  {
    vtkErrorMacro("Failed to convert input: reference volume node is invalid");
    return;
  }
  
  const double *spacing = referenceVolumeNode->GetSpacing();
  const int *extent = referenceVolumeNode->GetImageData()->GetExtent();
  vtkSmartPointer<vtkMatrix4x4> IJKToRAS = vtkSmartPointer<vtkMatrix4x4>::New();  
  referenceVolumeNode->GetIJKToRASMatrix(IJKToRAS);
  
  this->deformationField->DeepCopy(referenceVolumeNode->GetImageData());
  this->deformationField->SetScalarTypeToFloat();
  this->deformationField->SetNumberOfScalarComponents(3);
  this->deformationField->AllocateScalars();
  this->deformationField->SetSpacing(spacing[0], spacing[1], spacing[2]);
  this->deformationField->GetPointData()->SetActiveVectors("ImageScalars");
  this->deformationField->Update();
  
  //*
  float fixedPoint[3];
  float movingPoint[3];
  
  vtkSmartPointer<vtkGeneralTransform> inputTransform = inputVolumeNode->GetTransformToParent();
  float *ptr = (float *)this->deformationField->GetScalarPointer();
  for(int k = 0; k < extent[5]+1; k++)
  {
    for(int j = 0; j < extent[3]+1; j++)
    {
      for(int i = 0; i < extent[1]+1; i++)
      {
        fixedPoint[0] = i*IJKToRAS->GetElement(0,0) + j*IJKToRAS->GetElement(0,1) + k*IJKToRAS->GetElement(0,2) + IJKToRAS->GetElement(0,3);
        fixedPoint[1] = i*IJKToRAS->GetElement(1,0) + j*IJKToRAS->GetElement(1,1) + k*IJKToRAS->GetElement(1,2) + IJKToRAS->GetElement(1,3);
        fixedPoint[2] = i*IJKToRAS->GetElement(2,0) + j*IJKToRAS->GetElement(2,1) + k*IJKToRAS->GetElement(2,2) + IJKToRAS->GetElement(2,3);
        
        inputTransform->TransformPoint(fixedPoint, movingPoint);
        
        ptr[(i + j*(extent[1]+1) + k*(extent[1]+1)*(extent[3]+1))*3] = movingPoint[0] - fixedPoint[0];
        ptr[(i + j*(extent[1]+1) + k*(extent[1]+1)*(extent[3]+1))*3 + 1] = movingPoint[1] - fixedPoint[1];
        ptr[(i + j*(extent[1]+1) + k*(extent[1]+1)*(extent[3]+1))*3 + 2] = movingPoint[2] - fixedPoint[2];
      }
    }
  }
}

//----------------------------------------------------------------------------
double* vtkSlicerDeformationFieldVisualizerLogic::GetFieldRange()
{
  double* range = this->deformationField->GetPointData()->GetScalars()->GetRange(-1);
  range[0] = floor(range[0]*10000)/10000;
  range[1] = ceil(range[1]*10000)/10000;

  return range;
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::CreateVisualization(int visualizationMode)
{
  if (!this->DeformationFieldVisualizerNode || !this->GetMRMLScene())
  {
    vtkErrorMacro("CreateVisualization failed: Deformation Field Visualizer Node or scene is invalid");
    return;
  }
  
  vtkSmartPointer<vtkMRMLModelNode> outputModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetOutputModelNodeID()));
  outputModelNode->SetScene(this->GetMRMLScene());

  if (outputModelNode == NULL)
  {
    vtkErrorMacro("CreateVisualization failed: Model node is invalid");
    return;
  }
  
  const double *origin;
  double spacing[3] = {0,0,0};
  vtkSmartPointer<vtkMatrix4x4> ijkToRasDirections = vtkSmartPointer<vtkMatrix4x4>::New();

  //Initialize input
  if (strcmp((this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLVectorVolumeNode") == 0)
  {
  
    vtkSmartPointer<vtkMRMLVectorVolumeNode> inputVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()));
    if (inputVolumeNode == NULL)
    {
      vtkErrorMacro("CreateVisualization failed: Input node is invalid");
      return;
    }
    
    this->deformationField->DeepCopy(inputVolumeNode->GetImageData());
    this->deformationField->GetPointData()->SetActiveVectors("ImageScalars");
    
    origin = inputVolumeNode->GetOrigin();
    inputVolumeNode->GetIJKToRASDirectionMatrix(ijkToRasDirections);
    
    inputVolumeNode->GetSpacing(spacing);
    this->deformationField->SetSpacing(spacing);
  }
  else if (strcmp((this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLLinearTransformNode") == 0 || 
  strcmp((this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLBSplineTransformNode") == 0 ||
  strcmp((this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLGridTransformNode") == 0)
  {
    vtkSmartPointer<vtkMRMLVolumeNode> referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetReferenceVolumeNodeID()));
    if (referenceVolumeNode == NULL)
    {
      vtkErrorMacro("CreateVisualization failed: Reference volume node is invalid");
      return;
    }  
    
    origin = referenceVolumeNode->GetOrigin();
    referenceVolumeNode->GetIJKToRASDirectionMatrix(ijkToRasDirections);
  }
  else
  {
    std::stringstream errorMessage;
    errorMessage << "Invalid input node selected. Expected vtkMRMLVectorVolumeNode, vtkMRMLLinearTransformNode, vtkMRMLBSplineTransformNode, or vtkMRMLGridTransformNode, but got";
    errorMessage << (this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetInputVolumeNodeID()))->GetClassName() << "instead" << std::endl;
    return;
  }
  
  // The new field will be modified to the point where it no longer represents the original vectors. this->deformationField remains unmodified
  vtkSmartPointer<vtkImageData> field = vtkSmartPointer<vtkImageData>::New();
  field->DeepCopy(this->deformationField);

  vtkSmartPointer<vtkMatrix4x4> rasToIjkDirections = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToIjkDirections->DeepCopy(ijkToRasDirections);
  rasToIjkDirections->Invert();
  
  double x = 0;
  double y = 0;
  double z = 0;
  float *ptr = (float *)field->GetScalarPointer();
  for(int i = 0; i < field->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3)
  {
    x = ptr[i];
    y = ptr[i+1];
    z = ptr[i+2];
    ptr[i] = x*rasToIjkDirections->GetElement(0,0) + y*rasToIjkDirections->GetElement(0,1) + z*rasToIjkDirections->GetElement(0,2);
    ptr[i+1] = x*rasToIjkDirections->GetElement(1,0) + y*rasToIjkDirections->GetElement(1,1) + z*rasToIjkDirections->GetElement(1,2);
    ptr[i+2] = x*rasToIjkDirections->GetElement(2,0) + y*rasToIjkDirections->GetElement(2,1) + z*rasToIjkDirections->GetElement(2,2);    
  }  
  
  // Create IJKToRAS Matrix without spacing; spacing will be added to imagedata directly to avoid warping geometry
  vtkSmartPointer<vtkMatrix4x4> unspacedIjkToRas = vtkSmartPointer<vtkMatrix4x4>::New();
  unspacedIjkToRas->DeepCopy(ijkToRasDirections);
  unspacedIjkToRas->SetElement(0,3,origin[0]);
  unspacedIjkToRas->SetElement(1,3,origin[1]);
  unspacedIjkToRas->SetElement(2,3,origin[2]);

  vtkSmartPointer<vtkTransform> unspacedTransformToRas = vtkSmartPointer<vtkTransform>::New();
  unspacedTransformToRas->SetMatrix(unspacedIjkToRas);

  vtkSmartPointer<vtkTransformPolyDataFilter> polydataTransform = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polydataTransform->SetTransform(unspacedTransformToRas);
  
  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 
  
  // Output
  if (outputModelNode->GetModelDisplayNode()==NULL)
  {
    vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
    outputModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  }
  
  vtkPolyData* output = vtkPolyData::New();
  switch (visualizationMode){
    case VIS_MODE_GLYPH_3D:
      this->GlyphVisualization(field, output, this->DeformationFieldVisualizerNode->GetGlyphSourceOption());
      outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
      outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
      break;
    case VIS_MODE_GRID_3D:
      this->GridVisualization(field, output);
      outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
      outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
      outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
      break;
    case VIS_MODE_BLOCK_3D:
      this->BlockVisualization(field, output);
      outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
      outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
      outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
      break;      
    case VIS_MODE_CONTOUR_3D:
      this->ContourVisualization(field, output);
      outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
      outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
      outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
      outputModelNode->GetModelDisplayNode()->SetSliceIntersectionVisibility(1);
      break;
    case VIS_MODE_GLYPH_2D:
      if (vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetGlyphSliceNodeID())) == NULL)
      {
        vtkErrorMacro("Failed to create Glyph Slice visualization: Invalid slice node");
        return;
      }
      this->GlyphSliceVisualization(field, output, rasToIjkDirections);
      outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
      outputModelNode->GetModelDisplayNode()->SetActiveScalarName("OriginalVectorMagnitude");
      outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
      outputModelNode->GetModelDisplayNode()->SetSliceIntersectionVisibility(1);
      break;
    case VIS_MODE_GRID_2D:
      if (vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetGridSliceNodeID())) == NULL)
      {
        vtkErrorMacro("Failed to create Grid Slice visualization: Invalid slice node");
        return;
      }
      this->GridSliceVisualization(field, output, rasToIjkDirections);
      outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
      outputModelNode->GetModelDisplayNode()->SetActiveScalarName("OriginalVectorMagnitude");
      outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
      outputModelNode->GetModelDisplayNode()->SetSliceIntersectionVisibility(1);
      break;
  }
  polydataTransform->SetInput(output);
  polydataTransform->Update();
  
  outputModelNode->SetAndObservePolyData(polydataTransform->GetOutput());
  
  outputModelNode->SetHideFromEditors(0);
  outputModelNode->SetSelectable(1);
  outputModelNode->Modified();
  
  output->Delete();
  
  if (outputModelNode->GetModelDisplayNode()->GetColorNode()==NULL)
  {
    vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->AddNode(colorTableNode));
    
    colorTableNode->SetName("Deformation Field Colors");
    colorTableNode->SetAttribute("Category", "User Generated");
    colorTableNode->SetTypeToUser();
    colorTableNode->SetNumberOfColors(4);
    colorTableNode->GetLookupTable();
    colorTableNode->AddColor("negligible", 0.0, 0.0, 0.5, 1.0);
    colorTableNode->AddColor(       "low", 0.0, 1.0, 0.0, 1.0);
    colorTableNode->AddColor(    "medium", 1.0, 1.0, 0.0, 1.0);
    colorTableNode->AddColor(      "high", 1.0, 0.0, 0.0, 1.0);

    outputModelNode->GetModelDisplayNode()->SetAndObserveColorNodeID(colorTableNode->GetID());
  }

  vtkMRMLColorTableNode *colorNode = vtkMRMLColorTableNode::SafeDownCast(outputModelNode->GetModelDisplayNode()->GetColorNode());
  if (colorNode != NULL)
  {
    double* range = this->GetFieldRange();
    colorNode->GetLookupTable()->SetTableRange(range[0], range[1]);
  }
  
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::GlyphVisualization(vtkImageData* field, vtkPolyData* output, int sourceOption)
{
  vtkSmartPointer<vtkDeformationFieldVisualizerGlyph3D> glyphFilter = vtkSmartPointer<vtkDeformationFieldVisualizerGlyph3D>::New();
  glyphFilter->SetPointMax(this->DeformationFieldVisualizerNode->GetGlyphPointMax());
  glyphFilter->SetSeed(this->DeformationFieldVisualizerNode->GetGlyphSeed());
  glyphFilter->SetScaleModeToScaleByVector();
  glyphFilter->SetMagnitudeMax(this->DeformationFieldVisualizerNode->GetGlyphThresholdMax());
  glyphFilter->SetMagnitudeMin(this->DeformationFieldVisualizerNode->GetGlyphThresholdMin());
  glyphFilter->SetScaleFactor(this->DeformationFieldVisualizerNode->GetGlyphScale());
  glyphFilter->SetScaleDirectional(this->DeformationFieldVisualizerNode->GetGlyphScaleDirectional());
  glyphFilter->SetColorModeToColorByVector();

  switch (sourceOption){
    //Arrows
    case ARROW_3D:
    {
      vtkSmartPointer<vtkArrowSource> arrowSource = vtkSmartPointer<vtkArrowSource>::New();
      arrowSource->SetTipLength(this->DeformationFieldVisualizerNode->GetGlyphArrowTipLength());
      arrowSource->SetTipRadius(this->DeformationFieldVisualizerNode->GetGlyphArrowTipRadius());
      arrowSource->SetTipResolution(this->DeformationFieldVisualizerNode->GetGlyphArrowResolution());
      arrowSource->SetShaftRadius(this->DeformationFieldVisualizerNode->GetGlyphArrowShaftRadius());
      arrowSource->SetShaftResolution(this->DeformationFieldVisualizerNode->GetGlyphArrowResolution());
      
      glyphFilter->OrientOn();
      glyphFilter->SetSourceConnection(arrowSource->GetOutputPort());
      break;
    }
    //Cones
    case CONE_3D:
    {
      vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
      coneSource->SetHeight(this->DeformationFieldVisualizerNode->GetGlyphConeHeight());
      coneSource->SetRadius(this->DeformationFieldVisualizerNode->GetGlyphConeRadius());
      coneSource->SetResolution(this->DeformationFieldVisualizerNode->GetGlyphConeResolution());
      
      glyphFilter->OrientOn();
      glyphFilter->SetSourceConnection(coneSource->GetOutputPort());
      break;
    }
    //Spheres
    case SPHERE_3D:
    {
      vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
      sphereSource->SetRadius(1);
      sphereSource->SetThetaResolution(this->DeformationFieldVisualizerNode->GetGlyphSphereResolution());
      sphereSource->SetPhiResolution(this->DeformationFieldVisualizerNode->GetGlyphSphereResolution());
      
      glyphFilter->OrientOn();
      glyphFilter->SetSourceConnection(sphereSource->GetOutputPort());
      break;
    }
  }
  glyphFilter->SetInputConnection(field->GetProducerPort());
  glyphFilter->Update();
  
  output->ShallowCopy(glyphFilter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::GridVisualization(vtkImageData* field, vtkPolyData* output)
{
  const int subdivision = 1;
  int lineSpacing = this->DeformationFieldVisualizerNode->GetGridSpacingMM();
  
  vtkSmartPointer<vtkImageResample> resampled = vtkSmartPointer<vtkImageResample>::New();
  resampled->SetInput(field);
  resampled->SetAxisOutputSpacing(0, lineSpacing / subdivision);
  resampled->SetAxisOutputSpacing(1, lineSpacing / subdivision);
  resampled->SetAxisOutputSpacing(2, lineSpacing / subdivision);
  resampled->Update();

  vtkSmartPointer<vtkImageData> resampledField = vtkSmartPointer<vtkImageData>::New();
  resampledField = resampled->GetOutput();
  
  const double* origin = resampledField->GetOrigin();
  const double* spacing = resampledField->GetSpacing();
  const int* dimensions = resampledField->GetDimensions();  
  
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(dimensions[0]*dimensions[1]*dimensions[2]);
  vtkSmartPointer<vtkCellArray> grid = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
  
  int i,j,k;
  for (k = 0; k < dimensions[2]; k++)
  {
    for (j = 0; j < dimensions[1]; j++)
    {
      for (i = 0; i < dimensions[0]; i++)
      {
        points->SetPoint(i + j*dimensions[0] + k*dimensions[0]*dimensions[1],
          origin[0] + i + ((spacing[0]-1)*i),origin[1] + j + ((spacing[1]-1)*j),origin[2] + k + ((spacing[2]-1)*k));
      }
    }
  }
  
  for (k = 0; k < dimensions[2]; k+=subdivision)
  {
    for (j = 0; j < dimensions[1]; j+=subdivision)
    {
      for (i = 0; i < ((dimensions[0]-1)-((dimensions[0]-1)%subdivision)); i++)
      {
        line->GetPointIds()->SetId(0, (i) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
        line->GetPointIds()->SetId(1, (i+1) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
        grid->InsertNextCell(line);
      }
    }
  }

  for (k = 0; k < dimensions[2]; k+=subdivision)
  {
    for (j = 0; j < ((dimensions[1]-1)-((dimensions[1]-1)%subdivision)); j++)
    {
      for (i = 0; i < dimensions[0]; i+=subdivision)
      {
        line->GetPointIds()->SetId(0, (i) + ((j)*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
        line->GetPointIds()->SetId(1, (i) + ((j+1)*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
        grid->InsertNextCell(line);
      }
    }
  }
  
  for (k = 0; k < ((dimensions[2]-1)-((dimensions[2]-1)%subdivision)); k++)
  {
    for (j = 0; j < dimensions[1]; j+=subdivision)
    {
      for (i = 0; i < dimensions[0]; i+=subdivision)
      {
        line->GetPointIds()->SetId(0, (i) + ((j)*dimensions[0]) + ((k)*dimensions[0]*dimensions[1]));
        line->GetPointIds()->SetId(1, (i) + ((j)*dimensions[0]) + ((k+1)*dimensions[0]*dimensions[1]));
        grid->InsertNextCell(line);
      }
    }
  }  
  
  resampledField->GetPointData()->SetActiveVectors("ImageScalars");
  vtkSmartPointer<vtkFloatArray> vectorArray = vtkFloatArray::SafeDownCast(resampledField->GetPointData()->GetVectors());
  
  vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
  norm->SetInputConnection(resampledField->GetProducerPort());
  norm->Update();
  
  vtkSmartPointer<vtkFloatArray> vectorMagnitude = vtkFloatArray::SafeDownCast(norm->GetOutput()->GetPointData()->GetScalars());
  
  vtkSmartPointer<vtkPolyData> polygrid = vtkSmartPointer<vtkPolyData>::New();
  polygrid->SetPoints(points);
  polygrid->SetLines(grid);
  polygrid->GetPointData()->AddArray(vectorArray);
  polygrid->GetPointData()->SetActiveVectors(vectorArray->GetName());
  
  vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
  warp->SetInputConnection(polygrid->GetProducerPort());
  warp->SetScaleFactor(this->DeformationFieldVisualizerNode->GetGridScale());
  warp->Update();
  
  output->ShallowCopy(warp->GetPolyDataOutput());
  output->Update();
  output->GetPointData()->AddArray(vectorMagnitude);
  vectorMagnitude->SetName("VectorMagnitude");
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::BlockVisualization(vtkImageData* field, vtkPolyData* output)
{
  vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
  norm->SetInputConnection(field->GetProducerPort());
  norm->Update();
  vtkSmartPointer<vtkFloatArray> vectorMagnitude = vtkFloatArray::SafeDownCast(norm->GetOutput()->GetPointData()->GetScalars());

  vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
  warp->SetInputConnection(field->GetProducerPort());
  warp->SetScaleFactor(this->DeformationFieldVisualizerNode->GetBlockScale());

  //TODO: Current method of generating polydata is very inefficient but avoids bugs with possible extreme cases. Better method to be implemented.
  vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
  geometryFilter->SetInputConnection(warp->GetOutputPort());
  
  vtkPolyData* polyoutput = geometryFilter->GetOutput();
  polyoutput->Update();
  polyoutput->GetPointData()->AddArray(vectorMagnitude);
  vectorMagnitude->SetName("VectorMagnitude");
  
  if (this->DeformationFieldVisualizerNode->GetBlockDisplacementCheck())
  {
    vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
    normals->SetInput(polyoutput);  
    normals->Update();
    normals->GetOutput()->GetPointData()->SetVectors(field->GetPointData()->GetScalars());
    
    vtkSmartPointer<vtkVectorDot> dot = vtkSmartPointer<vtkVectorDot>::New();
    dot->SetInput(normals->GetOutput());
    dot->Update();
    vtkSmartPointer<vtkFloatArray> vectorDot = vtkFloatArray::SafeDownCast(dot->GetOutput()->GetPointData()->GetScalars());
    
    normals->GetOutput()->GetPointData()->AddArray(vectorDot);
    vectorDot->SetName("VectorDot");
    
    output->ShallowCopy(normals->GetOutput());
  }

  output->ShallowCopy(polyoutput);
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::ContourVisualization(vtkImageData* field, vtkPolyData* output)
{
  //Contour by vector magnitude
  vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
  norm->SetInputConnection(field->GetProducerPort());
  norm->Update();
    
  vtkSmartPointer<vtkMarchingCubes> iso = vtkSmartPointer<vtkMarchingCubes>::New();
	iso->SetInputConnection(norm->GetOutputPort());
  iso->ComputeScalarsOn();
  iso->ComputeNormalsOff();
  iso->ComputeGradientsOff();
  iso->GenerateValues(this->DeformationFieldVisualizerNode->GetContourNumber(), this->DeformationFieldVisualizerNode->GetContourMin(), this->DeformationFieldVisualizerNode->GetContourMax());
  iso->Update();
  
  vtkSmartPointer<vtkDecimatePro> decimator = vtkSmartPointer<vtkDecimatePro>::New();
  decimator->SetInputConnection(iso->GetOutputPort());
  decimator->SetFeatureAngle(60);
  decimator->SplittingOff();
  decimator->PreserveTopologyOn();
  decimator->SetMaximumError(1);
  decimator->SetTargetReduction(this->DeformationFieldVisualizerNode->GetContourDecimation());
  decimator->Update();

  decimator->GetOutput()->GetPointData()->GetScalars()->SetName("VectorMagnitude");
  
  output->ShallowCopy(decimator->GetOutput());
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::GlyphSliceVisualization(vtkImageData* inputField, vtkPolyData* output, vtkSmartPointer<vtkMatrix4x4> rasToIjkDirections)
{
  vtkSmartPointer<vtkImageData> field = vtkSmartPointer<vtkImageData>::New();
  field->DeepCopy(inputField);

  vtkSmartPointer<vtkRibbonFilter> ribbon = vtkSmartPointer<vtkRibbonFilter>::New();
  float sliceNormal[3] = {0,0,0};
  double width = 1;
  
  vtkSmartPointer<vtkMRMLSliceNode> sliceNode = NULL;
  vtkSmartPointer<vtkMRMLNode> node = this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetGlyphSliceNodeID());
  if (node != NULL)
  {
    sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
  }
  
  vtkSmartPointer<vtkMatrix4x4> sliceToIjk = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMatrix4x4::Multiply4x4(rasToIjkDirections,sliceNode->GetSliceToRAS(),sliceToIjk);
  
  sliceNormal[0] = sliceToIjk->GetElement(0,2);
  sliceNormal[1] = sliceToIjk->GetElement(1,2);
  sliceNormal[2] = sliceToIjk->GetElement(2,2);
  
  const double* spacing = field->GetSpacing();

  if (abs(sliceNormal[0]) > EPSILON)
  {
    width = spacing[0];
  }
  else if (abs(sliceNormal[1]) > EPSILON)
  {
    width = spacing[1];
  }    
  else if (abs(sliceNormal[2]) > EPSILON)
  {
    width = spacing[2];
  }
  //Oblique
  //TODO: Currently takes the largest spacing to avoid any issues. Only affects oblique slices, but will change later to a more precise method when slice options are updated
  else
  {
    if (spacing[0] >= spacing[1] && spacing[0] >= spacing[2])
    {
      width = spacing[0];
    }
    else if (spacing[1] >= spacing[0] && spacing[1] >= spacing[2])
    {
      width = spacing[1];
    }
    else
    {
      width = spacing[2];
    }
  }

  //Projection to slice plane
  float dot = 0;
  float *ptr = (float *)field->GetScalarPointer();
  for(int i = 0; i < field->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3)
  {
    dot = ptr[i]*sliceNormal[0] + ptr[i+1]*sliceNormal[1] + ptr[i+2]*sliceNormal[2];
    ptr[i] =  ptr[i] - dot*sliceNormal[0];
    ptr[i+1] =  ptr[i+1] - dot*sliceNormal[1];
    ptr[i+2] =  ptr[i+2] - dot*sliceNormal[2];
  }
  
  vtkSmartPointer<vtkDataArray> projected = field->GetPointData()->GetScalars();
  
  vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
  norm->SetInputConnection(inputField->GetProducerPort());
  norm->Update();
  vtkSmartPointer<vtkDataArray> vectorMagnitude = norm->GetOutput()->GetPointData()->GetScalars();
  
  field->GetPointData()->AddArray(projected);
  field->GetPointData()->GetArray(0)->SetName("Projected");  
  field->GetPointData()->AddArray(vectorMagnitude);
  field->GetPointData()->GetArray(1)->SetName("OriginalVectorMagnitude");  
    
  vtkSmartPointer<vtkTransform> rotateArrow = vtkSmartPointer<vtkTransform>::New();
  rotateArrow->RotateX(vtkMath::DegreesFromRadians(acos(abs(sliceNormal[2]))));
  
  vtkSmartPointer<vtkGlyphSource2D> arrow2DSource = vtkSmartPointer<vtkGlyphSource2D>::New();
  arrow2DSource->SetGlyphTypeToArrow();
  arrow2DSource->SetScale(1);
  arrow2DSource->SetFilled(0);
  
  vtkSmartPointer<vtkDeformationFieldVisualizerGlyph3D> glyphFilter = vtkSmartPointer<vtkDeformationFieldVisualizerGlyph3D>::New();
  glyphFilter->SetPointMax(this->DeformationFieldVisualizerNode->GetGlyphSlicePointMax());
  glyphFilter->SetSeed(this->DeformationFieldVisualizerNode->GetGlyphSliceSeed());
  glyphFilter->SetScaleModeToScaleByVector();
  glyphFilter->SetMagnitudeMax(this->DeformationFieldVisualizerNode->GetGlyphSliceThresholdMax());
  glyphFilter->SetMagnitudeMin(this->DeformationFieldVisualizerNode->GetGlyphSliceThresholdMin());
  glyphFilter->SetScaleFactor(this->DeformationFieldVisualizerNode->GetGlyphSliceScale());
  glyphFilter->SetScaleDirectional(false);
  glyphFilter->SetColorModeToColorByVector();
  glyphFilter->SetSourceTransform(rotateArrow);

  glyphFilter->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Projected");
  glyphFilter->SetInputArrayToProcess(3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Original");
  
  glyphFilter->SetSourceConnection(arrow2DSource->GetOutputPort());
  glyphFilter->SetInputConnection(field->GetProducerPort());
  glyphFilter->Update();
  
  ribbon->SetInputConnection(glyphFilter->GetOutputPort());
  ribbon->SetDefaultNormal(sliceNormal[0], sliceNormal[1], sliceNormal[2]);
  ribbon->SetWidth(width/2 + 0.15);
  ribbon->SetAngle(90.0);
  ribbon->UseDefaultNormalOn();
  ribbon->Update();
  
  output->ShallowCopy(ribbon->GetOutput());
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::GridSliceVisualization(vtkImageData* inputField, vtkPolyData* output, vtkSmartPointer<vtkMatrix4x4> rasToIjkDirections)
{
  vtkSmartPointer<vtkImageData> field = vtkSmartPointer<vtkImageData>::New();
  field->DeepCopy(inputField);
  
  vtkSmartPointer<vtkRibbonFilter> ribbon = vtkSmartPointer<vtkRibbonFilter>::New();
  float sliceNormal[3] = {0,0,0};
  double width = 1;

  vtkSmartPointer<vtkMRMLSliceNode> sliceNode = NULL;
  vtkSmartPointer<vtkMRMLNode> node = this->GetMRMLScene()->GetNodeByID(this->DeformationFieldVisualizerNode->GetGridSliceNodeID());
  if (node != NULL)
  {
    sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
  }
  
  vtkSmartPointer<vtkMatrix4x4> sliceToIjk = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMatrix4x4::Multiply4x4(rasToIjkDirections,sliceNode->GetSliceToRAS(),sliceToIjk);
  
  sliceNormal[0] = sliceToIjk->GetElement(0,2);
  sliceNormal[1] = sliceToIjk->GetElement(1,2);
  sliceNormal[2] = sliceToIjk->GetElement(2,2);
  
  const int subdivision = 1;
  int lineSpacing = this->DeformationFieldVisualizerNode->GetGridSliceSpacingMM();
  
  vtkSmartPointer<vtkImageResample> resampled = vtkSmartPointer<vtkImageResample>::New();
  resampled->SetInput(field);
  if (sliceNormal[0] < EPSILON)
  {
    resampled->SetAxisOutputSpacing(0, lineSpacing / subdivision);
  }
  if (sliceNormal[1] < EPSILON)
  {
    resampled->SetAxisOutputSpacing(1, lineSpacing / subdivision);
  }
  if (sliceNormal[2] < EPSILON)
  {
    resampled->SetAxisOutputSpacing(2, lineSpacing / subdivision);
  }
  resampled->Update();  
  
  vtkSmartPointer<vtkImageData> resampledField = vtkSmartPointer<vtkImageData>::New();
  resampledField = resampled->GetOutput();  
  
  const double* origin = resampledField->GetOrigin();  
  const double* spacing = resampledField->GetSpacing();
  const int* dimensions = resampledField->GetDimensions();  
  
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(dimensions[0]*dimensions[1]*dimensions[2]);
  vtkSmartPointer<vtkCellArray> grid = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
  
  //TODO: Need to ensure later that orientation of slice lines up with orientation of volume; no guarantee of that currently
  for (int k = 0; k < dimensions[2]; k++)
  {
    for (int j = 0; j < dimensions[1]; j++)
    {
      for (int i = 0; i < dimensions[0]; i++)
      {
        points->SetPoint(i + j*dimensions[0] + k*dimensions[0]*dimensions[1],
          origin[0] + i + ((spacing[0]-1)*i),origin[1] + j + ((spacing[1]-1)*j),origin[2] + k + ((spacing[2]-1)*k));
      }
    }
  }

  //Reformat not supported
  //TODO: Add support for reformat/oblique slices? 
  if (abs(sliceNormal[0]) < EPSILON)
  {
    for (int k = 0; k < dimensions[2]; k+=subdivision)
    {
      for (int j = 0; j < dimensions[1]; j+=subdivision)
      {
        for (int i = 0; i < ((dimensions[0]-1)-((dimensions[0]-1)%subdivision)); i++)
        {
          line->GetPointIds()->SetId(0, (i) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
          line->GetPointIds()->SetId(1, (i+1) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
          grid->InsertNextCell(line);
        }
      }
    }
  }
  else
  {
    width = spacing[0];
  }
  if (abs(sliceNormal[1]) < EPSILON)
  {
    for (int k = 0; k < dimensions[2]; k+=subdivision)
    {
      for (int j = 0; j < ((dimensions[1]-1)-((dimensions[1]-1)%subdivision)); j++)
      {
        for (int i = 0; i < dimensions[0]; i+=subdivision)
        {
          line->GetPointIds()->SetId(0, (i) + ((j)*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
          line->GetPointIds()->SetId(1, (i) + ((j+1)*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
          grid->InsertNextCell(line);
        }
      }
    }
  }
  else
  {
    width = spacing[1];
  }    
  if (abs(sliceNormal[2]) < EPSILON)
  {
    for (int k = 0; k < ((dimensions[2]-1)-((dimensions[2]-1)%subdivision)); k++)
    {
      for (int j = 0; j < dimensions[1]; j+=subdivision)
      {
        for (int i = 0; i < dimensions[0]; i+=subdivision)
        {
          line->GetPointIds()->SetId(0, (i) + ((j)*dimensions[0]) + ((k)*dimensions[0]*dimensions[1]));
          line->GetPointIds()->SetId(1, (i) + ((j)*dimensions[0]) + ((k+1)*dimensions[0]*dimensions[1]));
          grid->InsertNextCell(line);
        }
      }
    }
  }
  else
  {
    width = spacing[2];
  }

  resampledField->GetPointData()->SetActiveVectors("ImageScalars");
  
  vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
  norm->SetInputConnection(resampledField->GetProducerPort());
  norm->Update();
  
  vtkSmartPointer<vtkFloatArray> vectorMagnitude = vtkFloatArray::SafeDownCast(norm->GetOutput()->GetPointData()->GetScalars());  
  
  //Projection
  float dot = 0
  ;
  float *ptr = (float *)resampledField->GetScalarPointer();
  for(int i = 0; i < resampledField->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3)
  {
    dot = ptr[i]*sliceNormal[0] + ptr[i+1]*sliceNormal[1] + ptr[i+2]*sliceNormal[2];
    ptr[i] =  ptr[i] - dot*sliceNormal[0];
    ptr[i+1] =  ptr[i+1] - dot*sliceNormal[1];
    ptr[i+2] =  ptr[i+2] - dot*sliceNormal[2];
  }
  ribbon->SetDefaultNormal(sliceNormal[0], sliceNormal[1], sliceNormal[2]);

  vtkSmartPointer<vtkFloatArray> vectorArray = vtkFloatArray::SafeDownCast(resampledField->GetPointData()->GetVectors());

  vtkSmartPointer<vtkPolyData> polygrid = vtkSmartPointer<vtkPolyData>::New();
  polygrid->SetPoints(points);
  polygrid->SetLines(grid);
  polygrid->GetPointData()->AddArray(vectorArray);
  polygrid->GetPointData()->SetActiveVectors(vectorArray->GetName());
  
  vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
  warp->SetInputConnection(polygrid->GetProducerPort());
  warp->SetScaleFactor(this->DeformationFieldVisualizerNode->GetGridSliceScale());
  warp->Update();
  
  vtkPolyData* polyoutput = warp->GetPolyDataOutput();
  polyoutput->GetPointData()->AddArray(vectorMagnitude);
  vectorMagnitude->SetName("OriginalVectorMagnitude");
  
  // vtkRibbonFilter
  ribbon->SetInput(polyoutput);
  ribbon->SetWidth(width/2);
  ribbon->SetAngle(90);
  ribbon->UseDefaultNormalOn();
  ribbon->Update();
  
  output->ShallowCopy(ribbon->GetOutput());
}
