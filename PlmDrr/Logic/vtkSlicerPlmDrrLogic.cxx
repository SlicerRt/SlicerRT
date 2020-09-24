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

==============================================================================*/

// LoadableModuleTemplate Logic includes
#include "vtkSlicerPlmDrrLogic.h"

// SlicerRT PlanarImage includes
#include <vtkSlicerPlanarImageModuleLogic.h>
#include <vtkMRMLPlanarImageNode.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>

#include <vtkMRMLMarkupsClosedCurveNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsLineNode.h>

// SlicerRT MRML includes
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTPlanNode.h>
#include <vtkMRMLPlmDrrNode.h>

// SubjectHierarchy includes
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkSlicerSubjectHierarchyModuleLogic.h>

// VTK includes
#include <vtkTransform.h>
#include <vtkGeneralTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <string>

// ITK includes
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkMetaImageIO.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkFlipImageFilter.h>

// SlicerRT includes
#include <vtkSlicerRtCommon.h>

namespace
{

int InnerDrrNumber = 0;

} // namespace

const char* vtkSlicerPlmDrrLogic::IMAGER_BOUNDARY_MARKUPS_NODE_NAME = "ImagerBoundary"; // curve
const char* vtkSlicerPlmDrrLogic::IMAGE_WINDOW_MARKUPS_NODE_NAME = "ImageWindow"; // curve
const char* vtkSlicerPlmDrrLogic::FIDUCIALS_MARKUPS_NODE_NAME = "FiducialPoints"; // fiducial
const char* vtkSlicerPlmDrrLogic::NORMAL_VECTOR_MARKUPS_NODE_NAME = "NormalVector"; // line
const char* vtkSlicerPlmDrrLogic::VUP_VECTOR_MARKUPS_NODE_NAME = "VupVector"; // line

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlmDrrLogic);
//vtkCxxSetObjectMacro(vtkSlicerPlmDrrLogic, PlanarImageLogic, vtkSlicerPlanarImageModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerPlmDrrLogic::vtkSlicerPlmDrrLogic()
  :
  PlanarImageLogic(nullptr)
{
}

//----------------------------------------------------------------------------
vtkSlicerPlmDrrLogic::~vtkSlicerPlmDrrLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLRTPlanNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLRTBeamNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLPlmDrrNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLPlmDrrNode>::New());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::UpdateFromMRMLScene()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
    return;
  }

  if (node->IsA("vtkMRMLRTBeamNode"))
  {
    // Observe beam events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTBeamNode::BeamTransformModified);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
  else if (node->IsA("vtkMRMLRTPlanNode"))
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTPlanNode::IsocenterModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
  else if (node->IsA("vtkMRMLPlmDrrNode"))
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//----------------------------------------------------------------------------
bool vtkSlicerPlmDrrLogic::LoadRawImageFromFile( vtkMRMLPlmDrrNode* parameterNode, 
  vtkMRMLScalarVolumeNode* volumeNode, const std::string& filename)
{
  if (!parameterNode)
  {
    vtkErrorMacro("LoadRawImageFromFile: Invalid parameter set node");
    return false;
  }

  if (filename.empty())
  {
    vtkErrorMacro("LoadRawImageFromFile: MetaImageHeader file name is empty");
    return false;
  }

  if (!volumeNode)
  {
    vtkErrorMacro("LoadRawImageFromFile: volume node is invalid");
    return false;
  }

  // Plastimatch DRR input pixel type (long)
  using InputPixelType = signed long int;
  const unsigned int InputDimension = 3;

  using InputImageType = itk::Image< InputPixelType, InputDimension >;
  using ReaderType = itk::ImageFileReader< InputImageType >;

  // Inner output pixel type (double)
  using OutputPixelType = double;
  const unsigned int OutputDimension = 3;
  using OutputImageType = itk::Image< OutputPixelType, OutputDimension >;


  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(filename.c_str());
  try
  {
    reader->Update();
  }
  catch(itk::ExceptionObject& ex)
  {
    vtkErrorMacro("LoadRawImageFromFile: Reading exception caught " << ex);
    return false;
  }

  using CastFilterType = itk::CastImageFilter< InputImageType, OutputImageType >;
  CastFilterType::Pointer cast = CastFilterType::New();
  cast->SetInput(reader->GetOutput());
  try
  {
    cast->Update();
  }
  catch(itk::ExceptionObject& ex)
  {
    vtkErrorMacro("LoadRawImageFromFile: Image casting exception caught " << ex);
    return false;
  }

  using RescaleFilterType = itk::RescaleIntensityImageFilter< OutputImageType, OutputImageType >;
  RescaleFilterType::Pointer rescale = RescaleFilterType::New();
  rescale->SetOutputMinimum(0.);
  rescale->SetOutputMaximum(255.);
  rescale->SetInput(cast->GetOutput());
  try
  {
    rescale->Update();
  }
  catch(itk::ExceptionObject& ex)
  {
    vtkErrorMacro("LoadRawImageFromFile: Image rescaling exception caught " << ex);
    return false;
  }

  using InvertFilterType = itk::InvertIntensityImageFilter< OutputImageType, OutputImageType >;
  InvertFilterType::Pointer invert = InvertFilterType::New();
  invert->SetInput(rescale->GetOutput());
  invert->SetMaximum(0);
  try
  {
    invert->Update();
  }
  catch(itk::ExceptionObject& ex)
  {
    vtkErrorMacro("LoadRawImageFromFile: Problem inverting data \n" << ex);
    return false;
  }

  return vtkSlicerRtCommon::ConvertItkImageToVolumeNode< OutputPixelType >( invert->GetOutput(), volumeNode, VTK_DOUBLE);
}

//----------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::CreateMarkupsNodes(vtkMRMLPlmDrrNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateMarkupsNodes: Invalid MRML scene");
    return;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();
  vtkMRMLTransformNode* beamTransformNode = nullptr;
  if (beamNode)
  {
    beamTransformNode = beamNode->GetParentTransformNode();
  }

  // Create markups nodes if they don't exist

  // Imager boundary markups node
  vtkSmartPointer<vtkMRMLMarkupsClosedCurveNode> imagerMarkupsNode;
  if (!scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME))
  {
    imagerMarkupsNode = this->CreateImagerBoundary(parameterNode);
  }
  else
  {
    imagerMarkupsNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME));
    // Update imager points using parameter node data
    if (beamTransformNode)
    {
      imagerMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // Image window markups node
  vtkSmartPointer<vtkMRMLMarkupsClosedCurveNode> imageWindowMarkupsNode;
  if (!scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME))
  {
    imageWindowMarkupsNode = this->CreateImageWindow(parameterNode);
  }
  else
  {
    imageWindowMarkupsNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME));
    // Update image window points using parameter node data
    if (beamTransformNode)
    {
      imageWindowMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // imager normal vector markups node
  vtkSmartPointer<vtkMRMLMarkupsLineNode> normalVectorMarkupsNode;
  if (!scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME))
  {
    normalVectorMarkupsNode = this->CreateImagerNormal(parameterNode);
  }
  else
  {
    normalVectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME));
    // Update Normal vector points using parameter node data
    if (beamTransformNode)
    {
      normalVectorMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // imager vup vector markups node
  vtkSmartPointer<vtkMRMLMarkupsLineNode> vupVectorMarkupsNode;
  if (!scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME))
  {
    vupVectorMarkupsNode = this->CreateImagerVUP(parameterNode);
  }
  else
  {
    vupVectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME));
    // Update VUP vector points using parameter node data
    if (beamTransformNode)
    {
      vupVectorMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // fiducial markups node
  vtkSmartPointer<vtkMRMLMarkupsFiducialNode> pointsMarkupsNode;
  if (!scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME))
  {
    pointsMarkupsNode = this->CreateFiducials(parameterNode);
  }
  else
  {
    pointsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME));
    // Update fiducial points using parameter node data
    if (beamTransformNode)
    {
      pointsMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::UpdateMarkupsNodes(vtkMRMLPlmDrrNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid MRML scene");
    return;
  }
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid parameter set node");
    return;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();
  if (!beamNode)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid beam node");
    return;
  }

  vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
  if (!beamTransformNode)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Beam transform node is invalid");
    return;
  }

  // Imager boundary markups node
  if (scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsClosedCurveNode* imagerMarkupsNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME));

    double distance = parameterNode->GetIsocenterImagerDistance();
    
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double imagerHalfWidth = spacing[0] * dimention[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * dimention[1] / 2.; // rows

    // update points
    vtkVector3d imagerP0( -1. * imagerHalfWidth + offset[0], imagerHalfHeight + offset[1], -distance);
    vtkVector3d imagerP1( imagerHalfWidth + offset[0], imagerHalfHeight + offset[1], -distance);
    vtkVector3d imagerP2( imagerHalfWidth + offset[0], -1. * imagerHalfHeight + offset[1], -distance);
    vtkVector3d imagerP3( -1. * imagerHalfWidth + offset[0], -1. * imagerHalfHeight + offset[1], -distance);

    double p[3];
    imagerMarkupsNode->GetNthControlPointPosition( 0, p);
    p[0] = imagerP0.GetX();
    p[1] = imagerP0.GetY();
    p[2] = imagerP0.GetZ();
    imagerMarkupsNode->SetNthControlPointPosition( 0, p[0], p[1], p[2]);

    imagerMarkupsNode->GetNthControlPointPosition( 1, p);
    p[0] = imagerP1.GetX();
    p[1] = imagerP1.GetY();
    p[2] = imagerP1.GetZ();
    imagerMarkupsNode->SetNthControlPointPosition( 1, p[0], p[1], p[2]);

    imagerMarkupsNode->GetNthControlPointPosition( 2, p);
    p[0] = imagerP2.GetX();
    p[1] = imagerP2.GetY();
    p[2] = imagerP2.GetZ();
    imagerMarkupsNode->SetNthControlPointPosition( 2, p[0], p[1], p[2]);

    imagerMarkupsNode->GetNthControlPointPosition( 3, p);
    p[0] = imagerP3.GetX();
    p[1] = imagerP3.GetY();
    p[2] = imagerP3.GetZ();
    imagerMarkupsNode->SetNthControlPointPosition( 3, p[0], p[1], p[2]);

    // Update imager boundary markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = imagerMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode/* && beamTransformNode->GetID() != markupsTransformNode->GetID()*/)
    {
      imagerMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // Image window markups node
  if (scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsClosedCurveNode* imageWindowMarkupsNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME));

    imageWindowMarkupsNode->SetDisplayVisibility(parameterNode->GetImageWindowFlag());

    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    int imageWindow[4] = {};
    parameterNode->GetImageWindow(imageWindow);

    double imagerHalfWidth = spacing[0] * dimention[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * dimention[1] / 2.; // rows

    // imager top left corner
    vtkVector3d imagerP0( -1. * imagerHalfWidth + offset[0], imagerHalfHeight + offset[1], -distance);

    double r1 = -1. * imagerP0.GetY() + imageWindow[1] * spacing[1];
    double c1 = imagerP0.GetX() + imageWindow[0] * spacing[0];
    double r2 = -1. * imagerP0.GetY() + imageWindow[3] * spacing[1];
    double c2 = imagerP0.GetX() + imageWindow[2] * spacing[0];

    // update points
    vtkVector3d imageP0( c1, r1, -distance);
    vtkVector3d imageP1( c1, r2, -distance);
    vtkVector3d imageP2( c2, r2, -distance);
    vtkVector3d imageP3( c2, r1, -distance);

    double p[3];
    imageWindowMarkupsNode->GetNthControlPointPosition( 0, p);
    p[0] = imageP0.GetX();
    p[1] = imageP0.GetY();
    p[2] = imageP0.GetZ();
    imageWindowMarkupsNode->SetNthControlPointPosition( 0, p[0], p[1], p[2]);

    imageWindowMarkupsNode->GetNthControlPointPosition( 1, p);
    p[0] = imageP1.GetX();
    p[1] = imageP1.GetY();
    p[2] = imageP1.GetZ();
    imageWindowMarkupsNode->SetNthControlPointPosition( 1, p[0], p[1], p[2]);

    imageWindowMarkupsNode->GetNthControlPointPosition( 2, p);
    p[0] = imageP2.GetX();
    p[1] = imageP2.GetY();
    p[2] = imageP2.GetZ();
    imageWindowMarkupsNode->SetNthControlPointPosition( 2, p[0], p[1], p[2]);

    imageWindowMarkupsNode->GetNthControlPointPosition( 3, p);
    p[0] = imageP3.GetX();
    p[1] = imageP3.GetY();
    p[2] = imageP3.GetZ();
    imageWindowMarkupsNode->SetNthControlPointPosition( 3, p[0], p[1], p[2]);

    // Update image window markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = imageWindowMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode/* && beamTransformNode->GetID() != markupsTransformNode->GetID()*/)
    {
      imageWindowMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // normal vector markups line node
  if (scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME));

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double distance = parameterNode->GetIsocenterImagerDistance();

    // update points
    vtkVector3d p0( offset[0], offset[1], -distance);
    vtkVector3d p1( offset[0], offset[1], -distance + 100.);

    double p[3];
    vectorMarkupsNode->GetNthControlPointPosition( 0, p);
    p[0] = p0.GetX();
    p[1] = p0.GetY();
    p[2] = p0.GetZ();
    vectorMarkupsNode->SetNthControlPointPosition( 0, p[0], p[1], p[2]);

    vectorMarkupsNode->GetNthControlPointPosition( 1, p);
    p[0] = p1.GetX();
    p[1] = p1.GetY();
    p[2] = p1.GetZ();
    vectorMarkupsNode->SetNthControlPointPosition( 1, p[0], p[1], p[2]);

    // Update imager normal vector markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = vectorMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode/* && beamTransformNode->GetID() != markupsTransformNode->GetID()*/)
    {
      vectorMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // vup vector markups line node
  if (scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME));

    double distance = parameterNode->GetIsocenterImagerDistance();

    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double x = spacing[0] * dimention[0] / 2.; // center imager column

    // update points
    vtkVector3d p0( offset[0], offset[1], -distance);
    vtkVector3d p1( -x + offset[0], offset[1], -distance);

    double p[3];
    vectorMarkupsNode->GetNthControlPointPosition( 0, p);
    p[0] = p0.GetX();
    p[1] = p0.GetY();
    p[2] = p0.GetZ();
    vectorMarkupsNode->SetNthControlPointPosition( 0, p[0], p[1], p[2]);

    vectorMarkupsNode->GetNthControlPointPosition( 1, p);
    p[0] = p1.GetX();
    p[1] = p1.GetY();
    p[2] = p1.GetZ();
    vectorMarkupsNode->SetNthControlPointPosition( 1, p[0], p[1], p[2]);

    // Update VUP VECTOR markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = vectorMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode/* && beamTransformNode->GetID() != markupsTransformNode->GetID()*/)
    {
      vectorMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }

  // fiducial markups line node
  if (scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsFiducialNode* pointsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME));

    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double x = spacing[0] * dimention[0] / 2.; // imager center column
    double y = spacing[1] * dimention[1] / 2.; // imager center row

    // update points
    vtkVector3d p0( 0., 0., -distance); // imager center
    vtkVector3d p1( 0., 0., -distance + 100.); // imager normal vector
    vtkVector3d p2( -x + offset[0], -y + offset[1], -distance); // (0,0)
    vtkVector3d p3( -x + offset[0], 0., -distance); // vup vector

    double p[3];
    pointsMarkupsNode->GetNthControlPointPosition( 0, p);
    p[0] = p0.GetX();
    p[1] = p0.GetY();
    p[2] = p0.GetZ();
    pointsMarkupsNode->SetNthControlPointPosition( 0, p[0], p[1], p[2]);

    pointsMarkupsNode->GetNthControlPointPosition( 1, p);
    p[0] = p1.GetX();
    p[1] = p1.GetY();
    p[2] = p1.GetZ();
    pointsMarkupsNode->SetNthControlPointPosition( 1, p[0], p[1], p[2]);

    pointsMarkupsNode->GetNthControlPointPosition( 2, p);
    p[0] = p2.GetX();
    p[1] = p2.GetY();
    p[2] = p2.GetZ();
    pointsMarkupsNode->SetNthControlPointPosition( 2, p[0], p[1], p[2]);

    pointsMarkupsNode->GetNthControlPointPosition( 3, p);
    p[0] = p3.GetX();
    p[1] = p3.GetY();
    p[2] = p3.GetZ();
    pointsMarkupsNode->SetNthControlPointPosition( 3, p[0], p[1], p[2]);

    // Update fiducials markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = pointsMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode/* && beamTransformNode->GetID() != markupsTransformNode->GetID()*/)
    {
      pointsMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::ShowMarkupsNodes(vtkMRMLPlmDrrNode* parameterNode, bool toggled)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid MRML scene");
    return;
  }
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid parameter set node");
    return;
  }

  // Imager boundary markups node
  if (scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsClosedCurveNode* imagerMarkupsNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME));
    imagerMarkupsNode->SetDisplayVisibility(int(toggled));
  }

  // Image window markups node
  if (scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsClosedCurveNode* imageWindowMarkupsNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME));
    imageWindowMarkupsNode->SetDisplayVisibility(int(toggled));
  }

  // normal vector markups line node
  if (scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME));
    vectorMarkupsNode->SetDisplayVisibility(int(toggled));
  }

  // vup vector markups line node
  if (scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME));
    vectorMarkupsNode->SetDisplayVisibility(int(toggled));
  }

  // fiducial markups line node
  if (scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsFiducialNode* pointsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME));
    pointsMarkupsNode->SetDisplayVisibility(int(toggled));
  }
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsClosedCurveNode* vtkSlicerPlmDrrLogic::CreateImagerBoundary(vtkMRMLPlmDrrNode* parameterNode)
{
  auto imagerMarkupsNode = vtkSmartPointer<vtkMRMLMarkupsClosedCurveNode>::New();
  this->GetMRMLScene()->AddNode(imagerMarkupsNode);
  imagerMarkupsNode->SetName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME);
  imagerMarkupsNode->SetCurveTypeToLinear();
  imagerMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("DRR_") + IMAGER_BOUNDARY_MARKUPS_NODE_NAME;
  imagerMarkupsNode->SetSingletonTag(singletonTag.c_str());

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double imagerHalfWidth = spacing[0] * dimention[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * dimention[1] / 2.; // rows

    // add points
    vtkVector3d imagerP0( -1. * imagerHalfWidth + offset[0], imagerHalfHeight + offset[1], -distance);
    vtkVector3d imagerP1( imagerHalfWidth + offset[0], imagerHalfHeight + offset[1], -distance);
    vtkVector3d imagerP2( imagerHalfWidth + offset[0], -1. * imagerHalfHeight + offset[1], -distance);
    vtkVector3d imagerP3( -1. * imagerHalfWidth + offset[0], -1. * imagerHalfHeight + offset[1], -distance);

    imagerMarkupsNode->AddControlPoint(imagerP0); // "Upper Left", "-x,y"
    imagerMarkupsNode->AddControlPoint(imagerP1); // "Upper Right", "x,y"
    imagerMarkupsNode->AddControlPoint(imagerP2); // "Lower Right", "x,-y"
    imagerMarkupsNode->AddControlPoint(imagerP3); // "Lower Left", "-x,-y"

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
      if (beamTransformNode)
      {
        imagerMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
      }
    }
  }
  return imagerMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsClosedCurveNode* vtkSlicerPlmDrrLogic::CreateImageWindow(vtkMRMLPlmDrrNode* parameterNode)
{
  auto imageWindowMarkupsNode = vtkSmartPointer<vtkMRMLMarkupsClosedCurveNode>::New();
  this->GetMRMLScene()->AddNode(imageWindowMarkupsNode);
  imageWindowMarkupsNode->SetName(IMAGE_WINDOW_MARKUPS_NODE_NAME);
  imageWindowMarkupsNode->SetCurveTypeToLinear();
  imageWindowMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("DRR_") + IMAGE_WINDOW_MARKUPS_NODE_NAME;
  imageWindowMarkupsNode->SetSingletonTag(singletonTag.c_str());

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    int imageWindow[4] = {};
    parameterNode->GetImageWindow(imageWindow);

    double imagerHalfWidth = spacing[0] * dimention[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * dimention[1] / 2.; // rows

    // Imager upper left point
    vtkVector3d imagerP0( -1. * imagerHalfWidth + offset[0], imagerHalfHeight + offset[1], -distance);

    double r1 = -1. * imagerP0.GetY() + imageWindow[1] * spacing[1];
    double c1 = imagerP0.GetX() + imageWindow[0] * spacing[0];
    double r2 = -1. * imagerP0.GetY() + imageWindow[3] * spacing[1];
    double c2 = imagerP0.GetX() + imageWindow[2] * spacing[0];

    // add points
    vtkVector3d imageP0( c1, r1, -distance);
    vtkVector3d imageP1( c1, r2, -distance);
    vtkVector3d imageP2( c2, r2, -distance);
    vtkVector3d imageP3( c2, r1, -distance);

    imageWindowMarkupsNode->AddControlPoint(imageP0); // r1, c1
    imageWindowMarkupsNode->AddControlPoint(imageP1); // r2, c1
    imageWindowMarkupsNode->AddControlPoint(imageP2); // r2, c2
    imageWindowMarkupsNode->AddControlPoint(imageP3); // r1, c2

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
      if (beamTransformNode)
      {
        imageWindowMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
      }
    }
  }
  return imageWindowMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsLineNode* vtkSlicerPlmDrrLogic::CreateImagerNormal(vtkMRMLPlmDrrNode* parameterNode)
{
  auto vectorMarkupsNode = vtkSmartPointer<vtkMRMLMarkupsLineNode>::New();
  this->GetMRMLScene()->AddNode(vectorMarkupsNode);
  vectorMarkupsNode->SetName(NORMAL_VECTOR_MARKUPS_NODE_NAME);
  vectorMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("DRR_") + NORMAL_VECTOR_MARKUPS_NODE_NAME;
  vectorMarkupsNode->SetSingletonTag(singletonTag.c_str());

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();

    // add points
    vtkVector3d p0( 0, 0, -distance);
    vtkVector3d p1( 0, 0, -distance + 100.);

    vectorMarkupsNode->AddControlPoint(p0);
    vectorMarkupsNode->AddControlPoint(p1);

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
      if (beamTransformNode)
      {
        vectorMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
      }
    }
  }
  return vectorMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsLineNode* vtkSlicerPlmDrrLogic::CreateImagerVUP(vtkMRMLPlmDrrNode* parameterNode)
{
  auto vectorMarkupsNode = vtkSmartPointer<vtkMRMLMarkupsLineNode>::New();
  this->GetMRMLScene()->AddNode(vectorMarkupsNode);
  vectorMarkupsNode->SetName(VUP_VECTOR_MARKUPS_NODE_NAME);
  vectorMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("DRR_") + VUP_VECTOR_MARKUPS_NODE_NAME;
  vectorMarkupsNode->SetSingletonTag(singletonTag.c_str());

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double x = spacing[0] * dimention[0] / 2.; // center column
//    double y = spacing[1] * dimention[1] / 2.; // center row

    // add points
    vtkVector3d p0( 0 + offset[0], 0 + offset[1], -distance);
    vtkVector3d p1( -x + offset[0], 0 + offset[1], -distance);

    vectorMarkupsNode->AddControlPoint(p0);
    vectorMarkupsNode->AddControlPoint(p1);

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
      if (beamTransformNode)
      {
        vectorMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
      }
    }
  }
  return vectorMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkSlicerPlmDrrLogic::CreateFiducials(vtkMRMLPlmDrrNode* parameterNode)
{
  auto pointsMarkupsNode = vtkSmartPointer<vtkMRMLMarkupsFiducialNode>::New();
  this->GetMRMLScene()->AddNode(pointsMarkupsNode);
  pointsMarkupsNode->SetName(FIDUCIALS_MARKUPS_NODE_NAME);
  pointsMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("DRR_") + FIDUCIALS_MARKUPS_NODE_NAME;
  pointsMarkupsNode->SetSingletonTag(singletonTag.c_str());

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImageSpacing(spacing);

    int dimention[2] = {};
    parameterNode->GetImageDimention(dimention);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double x = spacing[0] * dimention[0] / 2.; // columns
    double y = spacing[1] * dimention[1] / 2.; // rows

    // add points
    vtkVector3d p0( 0, 0, -distance); // imager center
    vtkVector3d p1( 0, 0, -distance + 100.); // n
    vtkVector3d p2( -x + offset[0], -y + offset[1], -distance); // (0,0)
    vtkVector3d p3( -x + offset[0], 0.0, -distance); // vup

    pointsMarkupsNode->AddControlPoint( p0, "Imager center");
    pointsMarkupsNode->AddControlPoint( p1, "n");
    pointsMarkupsNode->AddControlPoint( p2, "(0,0)");
    pointsMarkupsNode->AddControlPoint( p3, "VUP");

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
      if (beamTransformNode)
      {
        pointsMarkupsNode->SetAndObserveTransformNodeID(beamTransformNode->GetID());
      }
    }
  }
  return pointsMarkupsNode;
}

//----------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLNodesEvents( caller, event, callData);

  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("ProcessMRMLNodesEvents: Invalid MRML scene");
    return;
  }
  if (mrmlScene->IsBatchProcessing())
  {
    return;
  }

  if (caller->IsA("vtkMRMLRTBeamNode"))
  {
//    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(caller);
    if (event == vtkMRMLRTBeamNode::BeamTransformModified)
    {
      vtkDebugMacro("ProcessMRMLNodesEvents: RTBeam transformation has been changed");
    }
    else if (event == vtkMRMLRTBeamNode::BeamGeometryModified)
    {
      vtkDebugMacro("ProcessMRMLNodesEvents: RTBeam geometry has been changed");
    }
  }
  else if (caller->IsA("vtkMRMLRTPlanNode"))
  {
//    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(caller);
    if (event == vtkMRMLRTPlanNode::IsocenterModifiedEvent)
    {
      vtkDebugMacro("ProcessMRMLNodesEvents: RTPlan isocenter has been changed");
    }
  }
  else if (caller->IsA("vtkMRMLPlmDrrNode"))
  {
//    vtkMRMLPlmDrrNode* parameterNode = vtkMRMLPlmDrrNode::SafeDownCast(caller);
    if (event == vtkCommand::ModifiedEvent)
    {
      vtkDebugMacro("ProcessMRMLNodesEvents: Plastimatch DRR node modified");
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerPlmDrrLogic::LoadRtImage( vtkMRMLPlmDrrNode* paramNode, const std::string& filename)
{
  if (!paramNode)
  {
    vtkErrorMacro("LoadRtImage: Invalid parameter node");
    return false;
  }

  vtkMRMLRTBeamNode* beamNode = paramNode->GetBeamNode();

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("LoadRtImage: Invalid MRML scene");
    return false;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("LoadRtImage: Failed to access subject hierarchy node");
    return false;
  }

  // Create node for the volume
  vtkNew<vtkMRMLScalarVolumeNode> drrVolumeNode;
  scene->AddNode(drrVolumeNode);

  if (!this->LoadRawImageFromFile( paramNode, drrVolumeNode, filename))
  {
    vtkErrorMacro("LoadRtImage: Unable to load raw image from file \"" << filename << "\"");
    return false;
  }

  // Create display node for the volume
  vtkNew<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode;
  scene->AddNode(volumeDisplayNode);
  volumeDisplayNode->SetDefaultColorMap();

  signed long int AutoscaleRange[2];
  paramNode->GetAutoscaleRange(AutoscaleRange);
  if (!paramNode->GetAutoscaleFlag() || (paramNode->GetAutoscaleFlag() &&
    !AutoscaleRange[0] && !AutoscaleRange[1]))
  {
    volumeDisplayNode->AutoWindowLevelOn();
  }
  else
  {
    // Apply given window level if available
    float center = float(AutoscaleRange[1] - AutoscaleRange[0]) / 2.;
    float width = float(AutoscaleRange[1] - AutoscaleRange[0]);

    volumeDisplayNode->AutoWindowLevelOff();
    volumeDisplayNode->SetWindowLevel(width, center);
  }

  drrVolumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

  // Set up subject hierarchy item
  vtkIdType drrVolumeShItemID = shNode->CreateItem( shNode->GetSceneItemID(), drrVolumeNode);

  double sid = beamNode->GetSAD() + paramNode->GetIsocenterImagerDistance();
  // Set RT image specific attributes
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME, "1");
  shNode->SetItemAttribute(drrVolumeShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName(), "");
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME, std::to_string(beamNode->GetSAD()));
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME, std::to_string(beamNode->GetGantryAngle()));
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME, std::to_string(beamNode->GetCouchAngle()));
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME, std::to_string(beamNode->GetCollimatorAngle()));
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME, std::to_string(beamNode->GetBeamNumber()));
  shNode->SetItemAttribute(drrVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME, std::to_string(sid));

  double rtImagePosition[2] = {};
  paramNode->GetRTImagePosition(rtImagePosition);
  std::string rtImagePositionString = std::to_string(rtImagePosition[0]) + std::string(" ") + std::to_string(rtImagePosition[1]);
  shNode->SetItemAttribute(drrVolumeShItemID,  vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME, rtImagePositionString);

  // Compute and set RT image geometry. Uses the referenced beam if available, otherwise the geometry will be set up when loading the referenced beam
  this->SetupRtImageGeometry( paramNode, drrVolumeNode, drrVolumeShItemID);

  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerPlmDrrLogic::SetupRtImageGeometry( vtkMRMLPlmDrrNode* paramNode,
  vtkMRMLScalarVolumeNode* drrVolumeNode, vtkIdType vtkNotUsed(rtImageShItemID))
{
  if (!paramNode)
  {
    vtkErrorMacro("SetupRtImageGeometry: Invalid parameter node");
    return false;
  }

  vtkMRMLRTBeamNode* beamNode = paramNode->GetBeamNode();
  if (!beamNode)
  {
    vtkErrorMacro("SetupRtImageGeometry: Invalid beam node");
    return false;
  }

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("SetupRtImageGeometry: Failed to access subject hierarchy node");
    return false;
  }

  vtkIdType rtImageShItemID = shNode->GetItemByDataNode(drrVolumeNode);
  if (!rtImageShItemID)
  {
    vtkErrorMacro("SetupRtImageGeometry: Failed to get DRR RTImage subject hierarchy node");
    return false;
  }

  // If the function is called from the LoadRtPlan function with a beam: find corresponding RT image
  if (beamNode)
  {
    // Get RT plan for beam
    vtkMRMLRTPlanNode *planNode = beamNode->GetParentPlanNode();
    if (!planNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid plan node for beam '" << beamNode->GetName() << "'");
      return false;
    }
    vtkIdType planShItemID = planNode->GetPlanSubjectHierarchyItemID();
    if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid plan subject hierarchy item for beam '" << beamNode->GetName() << "'");
      return false;
    }
    std::string rtPlanSopInstanceUid = shNode->GetItemUID(planShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
    if (rtPlanSopInstanceUid.empty())
    {
      vtkWarningMacro("SetupRtImageGeometry: Failed to get RT Plan DICOM UID for beam '" << beamNode->GetName() << "'");
    }

    // Return if a referenced displayed model is present for the RT image, because it means that the geometry has been set up successfully before
    if (drrVolumeNode)
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        drrVolumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (modelNode)
      {
        vtkWarningMacro("SetupRtImageGeometry: RT image '" << drrVolumeNode->GetName() << "' belonging to beam '" << beamNode->GetName() << "' seems to have been set up already.");
        return false;
      }
    }
      
    if (!drrVolumeNode)
    {
      // RT image for the isocenter is not loaded yet. Geometry will be set up upon loading the related RT image
      vtkWarningMacro("SetupRtImageGeometry: Cannot set up geometry of RT image corresponding to beam '" << beamNode->GetName()
        << "' because the RT image is not loaded yet. Will be set up upon loading the related RT image");
      return false;
    }

    // Set more user friendly DRR image name
    std::string drrName = std::string("DRR ") + std::to_string(InnerDrrNumber++) + std::string(" : ") + std::string(beamNode->GetName());
    drrVolumeNode->SetName(drrName.c_str());
  }
  else
  {
    vtkErrorMacro("SetupRtImageGeometry: Input node is neither a volume node nor an plan POIs markups fiducial node");
    return false;
  }

  // We have both the RT image and the isocenter, we can set up the geometry

  // Get source to RT image plane distance (along beam axis)
  double rtImageSid = 0.0;
  std::string rtImageSidStr = shNode->GetItemAttribute(rtImageShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME);
  if (!rtImageSidStr.empty())
  {
    rtImageSid = vtkVariant(rtImageSidStr).ToDouble();
  }

  // Get RT image position (the x and y coordinates (in mm) of the upper left hand corner of the image, in the IEC X-RAY IMAGE RECEPTOR coordinate system)
  double rtImagePosition[2] = {};
  std::string rtImagePositionStr = shNode->GetItemAttribute(rtImageShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME);
  if (!rtImagePositionStr.empty())
  {
    std::stringstream ss;
    ss << rtImagePositionStr;
    ss >> rtImagePosition[0] >> rtImagePosition[1];
  }

  // Extract beam-related parameters needed to compute RT image coordinate system
  double sourceAxisDistance = beamNode->GetSAD();
  double gantryAngle = beamNode->GetGantryAngle();
  double couchAngle = beamNode->GetCouchAngle();

  int window[4];
  paramNode->GetImageWindow(window);
  double spacing[2];
  paramNode->GetImageSpacing(spacing);

  // Get isocenter coordinates
  double isocenterWorldCoordinates[3] = {};
  if (!beamNode->GetPlanIsocenterPosition(isocenterWorldCoordinates))
  {
    vtkErrorMacro("SetupRtImageGeometry: Failed to get plan isocenter position");
    return false;
  }

  // Assemble transform from isocenter IEC to RT image RAS
  vtkSmartPointer<vtkTransform> fixedToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  fixedToIsocenterTransform->Identity();
  fixedToIsocenterTransform->Translate(isocenterWorldCoordinates);

  vtkSmartPointer<vtkTransform> couchToFixedTransform = vtkSmartPointer<vtkTransform>::New();
  couchToFixedTransform->Identity();
  couchToFixedTransform->RotateWXYZ(couchAngle, 0.0, 1.0, 0.0);

  vtkSmartPointer<vtkTransform> gantryToCouchTransform = vtkSmartPointer<vtkTransform>::New();
  gantryToCouchTransform->Identity();
  gantryToCouchTransform->RotateWXYZ(gantryAngle, 0.0, 0.0, 1.0);

  vtkSmartPointer<vtkTransform> sourceToGantryTransform = vtkSmartPointer<vtkTransform>::New();
  sourceToGantryTransform->Identity();
  sourceToGantryTransform->Translate(0.0, sourceAxisDistance, 0.0);

  vtkSmartPointer<vtkTransform> rtImageToSourceTransform = vtkSmartPointer<vtkTransform>::New();
  rtImageToSourceTransform->Identity();
  rtImageToSourceTransform->Translate(0.0, -1. * rtImageSid, 0.0);

  vtkSmartPointer<vtkTransform> rtImageCenterToCornerTransform = vtkSmartPointer<vtkTransform>::New();
  rtImageCenterToCornerTransform->Identity();
  rtImageCenterToCornerTransform->Translate( -1. * rtImagePosition[0], 0.0, rtImagePosition[1]);

  // Create isocenter to RAS transform
  // The transformation below is based section C.8.8 in DICOM standard volume 3:
  // "Note: IEC document 62C/269/CDV 'Amendment to IEC 61217: Radiotherapy Equipment -
  //  Coordinates, movements and scales' also defines a patient-based coordinate system, and
  //  specifies the relationship between the DICOM Patient Coordinate System (see Section
  //  C.7.6.2.1.1) and the IEC PATIENT Coordinate System. Rotating the IEC PATIENT Coordinate
  //  System described in IEC 62C/269/CDV (1999) by 90 degrees counter-clockwise (in the negative
  //  direction) about the x-axis yields the DICOM Patient Coordinate System, i.e. (XDICOM, YDICOM,
  //  ZDICOM) = (XIEC, -ZIEC, YIEC). Refer to the latest IEC documentation for the current definition of the
  //  IEC PATIENT Coordinate System."
  // The IJK to RAS transform already contains the LPS to RAS conversion, so we only need to consider this rotation
  vtkSmartPointer<vtkTransform> iecToLpsTransform = vtkSmartPointer<vtkTransform>::New();
  iecToLpsTransform->Identity();
  iecToLpsTransform->RotateX(90.0);
  iecToLpsTransform->RotateZ(-90.0);

  // Get RT image IJK to RAS matrix (containing the spacing and the LPS-RAS conversion)
  vtkSmartPointer<vtkMatrix4x4> rtImageIjkToRtImageRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  drrVolumeNode->GetIJKToRASMatrix(rtImageIjkToRtImageRasTransformMatrix);
  vtkSmartPointer<vtkTransform> rtImageIjkToRtImageRasTransform = vtkSmartPointer<vtkTransform>::New();
  rtImageIjkToRtImageRasTransform->SetMatrix(rtImageIjkToRtImageRasTransformMatrix);

  // Concatenate the transform components
  vtkSmartPointer<vtkTransform> isocenterToRtImageRas = vtkSmartPointer<vtkTransform>::New();
  isocenterToRtImageRas->Identity();
  isocenterToRtImageRas->PreMultiply();
  isocenterToRtImageRas->Concatenate(fixedToIsocenterTransform);
  isocenterToRtImageRas->Concatenate(couchToFixedTransform);
  isocenterToRtImageRas->Concatenate(gantryToCouchTransform);
  isocenterToRtImageRas->Concatenate(sourceToGantryTransform);
  isocenterToRtImageRas->Concatenate(rtImageToSourceTransform);
  isocenterToRtImageRas->Concatenate(rtImageCenterToCornerTransform);
  isocenterToRtImageRas->Concatenate(iecToLpsTransform); // LPS = IJK
  isocenterToRtImageRas->Concatenate(rtImageIjkToRtImageRasTransformMatrix);

  // Transform RT image to proper position and orientation
  drrVolumeNode->SetIJKToRASMatrix(isocenterToRtImageRas->GetMatrix());

  // Set up outputs for the planar image display
  vtkSmartPointer<vtkMRMLModelNode> displayedModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  this->GetMRMLScene()->AddNode(displayedModelNode);
  std::string displayedModelNodeName = vtkMRMLPlanarImageNode::PLANARIMAGE_MODEL_NODE_NAME_PREFIX + std::string(drrVolumeNode->GetName());
  displayedModelNode->SetName(displayedModelNodeName.c_str());
  displayedModelNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  // Create PlanarImage parameter set node
  std::string planarImageParameterSetNodeName;
  planarImageParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
    vtkMRMLPlanarImageNode::PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX + std::string(drrVolumeNode->GetName()) );
  vtkSmartPointer<vtkMRMLPlanarImageNode> planarImageParameterSetNode = vtkSmartPointer<vtkMRMLPlanarImageNode>::New();
  planarImageParameterSetNode->SetName(planarImageParameterSetNodeName.c_str());
  this->GetMRMLScene()->AddNode(planarImageParameterSetNode);
  planarImageParameterSetNode->SetAndObserveRtImageVolumeNode(drrVolumeNode);
  planarImageParameterSetNode->SetAndObserveDisplayedModelNode(displayedModelNode);

  // Create planar image model for the RT image
  this->PlanarImageLogic->CreateModelForPlanarImage(planarImageParameterSetNode);

  // Show the displayed planar image model by default
  displayedModelNode->SetDisplayVisibility(1);

  return true;
}

//------------------------------------------------------------------------------
void vtkSlicerPlmDrrLogic::SetPlanarImageLogic(vtkSlicerPlanarImageModuleLogic* planarImageLogic)
{
  this->PlanarImageLogic = planarImageLogic;
}
