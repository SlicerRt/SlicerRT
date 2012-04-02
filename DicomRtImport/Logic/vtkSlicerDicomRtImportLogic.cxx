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
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLAnnotationHierarchyNode.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLAnnotationTextDisplayNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkTriangleFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkImageCast.h>

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

  // RTSTRUCT
  if (rtReader->GetLoadRTStructureSetSuccessful())
  {
    this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

    // Create hierarchy node for the loaded structure sets
    vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
    std::string hierarchyNodeName;
    hierarchyNodeName = std::string(seriesname) + "_RTStructureSetHierarchy";
    modelHierarchyRootNode->SetName(hierarchyNodeName.c_str());
    modelHierarchyRootNode->AllowMultipleChildrenOn();
    modelHierarchyRootNode->HideFromEditorsOff();
    this->GetMRMLScene()->AddNode(modelHierarchyRootNode);

    // A hierarchy node needs a display node
    vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    hierarchyNodeName.append("Display");
    modelDisplayNode->SetName(hierarchyNodeName.c_str());
    modelDisplayNode->SetVisibility(1);
    this->GetMRMLScene()->AddNode(modelDisplayNode);
    modelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );
    modelDisplayNode = NULL;

    // Now get it again as a mrml node so can add things under it (copied from ModelMaker module)
    //vtkMRMLNode* modelHierarchyNodeCasted = this->GetMRMLScene()->GetNodeByID( modelHierarchyNode->GetID() );

    // Add ROIs
    int numberOfROI = rtReader->GetNumberOfROIs();
    for (int dicomRoiIndex=1; dicomRoiIndex<numberOfROI+1; dicomRoiIndex++) // DICOM starts indexing from 1
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
      vtkMRMLDisplayableNode* addedDisplayableNode = NULL;
      if (roiPoly->GetNumberOfPoints() == 1)
      {	
        // Point ROI
        addedDisplayableNode = AddRoiPoint(roiPoly->GetPoint(0), roiLabel, roiColor);
      }
      else
      {
        // Contour ROI
        addedDisplayableNode = AddRoiContour(roiPoly, roiLabel, roiColor);
      }

      // Add new node to the hierarchy node
      if (addedDisplayableNode)
      {
        // put it in the hierarchy
        vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        this->GetMRMLScene()->AddNode(modelHierarchyNode);
        modelHierarchyNode->SetParentNodeID( modelHierarchyRootNode->GetID() );
        modelHierarchyNode->SetModelNodeID( addedDisplayableNode->GetID() );
      }
    }

    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
    return true;
  }

  // RTDOSE
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
    volumeNode->SetAttribute("DoseUnits",rtReader->GetDoseUnits());
    volumeNode->SetAttribute("DoseGridScaling",rtReader->GetDoseGridScaling());

    // Apply dose grid scaling
    vtkImageData* originalVolumeData = volumeNode->GetImageData();
    vtkImageData* floatVolumeData = vtkImageData::New();

    vtkNew<vtkImageCast> imageCast;
    imageCast->SetInput(originalVolumeData);
    imageCast->SetOutputScalarTypeToFloat();
    imageCast->Update();
    floatVolumeData->DeepCopy(imageCast->GetOutput());

    double doseGridScaling = atof(rtReader->GetDoseGridScaling());
    float value = 0.0;
    float* floatPtr = (float*)floatVolumeData->GetScalarPointer();
    for (long i=0; i<floatVolumeData->GetNumberOfPoints(); ++i)
    {
      value = (*floatPtr) * doseGridScaling;
      (*floatPtr) = value;
      ++floatPtr;
    }

    volumeNode->SetAndObserveImageData(floatVolumeData);
    originalVolumeData->Delete();

    volumeNode->SetModifiedSinceRead(1); 

    // Set default colormap to rainbow
    if (volumeNode->GetVolumeDisplayNode()!=NULL)
    {
      volumeNode->GetVolumeDisplayNode()->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    }
    // Select as active volume
    if (this->GetApplicationLogic()!=NULL)
    {
      if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
      {
        this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(volumeNode->GetID());
        this->GetApplicationLogic()->PropagateVolumeSelection();
      }
    }
    return true;
  }

  // RTPLAN
  if (rtReader->GetLoadRTPlanSuccessful())
  {
    this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

    int numberOfBeams = rtReader->GetNumberOfBeams();
    for (int dicomBeamIndex = 1; dicomBeamIndex < numberOfBeams+1; dicomBeamIndex++) // DICOM starts indexing from 1
    {
      // Isocenter fiducial
      double isoColor[3] = { 1.0,1.0,1.0};
      AddRoiPoint(rtReader->GetBeamIsocenterPosition(dicomBeamIndex), rtReader->GetBeamName(dicomBeamIndex), isoColor);
    }

    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic::AddRoiPoint(double *roiPosition, const char* roiLabel, double *roiColor)
{
  vtkMRMLAnnotationFiducialNode* fiducialNode = vtkMRMLAnnotationFiducialNode::New();
  fiducialNode->SetName(roiLabel);
  fiducialNode->AddControlPoint(roiPosition, 0, 1);
  fiducialNode->SetLocked(1);
  this->GetMRMLScene()->AddNode(fiducialNode);

  fiducialNode->CreateAnnotationTextDisplayNode();
  fiducialNode->CreateAnnotationPointDisplayNode();
  fiducialNode->GetAnnotationPointDisplayNode()->SetGlyphType(vtkMRMLAnnotationPointDisplayNode::Sphere3D);
  fiducialNode->GetAnnotationPointDisplayNode()->SetColor(roiColor);
  fiducialNode->GetAnnotationTextDisplayNode()->SetColor(roiColor);

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

  this->InvokeEvent( vtkMRMLDisplayableNode::PolyDataModifiedEvent, displayNode);

  return modelNode;
}
