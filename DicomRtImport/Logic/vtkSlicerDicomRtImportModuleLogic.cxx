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
#include "vtkMRMLContourHierarchyNode.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLBeamsNode.h"

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

// Annotations includes
#include <vtkMRMLAnnotationHierarchyNode.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLAnnotationTextDisplayNode.h>

// VTK includes
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkImageCast.h>
#include <vtkStringArray.h>
#include <vtkLookupTable.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportModuleLogic, VolumesLogic, vtkSlicerVolumesLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportModuleLogic::vtkSlicerDicomRtImportModuleLogic()
{
  this->VolumesLogic = NULL;

  this->AutoContourOpacityOn();
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportModuleLogic::~vtkSlicerDicomRtImportModuleLogic()
{
  this->SetVolumesLogic(NULL); // release the volumes logic object
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerDicomRtImportModuleLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);

  //TODO: Move this in Contours module
  //  (make sure all the modules are loaded, e.g. bind an observer to the creation of the scene)
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourHierarchyNode>::New());
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
    vtkErrorMacro("Unable to load Dicom RT data due to invalid loadable information.");
    return loadSuccessful;
  }

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);
  std::cout << "Loading series '" << seriesName << "' from file '" << firstFileNameStr << "'" << std::endl;

  vtkSmartPointer<vtkSlicerDicomRtReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtReader>::New();
  rtReader->SetFileName(firstFileNameStr.c_str());
  rtReader->Update();

  // One series can contain composite information, e.g, an RTPLAN series can contain structure sets and plans as well

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

  return loadSuccessful;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtStructureSet(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo)
{
  vtkSmartPointer<vtkMRMLContourHierarchyNode> contourHierarchySeriesNode;
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchySeriesNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> structureModelHierarchyRootNode;

  vtkStdString firstFileNameStr = loadInfo->GetLoadableFiles(0)->GetValue(0);
  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string phSeriesNodeName(seriesName);
  phSeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Add color table node
  vtkSmartPointer<vtkMRMLColorTableNode> structureSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string structureSetColorTableNodeName;
  structureSetColorTableNodeName = std::string(seriesName) + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  structureSetColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(structureSetColorTableNodeName);
  structureSetColorTableNode->SetName(structureSetColorTableNodeName.c_str());
  structureSetColorTableNode->HideFromEditorsOff();
  structureSetColorTableNode->SetTypeToUser();
  this->GetMRMLScene()->AddNode(structureSetColorTableNode);

  // Add ROIs
  int numberOfRois = rtReader->GetNumberOfRois();
  structureSetColorTableNode->SetNumberOfColors(numberOfRois+2);
  structureSetColorTableNode->GetLookupTable()->SetTableRange(0,numberOfRois+1);
  structureSetColorTableNode->AddColor("Background", 0.0, 0.0, 0.0, 0.0); // Black background
  structureSetColorTableNode->AddColor("Invalid", 0.5, 0.5, 0.5, 1.0); // Color indicating invalid index

  vtkSmartPointer<vtkPolyDataCollection> roiCollection = vtkSmartPointer<vtkPolyDataCollection>::New();
  vtkSmartPointer<vtkCollection> displayNodeCollection = vtkSmartPointer<vtkCollection>::New();

  for (int internalROIIndex=0; internalROIIndex<numberOfRois; internalROIIndex++) // DICOM starts indexing from 1
  {
    const char* roiLabel = rtReader->GetRoiName(internalROIIndex);
    double *roiColor = rtReader->GetRoiDisplayColor(internalROIIndex);
    vtkMRMLDisplayableNode* addedDisplayableNode = NULL;

    // Save color into the color table
    structureSetColorTableNode->AddColor(roiLabel, roiColor[0], roiColor[1], roiColor[2]);

    // Get structure
    vtkPolyData* roiPoly = rtReader->GetRoiPolyData(internalROIIndex);
    if (roiPoly == NULL)
    {
      vtkWarningMacro("Cannot read polydata from file: " << firstFileNameStr << ", ROI: " << internalROIIndex);
      continue;
    }
    if (roiPoly->GetNumberOfPoints() < 1)
    {
      vtkWarningMacro("The ROI polydata does not contain any points, file: " << firstFileNameStr << ", ROI: " << internalROIIndex);
      continue;
    }

    if (roiPoly->GetNumberOfPoints() == 1)
    {	
      // Point ROI
      addedDisplayableNode = this->AddRoiPoint(roiPoly->GetPoint(0), roiLabel, roiColor);

      // Create PatientHierarchy node and associate it with the ROI
      if (addedDisplayableNode)
      {
        // Create root patient hierarchy node for the series, if it has not been created yet
        if (patientHierarchySeriesNode.GetPointer()==NULL)
        {
          patientHierarchySeriesNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
          patientHierarchySeriesNode->SetName(phSeriesNodeName.c_str());
          patientHierarchySeriesNode->HideFromEditorsOff();
          patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
            SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
          patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
            vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
          patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
            rtReader->GetSeriesInstanceUid());
          this->GetMRMLScene()->AddNode(patientHierarchySeriesNode);
        }

        // Create patient hierarchy entry for the ROI
        vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> patientHierarchyRoiNode = vtkSmartPointer<vtkMRMLAnnotationHierarchyNode>::New();
        std::string phNodeName;
        phNodeName = std::string(roiLabel) + SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX;
        phNodeName = this->GetMRMLScene()->GenerateUniqueName(phNodeName);
        patientHierarchyRoiNode->SetName(phNodeName.c_str());
        patientHierarchyRoiNode->AllowMultipleChildrenOn();
        patientHierarchyRoiNode->HideFromEditorsOff();
        patientHierarchyRoiNode->SetAssociatedNodeID(addedDisplayableNode->GetID());
        patientHierarchyRoiNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        patientHierarchyRoiNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
        patientHierarchyRoiNode->SetParentNodeID(patientHierarchySeriesNode->GetID());
        this->GetMRMLScene()->AddNode(patientHierarchyRoiNode);
      }
    } // endif Point ROI
    else
    {
      // Contour ROI
      std::string contourNodeName;
      contourNodeName = std::string(roiLabel) + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
      contourNodeName = this->GetMRMLScene()->GenerateUniqueName(contourNodeName);

      addedDisplayableNode = this->AddRoiContour(roiPoly, contourNodeName, roiColor);

      roiCollection->AddItem(roiPoly);

      // Create Contour and PatientHierarchy nodes and make association
      if (addedDisplayableNode)
      {
        // Create root contour hierarchy node for the series, if it has not been created yet
        if (contourHierarchySeriesNode.GetPointer()==NULL)
        {
          std::string contourHierarchySeriesNodeName(seriesName);
          contourHierarchySeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX);
          contourHierarchySeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
          contourHierarchySeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(contourHierarchySeriesNodeName);
          contourHierarchySeriesNode = vtkSmartPointer<vtkMRMLContourHierarchyNode>::New();
          contourHierarchySeriesNode->SetName(contourHierarchySeriesNodeName.c_str());
          contourHierarchySeriesNode->AllowMultipleChildrenOn();
          contourHierarchySeriesNode->HideFromEditorsOff();
          contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
            SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
          contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
            vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
          contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
            rtReader->GetSeriesInstanceUid());
          contourHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME.c_str(), seriesName);
          this->GetMRMLScene()->AddNode(contourHierarchySeriesNode);

          // A hierarchy node needs a display node
          vtkSmartPointer<vtkMRMLModelDisplayNode> contourHierarchySeriesDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
          contourHierarchySeriesNodeName.append("Display");
          contourHierarchySeriesDisplayNode->SetName(contourHierarchySeriesNodeName.c_str());
          contourHierarchySeriesDisplayNode->SetVisibility(1);
          this->GetMRMLScene()->AddNode(contourHierarchySeriesDisplayNode);
          contourHierarchySeriesNode->SetAndObserveDisplayNodeID( contourHierarchySeriesDisplayNode->GetID() );
        }

        // Create contour node
        vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
        contourNode = vtkMRMLContourNode::SafeDownCast(this->GetMRMLScene()->AddNode(contourNode));
        contourNode->SetName(contourNodeName.c_str());
        contourNode->SetStructureName(roiLabel);
        contourNode->SetAndObserveRibbonModelNodeId(addedDisplayableNode->GetID());
        contourNode->SetActiveRepresentationByNode(addedDisplayableNode);
        contourNode->HideFromEditorsOff();

        // Put the contour node in the hierarchy
        vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> contourHierarchyNode
          = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
        std::string phContourNodeName(contourNodeName);
        phContourNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
        phContourNodeName = this->GetMRMLScene()->GenerateUniqueName(phContourNodeName);
        contourHierarchyNode->SetName(phContourNodeName.c_str());
        contourHierarchyNode->SetParentNodeID( contourHierarchySeriesNode->GetID() );
        contourHierarchyNode->SetAssociatedNodeID( contourNode->GetID() );
        contourHierarchyNode->HideFromEditorsOff();
        contourHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        contourHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
        this->GetMRMLScene()->AddNode(contourHierarchyNode);

        displayNodeCollection->AddItem( vtkMRMLModelNode::SafeDownCast(addedDisplayableNode)->GetModelDisplayNode() );
      }
    } // endif Contour ROI

    // Add new node to the model hierarchy
    if (addedDisplayableNode)
    {
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

  // Add color table in patient hierarchy
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchyColorTableNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  std::string phColorTableNodeName;
  phColorTableNodeName = structureSetColorTableNodeName + SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX;
  phColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(phColorTableNodeName);
  patientHierarchyColorTableNode->SetName(phColorTableNodeName.c_str());
  patientHierarchyColorTableNode->HideFromEditorsOff();
  patientHierarchyColorTableNode->SetAssociatedNodeID(structureSetColorTableNode->GetID());
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  patientHierarchyColorTableNode->SetParentNodeID(
    patientHierarchySeriesNode.GetPointer() ? patientHierarchySeriesNode->GetID() : contourHierarchySeriesNode->GetID() );
  this->GetMRMLScene()->AddNode(patientHierarchyColorTableNode);

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
      vtkWarningMacro("Unable to auto-determine opacity: Number of ROIs and display nodes do not match!");
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
  phSeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
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

  if (volumeStorageNode->ReadData(volumeNode))
  {
    volumeNode->SetScene(this->GetMRMLScene());
    std::string volumeNodeName = this->GetMRMLScene()->GenerateUniqueName(seriesName);
    volumeNode->SetName(volumeNodeName.c_str());
    this->GetMRMLScene()->AddNode(volumeNode);

    // Set new spacing
    double* initialSpacing = volumeNode->GetSpacing();
    double* correctSpacing = rtReader->GetPixelSpacing();
    volumeNode->SetSpacing(correctSpacing[0], correctSpacing[1], initialSpacing[2]);
    volumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), rtReader->GetDoseUnits());
    volumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str(), rtReader->GetDoseGridScaling());

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

    // Set default colormap to rainbow
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    volumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    this->GetMRMLScene()->AddNode(volumeDisplayNode);
    volumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

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
  else
  {
    vtkErrorMacro("Failed to load dose volume file '" << firstFileNameStr << "' (series name '" << seriesName << "')");
    return false;
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportModuleLogic::LoadRtPlan(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo)
{
  vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> isocenterSeriesHierarchyRootNode;
  vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> sourceHierarchyRootNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyRootNode;

  const char* seriesName = loadInfo->GetLoadableName(0);

  std::string phSeriesNodeName(seriesName);
  phSeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  vtkMRMLDisplayableNode* addedDisplayableNode = NULL;
  int numberOfBeams = rtReader->GetNumberOfBeams();
  for (int dicomBeamIndex = 1; dicomBeamIndex < numberOfBeams+1; dicomBeamIndex++) // DICOM starts indexing from 1
  {
    // Isocenter fiducial
    double isoColor[3] = { 1.0, 1.0, 1.0 };
    addedDisplayableNode= this->AddRoiPoint(rtReader->GetBeamIsocenterPositionRas(dicomBeamIndex), rtReader->GetBeamName(dicomBeamIndex), isoColor);

    // Add new node to the hierarchy node
    if (addedDisplayableNode)
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
        beamModelHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
          SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
        beamModelHierarchyRootNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
          vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
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
        vtkMRMLAnnotationHierarchyNode::SafeDownCast( vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), addedDisplayableNode->GetID()) );
      isocenterHierarchyNode->SetParentNodeID( isocenterSeriesHierarchyRootNode->GetID() );
      isocenterHierarchyNode->SetIndexInParent(dicomBeamIndex);

      // Create patient hierarchy entry for the isocenter fiducial
      vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchyFiducialNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
      std::string phFiducialNodeName(rtReader->GetBeamName(dicomBeamIndex));
      phFiducialNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
      patientHierarchyFiducialNode->SetName(phFiducialNodeName.c_str());
      patientHierarchyFiducialNode->HideFromEditorsOff();
      patientHierarchyFiducialNode->SetAssociatedNodeID(addedDisplayableNode->GetID());
      patientHierarchyFiducialNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
        SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
      patientHierarchyFiducialNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
      patientHierarchyFiducialNode->SetParentNodeID(isocenterSeriesHierarchyRootNode->GetID());
      this->GetMRMLScene()->AddNode(patientHierarchyFiducialNode);

      // Add attributes containing beam information to the isocenter fiducial node
      // TODO: Add these in the PatientHierarchy node when available
      std::stringstream sourceAxisDistanceStream;
      sourceAxisDistanceStream << rtReader->GetBeamSourceAxisDistance(dicomBeamIndex);
      addedDisplayableNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str(),
        sourceAxisDistanceStream.str().c_str() );
      std::stringstream gantryAngleStream;
      gantryAngleStream << rtReader->GetBeamGantryAngle(dicomBeamIndex);
      addedDisplayableNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str(),
        gantryAngleStream.str().c_str() );
      std::stringstream couchAngleStream;
      couchAngleStream << rtReader->GetBeamPatientSupportAngle(dicomBeamIndex);
      addedDisplayableNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_COUCH_ANGLE_ATTRIBUTE_NAME.c_str(),
        couchAngleStream.str().c_str() );
      std::stringstream collimatorAngleStream;
      collimatorAngleStream << rtReader->GetBeamBeamLimitingDeviceAngle(dicomBeamIndex);
      addedDisplayableNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str(),
        collimatorAngleStream.str().c_str() );
      std::stringstream jawPositionsStream;
      double jawPositions[2][2] = {{0.0, 0.0},{0.0, 0.0}};
      rtReader->GetBeamLeafJawPositions(dicomBeamIndex, jawPositions);
      jawPositionsStream << jawPositions[0][0] << " " << jawPositions[0][1] << " "
        << jawPositions[1][0] << " " << jawPositions[1][1];
      addedDisplayableNode->SetAttribute( SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME.c_str(),
        jawPositionsStream.str().c_str() );

      // Create source fiducial and beam model nodes
      std::string sourceFiducialName;
      sourceFiducialName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_OUTPUT_SOURCE_FIDUCIAL_PREFIX + std::string(addedDisplayableNode->GetName()) );
      vtkSmartPointer<vtkMRMLAnnotationFiducialNode> sourceFiducialNode = vtkSmartPointer<vtkMRMLAnnotationFiducialNode>::New();
      sourceFiducialNode->SetName(sourceFiducialName.c_str());
      this->GetMRMLScene()->AddNode(sourceFiducialNode);

      // Set up automatically created source fiducial node properly
      vtkMRMLAnnotationHierarchyNode* sourceHierarchyNode =
        vtkMRMLAnnotationHierarchyNode::SafeDownCast( vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), sourceFiducialNode->GetID()) );
      sourceHierarchyNode->SetParentNodeID( sourceHierarchyRootNode->GetID() );
      sourceHierarchyNode->SetIndexInParent(dicomBeamIndex);

      // Add beam model node to the scene
      std::string beamModelName;
      beamModelName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX + std::string(addedDisplayableNode->GetName()) );
      vtkSmartPointer<vtkMRMLModelNode> beamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      beamModelNode->SetName(beamModelName.c_str());
      this->GetMRMLScene()->AddNode(beamModelNode);

      // Create Beams parameter set node
      std::string beamParameterSetNodeName;
      beamParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
        SlicerRtCommon::BEAMS_PARAMETER_SET_BASE_NAME_PREFIX + std::string(addedDisplayableNode->GetName()) );
      vtkSmartPointer<vtkMRMLBeamsNode> beamParameterSetNode = vtkSmartPointer<vtkMRMLBeamsNode>::New();
      beamParameterSetNode->SetName(beamParameterSetNodeName.c_str());
      beamParameterSetNode->SetAndObserveIsocenterFiducialNodeId(addedDisplayableNode->GetID());
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
        vtkWarningMacro("Failed to create beam geometry for isocenter: " << addedDisplayableNode->GetName());
      }

      // Put new beam model in the patient hierarchy
      vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      std::string phBeamNodeName = beamModelName + SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX;
      beamModelHierarchyNode->SetName(phBeamNodeName.c_str());
      beamModelHierarchyNode->HideFromEditorsOff();
      beamModelHierarchyNode->SetDisplayableNodeID(beamModelNode->GetID());
      beamModelHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
        SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
      beamModelHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
      beamModelHierarchyNode->SetParentNodeID(beamModelHierarchyRootNode->GetID());
      beamModelHierarchyNode->SetIndexInParent(dicomBeamIndex);
      this->GetMRMLScene()->AddNode(beamModelHierarchyNode);

    } //endif addedDisplayableNode
  }

  // Insert plan isocenter series in patient hierarchy
  this->InsertSeriesInPatientHierarchy(rtReader);

  // Insert beam model subseries under the study
  vtkMRMLHierarchyNode* studyNode = isocenterSeriesHierarchyRootNode->GetParentNode();
  if (studyNode && SlicerRtCommon::IsPatientHierarchyNode(studyNode))
  {
    beamModelHierarchyRootNode->SetParentNodeID(studyNode->GetID());
  }

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

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
  vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
    this->GetMRMLScene(), rtReader->GetPatientId(), rtReader->GetStudyInstanceUid(), rtReader->GetSeriesInstanceUid() );

  // Fill patient and study attributes if they have been just created
  if (patientNode == NULL)
  {
    patientNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
      this->GetMRMLScene(), rtReader->GetPatientId() );
    if (patientNode)
    {
      patientNode->SetName( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetPatientName())
        ? rtReader->GetPatientName() : SlicerRtCommon::DICOMRTIMPORT_NO_NAME.c_str() );
    }
    else
    {
      vtkErrorMacro("Patient node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }
  if (studyNode == NULL)
  {
    studyNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
      this->GetMRMLScene(), rtReader->GetStudyInstanceUid() );
    if (studyNode)
    {
      studyNode->SetName( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetStudyDescription())
        ? rtReader->GetStudyDescription() : SlicerRtCommon::DICOMRTIMPORT_NO_DESCRIPTION.c_str() );
    }
    else
    {
      vtkErrorMacro("Study node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }
}
