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
  University Health Network and Csaba Pinter, PerkLab, Queen's University and
  Andras Lasso, PerkLab, Queen's University, and was supported by Cancer Care
  Ontario (CCO)'s ACRU program with funds provided by the Ontario Ministry of
  Health and Long-Term Care and Ontario Consortium for Adaptive Interventions in
  Radiation Oncology (OCAIRO).

==============================================================================*/

// DicomRtImportExport includes
#include "vtkSlicerDicomRtImportExportModuleLogic.h"
#include "vtkSlicerDicomRtReader.h"
#include "vtkSlicerDicomRtWriter.h"
#include "vtkRibbonModelToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToRibbonModelConversionRule.h"
#include "vtkPlanarContourToClosedSurfaceConversionRule.h"

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
#include "vtkMRMLIsodoseNode.h"
#include "vtkMRMLPlanarImageNode.h"
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkSlicerPlanarImageModuleLogic.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentationStorageNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// vtkSegmentationCore includes
#include "vtkOrientedImageDataResample.h"
#include "vtkSegmentationConverterFactory.h"

// DCMTK includes
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofcond.h>
#include <dcmtk/ofstd/ofstring.h>
#include <dcmtk/ofstd/ofstd.h> // for class OFStandard
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
#include <vtkGeneralTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCutter.h>
#include <vtkStripper.h>
#include <vtkPlane.h>

// ITK includes
#include <itkImage.h>

// DICOMLib includes
#include "vtkSlicerDICOMLoadable.h"
#include "vtkSlicerDICOMExportable.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportExportModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportExportModuleLogic, IsodoseLogic, vtkSlicerIsodoseModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportExportModuleLogic, PlanarImageLogic, vtkSlicerPlanarImageModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportExportModuleLogic, BeamsLogic, vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportExportModuleLogic::vtkSlicerDicomRtImportExportModuleLogic()
{
  this->IsodoseLogic = NULL;
  this->PlanarImageLogic = NULL;
  this->BeamsLogic = NULL;

  this->BeamModelsInSeparateBranch = true;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportExportModuleLogic::~vtkSlicerDicomRtImportExportModuleLogic()
{
  this->SetIsodoseLogic(NULL);
  this->SetPlanarImageLogic(NULL);
  this->SetBeamsLogic(NULL);
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
}

//-----------------------------------------------------------------------------
void vtkSlicerDicomRtImportExportModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }

  // Register converter rules
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkRibbonModelToBinaryLabelmapConversionRule>::New() );
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkPlanarContourToRibbonModelConversionRule>::New() );
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkPlanarContourToClosedSurfaceConversionRule>::New() );
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
  // Number of loaded points. Used to prevent unreasonably long loading times with the downside of a less nice initial representation
  long maximumNumberOfPoints = -1;
  long totalNumberOfPoints = 0;

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
    if (maximumNumberOfPoints < roiPolyData->GetNumberOfPoints())
    {
      maximumNumberOfPoints = roiPolyData->GetNumberOfPoints();
    }
    totalNumberOfPoints += roiPolyData->GetNumberOfPoints();

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
      // Create subject hierarchy node for the series, if it has not been created yet.
      // Only create it for fiducials, as all structures are stored in a single segmentation node
      if (fiducialsSeriesSubjectHierarchyNode == NULL)
      {
        std::string fiducialsSeriesNodeName(seriesName);
        fiducialsSeriesNodeName.append(SlicerRtCommon::DICOMRTIMPORT_FIDUCIALS_HIERARCHY_NODE_NAME_POSTFIX);
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
      vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
        this->GetMRMLScene(), fiducialsSeriesSubjectHierarchyNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
        roiLabel, fiducialNode);
      fiducialSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str(), roiReferencedSeriesUid);
    }
    //
    // Contour ROI (segmentation)
    //
    else
    {
      // Create segmentation node for the structure set series, if not created yet
      if (segmentationNode.GetPointer() == NULL)
      {
        segmentationNode = vtkSmartPointer<vtkMRMLSegmentationNode>::New();
        std::string segmentationNodeName = this->GetMRMLScene()->GenerateUniqueName(seriesName);
        segmentationNode->SetName(segmentationNodeName.c_str());
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
            segmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(referencedVolumeNode);
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
  // Do not set closed surface display in case of extremely large structures, to prevent unreasonably long load times
  if (segmentationDisplayNode.GetPointer())
  {
    // Arbitrary thresholds, can revisit
    vtkDebugMacro("LoadRtStructureSet: Maximum number of points in a segment = " << maximumNumberOfPoints << ", Total number of points in segmentation = " << totalNumberOfPoints);
    if (maximumNumberOfPoints < 800000 && totalNumberOfPoints < 3000000)
    {
      segmentationDisplayNode->SetPreferredDisplayRepresentationName3D(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
      segmentationDisplayNode->SetPreferredDisplayRepresentationName2D(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
      segmentationDisplayNode->CalculateAutoOpacitiesForSegments();
    }
    else
    {
      vtkWarningMacro("LoadRtStructureSet: Structure set contains extremely large contours that will most likely take an unreasonably long time to load. No closed surface representation is thus created for nicer display, but the raw RICOM-RT planar contours are shown. It is possible to create nicer models in Segmentations module by converting to the lighter Ribbon model or the nicest Closed surface.");
    }
  }
  else if (segmentationNode.GetPointer())
  {
    vtkErrorMacro("LoadRtStructureSet: No display node was created for the segmentation node " << segmentationNode->GetName());
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
  if (!rtReader->GetDoseGridScaling())
  {
    vtkErrorMacro("LoadRtDose: Empty dose unit value found for dose volume " << volumeNode->GetName());
  }
  double doseGridScaling = vtkVariant(rtReader->GetDoseGridScaling()).ToDouble();

  vtkSmartPointer<vtkImageData> floatVolumeData = vtkSmartPointer<vtkImageData>::New();

  vtkSmartPointer<vtkImageCast> imageCast = vtkSmartPointer<vtkImageCast>::New();
  imageCast->SetInputData(volumeNode->GetImageData());
  imageCast->SetOutputScalarTypeToFloat();
  imageCast->Update();
  floatVolumeData->DeepCopy(imageCast->GetOutput());

  float value = 0.0;
  float* floatPtr = (float*)floatVolumeData->GetScalarPointer();
  for (long i=0; i<floatVolumeData->GetNumberOfPoints(); ++i)
  {
    value = (*floatPtr) * doseGridScaling;
    (*floatPtr) = value;
    ++floatPtr;
  }

  volumeNode->SetAndObserveImageData(floatVolumeData);      

  // Get default isodose color table and default dose color table
  vtkMRMLColorTableNode* defaultIsodoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultIsodoseColorTable(this->GetMRMLScene());
  vtkMRMLColorTableNode* defaultDoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(this->GetMRMLScene());
  if (!defaultIsodoseColorTable || !defaultDoseColorTable)
  {
    vtkErrorMacro("LoadRtDose: Failed to get default color tables!");
    return false;
  }

  //TODO: Generate isodose surfaces if chosen so by the user in the hanging protocol options (hanging protocol support not implemented yet)

  // Set default colormap to the loaded one if found or generated, or to rainbow otherwise
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  volumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
  this->GetMRMLScene()->AddNode(volumeDisplayNode);
  volumeNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());

  // Set window/level to match the isodose levels
  int minDoseInDefaultIsodoseLevels = vtkVariant(defaultIsodoseColorTable->GetColorName(0)).ToInt();
  int maxDoseInDefaultIsodoseLevels = vtkVariant(defaultIsodoseColorTable->GetColorName(defaultIsodoseColorTable->GetNumberOfColors()-1)).ToInt();

  volumeDisplayNode->AutoWindowLevelOff();
  volumeDisplayNode->SetWindowLevelMinMax(minDoseInDefaultIsodoseLevels, maxDoseInDefaultIsodoseLevels);

  // Set display threshold
  volumeDisplayNode->AutoThresholdOff();
  volumeDisplayNode->SetLowerThreshold(0.5 * doseGridScaling);
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
  if (studyHierarchyNode)
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
      double existingDoseUnitValue = vtkVariant(existingDoseUnitValueChars).ToDouble();
      double doseGridScaling = vtkVariant(rtReader->GetDoseGridScaling()).ToDouble();
      double currentDoseUnitValue = vtkVariant(rtReader->GetDoseGridScaling()).ToDouble();
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
  else
  {
    vtkErrorMacro("LoadRtDose: Unable to get parent study hierarchy node for dose volume '" << volumeNode->GetName() << "'");
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
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> beamModelSubjectHierarchyRootNode;
  vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyRootNode;

  const char* seriesName = loadable->GetName();
  std::string shSeriesNodeName(seriesName);
  shSeriesNodeName.append(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix());
  shSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(shSeriesNodeName);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create plan node
  vtkSmartPointer<vtkMRMLRTPlanNode> planNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
  planNode->SetName(seriesName);
  this->GetMRMLScene()->AddNode(planNode);

  // Set up plan subject hierarchy node
  vtkMRMLSubjectHierarchyNode* planShNode = planNode->GetPlanSubjectHierarchyNode();
  if (!planShNode)
  {
    vtkErrorMacro("LoadRtPlan: Created RTPlanNode, but it doesn't have a subject hierarchy node.");
    return false;
  }
  if (planShNode)
  {
    // Attach attributes to plan SH node
    planShNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(),
      rtReader->GetSeriesInstanceUid());
    planShNode->SetName(shSeriesNodeName.c_str());

    const char* referencedStructureSetSopInstanceUid = rtReader->GetRTPlanReferencedStructureSetSOPInstanceUID();
    const char* referencedDoseSopInstanceUids = rtReader->GetRTPlanReferencedDoseSOPInstanceUIDs();
    std::string referencedSopInstanceUids = "";
    if (referencedStructureSetSopInstanceUid)
    {
      referencedSopInstanceUids = std::string(referencedStructureSetSopInstanceUid);
    }
    if (referencedDoseSopInstanceUids)
    {        
      referencedSopInstanceUids = referencedSopInstanceUids +
        (referencedStructureSetSopInstanceUid?" ":"") + std::string(referencedDoseSopInstanceUids);
    }
    planShNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName().c_str(),
      referencedSopInstanceUids.c_str() );
  }

  // Load beams in plan
  int numberOfBeams = rtReader->GetNumberOfBeams();
  for (int beamIndex = 0; beamIndex < numberOfBeams; beamIndex++) // DICOM starts indexing from 1
  {
    unsigned int dicomBeamNumber = rtReader->GetBeamNumberForIndex(beamIndex);
    const char* beamName = rtReader->GetBeamName(dicomBeamNumber);

    // Create the beam node
    vtkSmartPointer<vtkMRMLRTBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
    beamNode->SetName(beamName);

    // Set beam geometry parameters from DICOM
    double jawPositions[2][2] = {{0.0, 0.0},{0.0, 0.0}};
    rtReader->GetBeamLeafJawPositions(dicomBeamNumber, jawPositions);
    beamNode->SetX1Jaw(jawPositions[0][0]);
    beamNode->SetX2Jaw(jawPositions[0][1]);
    beamNode->SetY1Jaw(jawPositions[1][0]);
    beamNode->SetY2Jaw(jawPositions[1][1]);

    beamNode->SetGantryAngle(rtReader->GetBeamGantryAngle(dicomBeamNumber));
    beamNode->SetCollimatorAngle(rtReader->GetBeamBeamLimitingDeviceAngle(dicomBeamNumber));
    beamNode->SetCouchAngle(rtReader->GetBeamPatientSupportAngle(dicomBeamNumber));

    beamNode->SetSAD(rtReader->GetBeamSourceAxisDistance(dicomBeamNumber));

    // Set isocenter to parent plan
    double* isocenter = rtReader->GetBeamIsocenterPositionRas(dicomBeamNumber);
    planNode->SetIsocenterSpecification(vtkMRMLRTPlanNode::ArbitraryPoint);
    if (beamIndex == 0)
    {
      if (!planNode->SetIsocenterPosition(isocenter))
      {
        vtkErrorMacro("LoadRtPlan: Failed to set isocenter position");
        return false;
      }
    }
    else
    {
      double planIsocenter[3] = {0.0, 0.0, 0.0};
      if (!planNode->GetIsocenterPosition(planIsocenter))
      {
        vtkErrorMacro("LoadRtPlan: Failed to get plan isocenter position");
        return false;
      }
      //TODO: Multiple isocenters per plan is not yet supported. Will be part of the beams group nodes developed later
      if ( !SlicerRtCommon::AreEqualWithTolerance(planIsocenter[0], isocenter[0])
        || !SlicerRtCommon::AreEqualWithTolerance(planIsocenter[1], isocenter[1])
        || !SlicerRtCommon::AreEqualWithTolerance(planIsocenter[2], isocenter[2]) )
      {
        vtkErrorMacro("LoadRtPlan: Different isocenters for each beam are not yet supported! The first isocenter will be used for the whole plan " << planNode->GetName() << ": (" << planIsocenter[0] << ", " << planIsocenter[1] << ", " << planIsocenter[2] << ")");
      }
    }

    // Add beam to scene (triggers poly data and transform creation and update)
    this->GetMRMLScene()->AddNode(beamNode);
    // Add beam to plan
    planNode->AddBeam(beamNode);

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
    }

    // Put beam model in the model hierarchy
    vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
    std::string beamModelHierarchyNodeName = std::string(beamNode->GetName()) + SlicerRtCommon::DICOMRTIMPORT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
    beamModelHierarchyNode->SetName(beamModelHierarchyNodeName.c_str());
    this->GetMRMLScene()->AddNode(beamModelHierarchyNode);
    beamModelHierarchyNode->SetAssociatedNodeID(beamNode->GetID());
    beamModelHierarchyNode->SetParentNodeID(beamModelHierarchyRootNode->GetID());
    beamModelHierarchyNode->SetIndexInParent(beamIndex);
    beamModelHierarchyNode->HideFromEditorsOn();

    // Create display node for the hierarchy node
    vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    std::string beamModelHierarchyDisplayNodeName = beamModelHierarchyNodeName + std::string("Display");
    beamModelHierarchyDisplayNode->SetName(beamModelHierarchyDisplayNodeName.c_str());
    beamModelHierarchyDisplayNode->SetVisibility(1);
    this->GetMRMLScene()->AddNode(beamModelHierarchyDisplayNode);
    beamModelHierarchyNode->SetAndObserveDisplayNodeID( beamModelHierarchyDisplayNode->GetID() );
  }

  // Insert plan isocenter series in subject hierarchy
  this->InsertSeriesInSubjectHierarchy(rtReader);

  // Put plan SH node underneath study
  vtkMRMLSubjectHierarchyNode* studyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
    this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), rtReader->GetStudyInstanceUid());
  if (!studyNode)
  {
    vtkWarningMacro("LoadRtPlan: No Study SH node found.");
    return false;
  }
  if (studyNode && planShNode)
  {
    planShNode->SetParentNodeID(studyNode->GetID());
  }
  // Put plan markups under study within SH
  vtkMRMLSubjectHierarchyNode* planMarkupsSHNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(
    planNode->GetPoisMarkupsFiducialNode(), this->GetMRMLScene() );
  if (planMarkupsSHNode && studyNode)
  {
    planMarkupsSHNode->SetParentNodeID(studyNode->GetID());
  }

  // Compute and set geometry of possible RT image that references the loaded beams.
  // Uses the referenced RT image if available, otherwise the geometry will be set up when loading the corresponding RT image
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  planNode->GetBeams(beams);
  if (beams)
  {
    for (int i=0; i<beams->GetNumberOfItems(); ++i)
    {
      vtkMRMLRTBeamNode *beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
      this->SetupRtImageGeometry(beamNode);
    }
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
        ? std::string(rtReader->GetPatientName()) : SlicerRtCommon::DICOMRTIMPORT_NO_NAME );
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
      studyNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMStudyInstanceUIDTagName().c_str(), rtReader->GetStudyInstanceUid() );
      studyNode->SetAttribute( vtkMRMLSubjectHierarchyConstants::GetDICOMStudyIDTagName().c_str(), rtReader->GetStudyId() );
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
      std::string studyNodeName = studyDescription + studyDate;
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
    // TODO: This is not correct for RTIMAGE, which may have several instances of DRRs within the same series
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
void vtkSlicerDicomRtImportExportModuleLogic::SetupRtImageGeometry(vtkMRMLNode* node)
{
  vtkMRMLScalarVolumeNode* rtImageVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  vtkMRMLSubjectHierarchyNode* rtImageSubjectHierarchyNode = NULL;
  vtkMRMLSubjectHierarchyNode* beamSHNode = NULL;

  // If the function is called from the LoadRtImage function with an RT image volume: find corresponding RT beam
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
    const char* referencedPlanSopInstanceUid = rtImageSubjectHierarchyNode->GetAttribute(
      vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName().c_str() );
    if (!referencedPlanSopInstanceUid)
    {
      vtkErrorMacro("SetupRtImageGeometry: Unable to find referenced plan SOP instance UID for RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }
    vtkMRMLSubjectHierarchyNode* rtPlanSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
      this->GetMRMLScene(), vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName(), referencedPlanSopInstanceUid );
    if (!rtPlanSubjectHierarchyNode)
    {
      vtkDebugMacro("SetupRtImageGeometry: Cannot set up geometry of RT image '" << rtImageVolumeNode->GetName()
        << "' without the referenced RT plan. Will be set up upon loading the related plan");
      return;
    }
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(rtPlanSubjectHierarchyNode->GetAssociatedNode());

    // Get referenced beam number
    const char* referencedBeamNumberChars = rtImageSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str());
    if (!referencedBeamNumberChars)
    {
      vtkErrorMacro("SetupRtImageGeometry: No referenced beam number specified in RT image '" << rtImageVolumeNode->GetName() << "'!");
      return;
    }
    int referencedBeamNumber = vtkVariant(referencedBeamNumberChars).ToInt();

    // Get beam according to referenced beam number
    beamNode = planNode->GetBeamByNumber(referencedBeamNumber);
    if (!beamNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve beam node for RT image '" << rtImageVolumeNode->GetName() << "' in RT plan '" << rtPlanSubjectHierarchyNode->GetName() << "'!");
      return;
    }
  }
  // If the function is called from the LoadRtPlan function with a beam: find corresponding RT image
  else if (beamNode)
  {
    // Get RT plan for beam
    vtkMRMLRTPlanNode *planNode = beamNode->GetParentPlanNode();
    if (!planNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid plan node for beam '" << beamNode->GetName() << "'!");
      return;
    }
    vtkMRMLSubjectHierarchyNode *planShNode = planNode->GetPlanSubjectHierarchyNode();
    if (!planShNode)
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to retrieve valid plan subject hierarchy node for beam '" << beamNode->GetName() << "'!");
      return;
    }
    std::string rtPlanSopInstanceUid = planShNode->GetUID(vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
    if (rtPlanSopInstanceUid.empty())
    {
      vtkErrorMacro("SetupRtImageGeometry: Failed to get RT Plan DICOM UID for beam '" << beamNode->GetName() << "'!");
      return;
    }

    // Get isocenter beam number
    int beamNumber = beamNode->GetBeamNumber();
    // Get number of beams in the plan (if there is only one, then the beam number may nor be correctly referenced, so we cannot find it that way
    bool oneBeamInPlan = (planShNode->GetNumberOfChildrenNodes() == 1);
    
    // Find corresponding RT image according to beam (isocenter) UID
    vtkSmartPointer<vtkCollection> hierarchyNodes = vtkSmartPointer<vtkCollection>::Take(this->GetMRMLScene()->GetNodesByClass("vtkMRMLSubjectHierarchyNode"));
    vtkObject* nextObject = NULL;
    for (hierarchyNodes->InitTraversal(); (nextObject = hierarchyNodes->GetNextItemAsObject()); )
    {
      vtkMRMLSubjectHierarchyNode* currentShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nextObject);
      bool currentShNodeReferencesPlan = false;
      if (currentShNode && currentShNode->GetAssociatedNode() && currentShNode->GetAssociatedNode()->IsA("vtkMRMLScalarVolumeNode")
        && currentShNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
      {
        // If current node is the subject hierarchy node of an RT image, then determine it references the RT plan by DICOM
        std::vector<vtkMRMLSubjectHierarchyNode*> referencedShNodes = currentShNode->GetSubjectHierarchyNodesReferencedByDICOM();
        for (std::vector<vtkMRMLSubjectHierarchyNode*>::iterator refIt=referencedShNodes.begin(); refIt!=referencedShNodes.end(); ++refIt)
        {
          if ((*refIt) == planShNode)
          {
            currentShNodeReferencesPlan = true;
            break;
          }
        }

        // If RT image node references plan node, then it is the corresponding RT image if beam numbers match
        if (currentShNodeReferencesPlan)
        {
          // Get RT image referenced beam number
          int referencedBeamNumber = vtkVariant(currentShNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME.c_str())).ToInt();
          // If the referenced beam number matches the isocenter beam number, or if there is one beam in the plan, then we found the RT image
          if (referencedBeamNumber == beamNumber || oneBeamInPlan)
          {
            rtImageVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(currentShNode->GetAssociatedNode());
            rtImageSubjectHierarchyNode = currentShNode;
            break;
          }
        }
      }

      // Return if a referenced displayed model is present for the RT image, because it means that the geometry has been set up successfully before
      if (rtImageVolumeNode)
      {
        vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
          rtImageVolumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
        if (modelNode)
        {
          vtkDebugMacro("SetupRtImageGeometry: RT image '" << rtImageVolumeNode->GetName() << "' belonging to beam '" << beamNode->GetName() << "' seems to have been set up already.");
          return;
        }
      }
    }

    if (!rtImageVolumeNode)
    {
      // RT image for the isocenter is not loaded yet. Geometry will be set up upon loading the related RT image
      vtkDebugMacro("SetupRtImageGeometry: Cannot set up geometry of RT image corresponding to beam '" << beamNode->GetName()
        << "' because the RT image is not loaded yet. Will be set up upon loading the related RT image");
      return;
    }
  }
  else
  {
    vtkErrorMacro("SetupRtImageGeometry: Input node is neither a volume node nor an plan POIs markups fiducial node!");
    return;
  }

  // We have both the RT image and the isocenter, we can set up the geometry

  // Get source to RT image plane distance (along beam axis)
  double rtImageSid = 0.0;
  const char* rtImageSidChars = rtImageSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME.c_str());
  if (rtImageSidChars != NULL)
  {
    rtImageSid = vtkVariant(rtImageSidChars).ToDouble();
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
  double sourceAxisDistance = beamNode->GetSAD();
  double gantryAngle = beamNode->GetGantryAngle();
  double couchAngle = beamNode->GetCouchAngle();

  // Get isocenter coordinates
  double isocenterWorldCoordinates[3] = {0.0, 0.0, 0.0};
  if (!beamNode->GetPlanIsocenterPosition(isocenterWorldCoordinates))
  {
    vtkErrorMacro("SetupRtImageGeometry: Failed to get plan isocenter position");
    return;
  }

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
  std::string displayedModelNodeName = vtkMRMLPlanarImageNode::PLANARIMAGE_MODEL_NODE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName());
  displayedModelNode->SetName(displayedModelNodeName.c_str());
  displayedModelNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  // Create PlanarImage parameter set node
  std::string planarImageParameterSetNodeName;
  planarImageParameterSetNodeName = this->GetMRMLScene()->GenerateUniqueName(
    vtkMRMLPlanarImageNode::PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX + std::string(rtImageVolumeNode->GetName()) );
  vtkSmartPointer<vtkMRMLPlanarImageNode> planarImageParameterSetNode = vtkSmartPointer<vtkMRMLPlanarImageNode>::New();
  planarImageParameterSetNode->SetName(planarImageParameterSetNodeName.c_str());
  this->GetMRMLScene()->AddNode(planarImageParameterSetNode);
  planarImageParameterSetNode->SetAndObserveRtImageVolumeNode(rtImageVolumeNode);
  planarImageParameterSetNode->SetAndObserveDisplayedModelNode(displayedModelNode);

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
  // These are the ones available through the DICOM Export widget
  vtkSlicerDICOMExportable* firstExportable = vtkSlicerDICOMExportable::SafeDownCast(exportables->GetItemAsObject(0));
  const char* patientName = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientNameTagName().c_str());
  const char* patientID = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientIDTagName().c_str());
  const char* patientSex = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientSexTagName().c_str());
  const char* studyDate = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyDateTagName().c_str());
  const char* studyTime = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyTimeTagName().c_str());
  const char* studyDescription = firstExportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyDescriptionTagName().c_str());
  if (studyDescription && !strcmp(studyDescription, "No study description"))
  {
    studyDescription = 0;
  }
  const char* imageSeriesDescription = 0;
  const char* imageSeriesNumber = 0;
  const char* imageSeriesModality = 0;
  const char* doseSeriesDescription = 0;
  const char* doseSeriesNumber = 0;
  const char* rtssSeriesDescription = 0;
  const char* rtssSeriesNumber = 0;

  // Get other common export parameters
  // These are the ones available in hierarchy
  std::string studyInstanceUid = "";
  std::string studyID = "";
  vtkMRMLSubjectHierarchyNode* firstSHNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(mrmlScene->GetNodeByID(firstExportable->GetNodeID()));
  if (firstSHNode)
  {
    vtkMRMLSubjectHierarchyNode* studySHNode = firstSHNode->GetAncestorAtLevel(
      vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy() );
    if (studySHNode)
    {
      studyInstanceUid = studySHNode->GetUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName());
      const char* studyIDChars = studySHNode->GetAttribute(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyIDTagName().c_str());
      if (studyIDChars)
      {
        studyID = std::string(studyIDChars);
      }
    }
    else
    {
      vtkWarningMacro("ExportDicomRTStudy: Failed to get ancestor study from exportable with node ID " + std::string(firstExportable->GetNodeID()));
    }
  }
  else
  {
    vtkWarningMacro("ExportDicomRTStudy: Failed to get SH node from exportable with node ID " + std::string(firstExportable->GetNodeID()));
  }

  const char* outputPath = firstExportable->GetDirectory();

  // Get nodes for the different roles from the exportable list
  vtkMRMLScalarVolumeNode* doseNode = NULL;
  vtkMRMLSegmentationNode* segmentationNode = NULL;
  vtkMRMLScalarVolumeNode* imageNode = NULL;
  std::vector<std::string> imageSliceUIDs;
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

    // GCS FIX TODO: The below logic seems to allow only a single dose, 
    // single image, and single segmentation per study.
    // However, there is no check to enforce this.

    // Check if dose volume and set it if found
    if (associatedNode && SlicerRtCommon::IsDoseVolumeNode(associatedNode))
    {
      doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(associatedNode);

      doseSeriesDescription = exportable->GetTag("SeriesDescription");
      if (doseSeriesDescription && !strcmp(doseSeriesDescription, "No series description"))
      {
        doseSeriesDescription = 0;
      }
      doseSeriesNumber = exportable->GetTag("SeriesNumber");
    }
    // Check if segmentation node and set if found
    else if (associatedNode && associatedNode->IsA("vtkMRMLSegmentationNode"))
    {
      segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(associatedNode);

      rtssSeriesDescription = exportable->GetTag("SeriesDescription");
      if (rtssSeriesDescription && !strcmp(rtssSeriesDescription, "No series description"))
      {
        rtssSeriesDescription = 0;
      }
      rtssSeriesNumber = exportable->GetTag("SeriesNumber");
    }
    // Check if other volume (anatomical volume role) and set if found
    else if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
    {
      imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(associatedNode);

      // Get series DICOM tags to export
      imageSeriesDescription = exportable->GetTag("SeriesDescription");
      if (imageSeriesDescription && !strcmp(imageSeriesDescription, "No series description"))
      {
        imageSeriesDescription = 0;
      }
      //TODO: Getter function adds "DICOM." prefix (which is for attribute names), while the exportable tags are without that
      // imageSeriesModality = exportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMSeriesModalityAttributeName());
      imageSeriesModality = exportable->GetTag("Modality");
      // imageSeriesNumber = exportable->GetTag(vtkMRMLSubjectHierarchyConstants::GetDICOMSeriesNumberAttributeName());
      imageSeriesNumber = exportable->GetTag("SeriesNumber");

      // Get slice instance UIDs
      std::string sliceInstanceUIDList = shNode->GetUID(vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
      vtkMRMLSubjectHierarchyNode::DeserializeUIDList(sliceInstanceUIDList, imageSliceUIDs);
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

  // Set study-level metadata
  rtWriter->SetPatientName(patientName);
  rtWriter->SetPatientID(patientID);
  rtWriter->SetPatientSex(patientSex);
  rtWriter->SetStudyDate(studyDate);
  rtWriter->SetStudyTime(studyTime);
  rtWriter->SetStudyDescription(studyDescription);
  rtWriter->SetStudyInstanceUid(studyInstanceUid.c_str());
  rtWriter->SetStudyID(studyID.c_str());

  // Set image-level metadata
  rtWriter->SetImageSeriesDescription(imageSeriesDescription);
  rtWriter->SetImageSeriesNumber(imageSeriesNumber);
  rtWriter->SetImageSeriesModality(imageSeriesModality);
  rtWriter->SetDoseSeriesDescription(doseSeriesDescription);
  rtWriter->SetDoseSeriesNumber(doseSeriesNumber);
  rtWriter->SetRtssSeriesDescription(rtssSeriesDescription);
  rtWriter->SetRtssSeriesNumber(rtssSeriesNumber);
  
  // Convert input image (CT/MR/etc) to the format Plastimatch can use
  vtkSmartPointer<vtkOrientedImageData> imageOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::New();
  if (!SlicerRtCommon::ConvertVolumeNodeToVtkOrientedImageData(imageNode, imageOrientedImageData))
  {
    error = "Failed to convert anatomical image " + std::string(imageNode->GetName()) + " to oriented image data";
    vtkErrorMacro("ExportDicomRTStudy: " + error);
    return error;
  }
  // Need to resample image data if its transform contains shear
  vtkSmartPointer<vtkMatrix4x4> imageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  imageOrientedImageData->GetImageToWorldMatrix(imageToWorldMatrix);
  if (vtkOrientedImageDataResample::DoesTransformMatrixContainShear(imageToWorldMatrix))
  {
    vtkSmartPointer<vtkTransform> imageToWorldTransform = vtkSmartPointer<vtkTransform>::New();
    imageToWorldTransform->SetMatrix(imageToWorldMatrix);
    vtkOrientedImageDataResample::TransformOrientedImage(imageOrientedImageData, imageToWorldTransform, false, true);
    // Set identity transform to image data so that it is at the same location
    vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    identityMatrix->Identity();
    imageOrientedImageData->SetGeometryFromImageToWorldMatrix(identityMatrix);
  }
  // Set anatomical image to RT writer
  Plm_image::Pointer plm_img = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(imageOrientedImageData);
  if (plm_img->dim(0) * plm_img->dim(1) * plm_img->dim(2) == 0)
  {
    error = "Failed to convert anatomical (CT/MR) image to Plastimatch format";
    vtkErrorMacro("ExportDicomRTStudy: " + error);
    return error;
  }
  rtWriter->SetImage(plm_img);

  // Convert input RTDose to the format Plastimatch can use
  if (doseNode)
  {
    vtkSmartPointer<vtkOrientedImageData> doseOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::New();
    if (!SlicerRtCommon::ConvertVolumeNodeToVtkOrientedImageData(doseNode, doseOrientedImageData))
    {
      error = "Failed to convert dose volume " + std::string(doseNode->GetName()) + " to oriented image data";
      vtkErrorMacro("ExportDicomRTStudy: " + error);
      return error;
    }
    // Need to resample image data if its transform contains shear
    vtkSmartPointer<vtkMatrix4x4> doseToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    doseOrientedImageData->GetImageToWorldMatrix(doseToWorldMatrix);
    if (vtkOrientedImageDataResample::DoesTransformMatrixContainShear(doseToWorldMatrix))
    {
      vtkSmartPointer<vtkTransform> doseToWorldTransform = vtkSmartPointer<vtkTransform>::New();
      doseToWorldTransform->SetMatrix(doseToWorldMatrix);
      vtkOrientedImageDataResample::TransformOrientedImage(doseOrientedImageData, doseToWorldTransform, false, true);
      // Set identity transform to image data so that it is at the same location
      vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      identityMatrix->Identity();
      doseOrientedImageData->SetGeometryFromImageToWorldMatrix(identityMatrix);
    }
    // Set anatomical image to RT writer
    Plm_image::Pointer dose_img = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(doseOrientedImageData);
    if (dose_img->dim(0) * dose_img->dim(1) * dose_img->dim(2) == 0)
    {
      error = "Failed to convert dose volume to Plastimatch format";
      vtkErrorMacro("ExportDicomRTStudy: " + error);
      return error;
    }
    rtWriter->SetDose(dose_img);
  }

  // Convert input segmentation to the format Plastimatch can use
  if (segmentationNode)
  {
    // If master representation is labelmap type, then export binary labelmap
    vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
    if (segmentation->IsMasterRepresentationImageData())
    {
      // Make sure segmentation contains binary labelmap
      if ( !segmentationNode->GetSegmentation()->CreateRepresentation(
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) )
      {
        error = "Failed to get binary labelmap representation from segmentation " + std::string(segmentationNode->GetName());
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
        if ( !vtkOrientedImageDataResample::DoGeometriesMatch(imageOrientedImageData, binaryLabelmapCopy)
          || !vtkOrientedImageDataResample::DoExtentsMatch(imageOrientedImageData, binaryLabelmapCopy) )
        {
          if (!vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(binaryLabelmapCopy, imageOrientedImageData, binaryLabelmapCopy))
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

        rtWriter->AddStructure(plmStructure->itk_uchar(), segmentName.c_str(), segmentColor);
      } // For each segment
    }
    // If master representation is poly data type, then export from closed surface
    else if (segmentation->IsMasterRepresentationPolyData())
    {
      // Make sure segmentation contains closed surface
      if ( !segmentationNode->GetSegmentation()->CreateRepresentation(
        vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() ) )
      {
        error = "Failed to get closed surface representation from segmentation " + std::string(segmentationNode->GetName());
        vtkErrorMacro("ExportDicomRTStudy: " + error);
        return error;
      }

      // Get transform  from segmentation to world (RAS)
      vtkSmartPointer<vtkGeneralTransform> nodeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
      nodeToWorldTransform->Identity();
      if (segmentationNode->GetParentTransformNode())
      {
        segmentationNode->GetParentTransformNode()->GetTransformToWorld(nodeToWorldTransform);
      }
      // Initialize poly data transformer
      vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
      transformPolyData->SetTransform(nodeToWorldTransform);

      // Initialize cutting plane with normal of the Z axis of the anatomical image
      vtkSmartPointer<vtkMatrix4x4> imageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      imageOrientedImageData->GetImageToWorldMatrix(imageToWorldMatrix);
      double normal[3] = { imageToWorldMatrix->GetElement(0,2), imageToWorldMatrix->GetElement(1,2), imageToWorldMatrix->GetElement(2,2) };
      vtkSmartPointer<vtkPlane> slicePlane = vtkSmartPointer<vtkPlane>::New();
      slicePlane->SetNormal(normal);

      // Export each segment in segmentation
      vtkSegmentation::SegmentMap segmentMap = segmentationNode->GetSegmentation()->GetSegments();
      for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
      {
        std::string segmentID = segmentIt->first;
        vtkSegment* segment = segmentIt->second;

        // Get closed surface representation
        vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(
          segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()) );
        if (!closedSurfacePolyData)
        {
          error = "Failed to get closed surface representation from segment " + segmentID;
          vtkErrorMacro("ExportDicomRTStudy: " + error);
          return error;
        }

        // Initialize cutter pipeline for segment
        transformPolyData->SetInputData(closedSurfacePolyData);
        vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
        cutter->SetInputConnection(transformPolyData->GetOutputPort());
        cutter->SetGenerateCutScalars(0);
        vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
        stripper->SetInputConnection(cutter->GetOutputPort());

        // Get segment bounding box
        double bounds[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
        transformPolyData->Update();
        transformPolyData->GetOutput()->GetBounds(bounds);

        // Containers to be passed to the writer
        std::vector<int> sliceNumbers;
        std::vector<std::string> sliceUIDs;
        std::vector<vtkPolyData*> sliceContours;

        // Create planar contours from closed surface based on each of the anatomical image slices
        int imageExtent[6] = {0,-1,0,-1,0,-1};
        imageOrientedImageData->GetExtent(imageExtent);
        for (int slice=imageExtent[0]; slice<imageExtent[1]; ++slice)
        {
          // Calculate slice origin
          double origin[3] = { imageToWorldMatrix->GetElement(0,3) + slice*normal[0],
                               imageToWorldMatrix->GetElement(1,3) + slice*normal[1],
                               imageToWorldMatrix->GetElement(2,3) + slice*normal[2] };
          slicePlane->SetOrigin(origin);
          if (origin[2] < bounds[4] || origin[2] > bounds[5])
          {
            // No contours outside surface bounds
            continue;
          }

          // Cut closed surface at slice
          cutter->SetCutFunction(slicePlane);
          
          // Get instance UID of corresponding slice
          int sliceNumber = slice-imageExtent[0];
          sliceNumbers.push_back(sliceNumber);
          std::string sliceInstanceUID = (imageSliceUIDs.size() > sliceNumber ? imageSliceUIDs[sliceNumber] : "");
          sliceUIDs.push_back(sliceInstanceUID);

          // Save slice contour
          stripper->Update();
          vtkPolyData* sliceContour = vtkPolyData::New();
          sliceContour->SetPoints(stripper->GetOutput()->GetPoints());
          sliceContour->SetPolys(stripper->GetOutput()->GetLines());
          sliceContours.push_back(sliceContour);
        } // For each anatomical image slice

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

        // Add contours to writer
        rtWriter->AddStructure(segmentName.c_str(), segmentColor, sliceNumbers, sliceUIDs, sliceContours);

        // Clean up slice contours
        for (std::vector<vtkPolyData*>::iterator contourIt=sliceContours.begin(); contourIt!=sliceContours.end(); ++contourIt)
        {
          (*contourIt)->Delete();
        }
      } // For each segment
    }
    else
    {
      error = "Structure set contains unsupported master representation!";
      vtkErrorMacro("ExportDicomRTStudy: " + error);
      return error;
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
