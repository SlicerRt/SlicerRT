/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada and
  Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// DicomRtImportExport includes
#include "vtkSlicerDicomRtImportExportModuleLogic.h"
#include "vtkSlicerDicomRtReader.h"
#include "vtkSlicerDicomRtWriter.h"

// Qt includes
#include <QSettings>

// CTK includes
#include <ctkDICOMDatabase.h>

// SubjectHierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "PlmCommon.h"
#include "vtkMRMLBeamsNode.h"
#include "vtkMRMLIsodoseNode.h"
#include "vtkMRMLPlanarImageNode.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkSlicerPlanarImageModuleLogic.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentationStorageNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkOrientedImageDataResample.h"

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
#include <dcmtk/dcmrt/drtdose.h>
#include <dcmtk/dcmrt/drtimage.h>
#include <dcmtk/dcmrt/drtplan.h>
#include <dcmtk/dcmrt/drtstrct.h>

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>

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

// ITK includes
#include <itkImage.h>

// DICOMLib includes
#include "vtkSlicerDICOMLoadable.h"
#include "vtkSlicerDICOMExportable.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportExportModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportExportModuleLogic, VolumesLogic, vtkSlicerVolumesLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportExportModuleLogic, IsodoseLogic, vtkSlicerIsodoseModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportExportModuleLogic, PlanarImageLogic, vtkSlicerPlanarImageModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportExportModuleLogic::vtkSlicerDicomRtImportExportModuleLogic()
{
  this->VolumesLogic = NULL;
  this->IsodoseLogic = NULL;
  this->PlanarImageLogic = NULL;

  this->BeamModelsInSeparateBranch = true;
  this->DefaultDoseColorTableNodeId = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportExportModuleLogic::~vtkSlicerDicomRtImportExportModuleLogic()
{
  this->SetVolumesLogic(NULL);
  this->SetIsodoseLogic(NULL);
  this->SetPlanarImageLogic(NULL);
  this->SetDefaultDoseColorTableNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->SetDefaultDoseColorTableNodeId(NULL);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::ExamineForLoad(vtkStringArray* fileList, vtkCollection* loadables)
{
  if (!fileList || !loadables)
  {
    return;
  }
  loadables->RemoveAllItems();

  for (int fileIndex=0; fileIndex<fileList->GetNumberOfValues(); ++fileIndex)
  {
    // Load file in DCMTK
    DcmFileFormat fileformat;
    vtkStdString fileName = fileList->GetValue(fileIndex);
    OFCondition result = fileformat.loadFile(fileName.c_str(), EXS_Unknown);
    if (!result.good())
    {
      continue; // Failed to parse this file, skip it
    }

    // Check SOP Class UID for one of the supported RT objects
    DcmDataset *dataset = fileformat.getDataset();
    OFString sopClass;
    if (!dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() || sopClass.empty())
    {
      continue; // Failed to parse this file, skip it
    }

    // DICOM parsing is successful, now check if the object is loadable 
    OFString name("");
    OFString seriesNumber("");
    std::vector<OFString> referencedSOPInstanceUIDs;
    dataset->findAndGetOFString(DCM_SeriesNumber, seriesNumber);
    if (!seriesNumber.empty())
    {
      name += seriesNumber + ": ";
    }

    // RTDose
    if (sopClass == UID_RTDoseStorage)
    {
      // Assemble name
      name += "RTDOSE";
      OFString instanceNumber;
      dataset->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
      OFString seriesDescription;
      dataset->findAndGetOFString(DCM_SeriesDescription, seriesDescription);
      if (!seriesDescription.empty())
      {
        name += ": " + seriesDescription; 
      }
      if (!instanceNumber.empty())
      {
        name += " [" + instanceNumber + "]"; 
      }

      // Find RTPlan name for RTDose series
      OFString referencedSOPInstanceUID("");
      DRTDoseIOD rtDoseObject;
      if (rtDoseObject.read(*dataset).good())
      {
        DRTReferencedRTPlanSequence &referencedRTPlanSequence = rtDoseObject.getReferencedRTPlanSequence();
        if (referencedRTPlanSequence.gotoFirstItem().good())
        {
          DRTReferencedRTPlanSequence::Item &referencedRTPlanSequenceItem = referencedRTPlanSequence.getCurrentItem();
          if (referencedRTPlanSequenceItem.isValid())
          {
            if (referencedRTPlanSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
            {
              referencedSOPInstanceUIDs.push_back(referencedSOPInstanceUID);
            }
          }
        }
      }

      // Create and open DICOM database to perform database operations for getting RTPlan name
      QSettings settings;
      QString databaseDirectory = settings.value("DatabaseDirectory").toString();
      QString databaseFile = databaseDirectory + vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_DATABASE_FILENAME.c_str();
      ctkDICOMDatabase* dicomDatabase = new ctkDICOMDatabase();
      dicomDatabase->openDatabase(databaseFile, vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());

      // Get RTPlan name to show it with the dose
      QString rtPlanLabelTag("300a,0002");
      QString rtPlanFileName = dicomDatabase->fileForInstance(referencedSOPInstanceUID.c_str());
      if (!rtPlanFileName.isEmpty())
      {
        name += OFString(": ") + OFString(dicomDatabase->fileValue(rtPlanFileName,rtPlanLabelTag).toLatin1().constData());
      }

      // Close and delete DICOM database
      dicomDatabase->closeDatabase();
      delete dicomDatabase;
      QSqlDatabase::removeDatabase(vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_CONNECTION_NAME.c_str());
      QSqlDatabase::removeDatabase(QString(vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_CONNECTION_NAME.c_str()) + "TagCache");
    }
    // RTPlan
    else if (sopClass == UID_RTPlanStorage)
    {
      // Assemble name
      name += "RTPLAN";
      OFString planLabel;
      dataset->findAndGetOFString(DCM_RTPlanLabel, planLabel);
      OFString planName;
      dataset->findAndGetOFString(DCM_RTPlanName, planName);
      if (!planLabel.empty() && !planName.empty())
      {
        if (planLabel.compare(planName)!=0)
        {
          // Plan label and name is different, display both
          name += ": " + planLabel + " (" + planName + ")";
        }
        else
        {
          name += ": " + planLabel;
        }
      }
      else if (!planLabel.empty() && planName.empty())
      {
        name += ": " + planLabel;
      }
      else if (planLabel.empty() && !planName.empty())
      {
        name += ": " + planName;
      }
    }
    // RTStructureSet
    else if (sopClass == UID_RTStructureSetStorage)
    {
      // Assemble name
      name += "RTSTRUCT";
      OFString structLabel;
      dataset->findAndGetOFString(DCM_StructureSetLabel, structLabel);
      if (!structLabel.empty())
      {
        name += ": " + structLabel;
      }

      // Get referenced image instance UIDs
      OFString referencedSOPInstanceUID("");
      DRTStructureSetIOD rtStructureSetObject;
      if (rtStructureSetObject.read(*dataset).good())
      {
        DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject.getROIContourSequence();
        if (rtROIContourSequenceObject.gotoFirstItem().good())
        {
          do // For all ROIs
          {
            DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
            if (currentRoiObject.isValid())
            {
              DRTContourSequence &rtContourSequenceObject = currentRoiObject.getContourSequence();
              if (rtContourSequenceObject.gotoFirstItem().good())
              {
                do // For all contours
                {
                  DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
                  if (!contourItem.isValid())
                  {
                    DRTContourImageSequence &rtContourImageSequenceObject = contourItem.getContourImageSequence();
                    if (rtContourImageSequenceObject.gotoFirstItem().good())
                    {
                      DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
                      if (rtContourImageSequenceItem.isValid())
                      {
                        OFString referencedSOPInstanceUID("");
                        if (rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
                        {
                          referencedSOPInstanceUIDs.push_back(referencedSOPInstanceUID);
                        }
                      }
                    }
                  }
                } // For all contours
                while (rtContourSequenceObject.gotoNextItem().good());
              }
            }
          } // For all ROIs
          while (rtROIContourSequenceObject.gotoNextItem().good());
        } // End ROIContourSequence

        // If the above tags do not store the referenced instance UIDs, then look at the other possible place
        if (referencedSOPInstanceUIDs.empty())
        {
          DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject.getReferencedFrameOfReferenceSequence();
          if (rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
          {
            DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
            if (currentReferencedFrameOfReferenceSequenceItem.isValid())
            {
              DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
              if (rtReferencedStudySequenceObject.gotoFirstItem().good())
              {
                DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
                if (rtReferencedStudySequenceItem.isValid())
                {
                  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
                  if (rtReferencedSeriesSequenceObject.gotoFirstItem().good())
                  {
                    if (rtReferencedSeriesSequenceObject.gotoFirstItem().good())
                    {
                      DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject.getCurrentItem();
                      if (rtReferencedSeriesSequenceItem.isValid())
                      {
                        DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
                        if (rtContourImageSequenceObject.gotoFirstItem().good())
                        {
                          do
                          {
                            DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
                            if (rtContourImageSequenceItem.isValid())
                            {
                              OFString referencedSOPInstanceUID("");
                              if (rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
                              {
                                referencedSOPInstanceUIDs.push_back(referencedSOPInstanceUID);
                              }
                            }
                          } // For all contours
                          while (rtContourImageSequenceObject.gotoNextItem().good());
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        } // End DRTReferencedFrameOfReferenceSequence
      } // End finding referenced instance UIDs
    }
    // RTImage
    else if (sopClass == UID_RTImageStorage)
    {
      // Assemble name
      name += "RTIMAGE";
      OFString imageLabel;
      dataset->findAndGetOFString(DCM_RTImageLabel, imageLabel);
      if (!imageLabel.empty())
      {
        name += ": " + imageLabel;
      }

      // Get referenced RTPlan
      OFString referencedSOPInstanceUID("");
      DRTImageIOD rtImageObject;
      if (rtImageObject.read(*dataset).good())
      {
        DRTReferencedRTPlanSequenceInRTImageModule &rtReferencedRtPlanSequenceObject = rtImageObject.getReferencedRTPlanSequence();
        if (rtReferencedRtPlanSequenceObject.gotoFirstItem().good())
        {
          DRTReferencedRTPlanSequenceInRTImageModule::Item &currentReferencedRtPlanSequenceObject = rtReferencedRtPlanSequenceObject.getCurrentItem();
          if (currentReferencedRtPlanSequenceObject.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
          {
            referencedSOPInstanceUIDs.push_back(referencedSOPInstanceUID);
          }
        }
      }
    }
    /* Not yet supported
    else if (sopClass == UID_RTTreatmentSummaryRecordStorage)
    else if (sopClass == UID_RTIonPlanStorage)
    else if (sopClass == UID_RTIonBeamsTreatmentRecordStorage)
    */
    else
    {
      continue; // Not an RT file
    }

    // The file is a loadable RT object, create and set up loadable
    vtkSmartPointer<vtkSlicerDICOMLoadable> loadable = vtkSmartPointer<vtkSlicerDICOMLoadable>::New();
    loadable->SetName(name.c_str());
    loadable->AddFile(fileName.c_str());
    loadable->SetConfidence(1.0);
    loadable->SetSelected(true);
    std::vector<OFString>::iterator uidIt;
    for (uidIt = referencedSOPInstanceUIDs.begin(); uidIt != referencedSOPInstanceUIDs.end(); ++uidIt)
    {
      loadable->AddReferencedInstanceUID(uidIt->c_str());
    }
    loadables->AddItem(loadable);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportExportModuleLogic::LoadDicomRT(vtkSlicerDICOMLoadable* loadable)
{
  bool loadSuccessful = false;

  if (!loadable || loadable->GetFiles()->GetNumberOfValues() < 1 || loadable->GetConfidence() == 0.0)
  {
    vtkErrorMacro("LoadDicomRT: Unable to load DICOM-RT data due to invalid loadable information!");
    return loadSuccessful;
  }

  const char* firstFileName = loadable->GetFiles()->GetValue(0);

  std::cout << "Loading series '" << loadable->GetName() << "' from file '" << firstFileName << "'" << std::endl;

  vtkSmartPointer<vtkSlicerDicomRtReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtReader>::New();
  rtReader->SetFileName(firstFileName);
  rtReader->Update();

  // One series can contain composite information, e.g, an RTPLAN series can contain structure sets and plans as well
  // TODO: vtkSlicerDicomRtReader class does not support this yet

  // RTSTRUCT
  if (rtReader->GetLoadRTStructureSetSuccessful())
  {
    loadSuccessful = this->LoadRtStructureSet(rtReader, loadable);
  }

  // RTDOSE
  if (rtReader->GetLoadRTDoseSuccessful())
  {
    loadSuccessful = this->LoadRtDose(rtReader, loadable);
  }

  // RTPLAN
  if (rtReader->GetLoadRTPlanSuccessful())
  {
    loadSuccessful = this->LoadRtPlan(rtReader, loadable);
  }

  // RTIMAGE
  if (rtReader->GetLoadRTImageSuccessful())
  {
    loadSuccessful = this->LoadRtImage(rtReader, loadable);
  }

  return loadSuccessful;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportExportModuleLogic::LoadRtStructureSet(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable)
{
  vtkMRMLSubjectHierarchyNode* fiducialsSeriesSubjectHierarchyNode = NULL;
  vtkMRMLSubjectHierarchyNode* segmentationSubjectHierarchyNode = NULL;
  vtkSmartPointer<vtkMRMLSegmentationNode> segmentationNode;
  vtkSmartPointer<vtkMRMLSegmentationDisplayNode> segmentationDisplayNode;

  const char* fileName = loadable->GetFiles()->GetValue(0);
  const char* seriesName = loadable->GetName();
  std::string structureSetReferencedSeriesUid("");

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Get referenced SOP instance UIDs
  const char* referencedSopInstanceUids = rtReader->GetRTStructureSetReferencedSOPInstanceUIDs();

  // Add ROIs
  int numberOfRois = rtReader->GetNumberOfRois();
  for (int internalROIIndex=0; internalROIIndex<numberOfRois; internalROIIndex++)
  {
    // Get name and color
    const char* roiLabel = rtReader->GetRoiName(internalROIIndex);
    double *roiColor = rtReader->GetRoiDisplayColor(internalROIIndex);

    // Get structure
    vtkPolyData* roiPolyData = rtReader->GetRoiPolyData(internalROIIndex);
    if (roiPolyData == NULL)
    {
      vtkWarningMacro("LoadRtStructureSet: Invalid structure ROI data for ROI named '"
        << (roiLabel?roiLabel:"Unnamed") << "' in file '" << fileName
        << "' (internal ROI index: " << internalROIIndex << ")");
      continue;
    }
    if (roiPolyData->GetNumberOfPoints() == 0)
    {
      vtkWarningMacro("LoadRtStructureSet: Structure ROI data does not contain any points for ROI named '"
        << (roiLabel?roiLabel:"Unnamed") << "' in file '" << fileName
        << "' (internal ROI index: " << internalROIIndex << ")");
      continue;
    }

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

    //
    // Point ROI (fiducial)
    //
    if (roiPolyData->GetNumberOfPoints() == 1)
    {
      // Do not allow both fiducial and contour type structures to be loaded
      if (segmentationSubjectHierarchyNode)
      {
        vtkWarningMacro("LoadRtStructureSet: Structure set contains both contour and fiducial type structures. The first structure being a contour, fiducials will be skipped!");
        continue;
      }

      // Create subject hierarchy node for the series, if it has not been created yet.
      // Only create it for fiducials, as all structures are stored in a single segmentation node
      if (fiducialsSeriesSubjectHierarchyNode == NULL)
      {
        std::string fiducialsSeriesNodeName(seriesName);
        fiducialsSeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_FIDUCIALS_HIERARCHY_NODE_NAME_POSTFIX);
        fiducialsSeriesNodeName.append(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix());
        fiducialsSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(fiducialsSeriesNodeName);
        fiducialsSeriesSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
          this->GetMRMLScene(), NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(),
          fiducialsSeriesNodeName.c_str());
        fiducialsSeriesSubjectHierarchyNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetSeriesInstanceUid());
      }

      // Creates fiducial MRML node and display node
      vtkMRMLDisplayableNode* fiducialNode = this->AddRoiPoint(roiPolyData->GetPoint(0), roiLabel, roiColor);

      // Create subject hierarchy entry for the ROI
      vtkSmartPointer<vtkMRMLSubjectHierarchyNode> fiducialSubjectHierarchyNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
      std::string fiducialSubjectHierarchyNodeName = std::string(roiLabel) + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix();
      fiducialSubjectHierarchyNodeName = this->GetMRMLScene()->GenerateUniqueName(fiducialSubjectHierarchyNodeName);
      vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
        this->GetMRMLScene(), fiducialsSeriesSubjectHierarchyNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
        fiducialSubjectHierarchyNodeName.c_str(), fiducialNode);
      fiducialSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(), roiReferencedSeriesUid);
    }
    //
    // Contour ROI (segmentation)
    //
    else
    {
      // Do not allow both fiducial and contour type structures to be loaded
      if (fiducialsSeriesSubjectHierarchyNode)
      {
        vtkWarningMacro("LoadRtStructureSet: Structure set contains both contour and fiducial type structures. The first structure being a fiducial, contours will be skipped!");
        continue;
      }

      // Create segmentation node for the structure set series, if not created yet
      if (segmentationNode.GetPointer() == NULL)
      {
        segmentationNode = vtkSmartPointer<vtkMRMLSegmentationNode>::New();
        segmentationNode->SetName(seriesName);
        this->GetMRMLScene()->AddNode(segmentationNode);

        // Set master representation to planar contour
        segmentationNode->GetSegmentation()->SetMasterRepresentationName(vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName());

        // Get image geometry from previously loaded volume if found
        // Segmentation node checks added nodes and sets the geometry parameter in case the referenced volume is loaded later
        vtkMRMLSubjectHierarchyNode* referencedVolumeShNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
          this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), roiReferencedSeriesUid);
        if (referencedVolumeShNode)
        {
          vtkMRMLScalarVolumeNode* referencedVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
            referencedVolumeShNode->GetAssociatedNode() );
          if (referencedVolumeNode)
          {
            vtkSmartPointer<vtkMatrix4x4> referenceImageGeometryMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
            referencedVolumeNode->GetIJKToRASMatrix(referenceImageGeometryMatrix);
            std::string serializedImageGeometry = vtkSegmentationConverter::SerializeImageGeometry(referenceImageGeometryMatrix, referencedVolumeNode->GetImageData());
            segmentationNode->GetSegmentation()->SetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName(), serializedImageGeometry);
          }
          else
          {
            vtkErrorMacro("LoadRtStructureSet: Referenced volume series node does not contain a volume!");
          }
        }

        // Set up subject hierarchy node for segmentation
        segmentationSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
          this->GetMRMLScene(), NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(),
          seriesName, segmentationNode);
        segmentationSubjectHierarchyNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetSeriesInstanceUid());
        segmentationSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(), structureSetReferencedSeriesUid.c_str());
        segmentationSubjectHierarchyNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName().c_str(),
          referencedSopInstanceUids);

        // Setup segmentation display and storage
        segmentationDisplayNode = vtkSmartPointer<vtkMRMLSegmentationDisplayNode>::New();
        this->GetMRMLScene()->AddNode(segmentationDisplayNode);
        segmentationNode->SetAndObserveDisplayNodeID(segmentationDisplayNode->GetID());
        segmentationDisplayNode->SetBackfaceCulling(0);
      }

      // Add segment for current structure
      vtkSmartPointer<vtkSegment> segment = vtkSmartPointer<vtkSegment>::New();
      segment->SetName(roiLabel);
      segment->SetDefaultColor(roiColor[0], roiColor[1], roiColor[2]);
      segment->AddRepresentation(vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName(), roiPolyData);
      segmentationNode->GetSegmentation()->AddSegment(segment);
    }
  } // for all ROIs

  // Force showing closed surface model instead of contour points and calculate auto opacity values for segments
  if (segmentationDisplayNode.GetPointer())
  {
    segmentationDisplayNode->SetPreferredPolyDataDisplayRepresentationName(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
    segmentationDisplayNode->CalculateAutoOpacitiesForSegments();
  }

  // Insert series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Fire modified events if loading is finished
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);

  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportExportModuleLogic::LoadRtDose(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable)
{
  const char* fileName = loadable->GetFiles()->GetValue(0);
  const char* seriesName = loadable->GetName();

  // Load Volume
  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> volumeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  vtkSmartPointer<vtkMRMLScalarVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  volumeStorageNode->SetFileName(fileName);
  volumeStorageNode->ResetFileNameList();
  volumeStorageNode->SetSingleFile(1);

  // Read volume from disk
  if (!volumeStorageNode->ReadData(volumeNode))
  {
    vtkErrorMacro("LoadRtDose: Failed to load dose volume file '" << fileName << "' (series name '" << seriesName << "')");
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
#if (VTK_MAJOR_VERSION <= 5)
  imageCast->SetInput(volumeNode->GetImageData());
#else
  imageCast->SetInputData(volumeNode->GetImageData());
#endif

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
    vtkSlicerIsodoseModuleLogic::ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX + volumeNodeName );
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

  // Setup subject hierarchy entry
  vtkMRMLSubjectHierarchyNode* subjectHierarchySeriesNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    this->GetMRMLScene(), NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), seriesName, volumeNode );
  subjectHierarchySeriesNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(),
    rtReader->GetSeriesInstanceUid());
  subjectHierarchySeriesNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName().c_str(),
    rtReader->GetRTDoseReferencedRTPlanSOPInstanceUID());

  // Insert series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Set dose unit attributes to subject hierarchy study node
  vtkMRMLSubjectHierarchyNode* studyHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(subjectHierarchySeriesNode->GetParentNode());
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
bool vtkSlicerDicomRtImportExportModuleLogic::LoadRtPlan(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable)
{
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> isocenterSeriesHierarchyRootNode;
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> beamModelSubjectHierarchyRootNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyRootNode;

  const char* seriesName = loadable->GetName();
  std::string shSeriesNodeName(seriesName);
  shSeriesNodeName.append(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix());
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
    vtkMRMLMarkupsDisplayNode* markupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(addedMarkupsNode->GetDisplayNode());
    if (markupsDisplayNode)
    {
      markupsDisplayNode->SetGlyphType(vtkMRMLMarkupsDisplayNode::StarBurst2D);
      markupsDisplayNode->SetGlyphScale(8.0);
    }

    // Add new node to the hierarchy node
    if (addedMarkupsNode)
    {
      // Create root isocenter hierarchy node for the plan series, if it has not been created yet
      if (isocenterSeriesHierarchyRootNode.GetPointer()==NULL)
      {
        isocenterSeriesHierarchyRootNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
        isocenterSeriesHierarchyRootNode->SetLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries());
        isocenterSeriesHierarchyRootNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(),
          rtReader->GetSeriesInstanceUid());
        isocenterSeriesHierarchyRootNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str(),
          rtReader->GetSOPInstanceUID());
        std::string isocenterHierarchyRootNodeName;
        isocenterHierarchyRootNodeName = seriesName + SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix();
        isocenterHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(isocenterHierarchyRootNodeName);
        isocenterSeriesHierarchyRootNode->SetName(isocenterHierarchyRootNodeName.c_str());
        this->GetMRMLScene()->AddNode(isocenterSeriesHierarchyRootNode);
      }

      // Create beam model hierarchy root node if has not been created yet
      if (beamModelHierarchyRootNode.GetPointer()==NULL)
      {
        beamModelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        std::string beamModelHierarchyRootNodeName = seriesName + SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX;
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
          std::string beamModelSubjectHierarchyRootNodeName = beamModelHierarchyRootNodeName + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix();
          beamModelSubjectHierarchyRootNodeName = this->GetMRMLScene()->GenerateUniqueName(beamModelSubjectHierarchyRootNodeName);
          beamModelSubjectHierarchyRootNode->SetName(beamModelSubjectHierarchyRootNodeName.c_str());
          beamModelSubjectHierarchyRootNode->SetLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries());
          this->GetMRMLScene()->AddNode(beamModelSubjectHierarchyRootNode);
        }
      }

      // Create subject hierarchy entry for the isocenter fiducial
      vtkMRMLSubjectHierarchyNode* subjectHierarchyFiducialNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
        this->GetMRMLScene(), isocenterSeriesHierarchyRootNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
        rtReader->GetBeamName(dicomBeamNumber), addedMarkupsNode);

      // Set beam related attributes
      std::stringstream beamNumberStream;
      beamNumberStream << dicomBeamNumber;
      subjectHierarchyFiducialNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str(),
        beamNumberStream.str().c_str());

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
      this->GetMRMLScene()->AddNode(beamModelNode); // SH node may be automatically created here depending on auto-creation, but we set it up later

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
      std::string errorMessage = beamsLogic->CreateBeamModel();
      if (!errorMessage.empty())
      {
        vtkWarningMacro("LoadRtPlan: Failed to create beam geometry for isocenter: " << addedMarkupsNode->GetName());
      }

      // Put new beam model in the model hierarchy
      vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      std::string beamModelHierarchyNodeName = beamModelName + SlicerRtCommon::DICOMRTIMPORT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
      beamModelHierarchyNode->SetName(beamModelHierarchyNodeName.c_str());
      beamModelHierarchyNode->SetDisplayableNodeID(beamModelNode->GetID());
      beamModelHierarchyNode->SetParentNodeID(beamModelHierarchyRootNode->GetID());
      this->GetMRMLScene()->AddNode(beamModelHierarchyNode);
      beamModelHierarchyNode->SetIndexInParent(beamIndex);

      // Create display node for the hierarchy node
      vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
      std::string beamModelHierarchyDisplayNodeName = beamModelHierarchyNodeName + std::string("Display");
      beamModelHierarchyDisplayNode->SetName(beamModelHierarchyDisplayNodeName.c_str());
      beamModelHierarchyDisplayNode->SetVisibility(1);
      this->GetMRMLScene()->AddNode(beamModelHierarchyDisplayNode);
      beamModelHierarchyNode->SetAndObserveDisplayNodeID( beamModelHierarchyDisplayNode->GetID() );

      // Setup beam model subject hierarchy node and set it up for nested association by associating it with the beam model hierarchy node
      vtkMRMLSubjectHierarchyNode* beamModelSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode( this->GetMRMLScene(), 
        (this->BeamModelsInSeparateBranch ? beamModelSubjectHierarchyRootNode.GetPointer() : subjectHierarchyFiducialNode),
        vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), beamModelName.c_str(), beamModelHierarchyNode );
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
bool vtkSlicerDicomRtImportExportModuleLogic::LoadRtImage(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable)
{
  const char* fileName = loadable->GetFiles()->GetValue(0);
  const char* seriesName = loadable->GetName();

  // Load Volume
  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> volumeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  vtkSmartPointer<vtkMRMLScalarVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  volumeStorageNode->SetFileName(fileName);
  volumeStorageNode->ResetFileNameList();
  volumeStorageNode->SetSingleFile(1);

  // Read image from disk
  if (!volumeStorageNode->ReadData(volumeNode))
  {
    vtkErrorMacro("LoadRtImage: Failed to load RT image file '" << fileName << "' (series name '" << seriesName << "')");
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

  // Set up subject hierarchy node
  vtkMRMLSubjectHierarchyNode* subjectHierarchySeriesNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    this->GetMRMLScene(), NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), seriesName, volumeNode );
  subjectHierarchySeriesNode->AddUID( vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(),
    rtReader->GetSeriesInstanceUid() );

  // Set RT image specific attributes
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  subjectHierarchySeriesNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME.c_str(),
    rtReader->GetRTImageReferencedRTPlanSOPInstanceUID());
  subjectHierarchySeriesNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName().c_str(),
    rtReader->GetRTImageReferencedRTPlanSOPInstanceUID());

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

  // Insert series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Compute and set RT image geometry. Uses the referenced beam if available, otherwise the geometry will be set up when loading the referenced beam
  this->SetupRtImageGeometry(volumeNode);

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkSlicerDicomRtImportExportModuleLogic::AddRoiPoint(double* roiPosition, std::string baseName, double* roiColor)
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
void vtkSlicerDicomRtImportExportModuleLogic::InsertSeriesInSubjectHierarchy( vtkSlicerDicomRtReader* rtReader )
{
  // Get the higher level parent nodes by their IDs (to fill their attributes later if they do not exist yet)
  vtkMRMLSubjectHierarchyNode* patientNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetPatientId() );
  vtkMRMLSubjectHierarchyNode* studyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetStudyInstanceUid() );

  // Insert series in hierarchy
  vtkMRMLSubjectHierarchyNode* seriesNode = vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
    this->GetMRMLScene(), rtReader->GetPatientId(), rtReader->GetStudyInstanceUid(), rtReader->GetSeriesInstanceUid() );

  // Fill patient and study attributes if they have been just created
  if (patientNode == NULL)
  {
    patientNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
      this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetPatientId() );
    if (patientNode)
    {
      // Add attributes for DICOM tags
      patientNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMPatientNameAttributeName().c_str(), rtReader->GetPatientName() );
      patientNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMPatientIDAttributeName().c_str(), rtReader->GetPatientId() );
      patientNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMPatientSexAttributeName().c_str(), rtReader->GetPatientSex() );
      patientNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMPatientBirthDateAttributeName().c_str(), rtReader->GetPatientBirthDate() );
      patientNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMPatientCommentsAttributeName().c_str(), rtReader->GetPatientComments() );

      // Set node name
      std::string patientNodeName = ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetPatientName())
        ? std::string(rtReader->GetPatientName()) + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix()
        : SlicerRtCommon::DICOMRTIMPORT_NO_NAME + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix() );
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
      this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetStudyInstanceUid() );
    if (studyNode)
    {
      // Add attributes for DICOM tags
      studyNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMStudyDescriptionAttributeName().c_str(), rtReader->GetStudyDescription() );
      studyNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMStudyDateAttributeName().c_str(), rtReader->GetStudyDate() );
      studyNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMStudyTimeAttributeName().c_str(), rtReader->GetStudyTime() );

      // Set node name
      std::string studyDescription = ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetStudyDescription())
        ? std::string(rtReader->GetStudyDescription())
        : SlicerRtCommon::DICOMRTIMPORT_NO_STUDY_DESCRIPTION );
      std::string studyDate =  ( !SlicerRtCommon::IsStringNullOrEmpty(rtReader->GetStudyDate())
        ? + " (" + std::string(rtReader->GetStudyDate()) + ")"
        : "" );
      std::string studyNodeName = studyDescription + studyDate + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix();
      studyNode->SetName( studyNodeName.c_str() );
    }
    else
    {
      vtkErrorMacro("InsertSeriesInSubjectHierarchy: Study node has not been created for series with Instance UID "
        << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    }
  }

  if (seriesNode)
  {
    // Add attributes for DICOM tags to the series hierarchy node
    seriesNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMSeriesModalityAttributeName().c_str(), rtReader->GetSeriesModality() );
    seriesNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMSeriesNumberAttributeName().c_str(), rtReader->GetSeriesNumber() );

    // Set SOP instance UID (RT objects are in one file so have one SOP instance UID per series)
    seriesNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName(), rtReader->GetSOPInstanceUID());
  }
  else
  {
    vtkErrorMacro("InsertSeriesInSubjectHierarchy: Failed to insert series with Instance UID "
      << (rtReader->GetSeriesInstanceUid() ? rtReader->GetSeriesInstanceUid() : "Missing UID") );
    return;
  }
}

//------------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::CreateDefaultDoseColorTable()
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
    vtkMRMLColorTableNode* doseColorTable = vtkMRMLColorTableNode::SafeDownCast(
      defaultDoseColorTableNodes->GetItemAsObject(0) );
    this->SetDefaultDoseColorTableNodeId(doseColorTable->GetID());
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
  defaultDoseColorTable->SetSingletonTag(SlicerRtCommon::DICOMRTIMPORT_DEFAULT_DOSE_COLOR_TABLE_NAME);
  defaultDoseColorTable->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  defaultDoseColorTable->HideFromEditorsOff();
  defaultDoseColorTable->SetNumberOfColors(256);

  SlicerRtCommon::StretchDiscreteColorTable(defaultIsodoseColorTable, defaultDoseColorTable);

  this->GetMRMLScene()->AddNode(defaultDoseColorTable);
  this->SetDefaultDoseColorTableNodeId(defaultDoseColorTable->GetID());
}

//------------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::SetupRtImageGeometry(vtkMRMLNode* node)
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
      isocenterNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(isocenterSubjectHierarchyNode->GetAssociatedNode());
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
            isocenterNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(isocenterSubjectHierarchyNode->GetAssociatedNode());
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
      if (hierarchyNode && hierarchyNode->GetAssociatedNode() && hierarchyNode->GetAssociatedNode()->IsA("vtkMRMLScalarVolumeNode"))
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
            rtImageVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(hierarchyNode->GetAssociatedNode());
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
  displayedModelNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  vtkSmartPointer<vtkMRMLScalarVolumeNode> textureVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  this->GetMRMLScene()->AddNode(textureVolumeNode);
  std::string textureVolumeNodeName = SlicerRtCommon::PLANARIMAGE_TEXTURE_NODE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName());
  textureVolumeNode->SetName(textureVolumeNodeName.c_str());
  textureVolumeNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

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

//----------------------------------------------------------------------------
std::string vtkSlicerDicomRtImportExportModuleLogic::ExportDicomRTStudy(vtkCollection* exportables)
{
  std::string error("");
  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    error = "MRML scene not valid";
    vtkErrorMacro("ExportDicomRTStudy: " + error);
    return error;
  }

  if (exportables->GetNumberOfItems() < 1)
  {
    error = "Exportable list contains no exportables";
    vtkErrorMacro("ExportDicomRTStudy: " + error);
    return error;
  }

  // Get common export parameters from first exportable
  vtkSlicerDICOMExportable* firstExportable = vtkSlicerDICOMExportable::SafeDownCast(exportables->GetItemAsObject(0));
  const char* patientName = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientNameTagName().c_str());
  const char* patientID = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientIDTagName().c_str());
  const char* outputPath = firstExportable->GetDirectory();

  // Get nodes for the different roles from the exportable list
  vtkMRMLScalarVolumeNode* doseNode = NULL;
  vtkMRMLSegmentationNode* segmentationNode = NULL;
  vtkMRMLScalarVolumeNode* imageNode = NULL;
  for (int index=0; index<exportables->GetNumberOfItems(); ++index)
  {
    vtkSlicerDICOMExportable* exportable = vtkSlicerDICOMExportable::SafeDownCast(
      exportables->GetItemAsObject(index) );
    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      mrmlScene->GetNodeByID(exportable->GetNodeID()) );
    if (!shNode)
    {
      vtkWarningMacro("ExportDicomRTStudy: Failed to get node from exportable with node ID " + std::string(exportable->GetNodeID()));
      // There might be enough exportables for a successful export, all roles are checked later
      continue;
    }
    vtkMRMLNode* associatedNode = shNode->GetAssociatedNode();

    // Check if dose volume and set it if found
    if (associatedNode && SlicerRtCommon::IsDoseVolumeNode(associatedNode))
    {
      doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(associatedNode);
    }
    // Check if segmentation node and set if found
    else if (associatedNode && associatedNode->IsA("vtkMRMLSegmentationNode"))
    {
      segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(associatedNode);;
    }
    // Check if other volume (anatomical volume role) and set if found
    else if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
    {
      imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(associatedNode);
    }
    // Report warning if a node cannot be assigned a role
    else
    {
      vtkWarningMacro("ExportDicomRTStudy: Unable to assign supported RT role to exported node " + shNode->GetNameWithoutPostfix());
    }
  }

  // Make sure there is an image node.  Don't check for struct / dose, as those are optional
  if (!imageNode)
  {
    error = "Must export the primary anatomical (CT/MR) image";
    vtkErrorMacro("ExportDicomRTStudy: " + error);
    return error;
  }

  // Create RT writer
  vtkSmartPointer<vtkSlicerDicomRtWriter> rtWriter = vtkSmartPointer<vtkSlicerDicomRtWriter>::New();

  // Set metadata
  rtWriter->SetPatientName(patientName);
  rtWriter->SetPatientID(patientID);

  // Convert input image (CT/MR/etc) to the format Plastimatch can use
  Plm_image::Pointer plm_img = PlmCommon::ConvertVolumeNodeToPlmImage(
    imageNode);
  plm_img->print();
  rtWriter->SetImage(plm_img);

  // Convert input RTDose to the format Plastimatch can use
  if (doseNode)
  {
    Plm_image::Pointer dose_img = PlmCommon::ConvertVolumeNodeToPlmImage(
      doseNode);
    dose_img->print();
    rtWriter->SetDose(dose_img);
  }

  // Convert input segmentation to the format Plastimatch can use
  if (segmentationNode)
  {
    // Make sure segmentation contains binary labelmap
    if ( !segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) )
    {
      error = "Failed to get binary labelmap representation from segmentation " + std::string(segmentationNode->GetName());
      vtkErrorMacro("ExportDicomRTStudy: " + error);
      return error;
    }

    // Convert anatomical image to oriented image data for geometry operations
    vtkSmartPointer<vtkOrientedImageData> imageData = vtkSmartPointer<vtkOrientedImageData>::Take(
      vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(imageNode) );
    if (!imageData.GetPointer())
    {
      error = "Failed to get image data from anatomical image volume";
      vtkErrorMacro("ExportDicomRTStudy: " + error);
      return error;
    }

    // Export each segment in segmentation
    vtkSegmentation::SegmentMap segmentMap = segmentationNode->GetSegmentation()->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      std::string segmentID = segmentIt->first;
      vtkSegment* segment = segmentIt->second;

      // Get binary labelmap representation
      vtkOrientedImageData* binaryLabelmap = vtkOrientedImageData::SafeDownCast(
        segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
      if (!binaryLabelmap)
      {
        error = "Failed to get binary labelmap representation from segment " + segmentID;
        vtkErrorMacro("ExportDicomRTStudy: " + error);
        return error;
      }
      // Temporarily copy labelmap image data as it will be probably resampled
      vtkSmartPointer<vtkOrientedImageData> binaryLabelmapCopy = vtkSmartPointer<vtkOrientedImageData>::New();
      binaryLabelmapCopy->DeepCopy(binaryLabelmap);

      // Apply parent transformation nodes if necessary
      if (segmentationNode->GetParentTransformNode())
      {
        if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(segmentationNode, binaryLabelmapCopy))
        {
          std::string errorMessage("Failed to apply parent transformation to exported segment!");
          vtkErrorMacro("ExportDicomRTStudy: " << errorMessage);
          return errorMessage;
        }
      }
      // Make sure the labelmap dimensions match the reference dimensions
      if ( !vtkOrientedImageDataResample::DoGeometriesMatch(imageData, binaryLabelmapCopy)
        || !SlicerRtCommon::AreExtentsEqual(imageData->GetExtent(), binaryLabelmapCopy->GetExtent()) )
      {
        if (!vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(binaryLabelmapCopy, imageData, binaryLabelmapCopy))
        {
          error = "Failed to resample segment " + segmentID + " to match anatomical image geometry";
          vtkErrorMacro("ExportDicomRTStudy: " + error);
          return error;
        }
      }

      // Convert mask to Plm image
      Plm_image::Pointer plmStructure = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(binaryLabelmapCopy);
      if (!plmStructure)
      {
        error = "Failed to convert segment labelmap " + segmentID + " to Plastimatch image";
        vtkErrorMacro("ExportDicomRTStudy: " + error);
        return error;
      }

      // Get segment properties
      std::string segmentName = segment->GetName();

      double segmentColor[3] = {0.5,0.5,0.5};
      segment->GetDefaultColor(segmentColor);
      vtkMRMLSegmentationDisplayNode* segmentationDisplayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
        segmentationNode->GetDisplayNode() );
      if (segmentationDisplayNode)
      {
        vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
        if (segmentationDisplayNode->GetSegmentDisplayProperties(segmentID, properties))
        {
          segmentColor[0] = properties.Color[0];
          segmentColor[1] = properties.Color[1];
          segmentColor[2] = properties.Color[2];
        }
      }

      printf (">>> name = %s\n", segmentName.c_str());
      printf (">>> color = %g %g %g\n", segmentColor[0], segmentColor[1], segmentColor[2]);

      rtWriter->AddStructure(plmStructure->itk_uchar(), segmentName.c_str(), segmentColor);
    }
  }

  // Write files to disk
  rtWriter->SetFileName(outputPath);
  rtWriter->Write();

  // Success (error is empty string)
  return error;
}

//-----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerDicomRtImportExportModuleLogic::GetReferencedVolumeByDicomForSegmentation(vtkMRMLSegmentationNode* segmentationNode)
{
  if (!segmentationNode)
  {
    return NULL;
  }

  // Get referenced series UID for segmentation
  vtkMRMLSubjectHierarchyNode* segmentationSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(segmentationNode);
  const char* referencedSeriesUid = segmentationSubjectHierarchyNode->GetAttribute(
    SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str() );
  if (!referencedSeriesUid)
  {
    vtkWarningWithObjectMacro(segmentationSubjectHierarchyNode, "No referenced series UID found for segmentation '" << segmentationSubjectHierarchyNode->GetName() << "'");
    return NULL;
  }

  // Get referenced volume subject hierarchy node by found UID
  vtkMRMLSubjectHierarchyNode* referencedSeriesNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    segmentationNode->GetScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), referencedSeriesUid);
  if (!referencedSeriesNode)
  {
    return NULL;
  }

  // Get and return referenced volume
  return vtkMRMLScalarVolumeNode::SafeDownCast(referencedSeriesNode->GetAssociatedNode());
}
