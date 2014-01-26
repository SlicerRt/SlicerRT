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

// MRMLDoseAccumulation includes
#include "vtkMRMLDoseComparisonNode.h"

// SlicerRT includes
#include "vtkMRMLContourNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* REFERENCE_DOSE_VOLUME_REFERENCE_ROLE = "referenceDoseVolumeRef";
static const char* COMPARE_DOSE_VOLUME_REFERENCE_ROLE = "compareDoseVolumeRef";
static const char* MASK_CONTOUR_REFERENCE_ROLE = "maskContourRef";
static const char* GAMMA_VOLUME_REFERENCE_ROLE = "outputGammaVolumeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDoseComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLDoseComparisonNode::vtkMRMLDoseComparisonNode()
{
  this->DtaDistanceToleranceMm = 3.0;
  this->DoseDifferenceTolerancePercent = 3.0;
  this->ReferenceDoseGy = 50.0;
  this->AnalysisThresholdPercent = 10.0;
  this->MaximumGamma = 2.0;
  this->UseMaximumDose = true;
  this->PassFractionPercent = -1.0;
  this->ResultsValidOff();

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseComparisonNode::~vtkMRMLDoseComparisonNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " DtaDistanceToleranceMm=\"" << this->DtaDistanceToleranceMm << "\"";
  of << indent << " DoseDifferenceTolerancePercent=\"" << this->DoseDifferenceTolerancePercent << "\"";
  of << indent << " ReferenceDoseGy=\"" << this->ReferenceDoseGy << "\"";
  of << indent << " AnalysisThresholdPercent=\"" << this->AnalysisThresholdPercent << "\"";
  of << indent << " MaximumGamma=\"" << this->MaximumGamma << "\"";
  of << indent << " UseMaximumDose=\"" << (this->UseMaximumDose ? "true" : "false") << "\"";
  of << indent << " PassFractionPercent=\"" << this->PassFractionPercent << "\"";
  of << indent << " ResultsValid=\"" << (this->ResultsValid ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "DtaDistanceToleranceMm")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->DtaDistanceToleranceMm = doubleAttValue;
      }
    else if (!strcmp(attName, "DoseDifferenceTolerancePercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->DoseDifferenceTolerancePercent = doubleAttValue;
      }
    else if (!strcmp(attName, "ReferenceDoseGy")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->ReferenceDoseGy = doubleAttValue;
      }
    else if (!strcmp(attName, "AnalysisThresholdPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->AnalysisThresholdPercent = doubleAttValue;
      }
    else if (!strcmp(attName, "MaximumGamma")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->MaximumGamma = doubleAttValue;
      }
    else if (!strcmp(attName, "UseMaximumDose")) 
      {
      this->UseMaximumDose = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "PassFractionPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->PassFractionPercent = doubleAttValue;
      }
    else if (!strcmp(attName, "ResultsValid")) 
      {
      this->ResultsValid = 
        (strcmp(attValue,"true") ? false : true);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDoseComparisonNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLDoseComparisonNode *node = (vtkMRMLDoseComparisonNode *) anode;

  this->DtaDistanceToleranceMm = node->DtaDistanceToleranceMm;
  this->DoseDifferenceTolerancePercent = node->DoseDifferenceTolerancePercent;
  this->ReferenceDoseGy = node->ReferenceDoseGy;
  this->AnalysisThresholdPercent = node->AnalysisThresholdPercent;
  this->MaximumGamma = node->MaximumGamma;
  this->PassFractionPercent = node->PassFractionPercent;
  this->UseMaximumDose = node->UseMaximumDose;
  this->ResultsValid = node->ResultsValid;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "DtaDistanceToleranceMm:   " << this->DtaDistanceToleranceMm << "\n";
  os << indent << "DoseDifferenceTolerancePercent:   " << this->DoseDifferenceTolerancePercent << "\n";
  os << indent << "ReferenceDoseGy:   " << this->ReferenceDoseGy << "\n";
  os << indent << "AnalysisThresholdPercent:   " << this->AnalysisThresholdPercent << "\n";
  os << indent << "MaximumGamma:   " << this->MaximumGamma << "\n";
  os << indent << "PassFractionPercent:   " << this->PassFractionPercent << "\n";
  os << indent << "UseMaximumDose:   " << (this->UseMaximumDose ? "true" : "false") << "\n";
  os << indent << "ResultsValid:   " << (this->ResultsValid ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseComparisonNode::GetReferenceDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(REFERENCE_DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveReferenceDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(REFERENCE_DOSE_VOLUME_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseComparisonNode::GetCompareDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(COMPARE_DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveCompareDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(COMPARE_DOSE_VOLUME_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLDoseComparisonNode::GetMaskContourNode()
{
  return vtkMRMLContourNode::SafeDownCast( this->GetNodeReference(MASK_CONTOUR_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveMaskContourNode(vtkMRMLContourNode* node)
{
  this->SetNodeReferenceID(MASK_CONTOUR_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseComparisonNode::GetGammaVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(GAMMA_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveGammaVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(GAMMA_VOLUME_REFERENCE_ROLE, node->GetID());
}

