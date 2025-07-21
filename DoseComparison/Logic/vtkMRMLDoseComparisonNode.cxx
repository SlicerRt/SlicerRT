/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// MRMLDoseAccumulation includes
#include "vtkMRMLDoseComparisonNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSegmentationNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* REFERENCE_DOSE_VOLUME_REFERENCE_ROLE = "referenceDoseVolumeRef";
static const char* COMPARE_DOSE_VOLUME_REFERENCE_ROLE = "compareDoseVolumeRef";
static const char* MASK_SEGMENTATION_REFERENCE_ROLE = "maskSegmentationRef";
static const char* GAMMA_VOLUME_REFERENCE_ROLE = "outputGammaVolumeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDoseComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLDoseComparisonNode::vtkMRMLDoseComparisonNode()
{
  this->MaskSegmentID = nullptr;
  this->DtaDistanceToleranceMm = 3.0;
  this->DoseDifferenceTolerancePercent = 3.0;
  this->ReferenceDoseGy = 50.0;
  this->AnalysisThresholdPercent = 10.0;
  this->MaximumGamma = 2.0;
  this->UseMaximumDose = true;
  this->UseGeometricGammaCalculation = true;
  this->DoseThresholdOnReferenceOnly = false;
  this->PassFractionPercent = -1.0;
  this->ResultsValid = false;
  this->ReportString = nullptr;
  this->LocalDoseDifference = false;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseComparisonNode::~vtkMRMLDoseComparisonNode()
{
  this->SetMaskSegmentID(nullptr);
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  of << " MaskSegmentID=\"" << (this->MaskSegmentID ? this->MaskSegmentID : "nullptr") << "\"";
  of << " DtaDistanceToleranceMm=\"" << this->DtaDistanceToleranceMm << "\"";
  of << " DoseDifferenceTolerancePercent=\"" << this->DoseDifferenceTolerancePercent << "\"";
  of << " ReferenceDoseGy=\"" << this->ReferenceDoseGy << "\"";
  of << " AnalysisThresholdPercent=\"" << this->AnalysisThresholdPercent << "\"";
  of << " MaximumGamma=\"" << this->MaximumGamma << "\"";
  of << " UseMaximumDose=\"" << (this->UseMaximumDose ? "true" : "false") << "\"";
  of << " UseGeometricGammaCalculation=\"" << (this->UseGeometricGammaCalculation ? "true" : "false") << "\"";
  of << " LocalDoseDifference=\"" << (this->LocalDoseDifference ? "true" : "false") << "\"";
  of << " DoseThresholdOnReferenceOnly=\"" << (this->DoseThresholdOnReferenceOnly ? "true" : "false") << "\"";
  of << " PassFractionPercent=\"" << this->PassFractionPercent << "\"";
  of << " ResultsValid=\"" << (this->ResultsValid ? "true" : "false") << "\"";
  of << " ReportString=\"" << (this->ReportString ? this->ReportString : "") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = nullptr;
  const char* attValue = nullptr;

  while (*atts != nullptr)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "MaskSegmentID"))
      {
      this->SetMaskSegmentID(vtkVariant(attValue).ToString().c_str());
      }
    else if (!strcmp(attName, "DtaDistanceToleranceMm"))
      {
      this->DtaDistanceToleranceMm = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "DoseDifferenceTolerancePercent"))
      {
      this->DoseDifferenceTolerancePercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "ReferenceDoseGy"))
      {
      this->ReferenceDoseGy = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "AnalysisThresholdPercent"))
      {
      this->AnalysisThresholdPercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "MaximumGamma"))
      {
      this->MaximumGamma = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "UseMaximumDose"))
      {
      this->UseMaximumDose = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "UseGeometricGammaCalculation"))
      {
      this->UseGeometricGammaCalculation = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "LocalDoseDifference"))
      {
      this->LocalDoseDifference = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "DoseThresholdOnReferenceOnly"))
      {
      this->DoseThresholdOnReferenceOnly = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "PassFractionPercent"))
      {
      this->PassFractionPercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "ResultsValid"))
      {
      this->ResultsValid = (strcmp(attValue,"true") ? false : true);
      }
    }

  // Note: ReportString is not read from XML, it is a strictly temporary value
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDoseComparisonNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLDoseComparisonNode *node = (vtkMRMLDoseComparisonNode *) anode;

  this->SetMaskSegmentID(node->MaskSegmentID);
  this->DtaDistanceToleranceMm = node->DtaDistanceToleranceMm;
  this->DoseDifferenceTolerancePercent = node->DoseDifferenceTolerancePercent;
  this->ReferenceDoseGy = node->ReferenceDoseGy;
  this->AnalysisThresholdPercent = node->AnalysisThresholdPercent;
  this->MaximumGamma = node->MaximumGamma;
  this->PassFractionPercent = node->PassFractionPercent;
  this->UseMaximumDose = node->UseMaximumDose;
  this->UseGeometricGammaCalculation = node->UseGeometricGammaCalculation;
  this->LocalDoseDifference = node->LocalDoseDifference;
  this->DoseThresholdOnReferenceOnly = node->DoseThresholdOnReferenceOnly;
  this->ResultsValid = node->ResultsValid;
  this->ReportString = node->ReportString;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "MaskSegmentID:   " << (this->MaskSegmentID ? this->MaskSegmentID : "nullptr") << "\n";
  os << indent << "DtaDistanceToleranceMm:   " << this->DtaDistanceToleranceMm << "\n";
  os << indent << "DoseDifferenceTolerancePercent:   " << this->DoseDifferenceTolerancePercent << "\n";
  os << indent << "ReferenceDoseGy:   " << this->ReferenceDoseGy << "\n";
  os << indent << "AnalysisThresholdPercent:   " << this->AnalysisThresholdPercent << "\n";
  os << indent << "MaximumGamma:   " << this->MaximumGamma << "\n";
  os << indent << "UseMaximumDose:   " << (this->UseMaximumDose ? "true" : "false") << "\n";
  os << indent << "UseGeometricGammaCalculation:   " << (this->UseGeometricGammaCalculation ? "true" : "false") << "\n";
  os << indent << "LocalDoseDifference:   " << (this->LocalDoseDifference ? "true" : "false") << "\n";
  os << indent << "DoseThresholdOnReferenceOnly:   " << (this->DoseThresholdOnReferenceOnly ? "true" : "false") << "\n";
  os << indent << "PassFractionPercent:   " << this->PassFractionPercent << "\n";
  os << indent << "ResultsValid:   " << (this->ResultsValid ? "true" : "false") << "\n";
  os << indent << "ReportString:   " << (this->ReportString ? this->ReportString : "") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseComparisonNode::GetReferenceDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(REFERENCE_DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveReferenceDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(REFERENCE_DOSE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseComparisonNode::GetCompareDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(COMPARE_DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveCompareDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(COMPARE_DOSE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLDoseComparisonNode::GetMaskSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(MASK_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveMaskSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(MASK_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseComparisonNode::GetGammaVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(GAMMA_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveGammaVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(GAMMA_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}
