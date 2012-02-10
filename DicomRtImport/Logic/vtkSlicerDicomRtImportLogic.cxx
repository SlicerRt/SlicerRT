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
#include "vtkSlicerDicomRtReader.h"

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
void vtkSlicerDicomRtImportLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportLogic::LoadDicomRT(const char *filename, const char* seriesname)
{
  std::cout << "Loading series '" << seriesname << "' from file '" << filename << "'" << std::endl;

  vtkSmartPointer<vtkSlicerDicomRtReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtReader>::New();
  rtReader->SetFileName(filename);
  rtReader->Update();

  if (rtReader->GetLoadRTStructureSetSuccessful())
  {
    int numberOfROI = rtReader->GetNumberOfROIs();
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

  if (rtReader->GetLoadRTDoseSuccessful())
  {
    // Load Volume
    vtkMRMLVolumeNode* volumeNode = this->VolumesLogic->AddArchetypeVolume(filename, seriesname);
    if (volumeNode == NULL)
    {
      vtkErrorMacro("Failed to load dose volume file '" << filename << "' (series name '" << seriesname << "')");
      return false;
    }

    // Set new spacing
    double* initialSpacing = volumeNode->GetSpacing();
    double* correctSpacing = rtReader->GetPixelSpacing();
    volumeNode->SetSpacing(correctSpacing[0], correctSpacing[1], initialSpacing[2]);

    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic::AddRoiPoint(double *roiPosition, const char* roiLabel, double *roiColor)
{
  vtkMRMLAnnotationPointDisplayNode* displayNode = vtkMRMLAnnotationPointDisplayNode::New();
  displayNode->SetScene(this->GetMRMLScene());
  displayNode->SetGlyphScale(1);
  displayNode->SetGlyphType(1);
  this->GetMRMLScene()->AddNode(displayNode);

  vtkMRMLAnnotationFiducialNode* fiducialNode = vtkMRMLAnnotationFiducialNode::New();
  fiducialNode->SetName(roiLabel);
  fiducialNode->SetScene(this->GetMRMLScene());
  fiducialNode->AddControlPoint(roiPosition, 0, 1);
  fiducialNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  fiducialNode->SetHideFromEditors(0);
  fiducialNode->SetSelectable(1);
  this->GetMRMLScene()->AddNode(fiducialNode);

  return fiducialNode;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic::AddRoiContour(vtkPolyData *roiPoly, const char* roiLabel, double *roiColor)
{
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode->SetScene(this->GetMRMLScene()); 
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
  displayNode->SetModifiedSinceRead(1); 
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn(); 
  displayNode->SetColor(roiColor[0], roiColor[1], roiColor[2]);

  // Disable backface culling to make the back side of the contour visible as well
  displayNode->SetBackfaceCulling(0);


  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  modelNode->SetScene(this->GetMRMLScene());
  modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(modelNode));
  modelNode->SetName(roiLabel);
  modelNode->SetAndObservePolyData(roiPoly);
  modelNode->SetModifiedSinceRead(1);
  modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  modelNode->SetHideFromEditors(0);
  modelNode->SetSelectable(1);

  vtkSmartPointer<vtkTriangleFilter> cleaner=vtkSmartPointer<vtkTriangleFilter>::New();
  cleaner->SetInput(roiPoly);

  /*
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
  normals->Update();

  //modelNode->SetAndObservePolyData(normals->GetOutput());
  */

  this->InvokeEvent( vtkMRMLDisplayableNode::PolyDataModifiedEvent, displayNode);

  return modelNode;
}
