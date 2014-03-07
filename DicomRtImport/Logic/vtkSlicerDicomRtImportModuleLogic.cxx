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

// DicomRtImport includes
#include "vtkSlicerDicomRtImportModuleLogic.h"
#include "vtkSlicerDicomRtReader.h"
#include "vtkDICOMImportInfo.h"
#include "vtkTopologicalHierarchy.h"

// SubjectHierarchy includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLBeamsNode.h"
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"
#include "vtkSlicerPlanarImageModuleLogic.h"
#include "vtkMRMLPlanarImageNode.h"

// Slicer Logic includes
#include "vtkSlicerVolumesLogic.h"

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
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLLinearTransformNode.h>

// Markups includes
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkImageCast.h>
#include <vtkStringArray.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportModuleLogic, VolumesLogic, vtkSlicerVolumesLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportModuleLogic, IsodoseLogic, vtkSlicerIsodoseModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportModuleLogic, PlanarImageLogic, vtkSlicerPlanarImageModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportModuleLogic::vtkSlicerDicomRtImportModuleLogic()
{
  this->VolumesLogic = NULL;
  this->IsodoseLogic = NULL;
  this->PlanarImageLogic = NULL;

  this->AutoContourOpacityOn();
  this->BeamModelsInSeparateBranchOn();
  this->DefaultDoseColorTableNodeId = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportModuleLogic::~vtkSlicerDicomRtImportModuleLogic()
{
  this->SetVolumesLogic(NULL);
  this->SetIsodoseLogic(NULL);
  this->SetPlanarImageLogic(NULL);
  this->SetDefaultDoseColorTableNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->SetDefaultDoseColorTableNodeId(NULL);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::Examine(vtkDICOMImportInfo *importInfo)
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
      result = fileformat.loadFile(fileName.c_str(), EXS_Unknown);
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
      double confidence=0.9; // almost sure, it's not 1.0 to allow user modules to override this importer

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
      else if (sopClass == UID_RTImageStorage)
      {
        name+="RTIMAGE";
        OFString imageLabel;
        dataset->findAndGetOFString(DCM_RTImageLabel, imageLabel);
        if (!imageLabel.empty())
        {
          name+=std::string(": ")+imageLabel.c_str();
        }
      }
      /* not yet supported
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
     
      importInfo->InsertNextLoadable(loadableFileList, name.c_str(), tooltip.c_str(), warning.c_str(), selected, confidence);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadDicomRT(vtkDICOMImportInfo *loadInfo)
{
  bool loadSuccessful = false;
  if (!loadInfo || !loadInfo->GetLoadableFiles(0) || loadInfo->GetLoadableFiles(0)->GetNumberOfValues() < 1)
  {
    vtkErrorMacro("LoadDicomRT: Unable to load Dicom RT data due to invalid loadable information.");
    return loadSuccessful;
  }

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);
  std::cout << "Loading series '" << seriesName << "' from file '" << firstFileNameStr << "'" << std::endl;

  vtkSmartPointer<vtkSlicerDicomRtReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtReader>::New();
  rtReader->SetFileName(firstFileNameStr.c_str());
  rtReader->Update();

  // One series can contain composite information, e.g, an RTPLAN series can contain structure sets and plans as well
  // TODO: vtkSlicerDicomRtReader class does not support this yet

  // RTSTRUCT
  if (rtReader->GetLoadRTStructureSetSuccessful())
  {
    loadSuccessful = this->LoadRtStructureSet(rtReader, loadInfo);
  }

  // RTDOSE
  if (rtReader->GetLoadRTDoseSuccessful())
  {
    loadSuccessful = this->LoadRtDose(rtReader, loadInfo);
  }

  // RTPLAN
  if (rtReader->GetLoadRTPlanSuccessful())
  {
    loadSuccessful = this->LoadRtPlan(rtReader, loadInfo);
  }

  // RTIMAGE
  if (rtReader->GetLoadRTImageSuccessful())
  {
    loadSuccessful = this->LoadRtImage(rtReader, loadInfo);
  }

  return loadSuccessful;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtStructureSet(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo)
{
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> contourHierarchySeriesNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> structureModelHierarchyRootNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string shSeriesNodeName(seriesName);
  shSeriesNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
  shSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(shSeriesNodeName);
  std::string contourNodeName;
  std::string structureSetReferencedSeriesUid;

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Add color table node
  vtkSmartPointer<vtkMRMLColorTableNode> contourSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string contourSetColorTableNodeName;
  contourSetColorTableNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  contourSetColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(contourSetColorTableNodeName);
  contourSetColorTableNode->SetName(contourSetColorTableNodeName.c_str());
  contourSetColorTableNode->HideFromEditorsOff();
  contourSetColorTableNode->SetTypeToUser();
  contourSetColorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  this->GetMRMLScene()->AddNode(contourSetColorTableNode);

  // Add ROIs
  int numberOfRois = rtReader->GetNumberOfRois();
  contourSetColorTableNode->SetNumberOfColors(numberOfRois+2);
  contourSetColorTableNode->GetLookupTable()->SetTableRange(0,numberOfRois+1);
  contourSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_BACKGROUND, 0.0, 0.0, 0.0, 0.0); // Black background
  contourSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_INVALID,
    SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
    SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] ); // Color indicating invalid index

  vtkSmartPointer<vtkPolyDataCollection> roiCollection = vtkSmartPointer<vtkPolyDataCollection>::New();
  vtkSmartPointer<vtkCollection> displayNodeCollection = vtkSmartPointer<vtkCollection>::New();

  for (int internalROIIndex=0; internalROIIndex<numberOfRois; internalROIIndex++)
  {
    const char* roiLabel = rtReader->GetRoiName(internalROIIndex);
    double *roiColor = rtReader->GetRoiDisplayColor(internalROIIndex);
    vtkMRMLDisplayableNode* addedDisplayableNode = NULL;

    // Get structure
    vtkPolyData* roiPolyTemp = rtReader->GetRoiPolyData(internalROIIndex);
    if (roiPolyTemp == NULL)
    {
      vtkWarningMacro("LoadRtStructureSet: Invalid ROI contour data for ROI named '"
        << (roiLabel?roiLabel:"Unnamed") << "' in file '" << firstFileNameStr
        << "' (internal ROI index: " << internalROIIndex << ")");
      continue;
    }
    if (roiPolyTemp->GetNumberOfPoints() <= 0)
    {
      vtkWarningMacro("LoadRtStructureSet: ROI contour data does not contain any points for ROI named '"
        << (roiLabel?roiLabel:"Unnamed") << "' in file '" << firstFileNameStr
        << "' (internal ROI index: " << internalROIIndex << ")");
      continue;
    }

    // Make a copy the poly data for further use (set the original points before ribbonization to the contour)
    vtkSmartPointer<vtkPolyData> roiPolyData = vtkSmartPointer<vtkPolyData>::New();
    roiPolyData->DeepCopy(roiPolyTemp);

    // Get referenced series UID
    const char* roiReferencedSeriesUid = rtReader->GetRoiReferencedSeriesUid(internalROIIndex);
    if (structureSetReferencedSeriesUid.empty())
    {
      structureSetReferencedSeriesUid = std::string(roiReferencedSeriesUid);
    }
    else if (roiReferencedSeriesUid && STRCASECMP(structureSetReferencedSeriesUid.c_str(), roiReferencedSeriesUid))
    {
      vtkWarningMacro("LoadRtStructureSet: ROIs in structure set '" << seriesName << "' have different referenced series UIDs!");
    }

    // Save color into the color table
    contourSetColorTableNode->AddColor(roiLabel, roiColor[0], roiColor[1], roiColor[2]);

    if (roiPolyData->GetNumberOfPoints() == 1)
    {
      // Point ROI
      addedDisplayableNode = this->AddRoiPoint(roiPolyData->GetPoint(0), roiLabel, roiColor);
    }
    else
    {
      // Contour ROI
      contourNodeName = std::string(roiLabel) + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
      contourNodeName = this->GetMRMLScene()->GenerateUniqueName(contourNodeName);

      // Create ribbon from ROI contour
      vtkSmartPointer<vtkPolyData> ribbonModelPolyData = vtkSmartPointer<vtkPolyData>::New();
      rtReader->CreateRibbonModelForRoi(internalROIIndex, ribbonModelPolyData);
      addedDisplayableNode = this->AddRoiContour(ribbonModelPolyData, contourNodeName, roiColor);

      roiCollection->AddItem(ribbonModelPolyData);
    }

    // Add new node to the model hierarchy
    if (addedDisplayableNode)
    {
      // Tag the representation as such
      addedDisplayableNode->SetAttribute(SlicerRtCommon::CONTOUR_REPRESENTATION_IDENTIFIER_ATTRIBUTE_NAME, "1");

      // Create root contour hierarchy node for the series, if it has not been created yet
      if (contourHierarchySeriesNode.GetPointer()==NULL)
      {
        std::string contourHierarchySeriesNodeName(seriesName);
        contourHierarchySeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX);
        contourHierarchySeriesNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
        contourHierarchySeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(contourHierarchySeriesNodeName);
        contourHierarchySeriesNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
        contourHierarchySeriesNode->SetName(contourHierarchySeriesNodeName.c_str());
        contourHierarchySeriesNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES);
        contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
        //TODO: If both point and contour can be found in the series, then 2 SubjectHierarchy nodes will be created with the same Series Instance UID!
        contourHierarchySeriesNode->AddUID(vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME,
          rtReader->GetSeriesInstanceUid());
        this->GetMRMLScene()->AddNode(contourHierarchySeriesNode);
      }

      if (roiPolyData->GetNumberOfPoints() == 1)
      {
        // Create subject hierarchy entry for the ROI
        vtkSmartPointer<vtkMRMLSubjectHierarchyNode> subjectHierarchyRoiNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
        std::string shNodeName;
        shNodeName = std::string(roiLabel) + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
        shNodeName = this->GetMRMLScene()->GenerateUniqueName(shNodeName);
        subjectHierarchyRoiNode->SetName(shNodeName.c_str());
        subjectHierarchyRoiNode->SetAssociatedNodeID(addedDisplayableNode->GetID());
        subjectHierarchyRoiNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);
        subjectHierarchyRoiNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(),
          roiReferencedSeriesUid);
        subjectHierarchyRoiNode->SetParentNodeID( contourHierarchySeriesNode->GetID() );
        this->GetMRMLScene()->AddNode(subjectHierarchyRoiNode);
      }
      else
      {
        // Create contour node
        vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
        contourNode = vtkMRMLContourNode::SafeDownCast(this->GetMRMLScene()->AddNode(contourNode));
        contourNode->SetName(contourNodeName.c_str());
        contourNode->SetAndObserveRibbonModelNodeId(addedDisplayableNode->GetID());
        contourNode->SetDicomRtRoiPoints(roiPolyData);
        contourNode->HideFromEditorsOff();
        std::map<double, vtkSmartPointer<vtkPlane> > orderedPlanes = rtReader->GetRoiOrderedContourPlanes(internalROIIndex);
        if( orderedPlanes.empty() )
        {
          vtkErrorMacro("Unable to retrieve ordered planes when creating contour node. They will be unaccessible.");
        }
        contourNode->SetOrderedContourPlanes(orderedPlanes);

        // Put the contour node in the hierarchy
        vtkSmartPointer<vtkMRMLSubjectHierarchyNode> contourSubjectHierarchyNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
        std::string shContourNodeName(contourNodeName);
        shContourNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
        shContourNodeName = this->GetMRMLScene()->GenerateUniqueName(shContourNodeName);
        contourSubjectHierarchyNode->SetName(shContourNodeName.c_str());
        contourSubjectHierarchyNode->SetParentNodeID( contourHierarchySeriesNode->GetID() );
        contourSubjectHierarchyNode->SetAssociatedNodeID( contourNode->GetID() );
        contourSubjectHierarchyNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);
        contourSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(),
          roiReferencedSeriesUid);
        contourSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(),
          roiLabel);
        this->GetMRMLScene()->AddNode(contourSubjectHierarchyNode);

        displayNodeCollection->AddItem( vtkMRMLModelNode::SafeDownCast(addedDisplayableNode)->GetModelDisplayNode() );

        // Create root model hierarchy node, if it has not been created yet
        if (structureModelHierarchyRootNode.GetPointer()==NULL)
        {
          structureModelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
          std::string rootModelHierarchyNodeName;
          rootModelHierarchyNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
          rootModelHierarchyNodeName = this->GetMRMLScene()->GenerateUniqueName(rootModelHierarchyNodeName);
          structureModelHierarchyRootNode->SetName(rootModelHierarchyNodeName.c_str());
          this->GetMRMLScene()->AddNode(structureModelHierarchyRootNode);

          // Create display node for the model hierarchy node
          vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
          rootModelHierarchyNodeName.append("Display");
          modelDisplayNode->SetName(rootModelHierarchyNodeName.c_str());
          modelDisplayNode->SetVisibility(1);
          this->GetMRMLScene()->AddNode(modelDisplayNode);
          structureModelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );
        }

        // Put the new node in the model hierarchy
        vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        std::string modelHierarchyNodeName;
        modelHierarchyNodeName = contourNodeName + "_ModelHierarchy";
        modelHierarchyNode->SetName(modelHierarchyNodeName.c_str());
        this->GetMRMLScene()->AddNode(modelHierarchyNode);
        modelHierarchyNode->SetParentNodeID( structureModelHierarchyRootNode->GetID() );
        modelHierarchyNode->SetModelNodeID( addedDisplayableNode->GetID() );
      }
    }
  }

  // Add reference from contour set to color table
  contourHierarchySeriesNode->SetNodeReferenceID(SlicerRtCommon::CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE, contourSetColorTableNode->GetID());

  // Set referenced series UID to the series subject hierarchy node
  contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(), structureSetReferencedSeriesUid.c_str());

  // Insert series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);


  // Set opacities according to topological hierarchy levels
  if (this->AutoContourOpacity)
  {
    if (roiCollection->GetNumberOfItems() == displayNodeCollection->GetNumberOfItems())
    {
      vtkSmartPointer<vtkTopologicalHierarchy> topologicalHierarchy = vtkSmartPointer<vtkTopologicalHierarchy>::New();
      topologicalHierarchy->SetInputPolyDataCollection(roiCollection);
      topologicalHierarchy->Update();
      vtkIntArray* levels = topologicalHierarchy->GetOutputLevels();

      int numberOfLevels = 0;
      for (int i=0; i<levels->GetNumberOfTuples(); ++i)
      {
        if (levels->GetValue(i) > numberOfLevels)
        {
          numberOfLevels = levels->GetValue(i);
        }
      }

      for (int i=0; i<roiCollection->GetNumberOfItems(); ++i)
      {
        int level = levels->GetValue(i);
        vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(
          displayNodeCollection->GetItemAsObject(i) );
        if (displayNode)
        {
          // The opacity level is set evenly distributed between 0 and 1 (excluding 0)
          // according to the topological hierarchy level of the contour
          displayNode->SetOpacity( 1.0 - ((double)level) / (numberOfLevels+1) );
        }
      }
    }
    else
    {
      vtkWarningMacro("LoadRtStructureSet: Unable to auto-determine opacity: Number of ROIs and display nodes do not match!");
    }
  }

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);

  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtDose(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo)
{
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> subjectHierarchySeriesNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string shSeriesNodeName(seriesName);
  shSeriesNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
  shSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(shSeriesNodeName);

  // Load Volume
  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> volumeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  vtkSmartPointer<vtkMRMLScalarVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  volumeStorageNode->SetFileName(firstFileNameStr.c_str());
  volumeStorageNode->ResetFileNameList();
  for (int fileIndex=0; fileIndex<loadInfo->GetLoadableFiles(0)->GetNumberOfValues(); ++fileIndex)
  {
    volumeStorageNode->AddFileName(loadInfo->GetLoadableFiles(0)->GetValue(fileIndex).c_str());
  }
  volumeStorageNode->SetSingleFile(0);

  // Read volume from disk
  if (!volumeStorageNode->ReadData(volumeNode))
  {
    vtkErrorMacro("LoadRtDose: Failed to load dose volume file '" << firstFileNameStr << "' (series name '" << seriesName << "')");
    return false;
  }

  volumeNode->SetScene(this->GetMRMLScene());
  std::string volumeNodeName = this->GetMRMLScene()->GenerateUniqueName(seriesName);
  volumeNode->SetName(volumeNodeName.c_str());
  this->GetMRMLScene()->AddNode(volumeNode);

  // Set new spacing
  double* initialSpacing = volumeNode->GetSpacing();
  double* correctSpacing = rtReader->GetPixelSpacing();
  volumeNode->SetSpacing(correctSpacing[0], correctSpacing[1], initialSpacing[2]);
  volumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Apply dose grid scaling
  vtkSmartPointer<vtkImageData> floatVolumeData = vtkSmartPointer<vtkImageData>::New();

  vtkSmartPointer<vtkImageCast> imageCast = vtkSmartPointer<vtkImageCast>::New();
  imageCast->SetInput(volumeNode->GetImageData());
  imageCast->SetOutputScalarTypeToFloat();
  imageCast->Update();
  floatVolumeData->DeepCopy(imageCast->GetOutput());

  std::stringstream ss;
  ss << rtReader->GetDoseGridScaling();
  double doubleValue;
  ss >> doubleValue;
  double doseGridScaling = doubleValue;

  float value = 0.0;
  float* floatPtr = (float*)floatVolumeData->GetScalarPointer();
  for (long i=0; i<floatVolumeData->GetNumberOfPoints(); ++i)
  {
    value = (*floatPtr) * doseGridScaling;
    (*floatPtr) = value;
    ++floatPtr;
  }

  volumeNode->SetAndObserveImageData(floatVolumeData);      

  // Create dose color table from default isodose color table
  if (!this->DefaultDoseColorTableNodeId)
  {
    this->CreateDefaultDoseColorTable();
    if (!this->DefaultDoseColorTableNodeId)
    {
      this->SetDefaultDoseColorTableNodeId("vtkMRMLColorTableNodeRainbow");
    }
  }

  vtkMRMLColorTableNode* defaultIsodoseColorTable = vtkMRMLColorTableNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseLogic->GetDefaultIsodoseColorTableNodeId()) );

  // Create isodose parameter set node and set color table to default
  std::string isodoseParameterSetNodeName;
  isodoseParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
    SlicerRtCommon::ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX + volumeNodeName );
  vtkSmartPointer<vtkMRMLIsodoseNode> isodoseParameterSetNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
  isodoseParameterSetNode->SetName(isodoseParameterSetNodeName.c_str());
  isodoseParameterSetNode->SetAndObserveDoseVolumeNode(volumeNode);
  if (this->IsodoseLogic && defaultIsodoseColorTable)
  {
    isodoseParameterSetNode->SetAndObserveColorTableNode(defaultIsodoseColorTable);
  }
  this->GetMRMLScene()->AddNode(isodoseParameterSetNode);

  //TODO: Generate isodose surfaces if chosen so by the user in the hanging protocol options

  // Set default colormap to the loaded one if found or generated, or to rainbow otherwise
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  volumeDisplayNode->SetAndObserveColorNodeID(this->DefaultDoseColorTableNodeId);
  this->GetMRMLScene()->AddNode(volumeDisplayNode);
  volumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

  // Set window/level to match the isodose levels
  if (this->IsodoseLogic && defaultIsodoseColorTable)
  {
    std::stringstream ssMin;
    ssMin << defaultIsodoseColorTable->GetColorName(0);;
    int minDoseInDefaultIsodoseLevels;
    ssMin >> minDoseInDefaultIsodoseLevels;

    std::stringstream ssMax;
    ssMax << defaultIsodoseColorTable->GetColorName(defaultIsodoseColorTable->GetNumberOfColors()-1);
    int maxDoseInDefaultIsodoseLevels;
    ssMax >> maxDoseInDefaultIsodoseLevels;

    volumeDisplayNode->AutoWindowLevelOff();
    volumeDisplayNode->SetWindowLevelMinMax(minDoseInDefaultIsodoseLevels, maxDoseInDefaultIsodoseLevels);
  }

  // Set display threshold
  double doseUnitScaling = 0.0;
  std::stringstream doseUnitScalingSs;
  doseUnitScalingSs << rtReader->GetDoseGridScaling();
  doseUnitScalingSs >> doseUnitScaling;
  volumeDisplayNode->AutoThresholdOff();
  volumeDisplayNode->SetLowerThreshold(0.5 * doseUnitScaling);
  volumeDisplayNode->SetApplyThreshold(1);

  // Create subject hierarchy entry
  subjectHierarchySeriesNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
  subjectHierarchySeriesNode->SetAssociatedNodeID(volumeNode->GetID());
  subjectHierarchySeriesNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES);
  subjectHierarchySeriesNode->AddUID(vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME,
    rtReader->GetSeriesInstanceUid());
  subjectHierarchySeriesNode->SetName(shSeriesNodeName.c_str());
  this->GetMRMLScene()->AddNode(subjectHierarchySeriesNode);

  // Insert series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Set dose unit attributes to subject hierarchy study node
  vtkMRMLSubjectHierarchyNode* studyHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    subjectHierarchySeriesNode->GetParentNode() );
  if (!studyHierarchyNode)
  {
    vtkErrorMacro("LoadRtDose: Unable to get parent study hierarchy node for dose volume '" << volumeNode->GetName() << "'");
  }
  else
  {
    const char* existingDoseUnitName = studyHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());
    if (!rtReader->GetDoseUnits())
    {
      vtkErrorMacro("LoadRtDose: Empty dose unit name found for dose volume " << volumeNode->GetName());
    }
    else if (existingDoseUnitName && STRCASECMP(existingDoseUnitName, rtReader->GetDoseUnits()))
    {
      vtkErrorMacro("LoadRtDose: Dose unit name already exists (" << existingDoseUnitName << ") for study and differs from current one (" << rtReader->GetDoseUnits() << ")!");
    }
    else
    {
      studyHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), rtReader->GetDoseUnits());
    }

    const char* existingDoseUnitValueChars = studyHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str());
    if (!rtReader->GetDoseGridScaling())
    {
      vtkErrorMacro("LoadRtDose: Empty dose unit value found for dose volume " << volumeNode->GetName());
    }
    else if (existingDoseUnitValueChars)
    {
      double existingDoseUnitValue = 0.0;
      {
        std::stringstream ss;
        ss << existingDoseUnitValueChars;
        ss >> existingDoseUnitValue;
      }
      double currentDoseUnitValue = 0.0;
      {
        std::stringstream ss;
        ss << rtReader->GetDoseGridScaling();
        ss >> currentDoseUnitValue;
      }
      if (fabs(existingDoseUnitValue - currentDoseUnitValue) > EPSILON)
      {
        vtkErrorMacro("LoadRtDose: Dose unit value already exists (" << existingDoseUnitValue << ") for study and differs from current one (" << currentDoseUnitValue << ")!");
      }
    }
    else
    {
      studyHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str(), rtReader->GetDoseGridScaling());
    }
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

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtPlan(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo)
{
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> isocenterSeriesHierarchyRootNode;
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> beamModelSubjectHierarchyRootNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyRootNode;

  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string shSeriesNodeName(seriesName);
  shSeriesNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
  shSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(shSeriesNodeName);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  vtkMRMLMarkupsFiducialNode* addedMarkupsNode = NULL;
  int numberOfBeams = rtReader->GetNumberOfBeams();
  for (int beamIndex = 0; beamIndex < numberOfBeams; beamIndex++) // DICOM starts indexing from 1
  {
    unsigned int dicomBeamNumber = rtReader->GetBeamNumberForIndex(beamIndex);

    // Isocenter fiducial
    double isoColor[3] = { 1.0, 1.0, 1.0 };
    addedMarkupsNode = this->AddRoiPoint(rtReader->GetBeamIsocenterPositionRas(dicomBeamNumber), rtReader->GetBeamName(dicomBeamNumber), isoColor);

    // Add new node to the hierarchy node
    if (addedMarkupsNode)
    {
      // Create root isocenter hierarchy node for the plan series, if it has not been created yet
      if (isocenterSeriesHierarchyRootNode.GetPointer()==NULL)
      {
        isocenterSeriesHierarchyRootNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
        isocenterSeriesHierarchyRootNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES);
        isocenterSeriesHierarchyRootNode->AddUID(vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME,
          rtReader->GetSeriesInstanceUid());
        isocenterSeriesHierarchyRootNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str(),
          rtReader->GetSOPInstanceUID());
        std::string isocenterHierarchyRootNodeName;
        isocenterHierarchyRootNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
        isocenterHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(isocenterHierarchyRootNodeName);
        isocenterSeriesHierarchyRootNode->SetName(isocenterHierarchyRootNodeName.c_str());
        this->GetMRMLScene()->AddNode(isocenterSeriesHierarchyRootNode);
      }

      // Create beam model hierarchy root node if has not been created yet
      if (beamModelHierarchyRootNode.GetPointer()==NULL)
      {
        beamModelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        std::string beamModelHierarchyRootNodeName;
        beamModelHierarchyRootNodeName = std::string(seriesName)
          + SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX;
        beamModelHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(beamModelHierarchyRootNodeName);
        beamModelHierarchyRootNode->SetName(beamModelHierarchyRootNodeName.c_str());
        this->GetMRMLScene()->AddNode(beamModelHierarchyRootNode);

        // Create display node for the hierarchy node
        vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelHierarchyRootDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
        std::string beamModelHierarchyRootDisplayNodeName = beamModelHierarchyRootNodeName + std::string("Display");
        beamModelHierarchyRootDisplayNode->SetName(beamModelHierarchyRootDisplayNodeName.c_str());
        beamModelHierarchyRootDisplayNode->SetVisibility(1);
        this->GetMRMLScene()->AddNode(beamModelHierarchyRootDisplayNode);
        beamModelHierarchyRootNode->SetAndObserveDisplayNodeID( beamModelHierarchyRootDisplayNode->GetID() );

        // Create a subject hierarchy node if the separate branch flag is on
        if (this->BeamModelsInSeparateBranch)
        {
          beamModelSubjectHierarchyRootNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
          std::string beamModelSubjectHierarchyRootNodeName;
          beamModelSubjectHierarchyRootNodeName = beamModelHierarchyRootNodeName
            + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
          beamModelSubjectHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(beamModelSubjectHierarchyRootNodeName);
          beamModelSubjectHierarchyRootNode->SetName(beamModelSubjectHierarchyRootNodeName.c_str());
          beamModelSubjectHierarchyRootNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);
          this->GetMRMLScene()->AddNode(beamModelSubjectHierarchyRootNode);
        }
      }

      // Create subject hierarchy entry for the isocenter fiducial
      vtkSmartPointer<vtkMRMLSubjectHierarchyNode> subjectHierarchyFiducialNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
      std::string shFiducialNodeName(rtReader->GetBeamName(dicomBeamNumber));
      shFiducialNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
      subjectHierarchyFiducialNode->SetName(shFiducialNodeName.c_str());
      subjectHierarchyFiducialNode->SetAssociatedNodeID(addedMarkupsNode->GetID());
      subjectHierarchyFiducialNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);

      // Set beam related attributes
      std::stringstream beamNumberStream;
      beamNumberStream << dicomBeamNumber;
      subjectHierarchyFiducialNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str(),
        beamNumberStream.str().c_str());
      subjectHierarchyFiducialNode->SetParentNodeID(isocenterSeriesHierarchyRootNode->GetID());
      this->GetMRMLScene()->AddNode(subjectHierarchyFiducialNode);

      // Add attributes containing beam information to the isocenter fiducial node
      std::stringstream sourceAxisDistanceStream;
      sourceAxisDistanceStream << rtReader->GetBeamSourceAxisDistance(dicomBeamNumber);
      subjectHierarchyFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str(),
        sourceAxisDistanceStream.str().c_str() );

      std::stringstream gantryAngleStream;
      gantryAngleStream << rtReader->GetBeamGantryAngle(dicomBeamNumber);
      subjectHierarchyFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str(),
        gantryAngleStream.str().c_str() );

      std::stringstream couchAngleStream;
      couchAngleStream << rtReader->GetBeamPatientSupportAngle(dicomBeamNumber);
      subjectHierarchyFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME.c_str(),
        couchAngleStream.str().c_str() );

      std::stringstream collimatorAngleStream;
      collimatorAngleStream << rtReader->GetBeamBeamLimitingDeviceAngle(dicomBeamNumber);
      subjectHierarchyFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str(),
        collimatorAngleStream.str().c_str() );

      std::stringstream jawPositionsStream;
      double jawPositions[2][2] = {{0.0, 0.0},{0.0, 0.0}};
      rtReader->GetBeamLeafJawPositions(dicomBeamNumber, jawPositions);
      jawPositionsStream << jawPositions[0][0] << " " << jawPositions[0][1] << " "
        << jawPositions[1][0] << " " << jawPositions[1][1];
      subjectHierarchyFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME.c_str(),
        jawPositionsStream.str().c_str() );

      // Set isocenter fiducial name
      std::string isocenterFiducialName = std::string(rtReader->GetBeamName(dicomBeamNumber)) + SlicerRtCommon::BEAMS_OUTPUT_ISOCENTER_FIDUCIAL_POSTFIX;
      addedMarkupsNode->SetNthFiducialLabel(0, isocenterFiducialName);

      // Add source fiducial in the isocenter markups node
      std::string sourceFiducialName = std::string(rtReader->GetBeamName(dicomBeamNumber)) + SlicerRtCommon::BEAMS_OUTPUT_SOURCE_FIDUCIAL_POSTFIX;
      addedMarkupsNode->AddFiducial(0.0, 0.0, 0.0);
      addedMarkupsNode->SetNthFiducialLabel(1, sourceFiducialName);

      // Create beam model node and add it to the scene
      std::string beamModelName;
      beamModelName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX + std::string(addedMarkupsNode->GetName()) );
      vtkSmartPointer<vtkMRMLModelNode> beamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      beamModelNode->SetName(beamModelName.c_str());
      this->GetMRMLScene()->AddNode(beamModelNode);

      // Create Beams parameter set node
      std::string beamParameterSetNodeName;
      beamParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_PARAMETER_SET_BASE_NAME_PREFIX + std::string(addedMarkupsNode->GetName()) );
      vtkSmartPointer<vtkMRMLBeamsNode> beamParameterSetNode = vtkSmartPointer<vtkMRMLBeamsNode>::New();
      beamParameterSetNode->SetName(beamParameterSetNodeName.c_str());
      beamParameterSetNode->SetAndObserveIsocenterMarkupsNode(addedMarkupsNode);
      beamParameterSetNode->SetAndObserveBeamModelNode(beamModelNode);
      this->GetMRMLScene()->AddNode(beamParameterSetNode);

      // Create beam geometry
      vtkSmartPointer<vtkSlicerBeamsModuleLogic> beamsLogic = vtkSmartPointer<vtkSlicerBeamsModuleLogic>::New();
      beamsLogic->SetAndObserveBeamsNode(beamParameterSetNode);
      beamsLogic->SetAndObserveMRMLScene(this->GetMRMLScene());
      std::string errorMessage;
      beamsLogic->CreateBeamModel(errorMessage);
      if (!errorMessage.empty())
      {
        vtkWarningMacro("LoadRtPlan: Failed to create beam geometry for isocenter: " << addedMarkupsNode->GetName());
      }

      // Put new beam model in the model hierarchy
      vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      beamModelHierarchyNode->SetName(beamModelName.c_str());
      beamModelHierarchyNode->SetDisplayableNodeID(beamModelNode->GetID());
      beamModelHierarchyNode->SetParentNodeID(beamModelHierarchyRootNode->GetID());
      this->GetMRMLScene()->AddNode(beamModelHierarchyNode);
      beamModelHierarchyNode->SetIndexInParent(beamIndex);

      // Put new beam model in the subject hierarchy
      vtkSmartPointer<vtkMRMLSubjectHierarchyNode> beamModelSubjectHierarchyNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
      std::string shBeamNodeName = beamModelName + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
      beamModelSubjectHierarchyNode->SetName(shBeamNodeName.c_str());
      beamModelSubjectHierarchyNode->SetAssociatedNodeID(beamModelHierarchyNode->GetID());
      if (this->BeamModelsInSeparateBranch)
      {
        beamModelSubjectHierarchyNode->SetParentNodeID(beamModelSubjectHierarchyRootNode->GetID());
      }
      else
      {
        beamModelSubjectHierarchyNode->SetParentNodeID(subjectHierarchyFiducialNode->GetID());
      }
      beamModelSubjectHierarchyNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);
      this->GetMRMLScene()->AddNode(beamModelSubjectHierarchyNode);
      beamModelSubjectHierarchyNode->SetIndexInParent(beamIndex);

      // Compute and set geometry of possible RT image that references the loaded beam.
      // Uses the referenced RT image if available, otherwise the geometry will be set up when loading the corresponding RT image
      this->SetupRtImageGeometry(addedMarkupsNode);

    } //endif addedDisplayableNode
  }

  // Insert plan isocenter series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Insert beam model subseries under the study if the separate branch flag is on
  vtkMRMLSubjectHierarchyNode* studyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    isocenterSeriesHierarchyRootNode->GetParentNode() );
  if (studyNode && this->BeamModelsInSeparateBranch)
  {
    beamModelSubjectHierarchyRootNode->SetParentNodeID(studyNode->GetID());
  }

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtImage(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo* loadInfo)
{
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> subjectHierarchySeriesNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  if (loadInfo->GetNumberOfLoadables() > 1 || loadInfo->GetLoadableFiles(0)->GetNumberOfValues() > 1)
  {
    vtkErrorMacro("LoadRtImage: Only one loadable and one file name is allowed for RT Image series '" << seriesName << "')");
    return false;
  }

  std::string shSeriesNodeName(seriesName);
  shSeriesNodeName.append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
  shSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(shSeriesNodeName);

  // Load Volume
  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> volumeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  vtkSmartPointer<vtkMRMLScalarVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  volumeStorageNode->SetFileName(firstFileNameStr.c_str());
  volumeStorageNode->ResetFileNameList();
  volumeStorageNode->SetSingleFile(1);

  // Read image from disk
  if (!volumeStorageNode->ReadData(volumeNode))
  {
    vtkErrorMacro("LoadRtImage: Failed to load RT image file '" << firstFileNameStr << "' (series name '" << seriesName << "')");
    return false;
  }

  volumeNode->SetScene(this->GetMRMLScene());
  std::string volumeNodeName = this->GetMRMLScene()->GenerateUniqueName(seriesName);
  volumeNode->SetName(volumeNodeName.c_str());
  this->GetMRMLScene()->AddNode(volumeNode);

  // Create display node for the volume
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  this->GetMRMLScene()->AddNode(volumeDisplayNode);
  volumeDisplayNode->SetDefaultColorMap();
  if (rtReader->GetWindowCenter() == 0.0 && rtReader->GetWindowWidth() == 0.0)
  {
    volumeDisplayNode->AutoWindowLevelOn();
  }
  else
  {
    // Apply given window level if available
    volumeDisplayNode->AutoWindowLevelOff();
    volumeDisplayNode->SetWindowLevel(rtReader->GetWindowWidth(), rtReader->GetWindowCenter());
  }
  volumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

  // Create subject hierarchy entry
  subjectHierarchySeriesNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
  subjectHierarchySeriesNode->SetAssociatedNodeID(volumeNode->GetID());
  subjectHierarchySeriesNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES);
  subjectHierarchySeriesNode->AddUID( vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME,
    rtReader->GetSeriesInstanceUid() );

  // Set RT image specific attributes
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str(),
    rtReader->GetReferencedRTPlanSOPInstanceUID());

  std::stringstream radiationMachineSadStream;
  radiationMachineSadStream << rtReader->GetRadiationMachineSAD();
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str(),
    radiationMachineSadStream.str().c_str());

  std::stringstream gantryAngleStream;
  gantryAngleStream << rtReader->GetGantryAngle();
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str(),
    gantryAngleStream.str().c_str());

  std::stringstream couchAngleStream;
  couchAngleStream << rtReader->GetPatientSupportAngle();
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME.c_str(),
    couchAngleStream.str().c_str());

  std::stringstream collimatorAngleStream;
  collimatorAngleStream << rtReader->GetBeamLimitingDeviceAngle();
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str(),
    collimatorAngleStream.str().c_str());

  std::stringstream referencedBeamNumberStream;
  referencedBeamNumberStream << rtReader->GetReferencedBeamNumber();
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str(),
    referencedBeamNumberStream.str().c_str());

  std::stringstream rtImageSidStream;
  rtImageSidStream << rtReader->GetRTImageSID();
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME.c_str(),
    rtImageSidStream.str().c_str());

  std::stringstream rtImagePositionStream;
  double rtImagePosition[2] = {0.0, 0.0};
  rtReader->GetRTImagePosition(rtImagePosition);
  rtImagePositionStream << rtImagePosition[0] << " " << rtImagePosition[1];
  subjectHierarchySeriesNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME.c_str(),
    rtImagePositionStream.str().c_str() );

  subjectHierarchySeriesNode->SetName(shSeriesNodeName.c_str());
  this->GetMRMLScene()->AddNode(subjectHierarchySeriesNode);

  // Insert series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Compute and set RT image geometry. Uses the referenced beam if available, otherwise the geometry will be set up when loading the referenced beam
  this->SetupRtImageGeometry(volumeNode);

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkSlicerDicomRtImportModuleLogic::AddRoiPoint(double* roiPosition, std::string baseName, double* roiColor)
{
  std::string fiducialNodeName = this->GetMRMLScene()->GenerateUniqueName(baseName);
  vtkSmartPointer<vtkMRMLMarkupsFiducialNode> markupsNode = vtkSmartPointer<vtkMRMLMarkupsFiducialNode>::New();
  this->GetMRMLScene()->AddNode(markupsNode);
  markupsNode->SetName(baseName.c_str());
  markupsNode->AddFiducialFromArray(roiPosition);
  markupsNode->SetLocked(1);

  vtkSmartPointer<vtkMRMLMarkupsDisplayNode> markupsDisplayNode = vtkSmartPointer<vtkMRMLMarkupsDisplayNode>::New();
  this->GetMRMLScene()->AddNode(markupsDisplayNode);
  markupsDisplayNode->SetGlyphType(vtkMRMLMarkupsDisplayNode::Sphere3D);
  markupsDisplayNode->SetColor(roiColor);
  markupsNode->SetAndObserveDisplayNodeID(markupsDisplayNode->GetID());

  // Hide the fiducial by default
  markupsNode->SetDisplayVisibility(0);

  return markupsNode;
}

//---------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerDicomRtImportModuleLogic::AddRoiContour(vtkPolyData* roiPoly, std::string baseName, double* roiColor)
{
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn(); 
  displayNode->SetColor(roiColor[0], roiColor[1], roiColor[2]);

  // Disable backface culling to make the back side of the contour visible as well
  displayNode->SetBackfaceCulling(0);

  std::string modelNodeName = baseName + SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX;
  modelNodeName = this->GetMRMLScene()->GenerateUniqueName(modelNodeName);

  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(modelNode));
  modelNode->SetName(modelNodeName.c_str());
  modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  modelNode->SetAndObservePolyData(roiPoly);
  modelNode->SetHideFromEditors(0);
  modelNode->SetSelectable(1);

  return modelNode;
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::InsertSeriesInSubjectHierarchy( vtkSlicerDicomRtReader* rtReader )
{
  // Get the higher level parent nodes by their IDs (to fill their attributes later if they do not exist yet)
  vtkMRMLHierarchyNode* patientNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    this->GetMRMLScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, rtReader->GetPatientId() );
  vtkMRMLHierarchyNode* studyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    this->GetMRMLScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, rtReader->GetStudyInstanceUid() );

  // Insert series in hierarchy
  vtkMRMLHierarchyNode* seriesNode = vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
    this->GetMRMLScene(), rtReader->GetPatientId(), rtReader->GetStudyInstanceUid(), rtReader->GetSeriesInstanceUid() );

  // Fill patient and study attributes if they have been just created
  if (patientNode == NULL)
  {
    patientNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
      this->GetMRMLScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, rtReader->GetPatientId() );
    if (patientNode)
    {
      std::string patientNodeName = ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetPatientName())
        ? std::string(rtReader->GetPatientName()) + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX
        : SlicerRtCommon::DICOMRTIMPORT_NO_NAME + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX );
      patientNode->SetName( patientNodeName.c_str() );
    }
    else
    {
      vtkErrorMacro("InsertSeriesInSubjectHierarchy: Patient node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }
  if (studyNode == NULL)
  {
    studyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
      this->GetMRMLScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, rtReader->GetStudyInstanceUid() );
    if (studyNode)
    {
      std::string studyNodeName = ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetStudyDescription())
        ? std::string(rtReader->GetStudyDescription()) + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX
        : SlicerRtCommon::DICOMRTIMPORT_NO_DESCRIPTION + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX );
      studyNode->SetName( studyNodeName.c_str() );
    }
    else
    {
      vtkErrorMacro("InsertSeriesInSubjectHierarchy: Study node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }
  if (seriesNode == NULL)
  {
    vtkErrorMacro("InsertSeriesInSubjectHierarchy: Failed to insert series with Instance UID "
      << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    return;
  }
  else
  {
    // Set DICOM tags to the hierarchy node
    seriesNode->SetAttribute( vtkSubjectHierarchyConstants::DICOMHIERARCHY_SERIES_MODALITY_ATTRIBUTE_NAME.c_str(), rtReader->GetSeriesModality() );
    seriesNode->SetAttribute( vtkSubjectHierarchyConstants::DICOMHIERARCHY_STUDY_DATE_ATTRIBUTE_NAME.c_str(), rtReader->GetStudyDate() );
    seriesNode->SetAttribute( vtkSubjectHierarchyConstants::DICOMHIERARCHY_STUDY_TIME_ATTRIBUTE_NAME.c_str(), rtReader->GetStudyTime() );
    seriesNode->SetAttribute( vtkSubjectHierarchyConstants::DICOMHIERARCHY_PATIENT_SEX_ATTRIBUTE_NAME.c_str(), rtReader->GetPatientSex() );
    seriesNode->SetAttribute( vtkSubjectHierarchyConstants::DICOMHIERARCHY_PATIENT_BIRTH_DATE_ATTRIBUTE_NAME.c_str(), rtReader->GetPatientBirthDate() );
  }

  // Handle special cases, make connections
  const char* modality = seriesNode->GetAttribute(vtkSubjectHierarchyConstants::DICOMHIERARCHY_SERIES_MODALITY_ATTRIBUTE_NAME.c_str());
  if (!modality)
  {
    vtkErrorMacro("InsertSeriesInSubjectHierarchy: Series '" << seriesNode->GetName() << "' has invalid modality attribute!");
  }
  // Put contour set under anatomical volume
  else if (!STRCASECMP(modality, "RTSTRUCT"))
  {
    const char* referencedSeriesUid = seriesNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str());
    if (referencedSeriesUid)
    {
      vtkSmartPointer<vtkMRMLSubjectHierarchyNode> referencedSeriesNode;
      vtkMRMLSubjectHierarchyNode* foundSubjectHierarchyNode =
        vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
          this->GetMRMLScene(), vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, referencedSeriesUid);
      if (foundSubjectHierarchyNode)
      {
        referencedSeriesNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::Take(foundSubjectHierarchyNode);
        referencedSeriesNode->Register(NULL);
      }
      else
      {
        // Create dummy anatomical volume node to put the contour set under. When the actual volume is loaded, it occupies the node
        referencedSeriesNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
        referencedSeriesNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES);
        referencedSeriesNode->AddUID( vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME,
          referencedSeriesUid);
        std::string referencedSeriesNodeName = SlicerRtCommon::CONTOURHIERARCHY_DUMMY_ANATOMICAL_VOLUME_NODE_NAME_PREFIX +
          vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
        referencedSeriesNode->SetName(referencedSeriesNodeName.c_str());
        this->GetMRMLScene()->AddNode(referencedSeriesNode);

        vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
          this->GetMRMLScene(), rtReader->GetPatientId(), rtReader->GetStudyInstanceUid(), referencedSeriesUid );
      }

      seriesNode->SetParentNodeID(referencedSeriesNode->GetID());
    }
    else
    {
      vtkErrorMacro("InsertSeriesInSubjectHierarchy: Contour set '" << seriesNode->GetName() << "' has no referenced series!");
    }
  }
}

//------------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::CreateDefaultDoseColorTable()
{
  if (!this->GetMRMLScene() || !this->IsodoseLogic)
  {
    vtkErrorMacro("CreateDefaultDoseColorTable: No scene or Isodose logic present!");
    return;
  }

  // Check if default color table node already exists
  vtkSmartPointer<vtkCollection> defaultDoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    this->GetMRMLScene()->GetNodesByName(SlicerRtCommon::DICOMRTIMPORT_DEFAULT_DOSE_COLOR_TABLE_NAME) );
  if (defaultDoseColorTableNodes->GetNumberOfItems() > 0)
  {
    vtkDebugMacro("CreateDefaultDoseColorTable: Default dose color table already exists");
    return;
  }

  vtkMRMLColorTableNode* defaultIsodoseColorTable = vtkMRMLColorTableNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseLogic->GetDefaultIsodoseColorTableNodeId()) );
  if (!defaultIsodoseColorTable)
  {
    vtkErrorMacro("CreateDefaultDoseColorTable: Invalid default isodose color table found in isodose logic!");
    return;
  }
  
  vtkSmartPointer<vtkMRMLColorTableNode> defaultDoseColorTable = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  defaultDoseColorTable->SetName(SlicerRtCommon::DICOMRTIMPORT_DEFAULT_DOSE_COLOR_TABLE_NAME);
  defaultDoseColorTable->SetTypeToUser();
  defaultDoseColorTable->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  defaultDoseColorTable->HideFromEditorsOn();
  defaultDoseColorTable->SetNumberOfColors(256);

  SlicerRtCommon::StretchDiscreteColorTable(defaultIsodoseColorTable, defaultDoseColorTable);

  this->GetMRMLScene()->AddNode(defaultDoseColorTable);
  this->SetDefaultDoseColorTableNodeId(defaultDoseColorTable->GetID());
}

//------------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::SetupRtImageGeometry(vtkMRMLNode* node)
{
  vtkMRMLScalarVolumeNode* rtImageVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  vtkMRMLSubjectHierarchyNode* rtImageSubjectHierarchyNode = NULL;
  vtkMRMLMarkupsFiducialNode* isocenterNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);
  vtkMRMLSubjectHierarchyNode* isocenterSubjectHierarchyNode = NULL;

  // If the function is called from the LoadRtImage function with an RT image volume
  if (rtImageVolumeNode)
  {
    // Get subject hierarchy node for RT image
    rtImageSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(rtImageVolumeNode);
    if (!rtImageSubjectHierarchyNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid subject hierarchy node for RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }

    // Find referenced RT plan node
    const char* referencedPlanSopInstanceUid = rtImageSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
    if (!referencedPlanSopInstanceUid)
    {
      vtkErrorMacro("SetupRtImageGeometry: Unable to find referenced plan SOP instance UID for RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }
    vtkMRMLSubjectHierarchyNode* rtPlanSubjectHierarchyNode = NULL;
    std::vector<vtkMRMLNode*> subjectHierarchyNodes;
    unsigned int numberOfNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLSubjectHierarchyNode", subjectHierarchyNodes);
    for (unsigned int shNodeIndex=0; shNodeIndex<numberOfNodes; shNodeIndex++)
    {
      vtkMRMLSubjectHierarchyNode* currentShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(subjectHierarchyNodes[shNodeIndex]);
      if (currentShNode)
      {
        const char* sopInstanceUid = currentShNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
        if (sopInstanceUid && !STRCASECMP(referencedPlanSopInstanceUid, sopInstanceUid))
        {
          rtPlanSubjectHierarchyNode = currentShNode;
        }
      }
    }
    if (!rtPlanSubjectHierarchyNode)
    {
      vtkDebugMacro("SetupRtImageGeometry: Cannot set up geometry of RT image '" << rtImageVolumeNode->GetName()
        << "' without the referenced RT plan. Will be set up upon loading the related plan");
      return;
    }

    // Get isocenters contained by the plan
    std::vector<vtkMRMLHierarchyNode*> isocenterSubjectHierarchyNodes =
      rtPlanSubjectHierarchyNode->GetChildrenNodes();

    // Get isocenter according to referenced beam number
    if (isocenterSubjectHierarchyNodes.size() == 1)
    {
      // If there is only one beam in the plan, then we don't need to search in the list
      isocenterSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(*(isocenterSubjectHierarchyNodes.begin()));
      isocenterNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(isocenterSubjectHierarchyNode->GetAssociatedDataNode());
    }
    else
    {
      // Get referenced beam number (string)
      const char* referencedBeamNumberChars = rtImageSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
      if (referencedBeamNumberChars)
      {
        for (std::vector<vtkMRMLHierarchyNode*>::iterator isocenterShIt = isocenterSubjectHierarchyNodes.begin(); isocenterShIt != isocenterSubjectHierarchyNodes.end(); ++isocenterShIt)
        {
          const char* isocenterBeamNumberChars = (*isocenterShIt)->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
          if (!isocenterBeamNumberChars)
          {
            continue;
          }
          if (!STRCASECMP(isocenterBeamNumberChars, referencedBeamNumberChars))
          {
            isocenterSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(*isocenterShIt);
            isocenterNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(isocenterSubjectHierarchyNode->GetAssociatedDataNode());
            break;
          }
        }
      }
    }
    if (!isocenterNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve isocenter node for RT image '" << rtImageVolumeNode->GetName() << "' in RT plan '" << rtPlanSubjectHierarchyNode->GetName() << "'!");
      return;
    }
  }
  // If the function is called from the LoadRtPlan function with an isocenter
  else if (isocenterNode)
  {
    // Get RT plan DICOM UID for isocenter
    isocenterSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(isocenterNode);
    if (!isocenterSubjectHierarchyNode || !isocenterSubjectHierarchyNode->GetParentNode())
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid subject hierarchy node for isocenter '" << isocenterNode->GetName() << "'!");
      return;
    }
    const char* rtPlanSopInstanceUid = isocenterSubjectHierarchyNode->GetParentNode()->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
    if (!rtPlanSopInstanceUid)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to get RT Plan DICOM UID for isocenter '" << isocenterNode->GetName() << "'!");
      return;
    }

    // Get isocenter beam number
    const char* isocenterBeamNumberChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());

    // Get number of beams in the plan (if there is only one, then the beam number may nor be correctly referenced, so we cannot find it that way
    bool oneBeamInPlan = false;
    if (isocenterSubjectHierarchyNode->GetParentNode() && isocenterSubjectHierarchyNode->GetParentNode()->GetNumberOfChildrenNodes() == 1)
    {
      oneBeamInPlan = true;
    }

    // Find corresponding RT image according to beam (isocenter) UID
    vtkSmartPointer<vtkCollection> hierarchyNodes = vtkSmartPointer<vtkCollection>::Take(
      this->GetMRMLScene()->GetNodesByClass("vtkMRMLSubjectHierarchyNode") );
    vtkObject* nextObject = NULL;
    for (hierarchyNodes->InitTraversal(); (nextObject = hierarchyNodes->GetNextItemAsObject()); )
    {
      vtkMRMLSubjectHierarchyNode* hierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nextObject);
      if (hierarchyNode && hierarchyNode->GetAssociatedDataNode() && hierarchyNode->GetAssociatedDataNode()->IsA("vtkMRMLScalarVolumeNode"))
      {
        // If this volume node has a referenced plan UID and it matches the isocenter UID then this may be the corresponding RT image
        const char* referencedPlanSopInstanceUid = hierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
        if (referencedPlanSopInstanceUid && !STRCASECMP(referencedPlanSopInstanceUid, rtPlanSopInstanceUid))
        {
          // Get RT image referenced beam number
          const char* referencedBeamNumberChars = hierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
          // If the referenced beam number matches the isocenter beam number, or if there is one beam in the plan, then we found the RT image
          if (!STRCASECMP(referencedBeamNumberChars, isocenterBeamNumberChars) || oneBeamInPlan)
          {
            rtImageVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(hierarchyNode->GetAssociatedDataNode());
            rtImageSubjectHierarchyNode = hierarchyNode;
            break;
          }
        }
      }

      // Return if a referenced displayed model is present for the RT image, because it means that the geometry has been set up successfully before
      if (rtImageVolumeNode)
      {
        vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
          rtImageVolumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
        if (modelNode)
        {
          vtkDebugMacro("SetupRtImageGeometry: RT image '" << rtImageVolumeNode->GetName() << "' belonging to isocenter '" << isocenterNode->GetName() << "' seems to have been set up already.");
          return;
        }
      }
    }

    if (!rtImageVolumeNode)
    {
      // RT image for the isocenter is not loaded yet. Geometry will be set up upon loading the related RT image
      vtkDebugMacro("SetupRtImageGeometry: Cannot set up geometry of RT image corresponding to isocenter fiducial '" << isocenterNode->GetName()
        << "' because the RT image is not loaded yet. Will be set up upon loading the related RT image");
      return;
    }
  }
  else
  {
    vtkErrorMacro("SetupRtImageGeometry: Input node is neither a volume node nor an isocenter fiducial node!");
    return;
  }

  // We have both the RT image and the isocenter, we can set up the geometry

  // Get source to RT image plane distance (along beam axis)
  double rtImageSid = 0.0;
  const char* rtImageSidChars = rtImageSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME.c_str());
  if (rtImageSidChars != NULL)
  {
    std::stringstream ss;
    ss << rtImageSidChars;
    ss >> rtImageSid;
  }
  // Get RT image position (the x and y coordinates (in mm) of the upper left hand corner of the image, in the IEC X-RAY IMAGE RECEPTOR coordinate system)
  double rtImagePosition[2] = {0.0, 0.0};
  const char* rtImagePositionChars = rtImageSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME.c_str());
  if (rtImagePositionChars != NULL)
  {
    std::stringstream ss;
    ss << rtImagePositionChars;
    ss >> rtImagePosition[0] >> rtImagePosition[1];
  }

  // Extract beam-related parameters needed to compute RT image coordinate system
  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }
  double gantryAngle = 0.0;
  const char* gantryAngleChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str());
  if (gantryAngleChars)
  {
    std::stringstream ss;
    ss << gantryAngleChars;
    ss >> gantryAngle;
  }
  double couchAngle = 0.0;
  const char* couchAngleChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME.c_str());
  if (couchAngleChars != NULL)
  {
    std::stringstream ss;
    ss << couchAngleChars;
    ss >> couchAngle;
  }

  // Get isocenter coordinates
  double isocenterWorldCoordinates[3] = {0.0, 0.0, 0.0};
  isocenterNode->GetNthFiducialPosition(0, isocenterWorldCoordinates);

  // Assemble transform from isocenter IEC to RT image RAS
  vtkSmartPointer<vtkTransform> fixedToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  fixedToIsocenterTransform->Identity();
  fixedToIsocenterTransform->Translate(isocenterWorldCoordinates);

  vtkSmartPointer<vtkTransform> couchToFixedTransform = vtkSmartPointer<vtkTransform>::New();
  couchToFixedTransform->Identity();
  couchToFixedTransform->RotateWXYZ((-1.0)*couchAngle, 0.0, 1.0, 0.0);

  vtkSmartPointer<vtkTransform> gantryToCouchTransform = vtkSmartPointer<vtkTransform>::New();
  gantryToCouchTransform->Identity();
  gantryToCouchTransform->RotateWXYZ(gantryAngle, 0.0, 0.0, 1.0);

  vtkSmartPointer<vtkTransform> sourceToGantryTransform = vtkSmartPointer<vtkTransform>::New();
  sourceToGantryTransform->Identity();
  sourceToGantryTransform->Translate(0.0, sourceAxisDistance, 0.0);

  vtkSmartPointer<vtkTransform> rtImageToSourceTransform = vtkSmartPointer<vtkTransform>::New();
  rtImageToSourceTransform->Identity();
  rtImageToSourceTransform->Translate(0.0, -rtImageSid, 0.0);

  vtkSmartPointer<vtkTransform> rtImageCenterToCornerTransform = vtkSmartPointer<vtkTransform>::New();
  rtImageCenterToCornerTransform->Identity();
  rtImageCenterToCornerTransform->Translate(-rtImagePosition[0], 0.0, rtImagePosition[1]);

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

  // Get RT image IJK to RAS matrix (containing the spacing and the LPS-RAS conversion)
  vtkSmartPointer<vtkMatrix4x4> rtImageIjkToRtImageRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  rtImageVolumeNode->GetIJKToRASMatrix(rtImageIjkToRtImageRasTransformMatrix);
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
  rtImageVolumeNode->SetIJKToRASMatrix(isocenterToRtImageRas->GetMatrix());

  // Set up outputs for the planar image display
  vtkSmartPointer<vtkMRMLModelNode> displayedModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  this->GetMRMLScene()->AddNode(displayedModelNode);
  std::string displayedModelNodeName = SlicerRtCommon::PLANARIMAGE_MODEL_NODE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName());
  displayedModelNode->SetName(displayedModelNodeName.c_str());
  displayedModelNode->SetAttribute(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_EXCLUDE_FROM_POTENTIAL_NODES_LIST_ATTRIBUTE_NAME.c_str(), "1");

  vtkSmartPointer<vtkMRMLScalarVolumeNode> textureVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  this->GetMRMLScene()->AddNode(textureVolumeNode);
  std::string textureVolumeNodeName = SlicerRtCommon::PLANARIMAGE_TEXTURE_NODE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName());
  textureVolumeNode->SetName(textureVolumeNodeName.c_str());
  textureVolumeNode->SetAttribute(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_EXCLUDE_FROM_POTENTIAL_NODES_LIST_ATTRIBUTE_NAME.c_str(), "1");

  // Create PlanarImage parameter set node
  std::string planarImageParameterSetNodeName;
  planarImageParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
    SlicerRtCommon::PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName()) );
  vtkSmartPointer<vtkMRMLPlanarImageNode> planarImageParameterSetNode = vtkSmartPointer<vtkMRMLPlanarImageNode>::New();
  planarImageParameterSetNode->SetName(planarImageParameterSetNodeName.c_str());
  this->GetMRMLScene()->AddNode(planarImageParameterSetNode);
  planarImageParameterSetNode->SetAndObserveRtImageVolumeNode(rtImageVolumeNode);
  planarImageParameterSetNode->SetAndObserveDisplayedModelNode(displayedModelNode);
  planarImageParameterSetNode->SetAndObserveTextureVolumeNode(textureVolumeNode);

  // Create planar image model for the RT image
  this->PlanarImageLogic->CreateModelForPlanarImage(planarImageParameterSetNode);

  // Hide the displayed planar image model by default
  displayedModelNode->SetDisplayVisibility(0);
}
