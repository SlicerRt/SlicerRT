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
#include "vtkMRMLSegmentComparisonNode.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* REFERENCE_SEGMENTATION_REFERENCE_ROLE = "referenceSegmentationRef";
static const char* COMPARE_SEGMENTATION_REFERENCE_ROLE = "compareSegmentationRef";
static const char* RASTERIZATION_REFERENCE_VOLUME_REFERENCE_ROLE = "rasterizationReferenceVolumeRef";
static const char* DICE_TABLE_REFERENCE_ROLE = "diceTableRef";
static const char* HAUSDORFF_TABLE_REFERENCE_ROLE = "hausdorffTableRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentComparisonNode::vtkMRMLSegmentComparisonNode()
{
  this->ReferenceSegmentID = nullptr;
  this->CompareSegmentID = nullptr;

  this->DiceCoefficient = -1.0;
  this->TruePositivesPercent = -1.0;
  this->TrueNegativesPercent = -1.0;
  this->FalsePositivesPercent = -1.0;
  this->FalseNegativesPercent = -1.0;
  this->ReferenceCenter[0] = this->ReferenceCenter[1] = this->ReferenceCenter[2] = 0.0;
  this->CompareCenter[0] = this->CompareCenter[1] = this->CompareCenter[2] = 0.0;
  this->ReferenceVolumeCc = -1.0;
  this->CompareVolumeCc = -1.0;
  this->DiceResultsValidOff();

  this->MaximumHausdorffDistanceForVolumeMm = -1.0;
  this->MaximumHausdorffDistanceForBoundaryMm = -1.0;
  this->AverageHausdorffDistanceForVolumeMm = -1.0;
  this->AverageHausdorffDistanceForBoundaryMm = -1.0;
  this->Percent95HausdorffDistanceForVolumeMm = -1.0;
  this->Percent95HausdorffDistanceForBoundaryMm = -1.0;
  this->HausdorffResultsValidOff();

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLSegmentComparisonNode::~vtkMRMLSegmentComparisonNode()
{
  this->SetReferenceSegmentID(nullptr);
  this->SetCompareSegmentID(nullptr);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  of << " ReferenceSegmentID=\"" << (this->ReferenceSegmentID ? this->ReferenceSegmentID : "nullptr") << "\"";
  of << " CompareSegmentID=\"" << (this->CompareSegmentID ? this->CompareSegmentID : "nullptr") << "\"";

  of << " DiceCoefficient=\"" << this->DiceCoefficient << "\"";
  of << " TruePositivesPercent=\"" << this->TruePositivesPercent << "\"";
  of << " TrueNegativesPercent=\"" << this->TrueNegativesPercent << "\"";
  of << " FalsePositivesPercent=\"" << this->FalsePositivesPercent << "\"";
  of << " FalseNegativesPercent=\"" << this->FalseNegativesPercent << "\"";

  {
    std::stringstream ss;
    of << " ReferenceCenter=\"";
    for (int i=0; i<3; ++i)
    {
      ss << this->ReferenceCenter[i];
      of << ss.str() << "|";
    }
    of << "\"";
  }
  {
    std::stringstream ss;
    of << " CompareCenter=\"";
    for (int i=0; i<3; ++i)
    {
      ss << this->CompareCenter[i];
      of << ss.str() << "|";
    }
    of << "\"";
  }

  of << " ReferenceVolumeCc=\"" << this->ReferenceVolumeCc << "\"";
  of << " CompareVolumeCc=\"" << this->CompareVolumeCc << "\"";

  of << " DiceResultsValid=\"" << (this->DiceResultsValid ? "true" : "false") << "\"";

  of << " MaximumHausdorffDistanceForVolumeMm=\"" << this->MaximumHausdorffDistanceForVolumeMm << "\"";
  of << " MaximumHausdorffDistanceForBoundaryMm=\"" << this->MaximumHausdorffDistanceForBoundaryMm << "\"";
  of << " AverageHausdorffDistanceForVolumeMm=\"" << this->AverageHausdorffDistanceForVolumeMm << "\"";
  of << " AverageHausdorffDistanceForBoundaryMm=\"" << this->AverageHausdorffDistanceForBoundaryMm << "\"";
  of << " Percent95HausdorffDistanceForVolumeMm=\"" << this->Percent95HausdorffDistanceForVolumeMm << "\"";
  of << " Percent95HausdorffDistanceForBoundaryMm=\"" << this->Percent95HausdorffDistanceForBoundaryMm << "\"";

  of << " HausdorffResultsValid=\"" << (this->HausdorffResultsValid ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = nullptr;
  const char* attValue = nullptr;

  while (*atts != nullptr) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ReferenceSegmentID")) 
      {
      this->SetReferenceSegmentID(attValue);
      }
    else if (!strcmp(attName, "CompareSegmentID")) 
      {
      this->SetCompareSegmentID(attValue);
      }
    else if (!strcmp(attName, "DiceCoefficient")) 
      {
      this->DiceCoefficient = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "TruePositivesPercent")) 
      {
      this->TruePositivesPercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "TrueNegativesPercent")) 
      {
      this->TrueNegativesPercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "FalsePositivesPercent")) 
      {
      this->FalsePositivesPercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "FalseNegativesPercent")) 
      {
      this->FalseNegativesPercent = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "ReferenceCenter")) 
      {
      std::string valueStr(attValue);
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );
      this->ReferenceCenter[0] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->ReferenceCenter[1] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->ReferenceCenter[2] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();
      }
    else if (!strcmp(attName, "CompareCenter")) 
      {
      std::string valueStr(attValue);
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );
      this->CompareCenter[0] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->CompareCenter[1] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->CompareCenter[2] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();
      }
    else if (!strcmp(attName, "ReferenceVolumeCc")) 
      {
      this->ReferenceVolumeCc = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "CompareVolumeCc")) 
      {
      this->CompareVolumeCc = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "DiceResultsValid")) 
      {
      this->DiceResultsValid = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "MaximumHausdorffDistanceForVolumeMm")) 
      {
      this->MaximumHausdorffDistanceForVolumeMm = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "MaximumHausdorffDistanceForBoundaryMm")) 
    {
      this->MaximumHausdorffDistanceForBoundaryMm = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "AverageHausdorffDistanceForVolumeMm")) 
      {
      this->AverageHausdorffDistanceForVolumeMm = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "AverageHausdorffDistanceForBoundaryMm")) 
    {
      this->AverageHausdorffDistanceForBoundaryMm = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "Percent95HausdorffDistanceForVolumeMm")) 
      {
      this->Percent95HausdorffDistanceForVolumeMm = vtkVariant(attValue).ToDouble();
      }
    else if (!strcmp(attName, "Percent95HausdorffDistanceForBoundaryMm")) 
    {
      this->Percent95HausdorffDistanceForBoundaryMm = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "HausdorffResultsValid")) 
      {
      this->HausdorffResultsValid = (strcmp(attValue,"true") ? false : true);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSegmentComparisonNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLSegmentComparisonNode *node = (vtkMRMLSegmentComparisonNode *) anode;

  this->SetReferenceSegmentID(node->ReferenceSegmentID);
  this->SetCompareSegmentID(node->CompareSegmentID);
  this->DiceCoefficient = node->DiceCoefficient;
  this->TruePositivesPercent = node->TruePositivesPercent;
  this->TrueNegativesPercent = node->TrueNegativesPercent;
  this->FalsePositivesPercent = node->FalsePositivesPercent;
  this->FalseNegativesPercent = node->FalseNegativesPercent;
  this->SetReferenceCenter( node->GetReferenceCenter() );
  this->SetCompareCenter( node->GetCompareCenter() );
  this->ReferenceVolumeCc = node->ReferenceVolumeCc;
  this->CompareVolumeCc = node->CompareVolumeCc;
  this->DiceResultsValid = node->DiceResultsValid;
  this->MaximumHausdorffDistanceForVolumeMm = node->MaximumHausdorffDistanceForVolumeMm;
  this->MaximumHausdorffDistanceForBoundaryMm = node->MaximumHausdorffDistanceForBoundaryMm;
  this->AverageHausdorffDistanceForVolumeMm = node->AverageHausdorffDistanceForVolumeMm;
  this->AverageHausdorffDistanceForBoundaryMm = node->AverageHausdorffDistanceForBoundaryMm;
  this->Percent95HausdorffDistanceForVolumeMm = node->Percent95HausdorffDistanceForVolumeMm;
  this->Percent95HausdorffDistanceForBoundaryMm = node->Percent95HausdorffDistanceForBoundaryMm;
  this->HausdorffResultsValid = node->HausdorffResultsValid;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " ReferenceSegmentID:   " << (this->ReferenceSegmentID ? this->ReferenceSegmentID : "nullptr") << "\n";
  os << indent << " CompareSegmentID:   " << (this->CompareSegmentID ? this->CompareSegmentID : "nullptr") << "\n";

  os << indent << " DiceCoefficient:   " << this->DiceCoefficient << "\n";
  os << indent << " TruePositivesPercent:   " << this->TruePositivesPercent << "\n";
  os << indent << " TrueNegativesPercent:   " << this->TrueNegativesPercent << "\n";
  os << indent << " FalsePositivesPercent:   " << this->FalsePositivesPercent << "\n";
  os << indent << " FalseNegativesPercent:   " << this->FalseNegativesPercent << "\n";

  {
    std::stringstream ss;
    os << indent << " ReferenceCenter:   ";
    for (int i=0; i<3; ++i)
    {
      ss << this->ReferenceCenter[i];
      os << ss.str() << "|";
    }
    os << "\n";
  }
  {
    std::stringstream ss;
    os << indent << " CompareCenter:   ";
    for (int i=0; i<3; ++i)
    {
      ss << this->CompareCenter[i];
      os << ss.str() << "|";
    }
    os << "\n";
  }

  os << indent << " ReferenceVolumeCc:   " << this->ReferenceVolumeCc << "\n";
  os << indent << " CompareVolumeCc:   " << this->CompareVolumeCc << "\n";

  os << indent << " DiceResultsValid:   " << (this->DiceResultsValid ? "true" : "false") << "\n";

  os << indent << " MaximumHausdorffDistanceForVolumeMm:   " << this->MaximumHausdorffDistanceForVolumeMm << "\n";
  os << indent << " MaximumHausdorffDistanceForBoundaryMm:   " << this->MaximumHausdorffDistanceForBoundaryMm << "\n";
  os << indent << " AverageHausdorffDistanceForVolumeMm:   " << this->AverageHausdorffDistanceForVolumeMm << "\n";
  os << indent << " AverageHausdorffDistanceForBoundaryMm:   " << this->AverageHausdorffDistanceForBoundaryMm << "\n";
  os << indent << " Percent95HausdorffDistanceForVolumeMm:   " << this->Percent95HausdorffDistanceForVolumeMm << "\n";
  os << indent << " Percent95HausdorffDistanceForBoundaryMm:   " << this->Percent95HausdorffDistanceForBoundaryMm << "\n";

  os << indent << " HausdorffResultsValid:   " << (this->HausdorffResultsValid ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentComparisonNode::GetReferenceSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(REFERENCE_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveReferenceSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(REFERENCE_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentComparisonNode::GetCompareSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(COMPARE_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveCompareSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(COMPARE_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLSegmentComparisonNode::GetRasterizationReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(RASTERIZATION_REFERENCE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveRasterizationReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(RASTERIZATION_REFERENCE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLSegmentComparisonNode::GetDiceTableNode()
{
  return vtkMRMLTableNode::SafeDownCast( this->GetNodeReference(DICE_TABLE_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveDiceTableNode(vtkMRMLTableNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(DICE_TABLE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLSegmentComparisonNode::GetHausdorffTableNode()
{
  return vtkMRMLTableNode::SafeDownCast( this->GetNodeReference(HAUSDORFF_TABLE_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveHausdorffTableNode(vtkMRMLTableNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(HAUSDORFF_TABLE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}
