/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTProtonBeamNode.h"
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageResample.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTProtonBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTProtonBeamNode::vtkMRMLRTProtonBeamNode()
{
  this->ProximalMargin = 0.0;
  this->DistalMargin = 0.0;

  this->BeamLineTypeActive = true;

  this->ManualEnergyLimits = false;
  this->MaximumEnergy = 0;
  this->MinimumEnergy = 0;

  this->EnergyResolution = 2.0;
  this->EnergySpread = 1.0;
  this->StepLength = 1.0;
  this->PencilBeamResolution = 1.0;
  this->RangeCompensatorSmearingRadius = 0.0;
  this->Algorithm = RayTracer;

  this->ApertureOffset = 1500.0;
  this->ApertureSpacing[0] = 1;
  this->ApertureSpacing[1] = 1;
  this->ApertureOrigin[0] = -1;
  this->ApertureOrigin[1] = -1;
  this->ApertureDim[0] = 1;
  this->ApertureDim[1] = 1;

  this->SourceSize = 0.0;
  this->SetRadiationType(Proton);

  this->LateralSpreadHomoApprox = false;
  this->RangeCompensatorHighland = false;
}

//----------------------------------------------------------------------------
vtkMRMLRTProtonBeamNode::~vtkMRMLRTProtonBeamNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLRTProtonBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " BeamLineTypeActive=\"" << (this->BeamLineTypeActive ? "true" : "false") << "\"";

  of << indent << " ManualEnergyLimits=\"" << (this->ManualEnergyLimits ? "true" : "false") << "\"";
  of << indent << " MinimumEnergy=\"" << this->MinimumEnergy << "\"";
  of << indent << " MaximumEnergy=\"" << this->MaximumEnergy << "\"";

  of << indent << " EnergyResolution=\"" << this->EnergyResolution << "\"";
  of << indent << " EnergySpread=\"" << this->EnergySpread << "\"";
  of << indent << " StepLength=\"" << this->StepLength << "\"";

  of << indent << " ProximalMargin=\"" << this->ProximalMargin << "\"";
  of << indent << " DistalMargin=\"" << this->DistalMargin << "\"";

  of << indent << " PencilBeamResolution=\"" << this->PencilBeamResolution << "\"";

  of << indent << " ApertureOffset=\"" << this->ApertureOffset << "\"";

  of << indent << " SourceSize=\"" << this->SourceSize << "\"";

  of << indent << " LateralSpreadHomoApprox=\"" << (this->LateralSpreadHomoApprox ? "true" : "false") << "\"";
  of << indent << " RangeCompensatorHighland=\"" << (this->RangeCompensatorHighland ? "true" : "false") << "\"";

  of << indent << " HavePrescription=\"" << (this->HavePrescription ? "true" : "false") << "\"";

  of << indent << " RangeCompensatorSmearingRadius=\"" << this->RangeCompensatorSmearingRadius << "\"";

  of << indent << " Algorithm=\"" << (int)this->Algorithm << "\"";

  {
    of << indent << " ApertureSpacing=\"";
    for (int i=0; i<2; ++i)
    {
      of << this->ApertureSpacing[i] << "|";
    }
    of << "\"";
  }
  {
    of << indent << " ApertureOrigin=\"";
    for (int i=0; i<2; ++i)
    {
      of << this->ApertureOrigin[i] << "|";
    }
    of << "\"";
  }
  {
    of << indent << " ReferenceCenter=\"";
    for (int i=0; i<2; ++i)
    {
      of << this->ApertureDim[i] << "|";
    }
    of << "\"";
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTProtonBeamNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "BeamNumber")) 
    {
      this->BeamLineTypeActive = (strcmp(attValue,"true") ? false : true);
    }
    else if (!strcmp(attName, "BeamDescription")) 
    {
      this->ManualEnergyLimits = (strcmp(attValue,"true") ? false : true);
    }
    else if (!strcmp(attName, "MinimumEnergy")) 
    {
      this->MinimumEnergy = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "MaximumEnergy")) 
    {
      this->MaximumEnergy = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "EnergyResolution")) 
    {
      this->EnergyResolution = vtkVariant(attValue).ToFloat();
    }
    else if (!strcmp(attName, "EnergySpread")) 
    {
      this->EnergySpread = vtkVariant(attValue).ToFloat();
    }
    else if (!strcmp(attName, "StepLength")) 
    {
      this->StepLength = vtkVariant(attValue).ToFloat();
    }
    else if (!strcmp(attName, "ProximalMargin")) 
    {
      this->ProximalMargin = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "DistalMargin")) 
    {
      this->DistalMargin = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "PencilBeamResolution")) 
    {
      this->PencilBeamResolution = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "ApertureOffset")) 
    {
      this->ApertureOffset = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "SourceSize")) 
    {
      this->SourceSize = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "LateralSpreadHomoApprox")) 
    {
      this->LateralSpreadHomoApprox = (strcmp(attValue,"true") ? false : true);
    }
    else if (!strcmp(attName, "RangeCompensatorHighland")) 
    {
      this->RangeCompensatorHighland = (strcmp(attValue,"true") ? false : true);
    }
    else if (!strcmp(attName, "HavePrescription")) 
    {
      this->HavePrescription = (strcmp(attValue,"true") ? false : true);
    }
    else if (!strcmp(attName, "RangeCompensatorSmearingRadius")) 
    {
      this->RangeCompensatorSmearingRadius = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "Algorithm")) 
    {
      this->Algorithm = (RTProtonAlgorithm)vtkVariant(attValue).ToInt();
    }
    else if (!strcmp(attName, "ApertureSpacing")) 
    {
      std::string valueStr(attValue);
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );
      this->ApertureSpacing[0] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->ApertureSpacing[1] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();
    }
    else if (!strcmp(attName, "ApertureOrigin")) 
    {
      std::string valueStr(attValue);
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );
      this->ApertureOrigin[0] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->ApertureOrigin[1] = vtkVariant(valueStr.substr(0, separatorPosition)).ToDouble();
    }
    else if (!strcmp(attName, "ApertureDim")) 
    {
      std::string valueStr(attValue);
      std::string separatorCharacter("|");

      size_t separatorPosition = valueStr.find( separatorCharacter );
      this->ApertureDim[0] = vtkVariant(valueStr.substr(0, separatorPosition)).ToInt();

      valueStr = valueStr.substr( separatorPosition+1 );
      separatorPosition = valueStr.find( separatorCharacter );
      this->ApertureDim[1] = vtkVariant(valueStr.substr(0, separatorPosition)).ToInt();
    }
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTProtonBeamNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLRTProtonBeamNode *node = (vtkMRMLRTProtonBeamNode *) anode;

  this->SetBeamLineTypeActive(node->GetBeamLineTypeActive());

  this->SetManualEnergyLimits(node->GetManualEnergyLimits());
  this->SetMaximumEnergy(node->GetMaximumEnergy());
  this->SetMinimumEnergy(node->GetMinimumEnergy());

  this->SetEnergyResolution(node->GetEnergyResolution());
  this->SetEnergySpread(node->GetEnergySpread());
  this->SetStepLength(node->GetStepLength());

  this->SetProximalMargin(node->GetProximalMargin());
  this->SetDistalMargin(node->GetDistalMargin());

  this->SetPencilBeamResolution(node->GetPencilBeamResolution());

  this->SetApertureOffset(node->GetApertureOffset());

  this->SetSourceSize(node->GetSourceSize());

  this->SetLateralSpreadHomoApprox(node->GetLateralSpreadHomoApprox());
  this->SetRangeCompensatorHighland(node->GetRangeCompensatorHighland());

  this->SetHavePrescription(node->GetHavePrescription());

  this->SetRangeCompensatorSmearingRadius(node->GetRangeCompensatorSmearingRadius());

  this->SetAlgorithm(node->GetAlgorithm());

  this->SetApertureSpacing(node->GetApertureSpacing());
  this->SetApertureOrigin(node->GetApertureOrigin());
  this->SetApertureDim(node->GetApertureDim());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTProtonBeamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " BeamLineTypeActive:   " << (this->BeamLineTypeActive ? "true" : "false") << "\n";

  os << indent << " ManualEnergyLimits:   " << (this->ManualEnergyLimits ? "true" : "false") << "\n";
  os << indent << " MinimumEnergy:   " << this->MinimumEnergy << "\n";
  os << indent << " MaximumEnergy:   " << this->MaximumEnergy << "\n";

  os << indent << " EnergyResolution:   " << this->EnergyResolution << "\n";
  os << indent << " EnergySpread:   " << this->EnergySpread << "\n";
  os << indent << " StepLength:   " << this->StepLength << "\n";

  os << indent << " ProximalMargin:   " << this->ProximalMargin << "\n";
  os << indent << " DistalMargin:   " << this->DistalMargin << "\n";

  os << indent << " PencilBeamResolution:   " << this->PencilBeamResolution << "\n";

  os << indent << " ApertureOffset:   " << this->ApertureOffset << "\n";

  os << indent << " SourceSize:   " << this->SourceSize << "\n";

  os << indent << " LateralSpreadHomoApprox:   " << (this->LateralSpreadHomoApprox ? "true" : "false") << "\n";
  os << indent << " RangeCompensatorHighland:   " << (this->RangeCompensatorHighland ? "true" : "false") << "\n";

  os << indent << " HavePrescription:   " << (this->HavePrescription ? "true" : "false") << "\n";

  os << indent << " RangeCompensatorSmearingRadius:   " << this->RangeCompensatorSmearingRadius << "\n";

  os << indent << " Algorithm:   " << (int)this->Algorithm << "\n";

  {
    os << indent << " ApertureSpacing:   ";
    for (int i=0; i<2; ++i)
    {
      os << this->ApertureSpacing[i] << (i<2?", ":"");
    }
    os << "\n";
  }
  {
    os << indent << " ApertureOrigin:   ";
    for (int i=0; i<2; ++i)
    {
      os << this->ApertureOrigin[i] << (i<2?", ":"");
    }
    os << "\n";
  }
  {
    os << indent << " ApertureDim:   ";
    for (int i=0; i<2; ++i)
    {
      os << this->ApertureDim[i] << (i<2?", ":"");
    }
    os << "\n";
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTProtonBeamNode::UpdateApertureParameters()
{
  if (this->SAD < 0 || this->SAD < this->ApertureOffset)
  {
    vtkErrorMacro("UpdateApertureParameters: SAD (=" << this->SAD << ") must be positive and greater than Aperture offset (" << this->ApertureOffset << ")");
    return;
  }
  double origin[2] = { this->X1Jaw * this->ApertureOffset / this->SAD , this->Y1Jaw * this->ApertureOffset / this->SAD };
  this->SetApertureOrigin(origin);

  double spacing_at_aperture[2] = { 1.0 / this->PencilBeamResolution * this->ApertureOffset / this->SAD, 1.0 / this->PencilBeamResolution * this->ApertureOffset / this->SAD };
  this->SetApertureSpacing(spacing_at_aperture);

  int dim[2] = { (int)((this->X2Jaw - this->X1Jaw) / this->PencilBeamResolution + 1 ), (int)((this->Y2Jaw - this->Y1Jaw) / this->PencilBeamResolution + 1 ) };
  this->SetApertureDim(dim);
}
