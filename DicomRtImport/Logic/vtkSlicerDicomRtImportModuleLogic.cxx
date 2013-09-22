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

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
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

// Annotations includes
#include <vtkMRMLAnnotationHierarchyNode.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLAnnotationTextDisplayNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkImageCast.h>
#include <vtkStringArray.h>

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

//-----------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::RegisterNodes()
{
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
  vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> contourHierarchySeriesNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> structureModelHierarchyRootNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string phSeriesNodeName(seriesName);
  phSeriesNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);
  std::string contourNodeName;
  std::string structureSetReferencedSeriesUid;

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Add color table node
  vtkSmartPointer<vtkMRMLColorTableNode> structureSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string structureSetColorTableNodeName;
  structureSetColorTableNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  structureSetColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(structureSetColorTableNodeName);
  structureSetColorTableNode->SetName(structureSetColorTableNodeName.c_str());
  structureSetColorTableNode->HideFromEditorsOff();
  structureSetColorTableNode->SetTypeToUser();
  structureSetColorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  this->GetMRMLScene()->AddNode(structureSetColorTableNode);

  // Add ROIs
  int numberOfRois = rtReader->GetNumberOfRois();
  structureSetColorTableNode->SetNumberOfColors(numberOfRois+2);
  structureSetColorTableNode->GetLookupTable()->SetTableRange(0,numberOfRois+1);
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_BACKGROUND, 0.0, 0.0, 0.0, 0.0); // Black background
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_INVALID,
    SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
    SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] ); // Color indicating invalid index

  vtkSmartPointer<vtkPolyDataCollection> roiCollection = vtkSmartPointer<vtkPolyDataCollection>::New();
  vtkSmartPointer<vtkCollection> displayNodeCollection = vtkSmartPointer<vtkCollection>::New();

  for (int internalROIIndex=0; internalROIIndex<numberOfRois; internalROIIndex++) // DICOM starts indexing from 1
  {
    const char* roiLabel = rtReader->GetRoiName(internalROIIndex);
    double *roiColor = rtReader->GetRoiDisplayColor(internalROIIndex);
    vtkMRMLDisplayableNode* addedDisplayableNode = NULL;

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
    structureSetColorTableNode->AddColor(roiLabel, roiColor[0], roiColor[1], roiColor[2]);

    // Get structure
    vtkPolyData* roiPoly = rtReader->GetRoiPolyData(internalROIIndex);
    if (roiPoly == NULL)
    {
      vtkWarningMacro("LoadRtStructureSet: Cannot read polydata from file: " << firstFileNameStr << ", ROI: " << internalROIIndex);
      continue;
    }
    if (roiPoly->GetNumberOfPoints() < 1)
    {
      vtkWarningMacro("LoadRtStructureSet: The ROI polydata does not contain any points, file: " << firstFileNameStr << ", ROI: " << internalROIIndex);
      continue;
    }

    if (roiPoly->GetNumberOfPoints() == 1)
    {
      // Point ROI
      addedDisplayableNode = this->AddRoiPoint(roiPoly->GetPoint(0), roiLabel, roiColor);
    }
    else
    {
      // Contour ROI
      contourNodeName = std::string(roiLabel) + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
      contourNodeName = this->GetMRMLScene()->GenerateUniqueName(contourNodeName);

      addedDisplayableNode = this->AddRoiContour(roiPoly, contourNodeName, roiColor);

      roiCollection->AddItem(roiPoly);
    }

    // Add new node to the model hierarchy
    if (addedDisplayableNode)
    {
      // Create root contour hierarchy node for the series, if it has not been created yet
      if (contourHierarchySeriesNode.GetPointer()==NULL)
      {
        std::string contourHierarchySeriesNodeName(seriesName);
        contourHierarchySeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX);
        contourHierarchySeriesNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
        contourHierarchySeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(contourHierarchySeriesNodeName);
        contourHierarchySeriesNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
        contourHierarchySeriesNode->SetName(contourHierarchySeriesNodeName.c_str());
        contourHierarchySeriesNode->AllowMultipleChildrenOn();
        contourHierarchySeriesNode->HideFromEditorsOff();
        contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
        contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
        //TODO: If both point and contour can be found in the series, then 2 PH nodes will be created with the same Series Instance UID!
        contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
          rtReader->GetSeriesInstanceUid());
        this->GetMRMLScene()->AddNode(contourHierarchySeriesNode);

        // A hierarchy node needs a display node
        vtkSmartPointer<vtkMRMLModelDisplayNode> contourHierarchySeriesDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
        contourHierarchySeriesNodeName.append("Display");
        contourHierarchySeriesDisplayNode->SetName(contourHierarchySeriesNodeName.c_str());
        contourHierarchySeriesDisplayNode->SetVisibility(1);
        this->GetMRMLScene()->AddNode(contourHierarchySeriesDisplayNode);
        contourHierarchySeriesNode->SetAndObserveDisplayNodeID( contourHierarchySeriesDisplayNode->GetID() );
      }

      if (roiPoly->GetNumberOfPoints() == 1)
      {
        // Create patient hierarchy entry for the ROI
        vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> patientHierarchyRoiNode = vtkSmartPointer<vtkMRMLAnnotationHierarchyNode>::New();
        std::string phNodeName;
        phNodeName = std::string(roiLabel) + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
        phNodeName = this->GetMRMLScene()->GenerateUniqueName(phNodeName);
        patientHierarchyRoiNode->SetName(phNodeName.c_str());
        patientHierarchyRoiNode->AllowMultipleChildrenOn();
        patientHierarchyRoiNode->HideFromEditorsOff();
        patientHierarchyRoiNode->SetAssociatedNodeID(addedDisplayableNode->GetID());
        patientHierarchyRoiNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        patientHierarchyRoiNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
        patientHierarchyRoiNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(),
          roiReferencedSeriesUid);
        patientHierarchyRoiNode->SetParentNodeID( contourHierarchySeriesNode->GetID() );
        this->GetMRMLScene()->AddNode(patientHierarchyRoiNode);
      }
      else
      {
        // Create contour node
        vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
        contourNode = vtkMRMLContourNode::SafeDownCast(this->GetMRMLScene()->AddNode(contourNode));
        contourNode->SetName(contourNodeName.c_str());
        contourNode->SetAndObserveRibbonModelNodeId(addedDisplayableNode->GetID());
        contourNode->HideFromEditorsOff();

        // Put the contour node in the hierarchy
        vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> contourPatientHierarchyNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
        std::string phContourNodeName(contourNodeName);
        phContourNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
        phContourNodeName = this->GetMRMLScene()->GenerateUniqueName(phContourNodeName);
        contourPatientHierarchyNode->SetName(phContourNodeName.c_str());
        contourPatientHierarchyNode->SetParentNodeID( contourHierarchySeriesNode->GetID() );
        contourPatientHierarchyNode->SetAssociatedNodeID( contourNode->GetID() );
        contourPatientHierarchyNode->HideFromEditorsOff();
        contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
        contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(),
          roiReferencedSeriesUid);
        contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(),
          roiLabel);
        this->GetMRMLScene()->AddNode(contourPatientHierarchyNode);

        displayNodeCollection->AddItem( vtkMRMLModelNode::SafeDownCast(addedDisplayableNode)->GetModelDisplayNode() );

        // Create root model hierarchy node, if it has not been created yet
        if (structureModelHierarchyRootNode.GetPointer()==NULL)
        {
          structureModelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
          std::string hierarchyNodeName;
          hierarchyNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
          hierarchyNodeName = this->GetMRMLScene()->GenerateUniqueName(hierarchyNodeName);
          structureModelHierarchyRootNode->SetName(hierarchyNodeName.c_str());
          structureModelHierarchyRootNode->AllowMultipleChildrenOn();
          structureModelHierarchyRootNode->HideFromEditorsOff();
          this->GetMRMLScene()->AddNode(structureModelHierarchyRootNode);

          // A hierarchy node needs a display node
          vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
          hierarchyNodeName.append("Display");
          modelDisplayNode->SetName(hierarchyNodeName.c_str());
          modelDisplayNode->SetVisibility(1);
          this->GetMRMLScene()->AddNode(modelDisplayNode);
          structureModelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );
        }

        // Put the new node in the hierarchy
        vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        this->GetMRMLScene()->AddNode(modelHierarchyNode);
        modelHierarchyNode->SetParentNodeID( structureModelHierarchyRootNode->GetID() );
        modelHierarchyNode->SetModelNodeID( addedDisplayableNode->GetID() );
      }
    }
  }

  // Add color table in patient hierarchy
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchyColorTableNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  std::string phColorTableNodeName;
  phColorTableNodeName = structureSetColorTableNodeName + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  phColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(phColorTableNodeName);
  patientHierarchyColorTableNode->SetName(phColorTableNodeName.c_str());
  patientHierarchyColorTableNode->HideFromEditorsOff();
  patientHierarchyColorTableNode->SetAssociatedNodeID(structureSetColorTableNode->GetID());
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  patientHierarchyColorTableNode->SetParentNodeID(contourHierarchySeriesNode->GetID());
  this->GetMRMLScene()->AddNode(patientHierarchyColorTableNode);

  // Set referenced series UID to the series patient hierarchy node
  contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(), structureSetReferencedSeriesUid.c_str());

  // Insert series in patient hierarchy
  this->InsertSeriesInPatientHierarchy(rtReader);


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
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchySeriesNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string phSeriesNodeName(seriesName);
  phSeriesNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);

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

  // Create isodose parameter set node and set color table to default
  std::string isodoseParameterSetNodeName;
  isodoseParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
    SlicerRtCommon::ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX + volumeNodeName );
  vtkSmartPointer<vtkMRMLIsodoseNode> isodoseParameterSetNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
  isodoseParameterSetNode->SetName(isodoseParameterSetNodeName.c_str());
  isodoseParameterSetNode->SetAndObserveDoseVolumeNodeId(volumeNode->GetID());
  if (this->IsodoseLogic)
  {
    isodoseParameterSetNode->SetAndObserveColorTableNodeId(this->IsodoseLogic->GetDefaultIsodoseColorTableNodeId());
  }
  this->GetMRMLScene()->AddNode(isodoseParameterSetNode);

  //TODO: Generate isodose surfaces if chosen so by the user in the hanging protocol options

  // Set default colormap to the loaded one if found or generated, or to rainbow otherwise
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  volumeDisplayNode->SetAndObserveColorNodeID(this->DefaultDoseColorTableNodeId);
  this->GetMRMLScene()->AddNode(volumeDisplayNode);
  volumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

  // Set window/level to match the isodose levels
  if (this->IsodoseLogic)
  {
    vtkMRMLColorTableNode* defaultIsodoseColorTable = vtkMRMLColorTableNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(this->IsodoseLogic->GetDefaultIsodoseColorTableNodeId()) );
    
    std::stringstream ssMin;
    ssMin << defaultIsodoseColorTable->GetColorName(0);;
    int minDoseInDefaultIsodoseLevels;
    ssMin >> minDoseInDefaultIsodoseLevels;

    std::stringstream ssMax;
    ssMax << defaultIsodoseColorTable->GetColorName( defaultIsodoseColorTable->GetNumberOfColors()-1 );;
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

  // Create patient hierarchy entry
  patientHierarchySeriesNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  patientHierarchySeriesNode->HideFromEditorsOff();
  patientHierarchySeriesNode->SetAssociatedNodeID(volumeNode->GetID());
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
    rtReader->GetSeriesInstanceUid());
  patientHierarchySeriesNode->SetName(phSeriesNodeName.c_str());
  this->GetMRMLScene()->AddNode(patientHierarchySeriesNode);

  // Insert series in patient hierarchy
  this->InsertSeriesInPatientHierarchy(rtReader);

  // Set dose unit attributes to patient hierarchy study node
  vtkMRMLHierarchyNode* studyHierarchyNode = patientHierarchySeriesNode->GetParentNode();
  if (!studyHierarchyNode || !SlicerRtCommon::IsPatientHierarchyNode(studyHierarchyNode))
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
  vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> isocenterSeriesHierarchyRootNode;
  vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> sourceHierarchyRootNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyRootNode;

  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string phSeriesNodeName(seriesName);
  phSeriesNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  vtkMRMLAnnotationFiducialNode* addedFiducialNode = NULL;
  int numberOfBeams = rtReader->GetNumberOfBeams();
  for (int dicomBeamIndex = 1; dicomBeamIndex < numberOfBeams+1; dicomBeamIndex++) // DICOM starts indexing from 1
  {
    // Isocenter fiducial
    double isoColor[3] = { 1.0, 1.0, 1.0 };
    addedFiducialNode = this->AddRoiPoint(rtReader->GetBeamIsocenterPositionRas(dicomBeamIndex), rtReader->GetBeamName(dicomBeamIndex), isoColor);

    // Add new node to the hierarchy node
    if (addedFiducialNode)
    {
      // Create root isocenter annotation hierarchy node for the plan series, if it has not been created yet
      if (isocenterSeriesHierarchyRootNode.GetPointer()==NULL)
      {
        isocenterSeriesHierarchyRootNode = vtkSmartPointer<vtkMRMLAnnotationHierarchyNode>::New();
        isocenterSeriesHierarchyRootNode->HideFromEditorsOff();
        isocenterSeriesHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        isocenterSeriesHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
        isocenterSeriesHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
          rtReader->GetSeriesInstanceUid());
        isocenterSeriesHierarchyRootNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str(),
          rtReader->GetSOPInstanceUID());
        std::string isocenterHierarchyRootNodeName;
        isocenterHierarchyRootNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX;
        isocenterHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(isocenterHierarchyRootNodeName);
        isocenterSeriesHierarchyRootNode->SetName(isocenterHierarchyRootNodeName.c_str());
        this->GetMRMLScene()->AddNode(isocenterSeriesHierarchyRootNode);

        // A hierarchy node needs a display node
        vtkSmartPointer<vtkMRMLAnnotationDisplayNode> isocenterSeriesHierarchyRootDisplayNode = vtkSmartPointer<vtkMRMLAnnotationDisplayNode>::New();
        isocenterHierarchyRootNodeName.append("Display");
        isocenterSeriesHierarchyRootDisplayNode->SetName(isocenterHierarchyRootNodeName.c_str());
        isocenterSeriesHierarchyRootDisplayNode->SetVisibility(1);
        this->GetMRMLScene()->AddNode(isocenterSeriesHierarchyRootDisplayNode);
        isocenterSeriesHierarchyRootNode->SetAndObserveDisplayNodeID( isocenterSeriesHierarchyRootDisplayNode->GetID() );
      }

      // Create root source annotation hierarchy node, if it has not been created yet
      if (sourceHierarchyRootNode.GetPointer()==NULL)
      {
        sourceHierarchyRootNode = vtkSmartPointer<vtkMRMLAnnotationHierarchyNode>::New();
        std::string sourceHierarchyRootNodeName;
        sourceHierarchyRootNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_SOURCE_HIERARCHY_NODE_NAME_POSTFIX;
        sourceHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(sourceHierarchyRootNodeName);
        sourceHierarchyRootNode->SetName(sourceHierarchyRootNodeName.c_str());
        sourceHierarchyRootNode->AllowMultipleChildrenOn();
        sourceHierarchyRootNode->HideFromEditorsOff();
        this->GetMRMLScene()->AddNode(sourceHierarchyRootNode);

        // A hierarchy node needs a display node
        vtkSmartPointer<vtkMRMLAnnotationDisplayNode> sourceHierarchyRootDisplayNode = vtkSmartPointer<vtkMRMLAnnotationDisplayNode>::New();
        sourceHierarchyRootNodeName.append("Display");
        sourceHierarchyRootDisplayNode->SetName(sourceHierarchyRootNodeName.c_str());
        sourceHierarchyRootDisplayNode->SetVisibility(1);
        this->GetMRMLScene()->AddNode(sourceHierarchyRootDisplayNode);
        sourceHierarchyRootNode->SetAndObserveDisplayNodeID( sourceHierarchyRootDisplayNode->GetID() );
      }

      // Create beam model patient hierarchy node if has not been created yet
      if (beamModelHierarchyRootNode.GetPointer()==NULL)
      {
        beamModelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        std::string beamsHierarchyNodeName;
        beamsHierarchyNodeName = std::string(seriesName)
          + SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX;
        beamsHierarchyNodeName = this->GetMRMLScene()->GenerateUniqueName(beamsHierarchyNodeName);
        // Only make it a patient hierarchy node if the separate branch flag is on
        if (this->BeamModelsInSeparateBranch)
        {
          beamModelHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
            SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
          beamModelHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
            vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
        }
        beamModelHierarchyRootNode->SetName(beamsHierarchyNodeName.c_str());
        beamModelHierarchyRootNode->AllowMultipleChildrenOn();
        beamModelHierarchyRootNode->HideFromEditorsOff();
        this->GetMRMLScene()->AddNode(beamModelHierarchyRootNode);

        // A hierarchy node needs a display node
        vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelHierarchyRootDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
        beamsHierarchyNodeName.append("Display");
        beamModelHierarchyRootDisplayNode->SetName(beamsHierarchyNodeName.c_str());
        beamModelHierarchyRootDisplayNode->SetVisibility(1);
        this->GetMRMLScene()->AddNode(beamModelHierarchyRootDisplayNode);
        beamModelHierarchyRootNode->SetAndObserveDisplayNodeID( beamModelHierarchyRootDisplayNode->GetID() );
      }

      // Set up automatically created isocenter fiducial node properly
      vtkMRMLAnnotationHierarchyNode* isocenterHierarchyNode =
        vtkMRMLAnnotationHierarchyNode::SafeDownCast( vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), addedFiducialNode->GetID()) );
      isocenterHierarchyNode->SetParentNodeID( isocenterSeriesHierarchyRootNode->GetID() );
      isocenterHierarchyNode->SetIndexInParent(dicomBeamIndex-1);

      // Create patient hierarchy entry for the isocenter fiducial
      vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> patientHierarchyFiducialNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
      std::string phFiducialNodeName(rtReader->GetBeamName(dicomBeamIndex));
      phFiducialNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
      patientHierarchyFiducialNode->SetName(phFiducialNodeName.c_str());
      patientHierarchyFiducialNode->HideFromEditorsOff();
      patientHierarchyFiducialNode->SetAssociatedNodeID(addedFiducialNode->GetID());
      patientHierarchyFiducialNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
        SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
      patientHierarchyFiducialNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
      std::stringstream beamNumberStream;
      beamNumberStream << dicomBeamIndex;
      patientHierarchyFiducialNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str(),
        beamNumberStream.str().c_str());
      patientHierarchyFiducialNode->SetParentNodeID(isocenterSeriesHierarchyRootNode->GetID());
      this->GetMRMLScene()->AddNode(patientHierarchyFiducialNode);

      // Add attributes containing beam information to the isocenter fiducial node
      // TODO: Add these in the PatientHierarchy node when available
      std::stringstream sourceAxisDistanceStream;
      sourceAxisDistanceStream << rtReader->GetBeamSourceAxisDistance(dicomBeamIndex);
      addedFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str(),
        sourceAxisDistanceStream.str().c_str() );
      std::stringstream gantryAngleStream;
      gantryAngleStream << rtReader->GetBeamGantryAngle(dicomBeamIndex);
      addedFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str(),
        gantryAngleStream.str().c_str() );
      std::stringstream couchAngleStream;
      couchAngleStream << rtReader->GetBeamPatientSupportAngle(dicomBeamIndex);
      addedFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_COUCH_ANGLE_ATTRIBUTE_NAME.c_str(),
        couchAngleStream.str().c_str() );
      std::stringstream collimatorAngleStream;
      collimatorAngleStream << rtReader->GetBeamBeamLimitingDeviceAngle(dicomBeamIndex);
      addedFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str(),
        collimatorAngleStream.str().c_str() );
      std::stringstream jawPositionsStream;
      double jawPositions[2][2] = {{0.0, 0.0},{0.0, 0.0}};
      rtReader->GetBeamLeafJawPositions(dicomBeamIndex, jawPositions);
      jawPositionsStream << jawPositions[0][0] << " " << jawPositions[0][1] << " "
        << jawPositions[1][0] << " " << jawPositions[1][1];
      addedFiducialNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME.c_str(),
        jawPositionsStream.str().c_str() );

      // Create source fiducial node
      std::string sourceFiducialName;
      sourceFiducialName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_OUTPUT_SOURCE_FIDUCIAL_PREFIX + std::string(addedFiducialNode->GetName()) );
      vtkSmartPointer<vtkMRMLAnnotationFiducialNode> sourceFiducialNode = vtkSmartPointer<vtkMRMLAnnotationFiducialNode>::New();
      sourceFiducialNode->SetName(sourceFiducialName.c_str());
      this->GetMRMLScene()->AddNode(sourceFiducialNode);

      // Set up automatically created source fiducial node properly
      vtkMRMLAnnotationHierarchyNode* sourceHierarchyNode =
        vtkMRMLAnnotationHierarchyNode::SafeDownCast( vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), sourceFiducialNode->GetID()) );
      sourceHierarchyNode->SetParentNodeID( sourceHierarchyRootNode->GetID() );
      sourceHierarchyNode->SetIndexInParent(dicomBeamIndex-1);

      // Create beam model node and add it to the scene
      std::string beamModelName;
      beamModelName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX + std::string(addedFiducialNode->GetName()) );
      vtkSmartPointer<vtkMRMLModelNode> beamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      beamModelNode->SetName(beamModelName.c_str());
      this->GetMRMLScene()->AddNode(beamModelNode);

      // Create Beams parameter set node
      std::string beamParameterSetNodeName;
      beamParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_PARAMETER_SET_BASE_NAME_PREFIX + std::string(addedFiducialNode->GetName()) );
      vtkSmartPointer<vtkMRMLBeamsNode> beamParameterSetNode = vtkSmartPointer<vtkMRMLBeamsNode>::New();
      beamParameterSetNode->SetName(beamParameterSetNodeName.c_str());
      beamParameterSetNode->SetAndObserveIsocenterFiducialNodeId(addedFiducialNode->GetID());
      beamParameterSetNode->SetAndObserveSourceFiducialNodeId(sourceFiducialNode->GetID());
      beamParameterSetNode->SetAndObserveBeamModelNodeId(beamModelNode->GetID());
      this->GetMRMLScene()->AddNode(beamParameterSetNode);

      // Create beam geometry
      vtkSmartPointer<vtkSlicerBeamsModuleLogic> beamsLogic = vtkSmartPointer<vtkSlicerBeamsModuleLogic>::New();
      beamsLogic->SetAndObserveBeamsNode(beamParameterSetNode);
      beamsLogic->SetAndObserveMRMLScene(this->GetMRMLScene());
      std::string errorMessage;
      beamsLogic->CreateBeamModel(errorMessage);
      if (!errorMessage.empty())
      {
        vtkWarningMacro("LoadRtPlan: Failed to create beam geometry for isocenter: " << addedFiducialNode->GetName());
      }

      // Put new beam model in the patient hierarchy
      vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      std::string phBeamNodeName = beamModelName + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
      beamModelHierarchyNode->SetName(phBeamNodeName.c_str());
      beamModelHierarchyNode->HideFromEditorsOff();
      beamModelHierarchyNode->SetDisplayableNodeID(beamModelNode->GetID());
      beamModelHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
        SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
      beamModelHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
      if (this->BeamModelsInSeparateBranch)
      {
        beamModelHierarchyNode->SetParentNodeID(beamModelHierarchyRootNode->GetID());
      }
      else
      {
        beamModelHierarchyNode->SetParentNodeID(patientHierarchyFiducialNode->GetID());
      }
      this->GetMRMLScene()->AddNode(beamModelHierarchyNode);
      beamModelHierarchyNode->SetIndexInParent(dicomBeamIndex-1);

      // Compute and set geometry of possible RT image that references the loaded beam.
      // Uses the referenced RT image if available, otherwise the geometry will be set up when loading the corresponding RT image
      this->SetupRtImageGeometry(addedFiducialNode);

    } //endif addedDisplayableNode
  }

  // Insert plan isocenter series in patient hierarchy
  this->InsertSeriesInPatientHierarchy(rtReader);

  // Insert beam model subseries under the study if the separate branch flag is on
  vtkMRMLHierarchyNode* studyNode = isocenterSeriesHierarchyRootNode->GetParentNode();
  if (studyNode && SlicerRtCommon::IsPatientHierarchyNode(studyNode) && this->BeamModelsInSeparateBranch)
  {
    beamModelHierarchyRootNode->SetParentNodeID(studyNode->GetID());
  }

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtImage(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo* loadInfo)
{
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchySeriesNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  if (loadInfo->GetNumberOfLoadables() > 1 || loadInfo->GetLoadableFiles(0)->GetNumberOfValues() > 1)
  {
    vtkErrorMacro("LoadRtImage: Only one loadable and one file name is allowed for RT Image series '" << seriesName << "')");
    return false;
  }

  std::string phSeriesNodeName(seriesName);
  phSeriesNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);

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

  // Create patient hierarchy entry
  patientHierarchySeriesNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  patientHierarchySeriesNode->HideFromEditorsOff();
  patientHierarchySeriesNode->SetAssociatedNodeID(volumeNode->GetID());
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
    rtReader->GetSeriesInstanceUid());
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str(),
    rtReader->GetReferencedRTPlanSOPInstanceUID());
  std::stringstream referencedBeamNumberStream;
  referencedBeamNumberStream << rtReader->GetReferencedBeamNumber();
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str(),
    referencedBeamNumberStream.str().c_str());
  std::stringstream rtImageSidStream;
  rtImageSidStream << rtReader->GetRTImageSID();
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME.c_str(),
    rtImageSidStream.str().c_str());
  std::stringstream rtImagePositionStream;
  double rtImagePosition[2] = {0.0, 0.0};
  rtReader->GetRTImagePosition(rtImagePosition);
  rtImagePositionStream << rtImagePosition[0] << " " << rtImagePosition[1];
  patientHierarchySeriesNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME.c_str(),
    rtImagePositionStream.str().c_str() );
  patientHierarchySeriesNode->SetName(phSeriesNodeName.c_str());
  this->GetMRMLScene()->AddNode(patientHierarchySeriesNode);

  // Insert series in patient hierarchy
  this->InsertSeriesInPatientHierarchy(rtReader);

  // Compute and set RT image geometry. Uses the referenced beam if available, otherwise the geometry will be set up when loading the referenced beam
  this->SetupRtImageGeometry(volumeNode);

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLAnnotationFiducialNode* vtkSlicerDicomRtImportModuleLogic::AddRoiPoint(double* roiPosition, std::string baseName, double* roiColor)
{
  std::string fiducialNodeName = this->GetMRMLScene()->GenerateUniqueName(baseName);
  vtkSmartPointer<vtkMRMLAnnotationFiducialNode> fiducialNode = vtkSmartPointer<vtkMRMLAnnotationFiducialNode>::New();
  fiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(this->GetMRMLScene()->AddNode(fiducialNode));
  fiducialNode->SetName(baseName.c_str());
  fiducialNode->AddControlPoint(roiPosition, 0, 1);
  fiducialNode->SetLocked(1);

  fiducialNode->CreateAnnotationTextDisplayNode();
  fiducialNode->CreateAnnotationPointDisplayNode();
  fiducialNode->GetAnnotationPointDisplayNode()->SetGlyphType(vtkMRMLAnnotationPointDisplayNode::Sphere3D);
  fiducialNode->GetAnnotationPointDisplayNode()->SetColor(roiColor);
  fiducialNode->GetAnnotationTextDisplayNode()->SetColor(roiColor);

  // Hide the fiducial by default
  fiducialNode->SetDisplayVisibility(0);

  return fiducialNode;
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
void vtkSlicerDicomRtImportModuleLogic::InsertSeriesInPatientHierarchy( vtkSlicerDicomRtReader* rtReader )
{
  // Get the higher level parent nodes by their IDs (to fill their attributes later if they do not exist yet)
  vtkMRMLHierarchyNode* patientNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
    this->GetMRMLScene(), rtReader->GetPatientId() );
  vtkMRMLHierarchyNode* studyNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
    this->GetMRMLScene(), rtReader->GetStudyInstanceUid() );

  // Insert series in hierarchy
  vtkMRMLHierarchyNode* seriesNode = vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
    this->GetMRMLScene(), rtReader->GetPatientId(), rtReader->GetStudyInstanceUid(), rtReader->GetSeriesInstanceUid() );

  // Fill patient and study attributes if they have been just created
  if (patientNode == NULL)
  {
    patientNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
      this->GetMRMLScene(), rtReader->GetPatientId() );
    if (patientNode)
    {
      std::string patientNodeName = ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetPatientName())
        ? std::string(rtReader->GetPatientName()) + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX
        : SlicerRtCommon::DICOMRTIMPORT_NO_NAME + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX );
      patientNode->SetName( patientNodeName.c_str() );
    }
    else
    {
      vtkErrorMacro("InsertSeriesInPatientHierarchy: Patient node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }
  if (studyNode == NULL)
  {
    studyNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
      this->GetMRMLScene(), rtReader->GetStudyInstanceUid() );
    if (studyNode)
    {
      std::string studyNodeName = ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetStudyDescription())
        ? std::string(rtReader->GetStudyDescription()) + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX
        : SlicerRtCommon::DICOMRTIMPORT_NO_DESCRIPTION + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX );
      studyNode->SetName( studyNodeName.c_str() );
    }
    else
    {
      vtkErrorMacro("InsertSeriesInPatientHierarchy: Study node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }
  if (seriesNode == NULL)
  {
    vtkErrorMacro("InsertSeriesInPatientHierarchy: Failed to insert series with Instance UID "
      << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    return;
  }
  else
  {
    // Set DICOM tags to the hierarchy node
    seriesNode->SetAttribute( SlicerRtCommon::PATIENTHIERARCHY_SERIES_MODALITY_ATTRIBUTE_NAME.c_str(), rtReader->GetSeriesModality() );
    seriesNode->SetAttribute( SlicerRtCommon::PATIENTHIERARCHY_STUDY_DATE_ATTRIBUTE_NAME.c_str(), rtReader->GetStudyDate() );
    seriesNode->SetAttribute( SlicerRtCommon::PATIENTHIERARCHY_STUDY_TIME_ATTRIBUTE_NAME.c_str(), rtReader->GetStudyTime() );
    seriesNode->SetAttribute( SlicerRtCommon::PATIENTHIERARCHY_PATIENT_SEX_ATTRIBUTE_NAME.c_str(), rtReader->GetPatientSex() );
    seriesNode->SetAttribute( SlicerRtCommon::PATIENTHIERARCHY_PATIENT_BIRTH_DATE_ATTRIBUTE_NAME.c_str(), rtReader->GetPatientBirthDate() );
  }

  // Handle special cases, make connections
  const char* modality = seriesNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_SERIES_MODALITY_ATTRIBUTE_NAME.c_str());
  if (!modality)
  {
    vtkErrorMacro("InsertSeriesInPatientHierarchy: Series '" << seriesNode->GetName() << "' has invalid modality attribute!");
  }
  // Put structure set under anatomical volume
  else if (!STRCASECMP(modality, "RTSTRUCT"))
  {
    const char* referencedSeriesUid = seriesNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str());
    if (referencedSeriesUid)
    {
      vtkSmartPointer<vtkMRMLHierarchyNode> referencedSeriesNode;
      vtkMRMLHierarchyNode* foundPatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(this->GetMRMLScene(), referencedSeriesUid);
      if (foundPatientHierarchyNode)
      {
        referencedSeriesNode = vtkSmartPointer<vtkMRMLHierarchyNode>::Take(foundPatientHierarchyNode);
        referencedSeriesNode->Register(NULL);
      }
      else
      {
        // Create dummy anatomical volume node to put the structure set under. When the actual volume is loaded, it occupies the node
        referencedSeriesNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
        referencedSeriesNode->HideFromEditorsOff();
        referencedSeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        referencedSeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
        referencedSeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
          referencedSeriesUid);
        std::string referencedSeriesNodeName = SlicerRtCommon::PATIENTHIERARCHY_DUMMY_ANATOMICAL_VOLUME_NODE_NAME_PREFIX + vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
        referencedSeriesNode->SetName(referencedSeriesNodeName.c_str());
        this->GetMRMLScene()->AddNode(referencedSeriesNode);

        vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
          this->GetMRMLScene(), rtReader->GetPatientId(), rtReader->GetStudyInstanceUid(), referencedSeriesUid );
      }

      seriesNode->SetParentNodeID(referencedSeriesNode->GetID());
    }
    else
    {
      vtkErrorMacro("InsertSeriesInPatientHierarchy: Structure set '" << seriesNode->GetName() << "' has no referenced series!");
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
  defaultDoseColorTable->HideFromEditorsOff();
  defaultDoseColorTable->SaveWithSceneOn();
  defaultDoseColorTable->SetNumberOfColors(256);

  SlicerRtCommon::StretchDiscreteColorTable(defaultIsodoseColorTable, defaultDoseColorTable);

  this->GetMRMLScene()->AddNode(defaultDoseColorTable);
  this->SetDefaultDoseColorTableNodeId(defaultDoseColorTable->GetID());
}

//------------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::SetupRtImageGeometry(vtkMRMLNode* node)
{
  vtkMRMLVolumeNode* rtImageVolumeNode = vtkMRMLVolumeNode::SafeDownCast(node);
  vtkMRMLHierarchyNode* rtImagePatientHierarchyNode = NULL;
  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(node);
  vtkMRMLHierarchyNode* isocenterPatientHierarchyNode = NULL;

  if (rtImageVolumeNode)
  {
    // Get patient hierarchy node for RT images
    rtImagePatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(rtImageVolumeNode);
    if (!rtImagePatientHierarchyNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid patient hierarchy node for RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }
    // Find referenced RT plan node
    const char* referencedPlanSopInstanceUid = rtImagePatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
    if (!referencedPlanSopInstanceUid)
    {
      vtkErrorMacro("SetupRtImageGeometry: Unable to find referenced plan SOP instance UID for RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }
    vtkMRMLHierarchyNode* rtPlanPatientHierarchyNode = NULL;
    std::vector<vtkMRMLNode *> patientHierarchyNodes;
    unsigned int numberOfNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLHierarchyNode", patientHierarchyNodes);
    for (unsigned int phNodeIndex=0; phNodeIndex<numberOfNodes; phNodeIndex++)
    {
      vtkMRMLHierarchyNode* currentPhNode = vtkMRMLHierarchyNode::SafeDownCast(patientHierarchyNodes[phNodeIndex]);
      if (currentPhNode && SlicerRtCommon::IsPatientHierarchyNode(currentPhNode))
      {
        const char* sopInstanceUid = currentPhNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
        if (sopInstanceUid && !STRCASECMP(referencedPlanSopInstanceUid, sopInstanceUid))
        {
          rtPlanPatientHierarchyNode = currentPhNode;
        }
      }
    }
    if (!rtPlanPatientHierarchyNode)
    {
      vtkDebugMacro("SetupRtImageGeometry: Cannot set up geometry of RT image '" << rtImageVolumeNode->GetName()
        << "' without the referenced RT plan. Will be set up upon loading the related plan");
      return;
    }

    // Get referenced beam number (string)
    const char* referencedBeamNumberChars = rtImagePatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
    if (!referencedBeamNumberChars)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve referenced beam number for RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }

    // Get isocenter according to referenced beam number
    std::vector<vtkMRMLHierarchyNode*> isocenterPatientHierarchyNodes = rtPlanPatientHierarchyNode->GetChildrenNodes();
    for (std::vector<vtkMRMLHierarchyNode*>::iterator isocenterPhIt = isocenterPatientHierarchyNodes.begin(); isocenterPhIt != isocenterPatientHierarchyNodes.end(); ++isocenterPhIt)
    {
      const char* isocenterBeamNumberChars = (*isocenterPhIt)->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
      if (!isocenterBeamNumberChars)
      {
        // Plan series patient hierarchy node has two hierarchy nodes for each annotation: an annotation hierarchy node and the PH node
        // TODO: review when utilizing the Markups module instead of Annotations (#385) 
        continue;
      }
      if (!STRCASECMP(isocenterBeamNumberChars, referencedBeamNumberChars))
      {
        isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast((*isocenterPhIt)->GetAssociatedNode());
        isocenterPatientHierarchyNode = (*isocenterPhIt);
        break;
      }
    }
    if (!isocenterNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve isocenter node for RT image '" << rtImageVolumeNode->GetName() << "' in RT plan '" << rtPlanPatientHierarchyNode->GetName() << "'!");
      return;
    }
  }
  else if (isocenterNode)
  {
    // Get RT plan DICOM UID for isocenter
    isocenterPatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(isocenterNode);
    if (!isocenterPatientHierarchyNode || !isocenterPatientHierarchyNode->GetParentNode())
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid patient hierarchy node for isocenter '" << isocenterNode->GetName() << "'!");
      return;
    }
    const char* rtPlanSopInstanceUid = isocenterPatientHierarchyNode->GetParentNode()->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
    if (!rtPlanSopInstanceUid)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to get RT Plan DICOM UID for isocenter '" << isocenterNode->GetName() << "'!");
      return;
    }

    // Get isocenter beam number
    const char* isocenterBeamNumberChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());

    // Find corresponding RT image according to beam (isocenter) UID
    vtkSmartPointer<vtkCollection> hierarchyNodes = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLHierarchyNode") );
    vtkObject* nextObject = NULL;
    for (hierarchyNodes->InitTraversal(); (nextObject = hierarchyNodes->GetNextItemAsObject()); )
    {
      vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(nextObject);
      if (hierarchyNode && hierarchyNode->GetAssociatedNode() && hierarchyNode->GetAssociatedNode()->IsA("vtkMRMLVolumeNode"))
      {
        // If this volume node has a referenced plan UID and it matches the isocenter UID then this may be the corresponding RT image
        const char* referencedPlanSopInstanceUid = hierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str());
        if (referencedPlanSopInstanceUid && !STRCASECMP(referencedPlanSopInstanceUid, rtPlanSopInstanceUid))
        {
          // Get RT image referenced beam number and compare it to the isocenter beam number
          const char* referencedBeamNumberChars = hierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
          if (!STRCASECMP(referencedBeamNumberChars, isocenterBeamNumberChars))
          {
            rtImageVolumeNode = vtkMRMLVolumeNode::SafeDownCast(hierarchyNode->GetAssociatedNode());
            rtImagePatientHierarchyNode = hierarchyNode;
            break;
          }
        }
      }
    }

    if (!rtImageVolumeNode)
    {
      vtkDebugMacro("SetupRtImageGeometry: Cannot set up geometry of RT image corresponding to isocenter fiducial '" << isocenterNode->GetName()
        << "' because the RT image is not loaded yet. Will be set up upon loading the related RT image");
      return;
    }
  }
  else
  {
    vtkErrorMacro("SetupRtImageGeometry: Input node is neither a volume node nor a beams parameter set node!");
    return;
  }

  // We have both the RT image and the isocenter, we can set up the geometry

  // Get source to RT image plane distance (along beam axis)
  double rtImageSid = 0.0;
  const char* rtImageSidChars = rtImagePatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME.c_str());
  if (rtImageSidChars != NULL)
  {
    std::stringstream ss;
    ss << rtImageSidChars;
    ss >> rtImageSid;
  }
  // Get RT image position (the x and y coordinates (in mm) of the upper left hand corner of the image, in the IEC X-RAY IMAGE RECEPTOR coordinate system)
  double rtImagePosition[2] = {0.0, 0.0};
  const char* rtImagePositionChars = rtImagePatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME.c_str());
  if (rtImagePositionChars != NULL)
  {
    std::stringstream ss;
    ss << rtImagePositionChars;
    ss >> rtImagePosition[0] >> rtImagePosition[1];
  }

  // Extract beam-related parameters needed to compute RT image coordinate system
  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars != NULL)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }
  double gantryAngle = 0.0;
  const char* gantryAngleChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str());
  if (gantryAngleChars != NULL)
  {
    std::stringstream ss;
    ss << gantryAngleChars;
    ss >> gantryAngle;
  }
  double couchAngle = 0.0;
  const char* couchAngleChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_COUCH_ANGLE_ATTRIBUTE_NAME.c_str());
  if (couchAngleChars != NULL)
  {
    std::stringstream ss;
    ss << couchAngleChars;
    ss >> couchAngle;
  }

  // Get isocenter coordinates
  double isocenterWorldCoordinates[3] = {0.0, 0.0, 0.0};
  isocenterNode->GetFiducialCoordinates(isocenterWorldCoordinates);

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
  displayedModelNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_EXCLUDE_FROM_POTENTIAL_NODES_LIST_ATTRIBUTE_NAME.c_str(), "1");

  vtkSmartPointer<vtkMRMLScalarVolumeNode> textureVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  this->GetMRMLScene()->AddNode(textureVolumeNode);
  std::string textureVolumeNodeName = SlicerRtCommon::PLANARIMAGE_TEXTURE_NODE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName());
  textureVolumeNode->SetName(textureVolumeNodeName.c_str());
  textureVolumeNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_EXCLUDE_FROM_POTENTIAL_NODES_LIST_ATTRIBUTE_NAME.c_str(), "1");

  // Create PlanarImage parameter set node
  std::string planarImageParameterSetNodeName;
  planarImageParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
    SlicerRtCommon::PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName()) );
  vtkSmartPointer<vtkMRMLPlanarImageNode> planarImageParameterSetNode = vtkSmartPointer<vtkMRMLPlanarImageNode>::New();
  planarImageParameterSetNode->SetName(planarImageParameterSetNodeName.c_str());
  this->GetMRMLScene()->AddNode(planarImageParameterSetNode);
  planarImageParameterSetNode->SetNodeReferenceID(vtkMRMLPlanarImageNode::PlanarImageVolumeNodeReferenceRole, rtImageVolumeNode->GetID());
  planarImageParameterSetNode->SetNodeReferenceID(vtkMRMLPlanarImageNode::DisplayedModelNodeReferenceRole, displayedModelNode->GetID());
  planarImageParameterSetNode->SetNodeReferenceID(vtkMRMLPlanarImageNode::TextureVolumeNodeReferenceRole, textureVolumeNode->GetID());

  // Create planar image model for the RT image
  this->PlanarImageLogic->CreateModelForPlanarImage(planarImageParameterSetNode);

  // Hide the displayed planar image model by default
  displayedModelNode->SetDisplayVisibility(0);
}
