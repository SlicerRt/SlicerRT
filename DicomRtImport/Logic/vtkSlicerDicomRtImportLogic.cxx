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

// ModuleTemplate includes
#include "vtkSlicerDicomRtImportLogic.h"
#include "vtkSlicerDicomRtImportReader.h"

// Slicer includes
#include "vtkSlicerVolumesLogic.h"

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLAnnotationHierarchyNode.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include <vtkSmartPointer.h>
#include "vtkTriangleFilter.h"
#include "vtkPolyDataNormals.h"
// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportLogic, VolumesLogic, vtkSlicerVolumesLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportLogic::vtkSlicerDicomRtImportLogic()
{
  this->VolumesLogic = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportLogic::~vtkSlicerDicomRtImportLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::InitializeEventListeners()
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(this->GetMRMLScene(), events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportLogic
::LoadDicomRT(const char *filename)
{
  vtkSmartPointer<vtkSlicerDicomRtImportReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtImportReader>::New();
  rtReader->SetFileName(filename);
  rtReader->Update();

  if (!rtReader->GetReadSuccessful())
  {
    return false;
  }

  int numberOfROI = rtReader->GetNumberOfROI();
  for (int dicomRoiIndex=1;dicomRoiIndex<numberOfROI+1; dicomRoiIndex++) // DICOM starts indexing from 1
  {
    vtkPolyData* roiPoly = rtReader->GetROI(dicomRoiIndex);
    if (roiPoly == NULL)
    {
      vtkWarningMacro("Cannot read polydata from file: "<<filename<<", ROI: "<<dicomRoiIndex);
      continue;
    }
    if (roiPoly->GetNumberOfPoints() < 1)
    {
      vtkWarningMacro("The ROI polydata does not contain any points, file: "<<filename<<", ROI: "<<dicomRoiIndex);
      continue;
    }

    const char* roiLabel=rtReader->GetROIName(dicomRoiIndex);
    double *roiColor = rtReader->GetROIDisplayColor(dicomRoiIndex);
    if (roiPoly->GetNumberOfPoints() == 1)
    {	
      // Point ROI
      AddRoiPoint(roiPoly->GetPoint(0), roiLabel, roiColor);
    }
    else
    {
      // Contour ROI
      AddRoiContour(roiPoly, roiLabel, roiColor);
    }
  }

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic::AddArchetypeDICOMObject(const char *filename, const char* name)
{
  std::cout << "Loading series '" << name << "' from file '" << filename << "'" << std::endl;

  // Try to load RT
  if ( LoadDicomRT(filename) )
  {
    return NULL;
  }
  else
  {
    // Try to load Volume
    return this->VolumesLogic->AddArchetypeVolume( filename, name );
  }
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic
::AddRoiPoint(double *roiPosition, const char* roiLabel, double *roiColor)
{
  vtkMRMLAnnotationFiducialNode* fnode = vtkMRMLAnnotationFiducialNode::New();
  //vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> hnode = vtkSmartPointer<vtkMRMLAnnotationHierarchyNode>::New();
  //hnode->SetScene(this->GetMRMLScene());
  fnode->SetName(roiLabel);
  fnode->SetScene(this->GetMRMLScene());

  fnode->AddControlPoint(roiPosition, 0, 1);
  //fnode->CreateAnnotationPointDisplayNode();
  //this->GetMRMLScene()->AddNode(hnode);
  this->GetMRMLScene()->AddNode(fnode);

  vtkMRMLAnnotationPointDisplayNode* dnode = vtkMRMLAnnotationPointDisplayNode::New();
  dnode->SetScene(this->GetMRMLScene());
  dnode->SetGlyphScale(1);
  dnode->SetGlyphType(1);
  this->GetMRMLScene()->AddNode(dnode);
  fnode->SetAndObserveDisplayNodeID(dnode->GetID());
  fnode->SetHideFromEditors(0);
  fnode->SetSelectable(1);

  //int fiducialIndex = roiPoints->AddFiducialWithXYZ(-point_LPS[0], -point_LPS[1], point_LPS[2], false);
  //roiPoints->SetNthFiducialLabelText(fiducialIndex, rtReader->GetROIName(roiIndex+1));
  //roiPoints->SetNthFiducialID(fiducialIndex, rtReader->GetROIName(roiIndex+1));
  //roiPoints->SetNthFiducialVisibility(fiducialIndex, true);    			

  /*
  vtkMRMLColorTableNode* cnode = 0;
  if (node.contains("color"))
  {
  cnode = vtkMRMLColorTableNode::SafeDownCast(
  q->mrmlScene()->GetNodeByID("vtkMRMLColorTableNodeSPLBrainAtlas"));
  Q_ASSERT(cnode);
  for (int i = 0; i < cnode->GetNumberOfColors(); ++i)
  {
  if (cnode->GetColorName(i) == node["color"])
  {
  dnode->SetColor(cnode->GetLookupTable()->GetTableValue(i));
  }
  }
  }
  */

  //roiPoints = vtkMRMLFiducialListNode::SafeDownCast(
  //	this->GetMRMLScene()->AddNode(roiPoints));
  ////Q_ASSERT(dnode);


  //if (!this->ParentID.isEmpty())
  //{
  //	hnode->SetParentNodeID(this->ParentID.toLatin1().data());
  //}

  //this->ParentID = hnode->GetID(); 

  return fnode;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic
::AddRoiContour(vtkPolyData *roiPoly, const char* roiLabel, double *roiColor)
{
  vtkSmartPointer<vtkMRMLModelNode> hnode = vtkSmartPointer<vtkMRMLModelNode>::New();
  vtkSmartPointer<vtkMRMLModelDisplayNode> dnode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  hnode->SetScene(this->GetMRMLScene());
  dnode->SetScene(this->GetMRMLScene()); 

  dnode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(dnode));
  //Q_ASSERT(dnode);
  hnode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(hnode));
  //Q_ASSERT(hnode);

  //dnode->SetVisibility(1);
  //hnode->SetAndObservePolyData(poly);
  hnode->SetName(roiLabel);

  vtkSmartPointer<vtkTriangleFilter> cleaner=vtkSmartPointer<vtkTriangleFilter>::New();
  cleaner->SetInput(roiPoly);

  vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  normals->SetInputConnection ( cleaner->GetOutputPort());
  //--- NOTE: This assumes a completely closed surface
  //---(i.e. no boundary edges) and no non-manifold edges.
  //--- If these constraints do not hold, the AutoOrientNormals
  //--- is not guaranteed to work.
  //normals->AutoOrientNormalsOn();
  //--- Flipping modifies both the normal direction
  //--- and the order of a cell's points.
  normals->FlipNormalsOn();
  normals->SplittingOff();
  //--- enforce consistent polygon ordering.
  normals->ConsistencyOn();

  //cleaner->Update();
  //hnode->SetAndObservePolyData(cleaner->GetOutput());

  normals->Update();
  //hnode->SetAndObservePolyData(normals->GetOutput());
  hnode->SetAndObservePolyData(roiPoly);

  hnode->SetModifiedSinceRead(1);
  dnode->SetModifiedSinceRead(1); 
  dnode->SliceIntersectionVisibilityOn();  
  dnode->VisibilityOn(); 

  //vtkMRMLColorTableNode* cnode = 0;
  /*
  vtkMRMLColorTableNode* cnode = 0;
  if (node.contains("color"))
  {
  cnode = vtkMRMLColorTableNode::SafeDownCast(
  q->mrmlScene()->GetNodeByID("vtkMRMLColorTableNodeSPLBrainAtlas"));
  Q_ASSERT(cnode);
  for (int i = 0; i < cnode->GetNumberOfColors(); ++i)
  {
  if (cnode->GetColorName(i) == node["color"])
  {
  dnode->SetColor(cnode->GetLookupTable()->GetTableValue(i));
  }
  }
  }
  */
  dnode->SetColor(roiColor[0], roiColor[1], roiColor[2]);

  // Disable backface culling to make the back side of the contour visible as well
  dnode->SetBackfaceCulling(0);

  //if (!this->ParentID.isEmpty())
  //{
  //	hnode->SetParentNodeID(this->ParentID.toLatin1().data());
  //}

  hnode->SetAndObserveDisplayNodeID(dnode->GetID());
  hnode->SetHideFromEditors(0);
  hnode->SetSelectable(1);

  this->InvokeEvent( vtkMRMLDisplayableNode::PolyDataModifiedEvent , dnode);

  //this->ParentID = hnode->GetID(); 

  return hnode;
}
