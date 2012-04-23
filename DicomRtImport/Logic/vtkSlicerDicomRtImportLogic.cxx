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
#include "vtkDICOMImportInfo.h"

// DCMTK includes
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofcond.h>
#include <dcmtk/ofstd/ofstring.h>
#include <dcmtk/ofstd/ofstd.h>        /* for class OFStandard */

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
#include <vtkStringArray.h>

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
void vtkSlicerDicomRtImportLogic::Examine(vtkDICOMImportInfo *importInfo)
{
  importInfo->RemoveAllLoadables();

  for (int fileListIndex=0; fileListIndex<importInfo->GetNumberOfFileLists(); fileListIndex++)
  {
    vtkStringArray *fileList=importInfo->GetFileList(fileListIndex);
    for (int fileIndex=0; fileIndex<fileList->GetNumberOfValues(); fileIndex++)
    {
      DcmFileFormat fileformat;

      vtkStdString fileName=fileList->GetValue(fileIndex);
      OFCondition result;
      result = fileformat.loadFile(fileName, EXS_Unknown);
      if (!result.good())
      {
        continue; // failed to parse this file, skip it
      }
      DcmDataset *dataset = fileformat.getDataset();
      // check SOP Class UID for one of the supported RT objects
      OFString sopClass;
      if (!dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() || sopClass.empty())
      {
        continue; // failed to parse this file, skip it
      }    
      
      // DICOM parsing is successful, now check if the object is loadable 
      std::string name;
      std::string tooltip;
      std::string warning;
      bool selected=true;

      OFString seriesNumber;
      dataset->findAndGetOFString(DCM_SeriesNumber, seriesNumber);
      if (!seriesNumber.empty())
      {
        name+=std::string(seriesNumber.c_str())+": ";
      }

      if (sopClass == UID_RTDoseStorage)
      {
        name+="RTDOSE";
        OFString instanceNumber;
        dataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
        OFString seriesDescription;
        dataset->findAndGetOFString(DCM_SeriesDescription, seriesDescription);
        if (!seriesDescription.empty())
        {
          name+=std::string(": ")+seriesDescription.c_str(); 
        }
        if (!instanceNumber.empty())
        {
          name+=std::string(" [")+instanceNumber.c_str()+"]"; 
        }
      }
      else if (sopClass == UID_RTPlanStorage)
      {
        name+="RTPLAN";
        OFString planLabel;
        dataset->findAndGetOFString(DCM_RTPlanLabel, planLabel);
        OFString planName;
        dataset->findAndGetOFString(DCM_RTPlanName, planName);
        if (!planLabel.empty() && !planName.empty())
        {
          if (planLabel.compare(planName)!=0)
          {
            // plan label and name is different, display both
            name+=std::string(": ")+planLabel.c_str()+" ("+planName.c_str()+")";
          }
          else
          {
            name+=std::string(": ")+planLabel.c_str();
          }
        }
        else if (!planLabel.empty() && planName.empty())
        {
          name+=std::string(": ")+planLabel.c_str();
        }
        else if (planLabel.empty() && !planName.empty())
        {
          name+=std::string(": ")+planName.c_str();
        }
      }
      else if (sopClass == UID_RTStructureSetStorage)
      {
        name+="RTSTRUCT";
        OFString structLabel;
        dataset->findAndGetOFString(DCM_StructureSetLabel, structLabel);
        if (!structLabel.empty())
        {
          name+=std::string(": ")+structLabel.c_str();
        }
      }
      /* not yet supported
      else if (sopClass == UID_RTImageStorage)
      else if (sopClass == UID_RTTreatmentSummaryRecordStorage)
      else if (sopClass == UID_RTIonPlanStorage)
      else if (sopClass == UID_RTIonBeamsTreatmentRecordStorage)
      */
      else
      {
        continue; // not an RT file
      }

      // The object is stored in a single file
      vtkSmartPointer<vtkStringArray> loadableFileList=vtkSmartPointer<vtkStringArray>::New();
      loadableFileList->InsertNextValue(fileName);
     
      importInfo->InsertNextLoadable(loadableFileList, name.c_str(), tooltip.c_str(), warning.c_str(), selected);
    }

  }

}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportLogic::LoadDicomRT(const char *filename, const char* seriesname)
{
  std::cout << "Loading series '" << seriesname << "' from file '" << filename << "'" << std::endl;

  vtkSmartPointer<vtkSlicerDicomRtReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtReader>::New();
  rtReader->SetFileName(filename);
  rtReader->Update();

  // One series can contain composite information, e.g, an RTPLAN series can contain structure sets and plans as well

  bool loadedSomething=false;
  bool loadingErrorsOccurred=false;

    // Hierarchy node for the loaded structure sets
    // It is not created here yet because maybe there won't be anything to put in it.
    vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode;

  // RTSTRUCT
  if (rtReader->GetLoadRTStructureSetSuccessful())
  {
    this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

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
        // Create root node, if it has not been created yet
        if (modelHierarchyRootNode.GetPointer()==NULL)
        {
          modelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
          std::string hierarchyNodeName;
          hierarchyNodeName = std::string(seriesname) + " - all structures";
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
        }

        // put the new node in the hierarchy
        vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        this->GetMRMLScene()->AddNode(modelHierarchyNode);
        modelHierarchyNode->SetParentNodeID( modelHierarchyRootNode->GetID() );
        modelHierarchyNode->SetModelNodeID( addedDisplayableNode->GetID() );
      }
    }

    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
    loadedSomething=true;
  }

  // RTDOSE
  if (rtReader->GetLoadRTDoseSuccessful())
  {
    // Load Volume
    vtkMRMLVolumeNode* volumeNode = this->VolumesLogic->AddArchetypeVolume(filename, seriesname);
    if (volumeNode == NULL)
    {
      vtkErrorMacro("Failed to load dose volume file '" << filename << "' (series name '" << seriesname << "')");
      loadingErrorsOccurred=true;
    }
    else
    {

      // Set new spacing
      double* initialSpacing = volumeNode->GetSpacing();
      double* correctSpacing = rtReader->GetPixelSpacing();
      volumeNode->SetSpacing(correctSpacing[0], correctSpacing[1], initialSpacing[2]);
      volumeNode->SetAttribute("DoseUnitName",rtReader->GetDoseUnits());
      volumeNode->SetAttribute("DoseUnitValue",rtReader->GetDoseGridScaling());

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
      loadedSomething=true;
    }
  }

  // RTPLAN
  if (rtReader->GetLoadRTPlanSuccessful())
  {
    this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

    vtkMRMLDisplayableNode* addedDisplayableNode = NULL;
    int numberOfBeams = rtReader->GetNumberOfBeams();
    for (int dicomBeamIndex = 1; dicomBeamIndex < numberOfBeams+1; dicomBeamIndex++) // DICOM starts indexing from 1
    {
      // Isocenter fiducial
      double isoColor[3] = { 1.0,1.0,1.0};
      addedDisplayableNode= this->AddRoiPoint(rtReader->GetBeamIsocenterPosition(dicomBeamIndex), rtReader->GetBeamName(dicomBeamIndex), isoColor);
       // Add new node to the hierarchy node
      if (addedDisplayableNode)
      {
        // Create root node, if it has not been created yet
        if (modelHierarchyRootNode.GetPointer()==NULL)
        {
          modelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
          std::string hierarchyNodeName;
          hierarchyNodeName = std::string(seriesname) + " - all structures";
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
        }

        // put the new node in the hierarchy
        vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        this->GetMRMLScene()->AddNode(modelHierarchyNode);
        modelHierarchyNode->SetParentNodeID( modelHierarchyRootNode->GetID() );
        modelHierarchyNode->SetModelNodeID( addedDisplayableNode->GetID() );
      }
    }

    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
    loadedSomething=true;
  }

  if (loadingErrorsOccurred)
  {
    return false;
  }
  return loadedSomething;
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
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
  displayNode->SetModifiedSinceRead(1); 
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn(); 
  displayNode->SetColor(roiColor[0], roiColor[1], roiColor[2]);

  // Disable backface culling to make the back side of the contour visible as well
  displayNode->SetBackfaceCulling(0);

  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(modelNode));
  modelNode->SetName(roiLabel);
  modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  modelNode->SetAndObservePolyData(roiPoly);
  modelNode->SetModifiedSinceRead(1);
  modelNode->SetHideFromEditors(0);
  modelNode->SetSelectable(1);

  return modelNode;
}
