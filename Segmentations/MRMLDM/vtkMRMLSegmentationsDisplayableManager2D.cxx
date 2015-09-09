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

// MRMLDisplayableManager includes
#include "vtkMRMLSegmentationsDisplayableManager2D.h"

// MRML includes
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSegmentationDisplayNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLTransformNode.h>

// SegmentationCore includes
#include "vtkSegmentation.h"

// VTK includes
#include <vtkActor2D.h>
#include <vtkCallbackCommand.h>
#include <vtkColorTransferFunction.h>
#include <vtkEventBroker.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWeakPointer.h>
#include <vtkPointLocator.h>
#include <vtkGeneralTransform.h>
#include <vtkPointData.h>
#include <vtkDataSetAttributes.h>

// VTK includes: customization
#include <vtkCutter.h>

// STD includes
#include <algorithm>
#include <cassert>
#include <set>
#include <map>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLSegmentationsDisplayableManager2D );

//---------------------------------------------------------------------------
class vtkMRMLSegmentationsDisplayableManager2D::vtkInternal
{
public:

  vtkInternal( vtkMRMLSegmentationsDisplayableManager2D* external );
  ~vtkInternal();

  struct Pipeline
    {
    std::string SegmentID;
    vtkSmartPointer<vtkActor2D> Actor;
    vtkSmartPointer<vtkTransform> TransformToSlice;
    vtkSmartPointer<vtkTransformPolyDataFilter> Transformer;
    vtkSmartPointer<vtkTransformPolyDataFilter> ModelWarper;
    vtkSmartPointer<vtkPlane> Plane;
    vtkSmartPointer<vtkCutter> Cutter;
    vtkSmartPointer<vtkGeneralTransform> NodeToWorld;
    };

  typedef std::map<std::string, const Pipeline*> PipelineMapType;
  typedef std::map < vtkMRMLSegmentationDisplayNode*, PipelineMapType > PipelinesCacheType;
  PipelinesCacheType DisplayPipelines;

  typedef std::map < vtkMRMLSegmentationNode*, std::set< vtkMRMLSegmentationDisplayNode* > > SegmentationToDisplayCacheType;
  SegmentationToDisplayCacheType SegmentationToDisplayNodes;

  // Segmentations
  void AddSegmentationNode(vtkMRMLSegmentationNode* displayableNode);
  void RemoveSegmentationNode(vtkMRMLSegmentationNode* displayableNode);

  // Transforms
  void UpdateDisplayableTransforms(vtkMRMLSegmentationNode *node);
  void GetNodeTransformToWorld(vtkMRMLTransformableNode* node, vtkGeneralTransform* transformToWorld);

  // Slice Node
  void SetSliceNode(vtkMRMLSliceNode* sliceNode);
  void UpdateSliceNode();
  void SetSlicePlaneFromMatrix(vtkMatrix4x4* matrix, vtkPlane* plane);

  // Display Nodes
  void AddDisplayNode(vtkMRMLSegmentationNode*, vtkMRMLSegmentationDisplayNode*);
  Pipeline* CreateSegmentPipeline(std::string segmentID);
  void UpdateDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode);
  void UpdateSegmentPipelines(vtkMRMLSegmentationDisplayNode*, PipelineMapType&);
  void UpdateDisplayNodePipeline(vtkMRMLSegmentationDisplayNode*, PipelineMapType);
  void RemoveDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode);

  // Observations
  void AddObservations(vtkMRMLSegmentationNode* node);
  void RemoveObservations(vtkMRMLSegmentationNode* node);
  bool IsNodeObserved(vtkMRMLSegmentationNode* node);

  // Helper functions
  bool IsVisible(vtkMRMLSegmentationDisplayNode* displayNode);
  bool UseDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode);
  bool UseDisplayableNode(vtkMRMLSegmentationNode* node);
  void ClearDisplayableNodes();

private:
  vtkSmartPointer<vtkMatrix4x4> SliceXYToRAS;
  vtkMRMLSegmentationsDisplayableManager2D* External;
  bool AddingSegmentationNode;
  vtkSmartPointer<vtkMRMLSliceNode> SliceNode;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::vtkInternal(vtkMRMLSegmentationsDisplayableManager2D* external)
: External(external)
, AddingSegmentationNode(false)
{
  this->SliceXYToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  this->SliceXYToRAS->Identity();
}

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::~vtkInternal()
{
  this->ClearDisplayableNodes();
  this->SliceNode = NULL;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UseDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode)
{
   // allow annotations to appear only in designated viewers
  if (displayNode && !displayNode->IsDisplayableInView(this->SliceNode->GetID()))
    {
    return false;
    }

  // Check whether DisplayNode should be shown in this view
  bool use = displayNode && displayNode->IsA("vtkMRMLSegmentationDisplayNode");

  return use;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::IsVisible(vtkMRMLSegmentationDisplayNode* displayNode)
{
  return displayNode
      && displayNode->GetSliceIntersectionVisibility()
      && displayNode->GetVisibility(this->External->GetMRMLSliceNode()->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::SetSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode || this->SliceNode == sliceNode)
    {
    return;
    }
  this->SliceNode=sliceNode;
  this->UpdateSliceNode();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateSliceNode()
{
  // Update the Slice node transform then update the DisplayNode pipelines to account for plane location
  this->SliceXYToRAS->DeepCopy( this->SliceNode->GetXYToRAS() );
  PipelinesCacheType::iterator displayNodeIt;
  for (displayNodeIt = this->DisplayPipelines.begin(); displayNodeIt != this->DisplayPipelines.end(); ++displayNodeIt)
    {
    this->UpdateDisplayNodePipeline(displayNodeIt->first, displayNodeIt->second);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::SetSlicePlaneFromMatrix(vtkMatrix4x4* sliceMatrix, vtkPlane* plane)
{
  double normal[3] = {0.0,0.0,0.0};
  double origin[3] = {0.0,0.0,0.0};

  // +/-1: orientation of the normal
  const int planeOrientation = 1;
  for (int i = 0; i < 3; i++)
    {
    normal[i] = planeOrientation * sliceMatrix->GetElement(i,2);
    origin[i] = sliceMatrix->GetElement(i,3);
    }

  plane->SetNormal(normal);
  plane->SetOrigin(origin);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::AddSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (this->AddingSegmentationNode)
    {
    return;
    }
  // Check if node should be used
  if (!this->UseDisplayableNode(node))
    {
    return;
    }

  this->AddingSegmentationNode = true;
  // Add Display Nodes
  int nnodes = node->GetNumberOfDisplayNodes();

  this->AddObservations(node);

  for (int i=0; i<nnodes; i++)
    {
    vtkMRMLSegmentationDisplayNode *dnode = vtkMRMLSegmentationDisplayNode::SafeDownCast(node->GetNthDisplayNode(i));
    if ( this->UseDisplayNode(dnode) )
      {
      this->SegmentationToDisplayNodes[node].insert(dnode);
      this->AddDisplayNode( node, dnode );
      }
    }
  this->AddingSegmentationNode = false;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::RemoveSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (!node)
    {
    return;
    }
  vtkInternal::SegmentationToDisplayCacheType::iterator displayableIt =
    this->SegmentationToDisplayNodes.find(node);
  if(displayableIt == this->SegmentationToDisplayNodes.end())
    {
    return;
    }

  std::set< vtkMRMLSegmentationDisplayNode* > dnodes = displayableIt->second;
  std::set< vtkMRMLSegmentationDisplayNode* >::iterator diter;
  for ( diter = dnodes.begin(); diter != dnodes.end(); ++diter)
    {
    this->RemoveDisplayNode(*diter);
    }
  this->RemoveObservations(node);
  this->SegmentationToDisplayNodes.erase(displayableIt);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::GetNodeTransformToWorld(vtkMRMLTransformableNode* node, vtkGeneralTransform* transformToWorld)
{
  if (!node || !transformToWorld)
    {
    return;
    }

  vtkMRMLTransformNode* tnode = node->GetParentTransformNode();

  transformToWorld->Identity();
  if (tnode)
    {
    tnode->GetTransformToWorld(transformToWorld);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateDisplayableTransforms(vtkMRMLSegmentationNode* mNode)
{
  // Update the NodeToWorld matrix for all tracked DisplayableNode
  PipelinesCacheType::iterator pipelinesIter;
  std::set<vtkMRMLSegmentationDisplayNode *> displayNodes = this->SegmentationToDisplayNodes[mNode];
  std::set<vtkMRMLSegmentationDisplayNode *>::iterator dnodesIter;
  for ( dnodesIter = displayNodes.begin(); dnodesIter != displayNodes.end(); dnodesIter++ )
    {
    if ( ((pipelinesIter = this->DisplayPipelines.find(*dnodesIter)) != this->DisplayPipelines.end()) )
      {
      this->UpdateDisplayNodePipeline(pipelinesIter->first, pipelinesIter->second);
      for (PipelineMapType::iterator pipelineIt=pipelinesIter->second.begin(); pipelineIt!=pipelinesIter->second.end(); ++pipelineIt)
        {
        const Pipeline* currentPipeline = pipelineIt->second;
        this->GetNodeTransformToWorld(mNode, currentPipeline->NodeToWorld);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::RemoveDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode)
{
  PipelinesCacheType::iterator pipelinesIter = this->DisplayPipelines.find(displayNode);
  if (pipelinesIter == this->DisplayPipelines.end())
    {
    return;
    }
  PipelineMapType::iterator pipelineIt;
  for (pipelineIt = pipelinesIter->second.begin(); pipelineIt != pipelinesIter->second.end(); ++pipelineIt)
    {
    const Pipeline* pipeline = pipelineIt->second;
    this->External->GetRenderer()->RemoveActor(pipeline->Actor);
    delete pipeline;
    }
  this->DisplayPipelines.erase(pipelinesIter);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::AddDisplayNode(vtkMRMLSegmentationNode* mNode, vtkMRMLSegmentationDisplayNode* displayNode)
{
  if (!mNode || !displayNode)
    {
    return;
    }

  // Do not add the display node if displayNodeIt is already associated with a pipeline object.
  // This happens when a segmentation node already associated with a display node
  // is copied into an other (using vtkMRMLNode::Copy()) and is added to the scene afterward.
  // Related issue are #3428 and #2608
  PipelinesCacheType::iterator displayNodeIt;
  displayNodeIt = this->DisplayPipelines.find(displayNode);
  if (displayNodeIt != this->DisplayPipelines.end())
    {
    return;
    }

  // Create pipelines for each segment
  vtkSegmentation* segmentation = mNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }
  PipelineMapType pipelineVector;
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
    pipelineVector[segmentIt->first] = this->CreateSegmentPipeline(segmentIt->first);
    }

  this->DisplayPipelines.insert( std::make_pair(displayNode, pipelineVector) );

  // Update cached matrices. Calls UpdateDisplayNodePipeline
  this->UpdateDisplayableTransforms(mNode);
}

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::Pipeline*
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::CreateSegmentPipeline(std::string segmentID)
{
  Pipeline* pipeline = new Pipeline();
  pipeline->SegmentID = segmentID;
  pipeline->Actor = vtkSmartPointer<vtkActor2D>::New();
  pipeline->TransformToSlice = vtkSmartPointer<vtkTransform>::New();
  pipeline->Transformer = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  pipeline->Cutter = vtkSmartPointer<vtkCutter>::New();
  pipeline->NodeToWorld = vtkSmartPointer<vtkGeneralTransform>::New();
  pipeline->ModelWarper = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  pipeline->Plane = vtkSmartPointer<vtkPlane>::New();

  // Set up pipeline
  pipeline->Transformer->SetTransform(pipeline->TransformToSlice);
  pipeline->Transformer->SetInputConnection(pipeline->Cutter->GetOutputPort());
  pipeline->Cutter->SetCutFunction(pipeline->Plane);
  pipeline->Cutter->SetGenerateCutScalars(0);
  pipeline->Cutter->SetInputConnection(pipeline->ModelWarper->GetOutputPort());
  vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  pipeline->Actor->SetMapper(mapper);
  mapper->SetInputConnection(pipeline->Transformer->GetOutputPort());
  pipeline->Actor->SetVisibility(0);

  // Add actor to Renderer and local cache
  this->External->GetRenderer()->AddActor( pipeline->Actor );

  return pipeline;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode)
{
  // If the DisplayNode already exists, just update. Otherwise, add as new node
  if (!displayNode)
    {
    return;
    }
  PipelinesCacheType::iterator displayNodeIt = this->DisplayPipelines.find(displayNode);
  if (displayNodeIt != this->DisplayPipelines.end())
    {
    this->UpdateSegmentPipelines(displayNode, displayNodeIt->second);
    this->UpdateDisplayNodePipeline(displayNode, displayNodeIt->second);
    }
  else
    {
    this->AddSegmentationNode( vtkMRMLSegmentationNode::SafeDownCast(displayNode->GetDisplayableNode()) );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateSegmentPipelines(vtkMRMLSegmentationDisplayNode* displayNode, PipelineMapType &pipelines)
{
  // Get segmentation
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(displayNode->GetDisplayableNode());
  if (!segmentationNode)
    {
    return;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }

  // Get segments
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();

  // Make sure each segment has a pipeline
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
    // If segment does not have a pipeline, create one
    PipelineMapType::iterator pipelineIt = pipelines.find(segmentIt->first);
    if (pipelineIt == pipelines.end())
      {
      pipelines[segmentIt->first] = this->CreateSegmentPipeline(segmentIt->first);
      }
    }

  // Make sure each pipeline belongs to an existing segment
  PipelineMapType::iterator pipelineIt = pipelines.begin();
  while (pipelineIt != pipelines.end())
    {
    const Pipeline* pipeline = pipelineIt->second;
    vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.find(pipeline->SegmentID);
    if (segmentIt == segmentMap.end())
      {
      PipelineMapType::iterator erasedIt = pipelineIt;
      ++pipelineIt;
      pipelines.erase(erasedIt);
      this->External->GetRenderer()->RemoveActor(pipeline->Actor);
      delete pipeline;
      }
    else
      {
      ++pipelineIt;
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateDisplayNodePipeline(vtkMRMLSegmentationDisplayNode* displayNode, PipelineMapType pipelines)
{
  // Sets visibility, set pipeline polydata input, update color calculate and set pipeline segments.
  if (!displayNode)
    {
    return;
    }
  bool displayNodeVisible = this->IsVisible(displayNode);

  // Get segmentation display node
  vtkMRMLSegmentationDisplayNode* segmentationDisplayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(displayNode);

  // Determine which representation to show
  std::string polyDataRepresenatationName = segmentationDisplayNode->DeterminePolyDataDisplayRepresentationName();
  if (polyDataRepresenatationName.empty())
    {
    return;
    }

  // Get segmentation
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
    segmentationDisplayNode->GetDisplayableNode() );
  if (!segmentationNode)
    {
    return;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }
  // Make sure the requested representation exists
  if (!segmentation->CreateRepresentation(polyDataRepresenatationName))
    {
    return;
    }

  // For all pipelines (pipeline per segment)
  for (PipelineMapType::iterator pipelineIt=pipelines.begin(); pipelineIt!=pipelines.end(); ++pipelineIt)
    {
    const Pipeline* pipeline = pipelineIt->second;

    // Update visibility
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    if (!displayNode->GetSegmentDisplayProperties(pipeline->SegmentID, properties))
      {
      continue;
      }
    bool segmentVisible = displayNodeVisible && properties.Visible;
    pipeline->Actor->SetVisibility(segmentVisible);
    if (!segmentVisible)
      {
      continue;
      }

    // Get poly data to display
    vtkPolyData* polyData = vtkPolyData::SafeDownCast(
      segmentation->GetSegmentRepresentation(pipeline->SegmentID, polyDataRepresenatationName) );
    if (!polyData || polyData->GetNumberOfPoints() == 0)
      {
      continue;
      }

    //polyData->Modified(); // If we call modified on the master representation, then it causes deletion of all others
#if (VTK_MAJOR_VERSION <= 5)
    pipeline->ModelWarper->SetInput(polyData);
#else
    pipeline->ModelWarper->SetInputData(polyData);
#endif

    pipeline->ModelWarper->SetTransform(pipeline->NodeToWorld);

    // Set Plane Transform
    this->SetSlicePlaneFromMatrix(this->SliceXYToRAS, pipeline->Plane);
    pipeline->Plane->Modified();

    // Set PolyData Transform
    vtkNew<vtkMatrix4x4> rasToSliceXY;
    vtkMatrix4x4::Invert(this->SliceXYToRAS, rasToSliceXY.GetPointer());
    pipeline->TransformToSlice->SetMatrix(rasToSliceXY.GetPointer());

    // Optimization for slice to slice intersections which are 1 quad polydatas
    // no need for 50^3 default locator divisions
    if (polyData->GetPoints() != NULL && polyData->GetNumberOfPoints() <= 4)
      {
      vtkNew<vtkPointLocator> locator;
      double *bounds = polyData->GetBounds();
      locator->SetDivisions(2,2,2);
      locator->InitPointInsertion(polyData->GetPoints(), bounds);
      pipeline->Cutter->SetLocator(locator.GetPointer());
      }

    // Update pipeline actor
    vtkActor2D* actor = vtkActor2D::SafeDownCast(pipeline->Actor);

    actor->SetPosition(0,0);
    vtkProperty2D* actorProperties = actor->GetProperty();
    actorProperties->SetColor(properties.Color[0], properties.Color[1], properties.Color[2]);
    actorProperties->SetLineWidth(displayNode->GetSliceIntersectionThickness() );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::AddObservations(vtkMRMLSegmentationNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  if (!broker->GetObservationExist(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkSegmentation::MasterRepresentationModified, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkSegmentation::MasterRepresentationModified, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkSegmentation::RepresentationCreated, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkSegmentation::RepresentationCreated, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::RemoveObservations(vtkMRMLSegmentationNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  observations = broker->GetObservations(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkSegmentation::MasterRepresentationModified, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkSegmentation::RepresentationCreated, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::IsNodeObserved(vtkMRMLSegmentationNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkCollection* observations = broker->GetObservationsForSubject(node);
  if (observations->GetNumberOfItems() > 0)
    {
    return true;
    }
  else
    {
    return false;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::ClearDisplayableNodes()
{
  while(this->SegmentationToDisplayNodes.size() > 0)
    {
    this->RemoveSegmentationNode(this->SegmentationToDisplayNodes.begin()->first);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UseDisplayableNode(vtkMRMLSegmentationNode* node)
{
  bool use = node && node->IsA("vtkMRMLSegmentationNode");
  return use;
}

//---------------------------------------------------------------------------
// vtkMRMLSegmentationsDisplayableManager2D methods

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkMRMLSegmentationsDisplayableManager2D()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::~vtkMRMLSegmentationsDisplayableManager2D()
{
  delete this->Internal;
  this->Internal = NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkMRMLSegmentationsDisplayableManager2D: " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if ( !node->IsA("vtkMRMLSegmentationNode") )
    {
    return;
    }

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    this->SetUpdateFromMRMLRequested(1);
    return;
    }

  this->Internal->AddSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if ( node
    && (!node->IsA("vtkMRMLSegmentationNode"))
    && (!node->IsA("vtkMRMLSegmentationDisplayNode")) )
    {
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = NULL;
  vtkMRMLSegmentationDisplayNode* displayNode = NULL;

  bool modified = false;
  if ( (segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node)) )
    {
    this->Internal->RemoveSegmentationNode(segmentationNode);
    modified = true;
    }
  else if ( (displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(node)) )
    {
    this->Internal->RemoveDisplayNode(displayNode);
    modified = true;
    }
  if (modified)
    {
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  vtkMRMLScene* scene = this->GetMRMLScene();

  if ( scene->IsBatchProcessing() )
    {
    return;
    }

  vtkMRMLSegmentationNode* displayableNode = vtkMRMLSegmentationNode::SafeDownCast(caller);

  if (displayableNode)
    {
    if (event == vtkMRMLDisplayableNode::DisplayModifiedEvent)
      {
      vtkMRMLNode* callDataNode = reinterpret_cast<vtkMRMLDisplayNode *> (callData);
      vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(callDataNode);
      if (displayNode)
        {
        this->Internal->UpdateDisplayNode(displayNode);
        this->RequestRender();
        }
      }
    else if ( (event == vtkMRMLDisplayableNode::TransformModifiedEvent)
           || (event == vtkMRMLTransformableNode::TransformModifiedEvent)
           || (event == vtkSegmentation::RepresentationCreated) )
      {
      this->Internal->UpdateDisplayableTransforms(displayableNode);
      this->RequestRender();
      }
    }
  else if ( vtkMRMLSliceNode::SafeDownCast(caller) )
      {
      this->Internal->UpdateSliceNode();
      this->RequestRender();
      }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::UpdateFromMRML()
{
  this->SetUpdateFromMRMLRequested(0);

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
    {
    vtkDebugMacro( "vtkMRMLSegmentationsDisplayableManager2D->UpdateFromMRML: Scene is not set.")
    return;
    }
  this->Internal->ClearDisplayableNodes();

  vtkMRMLSegmentationNode* mNode = NULL;
  std::vector<vtkMRMLNode *> mNodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLSegmentationNode", mNodes) : 0;
  for (int i=0; i<nnodes; i++)
    {
    mNode  = vtkMRMLSegmentationNode::SafeDownCast(mNodes[i]);
    if (mNode && this->Internal->UseDisplayableNode(mNode))
      {
      this->Internal->AddSegmentationNode(mNode);
      }
    }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::UnobserveMRMLScene()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneStartClose()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneEndClose()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneEndBatchProcess()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::Create()
{
  this->Internal->SetSliceNode(this->GetMRMLSliceNode());
  this->SetUpdateFromMRMLRequested(1);
}
