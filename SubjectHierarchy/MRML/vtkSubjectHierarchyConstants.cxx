/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

#include "vtkSubjectHierarchyConstants.h"

//----------------------------------------------------------------------------
// Constant strings
//----------------------------------------------------------------------------

// Subject hierarchy constants
const std::string vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX = "_SubjectHierarchy";
const std::string vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_ATTRIBUTE_PREFIX = "SubjectHierarchy.";
const std::string vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_EXCLUDE_FROM_POTENTIAL_NODES_LIST_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_ATTRIBUTE_PREFIX + "ExcludeFromPotentialNodesList"; // Identifier
const std::string vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NEW_NODE_NAME_PREFIX = "NewSubjectHierarchyNode_";

const char* vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT = "Subject";
const char* vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY = "Study";

// DICOM plugin constants
const char* vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES = "Series";
const char* vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES = "Subseries";

const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX = "DICOMHierarchy.";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_PATIENT_NAME_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "PatientName";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_PATIENT_ID_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "PatientId";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_PATIENT_SEX_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "PatientSex";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_PATIENT_BIRTH_DATE_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "PatientBirthDate";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_STUDY_DATE_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "StudyDate";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_STUDY_TIME_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "StudyTime";
const std::string vtkSubjectHierarchyConstants::DICOMHIERARCHY_SERIES_MODALITY_ATTRIBUTE_NAME = vtkSubjectHierarchyConstants::DICOMHIERARCHY_ATTRIBUTE_PREFIX + "SeriesModality";
const char* vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME = "DICOM";
