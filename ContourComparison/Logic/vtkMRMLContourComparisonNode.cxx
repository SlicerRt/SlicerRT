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
#include "vtkMRMLContourComparisonNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
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
std::string vtkMRMLContourComparisonNode::ReferenceContourReferenceRole = std::string("referenceContour") + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;
std::string vtkMRMLContourComparisonNode::CompareContourReferenceRole = std::string("compareContour") + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;
std::string vtkMRMLContourComparisonNode::RasterizationReferenceVolumeReferenceRole = std::string("rasterizationReferenceVolume") + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLContourComparisonNode::vtkMRMLContourComparisonNode()
{
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
vtkMRMLContourComparisonNode::~vtkMRMLContourComparisonNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

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
void vtkMRMLContourComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "DiceCoefficient")) 
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
void vtkMRMLContourComparisonNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLContourComparisonNode *node = (vtkMRMLContourComparisonNode *) anode;

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
void vtkMRMLContourComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " DiceCoefficient=\"" << this->DiceCoefficient << "\"";
  os << indent << " TruePositivesPercent=\"" << this->TruePositivesPercent << "\"";
  os << indent << " TrueNegativesPercent=\"" << this->TrueNegativesPercent << "\"";
  os << indent << " FalsePositivesPercent=\"" << this->FalsePositivesPercent << "\"";
  os << indent << " FalseNegativesPercent=\"" << this->FalseNegativesPercent << "\"";

  {
    std::stringstream ss;
    os << indent << " ReferenceCenter=\"";
    for (int i=0; i<3; ++i)
    {
      ss << this->ReferenceCenter[i];
      os << ss.str() << "|";
    }
    os << "\"";
  }
  {
    std::stringstream ss;
    os << indent << " CompareCenter=\"";
    for (int i=0; i<3; ++i)
    {
      ss << this->CompareCenter[i];
      os << ss.str() << "|";
    }
    os << "\"";
  }

  os << indent << " ReferenceVolumeCc=\"" << this->ReferenceVolumeCc << "\"";
  os << indent << " CompareVolumeCc=\"" << this->CompareVolumeCc << "\"";

  os << indent << " DiceResultsValid=\"" << (this->DiceResultsValid ? "true" : "false") << "\"";

  os << indent << " MaximumHausdorffDistanceForVolumeMm=\"" << this->MaximumHausdorffDistanceForVolumeMm << "\"";
  os << indent << " MaximumHausdorffDistanceForBoundaryMm=\"" << this->MaximumHausdorffDistanceForBoundaryMm << "\"";
  os << indent << " AverageHausdorffDistanceForVolumeMm=\"" << this->AverageHausdorffDistanceForVolumeMm << "\"";
  os << indent << " AverageHausdorffDistanceForBoundaryMm=\"" << this->AverageHausdorffDistanceForBoundaryMm << "\"";
  os << indent << " Percent95HausdorffDistanceForVolumeMm=\"" << this->Percent95HausdorffDistanceForVolumeMm << "\"";
  os << indent << " Percent95HausdorffDistanceForBoundaryMm=\"" << this->Percent95HausdorffDistanceForBoundaryMm << "\"";

  os << indent << " HausdorffResultsValid=\"" << (this->HausdorffResultsValid ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLContourComparisonNode::GetReferenceContourNode()
{
  return vtkMRMLContourNode::SafeDownCast(
    this->GetNodeReference(vtkMRMLContourComparisonNode::ReferenceContourReferenceRole.c_str()) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveReferenceContourNode(vtkMRMLContourNode* node)
{
  this->SetNthNodeReferenceID(vtkMRMLContourComparisonNode::ReferenceContourReferenceRole.c_str(), 0, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLContourComparisonNode::GetCompareContourNode()
{
  return vtkMRMLContourNode::SafeDownCast(
    this->GetNodeReference(vtkMRMLContourComparisonNode::CompareContourReferenceRole.c_str()) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveCompareContourNode(vtkMRMLContourNode* node)
{
  this->SetNthNodeReferenceID(vtkMRMLContourComparisonNode::CompareContourReferenceRole.c_str(), 0, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourComparisonNode::GetRasterizationReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetNodeReference(vtkMRMLContourComparisonNode::RasterizationReferenceVolumeReferenceRole.c_str()) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveRasterizationReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNthNodeReferenceID(vtkMRMLContourComparisonNode::RasterizationReferenceVolumeReferenceRole.c_str(), 0, node->GetID());
}
