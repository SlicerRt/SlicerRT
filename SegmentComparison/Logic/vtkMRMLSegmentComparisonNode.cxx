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

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* REFERENCE_SEGMENTATION_REFERENCE_ROLE = "referenceSegmentationRef";
static const char* COMPARE_SEGMENTATION_REFERENCE_ROLE = "compareSegmentationRef";
static const char* RASTERIZATION_REFERENCE_VOLUME_REFERENCE_ROLE = "rasterizationReferenceVolumeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentComparisonNode::vtkMRMLSegmentComparisonNode()
{
  this->ReferenceSegmentID = NULL;
  this->CompareSegmentID = NULL;

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
  this->SetReferenceSegmentID(NULL);
  this->SetCompareSegmentID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " ReferenceSegmentID=\"" << (this->ReferenceSegmentID ? this->ReferenceSegmentID : "NULL") << "\"";
  of << indent << " CompareSegmentID=\"" << (this->CompareSegmentID ? this->CompareSegmentID : "NULL") << "\"";

  of << indent << " DiceCoefficient=\"" << this->DiceCoefficient << "\"";
  of << indent << " TruePositivesPercent=\"" << this->TruePositivesPercent << "\"";
  of << indent << " TrueNegativesPercent=\"" << this->TrueNegativesPercent << "\"";
  of << indent << " FalsePositivesPercent=\"" << this->FalsePositivesPercent << "\"";
  of << indent << " FalseNegativesPercent=\"" << this->FalseNegativesPercent << "\"";

  {
    std::stringstream ss;
    of << indent << " ReferenceCenter=\"";
    for (int i=0; i<3; ++i)
    {
      ss << this->ReferenceCenter[i];
      of << ss.str() << "|";
    }
    of << "\"";
  }
  {
    std::stringstream ss;
    of << indent << " CompareCenter=\"";
    for (int i=0; i<3; ++i)
    {
      ss << this->CompareCenter[i];
      of << ss.str() << "|";
    }
    of << "\"";
  }

  of << indent << " ReferenceVolumeCc=\"" << this->ReferenceVolumeCc << "\"";
  of << indent << " CompareVolumeCc=\"" << this->CompareVolumeCc << "\"";

  of << indent << " DiceResultsValid=\"" << (this->DiceResultsValid ? "true" : "false") << "\"";

  of << indent << " MaximumHausdorffDistanceForVolumeMm=\"" << this->MaximumHausdorffDistanceForVolumeMm << "\"";
  of << indent << " MaximumHausdorffDistanceForBoundaryMm=\"" << this->MaximumHausdorffDistanceForBoundaryMm << "\"";
  of << indent << " AverageHausdorffDistanceForVolumeMm=\"" << this->AverageHausdorffDistanceForVolumeMm << "\"";
  of << indent << " AverageHausdorffDistanceForBoundaryMm=\"" << this->AverageHausdorffDistanceForBoundaryMm << "\"";
  of << indent << " Percent95HausdorffDistanceForVolumeMm=\"" << this->Percent95HausdorffDistanceForVolumeMm << "\"";
  of << indent << " Percent95HausdorffDistanceForBoundaryMm=\"" << this->Percent95HausdorffDistanceForBoundaryMm << "\"";

  of << indent << " HausdorffResultsValid=\"" << (this->HausdorffResultsValid ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ReferenceSegmentID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetReferenceSegmentID(ss.str().c_str());
      }
    else if (!strcmp(attName, "CompareSegmentID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetCompareSegmentID(ss.str().c_str());
      }
    else if (!strcmp(attName, "DiceCoefficient")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->DiceCoefficient = doubleAttValue;
      }
    else if (!strcmp(attName, "TruePositivesPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->TruePositivesPercent = doubleAttValue;
      }
    else if (!strcmp(attName, "TrueNegativesPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->TrueNegativesPercent = doubleAttValue;
      }
    else if (!strcmp(attName, "FalsePositivesPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->FalsePositivesPercent = doubleAttValue;
      }
    else if (!strcmp(attName, "FalseNegativesPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->FalseNegativesPercent = doubleAttValue;
      }
    else if (!strcmp(attName, "ReferenceCenter")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );

        {
        std::stringstream ss;
        ss << valueStr.substr(0, separatorPosition);
        double doubleValue;
        ss >> doubleValue;
        this->ReferenceCenter[0] = doubleValue;
        }
        {
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        std::stringstream ss;
        ss << valueStr.substr(0, separatorPosition);
        double doubleValue;
        ss >> doubleValue;
        this->ReferenceCenter[1] = doubleValue;
        }
        {
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        std::stringstream ss;
        ss << valueStr.substr(0, separatorPosition);
        double doubleValue;
        ss >> doubleValue;
        this->ReferenceCenter[2] = doubleValue;
        }
      }
    else if (!strcmp(attName, "CompareCenter")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );

        {
        std::stringstream ss;
        ss << valueStr.substr(0, separatorPosition);
        double doubleValue;
        ss >> doubleValue;
        this->CompareCenter[0] = doubleValue;
        }
        {
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        std::stringstream ss;
        ss << valueStr.substr(0, separatorPosition);
        double doubleValue;
        ss >> doubleValue;
        this->CompareCenter[1] = doubleValue;
        }
        {
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        std::stringstream ss;
        ss << valueStr.substr(0, separatorPosition);
        double doubleValue;
        ss >> doubleValue;
        this->CompareCenter[2] = doubleValue;
        }
      }
    else if (!strcmp(attName, "ReferenceVolumeCc")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->ReferenceVolumeCc = doubleAttValue;
      }
    else if (!strcmp(attName, "CompareVolumeCc")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->CompareVolumeCc = doubleAttValue;
      }
    else if (!strcmp(attName, "DiceResultsValid")) 
      {
      this->DiceResultsValid = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "MaximumHausdorffDistanceForVolumeMm")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->MaximumHausdorffDistanceForVolumeMm = doubleAttValue;
      }
    else if (!strcmp(attName, "MaximumHausdorffDistanceForBoundaryMm")) 
    {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->MaximumHausdorffDistanceForBoundaryMm = doubleAttValue;
    }
    else if (!strcmp(attName, "AverageHausdorffDistanceForVolumeMm")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->AverageHausdorffDistanceForVolumeMm = doubleAttValue;
      }
    else if (!strcmp(attName, "AverageHausdorffDistanceForBoundaryMm")) 
    {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->AverageHausdorffDistanceForBoundaryMm = doubleAttValue;
    }
    else if (!strcmp(attName, "Percent95HausdorffDistanceForVolumeMm")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->Percent95HausdorffDistanceForVolumeMm = doubleAttValue;
      }
    else if (!strcmp(attName, "Percent95HausdorffDistanceForBoundaryMm")) 
    {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->Percent95HausdorffDistanceForBoundaryMm = doubleAttValue;
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

  os << indent << " ReferenceSegmentID:   " << (this->ReferenceSegmentID ? this->ReferenceSegmentID : "NULL") << "\n";
  os << indent << " CompareSegmentID:   " << (this->CompareSegmentID ? this->CompareSegmentID : "NULL") << "\n";

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
  this->SetNodeReferenceID(REFERENCE_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentComparisonNode::GetCompareSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(COMPARE_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveCompareSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(COMPARE_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLSegmentComparisonNode::GetRasterizationReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(RASTERIZATION_REFERENCE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentComparisonNode::SetAndObserveRasterizationReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(RASTERIZATION_REFERENCE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}
