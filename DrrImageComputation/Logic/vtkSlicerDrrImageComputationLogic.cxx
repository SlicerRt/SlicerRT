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

// SlicerRT DrrImageComputation Logic includes
#include "vtkSlicerDrrImageComputationLogic.h"

// Slicer includes
#include <vtkSlicerCLIModuleLogic.h>

// SlicerRT PlanarImage Logic includes
#include <vtkSlicerPlanarImageModuleLogic.h>

// Slicer MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLCameraNode.h>

#include <vtkMRMLMarkupsPlaneNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsLineNode.h>

// SlicerRT Beams MRML includes
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTPlanNode.h>

// SlicerRT DrrImageComputation MRML includes
#include <vtkMRMLDrrImageComputationNode.h>

// SlicerRT Beams Logic includes
#include <vtkSlicerBeamsModuleLogic.h>
#include <vtkSlicerIECTransformLogic.h>

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
#include <vtkCamera.h>
#include <vtkMath.h>

// std includes
#include <cmath>

// SlicerRT includes
#include <vtkSlicerRtCommon.h>

const char* vtkSlicerDrrImageComputationLogic::IMAGER_BOUNDARY_MARKUPS_NODE_NAME = "ImagerBoundary"; // plane
const char* vtkSlicerDrrImageComputationLogic::IMAGE_WINDOW_MARKUPS_NODE_NAME = "ImageWindow"; // plane
const char* vtkSlicerDrrImageComputationLogic::FIDUCIALS_MARKUPS_NODE_NAME = "FiducialPoints"; // fiducial
const char* vtkSlicerDrrImageComputationLogic::NORMAL_VECTOR_MARKUPS_NODE_NAME = "NormalVector"; // line
const char* vtkSlicerDrrImageComputationLogic::VUP_VECTOR_MARKUPS_NODE_NAME = "VupVector"; // line

const char* vtkSlicerDrrImageComputationLogic::RTIMAGE_TRANSFORM_NODE_NAME = "DrrImageComputationTransform";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDrrImageComputationLogic);

//----------------------------------------------------------------------------
vtkSlicerDrrImageComputationLogic::vtkSlicerDrrImageComputationLogic()
  :
  PlanarImageLogic(nullptr),
  PlastimatchDRRComputationLogic(nullptr),
  BeamsLogic(nullptr)
{
}

//----------------------------------------------------------------------------
vtkSlicerDrrImageComputationLogic::~vtkSlicerDrrImageComputationLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
//  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
//  if (!scene->IsNodeClassRegistered("vtkMRMLRTPlanNode"))
//  {
//    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
//  }
//  if (!scene->IsNodeClassRegistered("vtkMRMLRTBeamNode"))
//  {
//    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
//  }
  if (!scene->IsNodeClassRegistered("vtkMRMLDrrImageComputationNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDrrImageComputationNode>::New());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::UpdateFromMRMLScene()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
    return;
  }

//  if (node->IsA("vtkMRMLRTBeamNode"))
//  {
//    // Observe beam events
//    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
//    events->InsertNextValue(vtkMRMLRTBeamNode::BeamGeometryModified);
//    events->InsertNextValue(vtkMRMLRTBeamNode::BeamTransformModified);
//    vtkObserveMRMLNodeEventsMacro(node, events);
//  }
  if (node->IsA("vtkMRMLDrrImageComputationNode"))
  {
    vtkNew<vtkIntArray> events;
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndImport: Invalid MRML scene");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLNodesEvents(caller, event, callData);

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

//  if (caller->IsA("vtkMRMLRTBeamNode"))
//  {
//    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(caller);
//    if (event == vtkMRMLRTBeamNode::BeamTransformModified)
//    {
//    }
//    else if (event == vtkMRMLRTBeamNode::BeamGeometryModified)
//    {
//    }
//  }
  if (caller->IsA("vtkMRMLDrrImageComputationNode"))
  {
    vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(caller);

    if (event == vtkCommand::ModifiedEvent)
    {
      // Update parameters using beam node data and create/update markups transform if they weren't created/updated
      this->UpdateNormalAndVupVectors(parameterNode);
      this->CreateMarkupsNodes(parameterNode); // create markups or update markups transform
      this->UpdateMarkupsNodes(parameterNode); // update markups geometry
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::CreateMarkupsNodes(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateMarkupsNodes: Invalid MRML scene");
    return;
  }

  if (!parameterNode)
  {
    vtkErrorMacro("CreateMarkupsNodes: Invalid parameter node");
    return;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();
  vtkMRMLTransformNode* transformNode = nullptr;
  if (beamNode)
  {
    transformNode = this->UpdateImageTransformFromBeam(beamNode);
  }

  // Create markups nodes if they don't exist

  // Imager boundary markups node
  vtkSmartPointer<vtkMRMLMarkupsPlaneNode> imagerMarkupsNode;
  if (!scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME))
  {
    imagerMarkupsNode = this->CreateImagerBoundary(parameterNode);
  }
  else
  {
    imagerMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME));
    // Update imager points using DrrImageComputation node data
    if (transformNode)
    {
      imagerMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // Image window markups node
  vtkSmartPointer<vtkMRMLMarkupsPlaneNode> imageWindowMarkupsNode;
  if (!scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME))
  {
    imageWindowMarkupsNode = this->CreateImageWindow(parameterNode);
  }
  else
  {
    imageWindowMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME));
    // Update image window points using DrrImageComputation node data
    if (transformNode)
    {
      imageWindowMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // Imager normal vector markups node
  vtkSmartPointer<vtkMRMLMarkupsLineNode> normalVectorMarkupsNode;
  if (!scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME))
  {
    normalVectorMarkupsNode = this->CreateImagerNormal(parameterNode);
  }
  else
  {
    normalVectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME));
    // Update Normal vector points using DrrImageComputation node data
    if (transformNode)
    {
      normalVectorMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // Imager vup vector markups node
  vtkSmartPointer<vtkMRMLMarkupsLineNode> vupVectorMarkupsNode;
  if (!scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME))
  {
    vupVectorMarkupsNode = this->CreateImagerVUP(parameterNode);
  }
  else
  {
    vupVectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME));
    // Update VUP vector points using DrrImageComputation node data
    if (transformNode)
    {
      vupVectorMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // Fiducial markups node
  vtkSmartPointer<vtkMRMLMarkupsFiducialNode> pointsMarkupsNode;
  if (!scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME))
  {
    pointsMarkupsNode = this->CreateFiducials(parameterNode);
  }
  else
  {
    pointsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME));
    // Update fiducial points using DrrImageComputation node data
    if (transformNode)
    {
      pointsMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::UpdateMarkupsNodes(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid MRML scene");
    return;
  }

  if (!parameterNode)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid parameter node");
    return;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();
  if (!beamNode)
  {
    vtkErrorMacro("UpdateMarkupsNodes: Invalid beam node");
    return;
  }

  vtkMRMLTransformNode* transformNode = this->UpdateImageTransformFromBeam(beamNode);

  double distance = parameterNode->GetIsocenterImagerDistance();
    
  double spacing[2] = {};
  parameterNode->GetImagerSpacing(spacing);

  int resolution[2] = {};
  parameterNode->GetImagerResolution(resolution);

  double offset[2] = {};
  parameterNode->GetImagerCenterOffset(offset);

  int imageWindow[4] = {};
  parameterNode->GetImageWindow(imageWindow);

  double imagerHalfWidth = spacing[0] * resolution[0] / 2.; // columns
  double imagerHalfHeight = spacing[1] * resolution[1] / 2.; // rows
  double& x = imagerHalfWidth;
  double& y = imagerHalfHeight;

  // Imager boundary markups node
  if (scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsPlaneNode* imagerMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME));

    // update imager plane node
    double originWorld[3] = { offset[0], offset[1], -1. * distance };

    imagerMarkupsNode->SetOrigin(originWorld);
    imagerMarkupsNode->SetPlaneBounds( -1. * y, y, -1. * x, x);
    imagerMarkupsNode->SetSize( 2. * y, 2. * x);

    // Update imager boundary markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = imagerMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode)
    {
      imagerMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // Image window markups node
  if (scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsPlaneNode* imageWindowMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME));

    imageWindowMarkupsNode->SetDisplayVisibility(parameterNode->GetImageWindowFlag());

    // Imager upper left point
    vtkVector3d imagerP0( -1. * y + offset[0], -1. * x + offset[1], -distance);

    double r1 = imagerP0.GetX() + imageWindow[1] * spacing[1];
    double c1 = imagerP0.GetY() + imageWindow[0] * spacing[0];
    double r2 = imagerP0.GetX() + imageWindow[3] * spacing[1];
    double c2 = imagerP0.GetY() + imageWindow[2] * spacing[0];

    // update image window plane node
    double originWorld[3] = { r1 + (r2 - r1) / 2., c1 + (c2 - c1) / 2., -1. * distance };

    imageWindowMarkupsNode->SetOrigin(originWorld);
    imageWindowMarkupsNode->SetPlaneBounds( -1. * (r2 - r1) / 2., (r2 - r1) / 2., -1. * (c2 - c1) / 2., (c2 - c1) / 2.);
    imageWindowMarkupsNode->SetSize( r2 - r1, c2 - c1);

    // Update image window markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = imageWindowMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode)
    {
      imageWindowMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // normal vector markups line node
  if (scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(NORMAL_VECTOR_MARKUPS_NODE_NAME));

    // update points
    vtkVector3d p0( offset[0], offset[1], -distance);
    vtkVector3d p1( offset[0], offset[1], -distance + 100.);

    double* p = vectorMarkupsNode->GetNthControlPointPosition(0);
    if (p)
    {
      vectorMarkupsNode->SetNthControlPointPosition( 0, p0.GetX(), p0.GetY(), p0.GetZ());
    }
    else
    {
      vectorMarkupsNode->AddControlPoint(p0);
    }
    
    p = vectorMarkupsNode->GetNthControlPointPosition(1);
    if (p)
    {
      vectorMarkupsNode->SetNthControlPointPosition( 1, p1.GetX(), p1.GetY(), p1.GetZ());
    }
    else
    {
      vectorMarkupsNode->AddControlPoint(p1);
    }

    // Update imager normal vector markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = vectorMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode)
    {
      vectorMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // vup vector markups line node
  if (scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
      scene->GetFirstNodeByName(VUP_VECTOR_MARKUPS_NODE_NAME));

    // update points
    vtkVector3d p0( offset[0], offset[1], -distance);
    vtkVector3d p1( -1. * y + offset[0], 0. + offset[1], -distance); // vup

    double* p = vectorMarkupsNode->GetNthControlPointPosition(0);
    if (p)
    {
      vectorMarkupsNode->SetNthControlPointPosition( 0, p0.GetX(), p0.GetY(), p0.GetZ());
    }
    else
    {
      vectorMarkupsNode->AddControlPoint(p0);
    }
    
    p = vectorMarkupsNode->GetNthControlPointPosition(1);
    if (p)
    {
      vectorMarkupsNode->SetNthControlPointPosition( 1, p1.GetX(), p1.GetY(), p1.GetZ());
    }
    else
    {
      vectorMarkupsNode->AddControlPoint(p1);
    }

    // Update VUP VECTOR markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = vectorMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode)
    {
      vectorMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // fiducial markups line node
  if (scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsFiducialNode* pointsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      scene->GetFirstNodeByName(FIDUCIALS_MARKUPS_NODE_NAME));

    // update points
    vtkVector3d p0( 0., 0., -distance); // imager center
    vtkVector3d p1( 0., 0., -distance + 100.); // n
    vtkVector3d p2( -1. * y + offset[0], -1. * x + offset[1], -distance); // (0,0)
    vtkVector3d p3( -1. * y + offset[0], 0.0, -distance); // vup

    double* p = pointsMarkupsNode->GetNthControlPointPosition(0);
    if (p)
    {
      pointsMarkupsNode->SetNthControlPointPosition( 0, p0.GetX(), p0.GetY(), p0.GetZ());
    }
    else
    {
      pointsMarkupsNode->AddControlPoint(p0);
    }
    
    p = pointsMarkupsNode->GetNthControlPointPosition(1);
    if (p)
    {
      pointsMarkupsNode->SetNthControlPointPosition( 1, p1.GetX(), p1.GetY(), p1.GetZ());
    }
    else
    {
      pointsMarkupsNode->AddControlPoint(p1);
    }
    
    p = pointsMarkupsNode->GetNthControlPointPosition(2);
    if (p)
    {
      pointsMarkupsNode->SetNthControlPointPosition( 2, p2.GetX(), p2.GetY(), p2.GetZ());
    }
    else
    {
      pointsMarkupsNode->AddControlPoint(p2);
    }
    
    p = pointsMarkupsNode->GetNthControlPointPosition(3);
    if (p)
    {
      pointsMarkupsNode->SetNthControlPointPosition( 3, p3.GetX(), p3.GetY(), p3.GetZ());
    }
    else
    {
      pointsMarkupsNode->AddControlPoint(p3);
    }

    // Update fiducials markups transform node if it's changed    
    vtkMRMLTransformNode* markupsTransformNode = pointsMarkupsNode->GetParentTransformNode();

    if (markupsTransformNode)
    {
      pointsMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

}

//----------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::ShowMarkupsNodes(bool toggled)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("ShowMarkupsNodes: Invalid MRML scene");
    return;
  }

  // Imager boundary markups node
  if (scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsPlaneNode* imagerMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
      scene->GetFirstNodeByName(IMAGER_BOUNDARY_MARKUPS_NODE_NAME));
    imagerMarkupsNode->SetDisplayVisibility(int(toggled));
  }

  // Image window markups node
  if (scene->GetFirstNodeByName(IMAGE_WINDOW_MARKUPS_NODE_NAME))
  {
    vtkMRMLMarkupsPlaneNode* imageWindowMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
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
vtkMRMLMarkupsPlaneNode* vtkSlicerDrrImageComputationLogic::CreateImagerBoundary(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateImagerBoundary: Invalid MRML scene");
    return nullptr;
  }

  vtkMRMLMarkupsPlaneNode* imagerMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
    scene->AddNewNodeByClass( "vtkMRMLMarkupsPlaneNode", IMAGER_BOUNDARY_MARKUPS_NODE_NAME));

  if (!imagerMarkupsNode)
  {
    vtkErrorMacro("CreateImagerBoundary: Invalid imager plane node");
    return nullptr;
  }
  imagerMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("RTIMAGE_") + IMAGER_BOUNDARY_MARKUPS_NODE_NAME;
  imagerMarkupsNode->SetSingletonTag(singletonTag.c_str());
  imagerMarkupsNode->LockedOn();

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImagerSpacing(spacing);

    int resolution[2] = {};
    parameterNode->GetImagerResolution(resolution);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double imagerHalfWidth = spacing[0] * resolution[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * resolution[1] / 2.; // rows

    double& x = imagerHalfWidth;
    double& y = imagerHalfHeight;

    // add points from markups fiducial node
    double originWorld[3] = { offset[0], offset[1], -1. * distance };

    imagerMarkupsNode->SetOrigin(originWorld);
    imagerMarkupsNode->SetPlaneBounds( -1. * y, y, -1. * x, x);
    imagerMarkupsNode->SetSize( 2. * y, 2. * x);
    imagerMarkupsNode->SetNormal( 0., 0., 1.);
    imagerMarkupsNode->SetSizeMode(vtkMRMLMarkupsPlaneNode::SizeModeAuto);
    imagerMarkupsNode->SetPlaneType(vtkMRMLMarkupsPlaneNode::PlaneTypePointNormal);

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* transformNode = this->UpdateImageTransformFromBeam(beamNode);

      if (transformNode)
      {
        imagerMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }
  return imagerMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsPlaneNode* vtkSlicerDrrImageComputationLogic::CreateImageWindow(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateImageWindow: Invalid MRML scene");
    return nullptr;
  }

  vtkMRMLMarkupsPlaneNode* imageWindowMarkupsNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
    scene->AddNewNodeByClass( "vtkMRMLMarkupsPlaneNode", IMAGE_WINDOW_MARKUPS_NODE_NAME));

  if (!imageWindowMarkupsNode)
  {
    vtkErrorMacro("CreateImageWindow: Invalid image window plane node");
    return nullptr;
  }
  imageWindowMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("RTIMAGE_") + IMAGE_WINDOW_MARKUPS_NODE_NAME;
  imageWindowMarkupsNode->SetSingletonTag(singletonTag.c_str());
  imageWindowMarkupsNode->LockedOn();

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImagerSpacing(spacing);

    int resolution[2] = {};
    parameterNode->GetImagerResolution(resolution);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    int imageWindow[4] = {};
    parameterNode->GetImageWindow(imageWindow);

    double imagerHalfWidth = spacing[0] * resolution[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * resolution[1] / 2.; // rows

    double& x = imagerHalfWidth;
    double& y = imagerHalfHeight;

    // Imager upper left point
    vtkVector3d imagerP0( -1. * y + offset[0], -1. * x + offset[1], -distance);

    double r1 = imagerP0.GetX() + imageWindow[1] * spacing[1];
    double c1 = imagerP0.GetY() + imageWindow[0] * spacing[0];
    double r2 = imagerP0.GetX() + imageWindow[3] * spacing[1];
    double c2 = imagerP0.GetY() + imageWindow[2] * spacing[0];

    // add points from markups fiducial node
    double originWorld[3] = { r1 + (r2 - r1) / 2., c1 + (c2 - c1) / 2., -1. * distance };

    imageWindowMarkupsNode->SetOrigin(originWorld);
    imageWindowMarkupsNode->SetPlaneBounds( -1. * (r2 - r1) / 2., (r2 - r1) / 2., -1. * (c2 - c1) / 2., (c2 - c1) / 2.);
    imageWindowMarkupsNode->SetSize( r2 - r1, c2 - c1);
    imageWindowMarkupsNode->SetNormal( 0., 0., 1.);
    imageWindowMarkupsNode->SetSizeMode(vtkMRMLMarkupsPlaneNode::SizeModeAuto);
    imageWindowMarkupsNode->SetPlaneType(vtkMRMLMarkupsPlaneNode::PlaneTypePointNormal);

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* transformNode = this->UpdateImageTransformFromBeam(beamNode);

      if (transformNode)
      {
        imageWindowMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }
  return imageWindowMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsLineNode* vtkSlicerDrrImageComputationLogic::CreateImagerNormal(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateImagerNormal: Invalid MRML scene");
    return nullptr;
  }

  vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
    scene->AddNewNodeByClass( "vtkMRMLMarkupsLineNode", NORMAL_VECTOR_MARKUPS_NODE_NAME));

  if (!vectorMarkupsNode)
  {
    vtkErrorMacro("CreateImagerNormal: Invalid normal line node");
    return nullptr;
  }
  vectorMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("RTIMAGE_") + NORMAL_VECTOR_MARKUPS_NODE_NAME;
  vectorMarkupsNode->SetSingletonTag(singletonTag.c_str());
  vectorMarkupsNode->LockedOn();

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
      vtkMRMLTransformNode* transformNode = this->UpdateImageTransformFromBeam(beamNode);

      if (transformNode)
      {
        vectorMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }
  return vectorMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsLineNode* vtkSlicerDrrImageComputationLogic::CreateImagerVUP(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateImagerVUP: Invalid MRML scene");
    return nullptr;
  }

  vtkMRMLMarkupsLineNode* vectorMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast(
    scene->AddNewNodeByClass( "vtkMRMLMarkupsLineNode", VUP_VECTOR_MARKUPS_NODE_NAME));

  if (!vectorMarkupsNode)
  {
    vtkErrorMacro("CreateImagerVUP: Invalid view-up line node");
    return nullptr;
  }
  vectorMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("RTIMAGE_") + VUP_VECTOR_MARKUPS_NODE_NAME;
  vectorMarkupsNode->SetSingletonTag(singletonTag.c_str());
  vectorMarkupsNode->LockedOn();

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImagerSpacing(spacing);

    int resolution[2] = {};
    parameterNode->GetImagerResolution(resolution);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double imagerHalfHeight = spacing[1] * resolution[1] / 2.; // rows
    double& y = imagerHalfHeight;

    // add points
    vtkVector3d p0( 0. + offset[0], 0. + offset[1], -distance);
    vtkVector3d p1( -1. * y + offset[0], 0. + offset[1], -distance); // vup

    vectorMarkupsNode->AddControlPoint(p0);
    vectorMarkupsNode->AddControlPoint(p1);

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* transformNode = this->UpdateImageTransformFromBeam(beamNode);

      if (transformNode)
      {
        vectorMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }
  return vectorMarkupsNode;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkSlicerDrrImageComputationLogic::CreateFiducials(vtkMRMLDrrImageComputationNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("CreateFiducials: Invalid MRML scene");
    return nullptr;
  }

  vtkMRMLMarkupsFiducialNode* pointsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    scene->AddNewNodeByClass( "vtkMRMLMarkupsFiducialNode", FIDUCIALS_MARKUPS_NODE_NAME));

  if (!pointsMarkupsNode)
  {
    vtkErrorMacro("CreateFiducials: Invalid fiducials node");
    return nullptr;
  }
  pointsMarkupsNode->SetHideFromEditors(1);
  std::string singletonTag = std::string("RTIMAGE_") + FIDUCIALS_MARKUPS_NODE_NAME;
  pointsMarkupsNode->SetSingletonTag(singletonTag.c_str());
  pointsMarkupsNode->LockedOn();

  if (parameterNode)
  {
    double distance = parameterNode->GetIsocenterImagerDistance();
     
    double spacing[2] = {};
    parameterNode->GetImagerSpacing(spacing);

    int resolution[2] = {};
    parameterNode->GetImagerResolution(resolution);

    double offset[2] = {};
    parameterNode->GetImagerCenterOffset(offset);

    double imagerHalfWidth = spacing[0] * resolution[0] / 2.; // columns
    double imagerHalfHeight = spacing[1] * resolution[1] / 2.; // rows

    double& x = imagerHalfWidth;
    double& y = imagerHalfHeight;

    // add points
    vtkVector3d p0( 0., 0., -distance); // imager center
    vtkVector3d p1( 0., 0., -distance + 100.); // n
    vtkVector3d p2( -1. * y + offset[0], -1. * x + offset[1], -distance); // (0,0)
    vtkVector3d p3( -1. * y + offset[0], 0.0, -distance); // vup

    pointsMarkupsNode->AddControlPoint( p0, "Imager center");
    pointsMarkupsNode->AddControlPoint( p1, "n");
    pointsMarkupsNode->AddControlPoint( p2, "(0,0)");
    pointsMarkupsNode->AddControlPoint( p3, "VUP");

    if (vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode())
    {
      vtkMRMLTransformNode* transformNode = this->UpdateImageTransformFromBeam(beamNode);

      if (transformNode)
      {
        pointsMarkupsNode->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }
  return pointsMarkupsNode;
}

//------------------------------------------------------------------------------
bool vtkSlicerDrrImageComputationLogic::ComputePlastimatchDRR( vtkMRMLDrrImageComputationNode* parameterNode, 
  vtkMRMLScalarVolumeNode* ctVolumeNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("ComputePlastimatchDRR: Invalid MRML scene");
    return false;
  }

  if (!parameterNode)
  {
    vtkErrorMacro("ComputePlastimatchDRR: Invalid parameter node");
    return false;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();
  if (!beamNode)
  {
    vtkErrorMacro("ComputePlastimatchDRR: Invalid RT Beam node");
    return false;
  }

  if (!ctVolumeNode)
  {
    vtkErrorMacro("ComputePlastimatchDRR: Invalid input CT volume node");
    return false;
  }

  if (!this->PlastimatchDRRComputationLogic)
  {
    vtkErrorMacro("ComputePlastimatchDRR: slicer_plastimatch_drr logic is not set");
    return false;
  }

  vtkMRMLCommandLineModuleNode* cmdNode = this->PlastimatchDRRComputationLogic->CreateNodeInScene();
  if (!cmdNode)
  {
    vtkErrorMacro("ComputePlastimatchDRR: failed to create CLI module node");
    return false;
  }

  // Create node for the DRR image volume
  vtkNew<vtkMRMLScalarVolumeNode> drrVolumeNode;
  scene->AddNode(drrVolumeNode);

  cmdNode->SetParameterAsNode( "inputVolume", ctVolumeNode);
  cmdNode->SetParameterAsNode( "outputVolume", drrVolumeNode);
  cmdNode->SetParameterAsDouble( "sourceAxisDistance", beamNode->GetSAD());
  cmdNode->SetParameterAsDouble( "sourceImagerDistance", beamNode->GetSAD() + parameterNode->GetIsocenterImagerDistance());

  // Fill CLI cmd node data
  std::stringstream vupStream;
  double vup[3] = { -1., 0., 0. };
  parameterNode->GetViewUpVector(vup);
  vupStream << vup[0] << "," << vup[1] << "," << vup[2];
  cmdNode->SetParameterAsString( "viewUpVector", vupStream.str());

  std::stringstream normalStream;
  double n[3] = { 0., 0., 1. };
  parameterNode->GetNormalVector(n);
  normalStream << n[0] << "," << n[1] << "," << n[2];
  cmdNode->SetParameterAsString( "normalVector", normalStream.str());

  std::stringstream isocenterStream;
  double isocenter[3] = {};
  parameterNode->GetIsocenterPositionLPS(isocenter);
  isocenterStream << isocenter[0] << "," << isocenter[1] << "," << isocenter[2];
  cmdNode->SetParameterAsString( "isocenterPosition", isocenterStream.str());
  
  std::stringstream imagerResolutionStream;
  int imagerResolution[2] = { 1024, 768 };
  parameterNode->GetImagerResolution(imagerResolution);
  imagerResolutionStream << imagerResolution[0] << "," << imagerResolution[1];
  cmdNode->SetParameterAsString( "imagerResolution", imagerResolutionStream.str());

  std::stringstream imagerSpacingStream;
  double imagerSpacing[2] = { 0.25, 0.25 };
  parameterNode->GetImagerSpacing(imagerSpacing);
  imagerSpacingStream << imagerSpacing[0] << "," << imagerSpacing[1];
  cmdNode->SetParameterAsString( "imagerSpacing", imagerSpacingStream.str());

  cmdNode->SetParameterAsBool( "useImageWindow", parameterNode->GetImageWindowFlag());
  if (parameterNode->GetImageWindowFlag())
  {
    std::stringstream imageWindowStream;
    int imageWindow[4] = { 0, 0, 1023, 767 };
    parameterNode->GetImageWindow(imageWindow);
    imageWindowStream << imageWindow[0] << "," << imageWindow[1] << "," << imageWindow[2] << "," << imageWindow[3];
    cmdNode->SetParameterAsString( "imageWindow", imageWindowStream.str());
  }

  cmdNode->SetParameterAsBool( "autoscale", parameterNode->GetAutoscaleFlag());
  
  std::stringstream autoscaleRangeStream;
  float autoscaleRange[2] = { 0., 255. };
  parameterNode->GetAutoscaleRange(autoscaleRange);
  autoscaleRangeStream << autoscaleRange[0] << "," << autoscaleRange[1];
  cmdNode->SetParameterAsString( "autoscaleRange", autoscaleRangeStream.str());

  cmdNode->SetParameterAsBool( "exponentialMapping", parameterNode->GetExponentialMappingFlag());
  cmdNode->SetParameterAsInt( "thresholdBelow", parameterNode->GetHUThresholdBelow());
  
  std::string threadingString = "cpu";
  switch (parameterNode->GetThreading())
  {
  case vtkMRMLDrrImageComputationNode::CPU:
    threadingString = "cpu";
    break;
  case vtkMRMLDrrImageComputationNode::CUDA:
    threadingString = "cuda";
    break;
  case vtkMRMLDrrImageComputationNode::OpenCL:
    threadingString = "opencl";
    break;
  default:
    break;
  }
  cmdNode->SetParameterAsString( "threading", threadingString);

  std::string huconversionString = "preprocess";
  switch (parameterNode->GetHUConversion())
  {
  case vtkMRMLDrrImageComputationNode::Inline:
    huconversionString = "inline";
    break;
  case vtkMRMLDrrImageComputationNode::Preprocess:
    huconversionString = "preprocess";
    break;
  case vtkMRMLDrrImageComputationNode::None:
    huconversionString = "none";
    break;
  default:
    break;
  }
  cmdNode->SetParameterAsString( "huconversion", huconversionString);

  std::string algorithmString = "exact";
  switch (parameterNode->GetAlgorithmReconstuction())
  {
  case vtkMRMLDrrImageComputationNode::Exact:
    algorithmString = "exact";
    break;
  case vtkMRMLDrrImageComputationNode::Uniform:
    algorithmString = "uniform";
    break;
  default:
    break;
  }
  cmdNode->SetParameterAsString( "algorithm", algorithmString);
  cmdNode->SetParameterAsBool( "invertIntensity", parameterNode->GetInvertIntensityFlag());
  cmdNode->SetParameterAsString( "outputFormat", "raw");

  this->PlastimatchDRRComputationLogic->ApplyAndWait( cmdNode, true);

  scene->RemoveNode(cmdNode);
  // TODO: Add results checking ( image size is valid, and computation didn't crash )

  bool res = false;
  if (drrVolumeNode->GetImageData() && drrVolumeNode->GetSpacing())
  {
    // Set more user friendly DRR image name
    std::string drrName = scene->GenerateUniqueName(std::string("DRR : ") + std::string(beamNode->GetName()));
    drrVolumeNode->SetName(drrName.c_str());

    // Create parameter node name, and observe calculated drr volume
    std::string parameterSetNodeName;
    parameterSetNodeName = vtkMRMLPlanarImageNode::PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX + drrName;
    parameterNode->SetName(parameterSetNodeName.c_str());
    parameterNode->SetAndObserveRtImageVolumeNode(drrVolumeNode);

    res = this->SetupDisplayAndSubjectHierarchyNodes( parameterNode, drrVolumeNode);
  }
  return res;
}

//------------------------------------------------------------------------------
bool vtkSlicerDrrImageComputationLogic::SetupDisplayAndSubjectHierarchyNodes( vtkMRMLDrrImageComputationNode* parameterNode, 
  vtkMRMLScalarVolumeNode* drrVolumeNode)
{
  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("SetupDisplayAndSubjectHierarchyNodes: Failed to access subject hierarchy node");
    return false;
  }

  // Create display node for the volume
  vtkNew<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode;
  this->GetMRMLScene()->AddNode(volumeDisplayNode);
  volumeDisplayNode->SetDefaultColorMap();

  float autoscaleRange[2] = { 0.f, 255.f };
  parameterNode->GetAutoscaleRange(autoscaleRange);
  
  // TODO: add manual level setting
  volumeDisplayNode->AutoWindowLevelOn();

  drrVolumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

  // Set up subject hierarchy item
  vtkIdType rtImageVolumeShItemID = shNode->CreateItem( shNode->GetSceneItemID(), drrVolumeNode);

  double sid = beamNode->GetSAD() + parameterNode->GetIsocenterImagerDistance();
  // Set RT image specific attributes
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME, "1");
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName(), "");
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME, std::to_string(beamNode->GetSAD()));
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME, std::to_string(beamNode->GetGantryAngle()));
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME, std::to_string(beamNode->GetCouchAngle()));
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME, std::to_string(beamNode->GetCollimatorAngle()));
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME, std::to_string(beamNode->GetBeamNumber()));
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME, std::to_string(sid));

  double rtImagePosition[2] = {};
  parameterNode->GetRTImagePosition(rtImagePosition);
  std::string rtImagePositionString = std::to_string(rtImagePosition[0]) + std::string(" ") + std::to_string(rtImagePosition[1]);
  shNode->SetItemAttribute(rtImageVolumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME, rtImagePositionString);

  // Compute and set RT image geometry. Uses the referenced beam 
  return this->SetupGeometry( parameterNode, drrVolumeNode);
}

//------------------------------------------------------------------------------
bool vtkSlicerDrrImageComputationLogic::SetupGeometry( vtkMRMLDrrImageComputationNode* parameterNode, vtkMRMLScalarVolumeNode* drrVolumeNode)
{
  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());

  // Get RT plan for beam
  vtkMRMLRTPlanNode *planNode = beamNode->GetParentPlanNode();
  if (!planNode)
  {
    vtkErrorMacro("SetupGeometry: Failed to retrieve valid plan node for beam '" << beamNode->GetName() << "'");
    return false;
  }
  vtkIdType planShItemID = planNode->GetPlanSubjectHierarchyItemID();
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkErrorMacro("SetupGeometry: Failed to retrieve valid plan subject hierarchy item for beam '" << beamNode->GetName() << "'");
    return false;
  }
  std::string rtPlanSopInstanceUid = shNode->GetItemUID(planShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
  if (rtPlanSopInstanceUid.empty())
  {
    vtkWarningMacro("SetupGeometry: Failed to get RT Plan DICOM UID for beam '" << beamNode->GetName() << "'");
  }

  // Return if a referenced displayed model is present for the RT image, because it means that the geometry has been set up successfully before
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
    drrVolumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
  if (modelNode)
  {
    vtkWarningMacro("SetupGeometry: RT image '" << drrVolumeNode->GetName() << "' belonging to beam '" << beamNode->GetName() << "' seems to have been set up already.");
    return false;
  }

  double gantryAngle = beamNode->GetGantryAngle();
  double couchAngle = beamNode->GetCouchAngle();

  // RT image position (the x and y coordinates (in mm) of the upper left hand corner of the image, in the IEC X-RAY IMAGE RECEPTOR coordinate system)
  double rtImagePosition[2] = {};
  parameterNode->GetRTImagePosition(rtImagePosition);

  // Get isocenter coordinates
  double isocenterWorldCoordinates[3] = {};
  if (!beamNode->GetPlanIsocenterPosition(isocenterWorldCoordinates))
  {
    vtkErrorMacro("SetupGeometry: Failed to get plan isocenter position");
    return false;
  }

  // Assemble transform from isocenter IEC to RT image RAS
  vtkNew<vtkTransform> fixedToIsocenterTransform;
  fixedToIsocenterTransform->Identity();
  fixedToIsocenterTransform->Translate(isocenterWorldCoordinates);

  vtkNew<vtkTransform> couchToFixedTransform;
  couchToFixedTransform->Identity();
  couchToFixedTransform->RotateWXYZ(-1. * couchAngle, 0.0, 1.0, 0.0);

  vtkNew<vtkTransform> gantryToCouchTransform;
  gantryToCouchTransform->Identity();
  gantryToCouchTransform->RotateWXYZ(gantryAngle, 0.0, 0.0, 1.0);

  vtkNew<vtkTransform> rtImageCenterToGantryTransform;
  rtImageCenterToGantryTransform->Identity();
  rtImageCenterToGantryTransform->Translate(0.0, -1. * parameterNode->GetIsocenterImagerDistance(), 0.0);

  vtkNew<vtkTransform> rtImageCenterToCornerTransform;
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
  vtkNew<vtkTransform> iecToLpsTransform;
  iecToLpsTransform->Identity();
  iecToLpsTransform->RotateX(90.0);
  iecToLpsTransform->RotateZ(-90.0);

  // Get RT image IJK to RAS matrix (containing the spacing and the LPS-RAS conversion)
  vtkNew<vtkMatrix4x4> rtImageIjkToRtImageRasTransformMatrix;
  drrVolumeNode->GetIJKToRASMatrix(rtImageIjkToRtImageRasTransformMatrix);
  vtkNew<vtkTransform> rtImageIjkToRtImageRasTransform;
  rtImageIjkToRtImageRasTransform->SetMatrix(rtImageIjkToRtImageRasTransformMatrix);

  // Concatenate the transform components
  vtkNew<vtkTransform> isocenterToRtImageRas;
  isocenterToRtImageRas->Identity();
  isocenterToRtImageRas->PreMultiply();
  isocenterToRtImageRas->Concatenate(fixedToIsocenterTransform);
  isocenterToRtImageRas->Concatenate(couchToFixedTransform);
  isocenterToRtImageRas->Concatenate(gantryToCouchTransform);
  isocenterToRtImageRas->Concatenate(rtImageCenterToGantryTransform);
  isocenterToRtImageRas->Concatenate(rtImageCenterToCornerTransform);
  isocenterToRtImageRas->Concatenate(iecToLpsTransform); // LPS = IJK
  isocenterToRtImageRas->Concatenate(rtImageIjkToRtImageRasTransformMatrix);

  // Transform RT image to proper position and orientation
  drrVolumeNode->SetIJKToRASMatrix(isocenterToRtImageRas->GetMatrix());

  // Set up outputs for the planar image display
  vtkNew<vtkMRMLModelNode> displayedModelNode;
  this->GetMRMLScene()->AddNode(displayedModelNode);
  std::string displayedModelNodeName = vtkMRMLPlanarImageNode::PLANARIMAGE_MODEL_NODE_NAME_PREFIX + std::string(drrVolumeNode->GetName());
  displayedModelNode->SetName(displayedModelNodeName.c_str());
  displayedModelNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");
  parameterNode->SetAndObserveDisplayedModelNode(displayedModelNode);

  // Create planar image model for the RT Image
  this->PlanarImageLogic->CreateModelForPlanarImage(parameterNode);

  // Show the displayed planar image model by default
  displayedModelNode->SetDisplayVisibility(1);

  return true;
}

//------------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerDrrImageComputationLogic::UpdateImageTransformFromBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateImageTransformFromBeam: Invalid beam node");
    return nullptr;
  }
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateImageTransformFromBeam: Invalid MRML scene");
    return nullptr;
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode;
  if (!scene->GetFirstNodeByName(RTIMAGE_TRANSFORM_NODE_NAME))
  {
    transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    transformNode->SetName(RTIMAGE_TRANSFORM_NODE_NAME);
    transformNode->SetHideFromEditors(1);
    transformNode->SetSingletonTag("RTIMAGE_Transform");
    scene->AddNode(transformNode);
  }
  else
  {
    transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(RTIMAGE_TRANSFORM_NODE_NAME));
  }

  vtkNew<vtkSlicerIECTransformLogic> iecLogic;
  iecLogic->SetMRMLScene(scene);

  // Update transforms in IEC logic from beam node parameters
  iecLogic->UpdateIECTransformsFromBeam(beamNode);

  // Dynamic transform from Gantry to RAS
  // Transformation path:
  // Gantry -> FixedReference -> PatientSupport -> TableTopEccentricRotation -> TableTop -> Patient -> RAS
  using IEC = vtkSlicerIECTransformLogic::CoordinateSystemIdentifier;
  vtkNew<vtkGeneralTransform> generalTransform;
  if (iecLogic->GetTransformBetween( IEC::Gantry, IEC::RAS, generalTransform))
  {
    // Convert general transform to linear
    // This call also makes hard copy of the transform so that it doesn't change when other beam transforms change
    vtkNew<vtkTransform> linearTransform;
    if (!vtkMRMLTransformNode::IsGeneralTransformLinear(generalTransform, linearTransform))
    {
      vtkErrorMacro("UpdateImageTransformFromBeam: Unable to set transform with non-linear components to beam " << beamNode->GetName());
      return nullptr;
    }
    // Set transform to node
    transformNode->SetAndObserveTransformToParent(linearTransform);
  }
  return transformNode;
}

//------------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::UpdateNormalAndVupVectors(vtkMRMLDrrImageComputationNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateNormalAndVupVectors: Parameter node is invalid");
    return;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();

  if (!beamNode)
  {
    vtkErrorMacro("UpdateNormalAndVupVectors: RT Beam node is invalid");
    return;
  }

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateNormalAndVupVectors: Invalid MRML scene");
    return;
  }

  vtkMRMLTransformNode* beamTransformNode = nullptr;
  if (vtkMRMLNode* node = scene->GetFirstNodeByName(RTIMAGE_TRANSFORM_NODE_NAME))
  {
    beamTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
  }

  vtkTransform* beamTransform = nullptr;
  vtkNew<vtkMatrix4x4> mat; // DICOM beam transform matrix
  mat->Identity();

  if (beamTransformNode)
  {
    beamTransform = vtkTransform::SafeDownCast(beamTransformNode->GetTransformToParent());

    vtkNew<vtkTransform> rasToLpsTransform;
    rasToLpsTransform->Identity();
    rasToLpsTransform->RotateZ(180.0);
    
    vtkNew<vtkTransform> dicomBeamTransform;
    dicomBeamTransform->Identity();
    dicomBeamTransform->PreMultiply();
    dicomBeamTransform->Concatenate(rasToLpsTransform);
    dicomBeamTransform->Concatenate(beamTransform);

    dicomBeamTransform->GetMatrix(mat);
  }
  else
  {
    vtkWarningMacro("UpdateNormalAndVupVectors: Beam transform node is invalid, identity matrix will be used instead");
  }

  double n[4], vup[4];
  const double normalVector[4] = { 0., 0., 1., 0. }; // beam positive Z-axis
  const double viewUpVector[4] = { -1., 0., 0., 0. }; // beam negative X-axis

  mat->MultiplyPoint( normalVector, n);
  mat->MultiplyPoint( viewUpVector, vup);

  parameterNode->SetNormalVector(n);
  parameterNode->SetViewUpVector(vup);
}

//------------------------------------------------------------------------------
bool vtkSlicerDrrImageComputationLogic::UpdateBeamFromCamera(vtkMRMLDrrImageComputationNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateBeamFromCameras: Parameter node is invalid");
    return false;
  }

  vtkMRMLRTBeamNode* beamNode = parameterNode->GetBeamNode();

  if (!beamNode)
  {
    // Observed beam node is invalid
    vtkErrorMacro("UpdateBeamFromCamera: RT Beam node is invalid");
    return false;
  }

  vtkMRMLCameraNode* cameraNode = parameterNode->GetCameraNode();
  if (!cameraNode)
  {
    // Observed camera node is invalid
    vtkErrorMacro("UpdateBeamFromCamera: Camera node is invalid");
    return false;
  }

  // Transform RAS to IEC Patient 
  vtkNew<vtkTransform> rasToPatientTransform;
  rasToPatientTransform->Identity();
  rasToPatientTransform->RotateX(-90.);
  rasToPatientTransform->RotateZ(180.);
  rasToPatientTransform->Inverse();

  // Calculate gantry angle (Theta), patient support angle (Phi) from camera and CT volume
  // Update beam transform
  vtkCamera* camera = cameraNode->GetCamera();
  if (camera)
  {
    double cameraProjRAS[4] = {};
    camera->GetDirectionOfProjection(cameraProjRAS);
    double cameraProjIEC[4] = {};
    rasToPatientTransform->MultiplyPoint( cameraProjRAS, cameraProjIEC);
    cameraProjIEC[0] *= -1.;
    cameraProjIEC[1] *= -1.;
    cameraProjIEC[2] *= -1.;
    // Theta [0, pi], phi [0, 2*pi]
    double phi_x = vtkMath::DegreesFromRadians(acos(cameraProjIEC[0] / vtkMath::Norm(cameraProjIEC, 2)));
    double phi_y = vtkMath::DegreesFromRadians(asin(cameraProjIEC[1] / vtkMath::Norm(cameraProjIEC, 2)));
    double phi = vtkMath::DegreesFromRadians(atan(cameraProjIEC[1] / cameraProjIEC[0]));
    double theta = vtkMath::DegreesFromRadians(acos(cameraProjIEC[2] / vtkMath::Norm(cameraProjIEC, 3)));
    if (phi_x > 0. && phi_y > 0.)
    {
      phi = 360. - phi_x;
    }
    else
    {
      phi = phi_x;
    }
    beamNode->DisableModifiedEventOn();
    beamNode->SetGantryAngle(theta);
    beamNode->SetCouchAngle(phi);
    beamNode->SetSAD(camera->GetDistance());
    beamNode->DisableModifiedEventOff();
    // Update geometry and transform of the beam
    beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
    beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);

    return true;
  }
  else
  {
    // vtkCamera is invalid
    return false;
  }
}

//------------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::SetPlanarImageLogic(vtkSlicerPlanarImageModuleLogic* planarImageLogic)
{
  this->PlanarImageLogic = planarImageLogic;
}

//------------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::SetDRRComputationLogic(vtkSlicerCLIModuleLogic* plastimatchDrrLogic)
{
  this->PlastimatchDRRComputationLogic = plastimatchDrrLogic;
}

//------------------------------------------------------------------------------
void vtkSlicerDrrImageComputationLogic::SetBeamsLogic(vtkSlicerBeamsModuleLogic* beamsLogic)
{
  this->BeamsLogic = beamsLogic;
}
