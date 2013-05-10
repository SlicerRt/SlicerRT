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

// MRMLBeams includes
#include "vtkMRMLProtonBeamsNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLProtonBeamsNode);

//----------------------------------------------------------------------------
vtkMRMLProtonBeamsNode::vtkMRMLProtonBeamsNode()
{
  this->IsocenterFiducialNodeId = NULL;
  this->SourceFiducialNodeId = NULL;
  this->BeamModelNodeId = NULL;

  this->BeamModelOpacity = 0.08;

  this->HideFromEditors = false;

  m_beamName = "defaultname";
	m_isocenter_x = 0;
	m_isocenter_y = 0;
	m_isocenter_z = 0;
	m_gantryAngle = 0;
	m_collimatorAngle = 0;
	m_range = 0;
	m_mod = 0;
	m_jaw_x = 10;
	m_jaw_y = 10;
	m_couchRotation = 0;
	m_couchPitch = 0;
	m_couchRoll = 0;
	m_sourceX = 0;   // position initial of the source in CC
	m_sourceY = 0;
	m_sourceZ = 10000; 
	m_DSA = 10000; // DSA far away to create a parallele beam
}

//----------------------------------------------------------------------------
vtkMRMLProtonBeamsNode::~vtkMRMLProtonBeamsNode()
{
  this->SetIsocenterFiducialNodeId(NULL);
  this->SetSourceFiducialNodeId(NULL);
  this->SetBeamModelNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->IsocenterFiducialNodeId )
      {
      ss << this->IsocenterFiducialNodeId;
      of << indent << " IsocenterFiducialNodeId=\"" << ss.str() << "\"";
     }
  }
  {
    std::stringstream ss;
    if ( this->SourceFiducialNodeId )
    {
      ss << this->SourceFiducialNodeId;
      of << indent << " SourceFiducialNodeId=\"" << ss.str() << "\"";
    }
  }
  {
    std::stringstream ss;
    if ( this->BeamModelNodeId )
      {
      ss << this->BeamModelNodeId;
      of << indent << " BeamModelNodeId=\"" << ss.str() << "\"";
     }
  }
  {
    std::stringstream ss;
    if ( this->BeamModelOpacity )
    {
      ss << this->BeamModelOpacity;
      of << indent << " BeamModelOpacity=\"" << ss.str() << "\"";
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "IsocenterFiducialNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveIsocenterFiducialNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "SourceFiducialNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveSourceFiducialNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "BeamModelNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveBeamModelNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "BeamModelOpacity")) 
      {
      std::stringstream ss;
      ss << attValue;
      double beamModelOpacity;
      ss >> beamModelOpacity;
      this->BeamModelOpacity = beamModelOpacity;
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLProtonBeamsNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLProtonBeamsNode *node = (vtkMRMLProtonBeamsNode *) anode;

  this->SetAndObserveIsocenterFiducialNodeId(node->IsocenterFiducialNodeId);
  this->SetAndObserveSourceFiducialNodeId(node->SourceFiducialNodeId);
  this->SetAndObserveBeamModelNodeId(node->BeamModelNodeId);
  this->SetBeamModelOpacity(node->GetBeamModelOpacity());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "IsocenterFiducialNodeId:   " << this->IsocenterFiducialNodeId << "\n";
  os << indent << "SourceFiducialNodeId:   " << this->SourceFiducialNodeId << "\n";
  os << indent << "BeamModelNodeId:   " << this->BeamModelNodeId << "\n";
  os << indent << "BeamModelOpacity:   " << this->BeamModelOpacity << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::SetAndObserveIsocenterFiducialNodeId(const char* id)
{
  if (this->IsocenterFiducialNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->IsocenterFiducialNodeId, this);
  }

  this->SetIsocenterFiducialNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->IsocenterFiducialNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::SetAndObserveSourceFiducialNodeId(const char* id)
{
  if (this->SourceFiducialNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->SourceFiducialNodeId, this);
  }

  this->SetSourceFiducialNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->SourceFiducialNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::SetAndObserveBeamModelNodeId(const char* id)
{
  if (this->BeamModelNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->BeamModelNodeId, this);
  }

  this->SetBeamModelNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->BeamModelNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLProtonBeamsNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->IsocenterFiducialNodeId && !strcmp(oldID, this->IsocenterFiducialNodeId))
    {
    this->SetAndObserveIsocenterFiducialNodeId(newID);
    }
  if (this->SourceFiducialNodeId && !strcmp(oldID, this->SourceFiducialNodeId))
    {
    this->SetAndObserveSourceFiducialNodeId(newID);
    }
  if (this->BeamModelNodeId && !strcmp(oldID, this->BeamModelNodeId))
    {
    this->SetAndObserveBeamModelNodeId(newID);
    }
}

// accessors definitions

void vtkMRMLProtonBeamsNode::accessBeamName(std::string newBeamName) 
{
	m_beamName = newBeamName;
}

void vtkMRMLProtonBeamsNode::accessIsocenter_x(double newIsocenter_x)
{
	m_isocenter_x = newIsocenter_x;
}
void vtkMRMLProtonBeamsNode::accessIsocenter_y(double newIsocenter_y)
{
	m_isocenter_y = newIsocenter_y;
	
}
void vtkMRMLProtonBeamsNode::accessIsocenter_z(double newIsocenter_z)
{
	m_isocenter_z = newIsocenter_z;
}
void vtkMRMLProtonBeamsNode::accessGantryAngle(double newGantryAngle)
{
	m_gantryAngle = newGantryAngle;
}
void vtkMRMLProtonBeamsNode::accessCollimatorAngle(double newCollimatorAngle)
{
	m_collimatorAngle = newCollimatorAngle;
}
void vtkMRMLProtonBeamsNode::accessRange(double newRange)
{
	m_range = newRange;
}
void vtkMRMLProtonBeamsNode::accessMod(double newMod)
{
	m_mod = newMod;
}
void vtkMRMLProtonBeamsNode::accessDistanceMin(double newDistanceMin)
{
	m_distance_max = newDistanceMin;
}
void vtkMRMLProtonBeamsNode::accessDistanceMax(double newDistanceMax)
{
	m_distance_max = newDistanceMax;
}
void vtkMRMLProtonBeamsNode::accessJaw_x(double newJaw_x)
{
	m_jaw_x = newJaw_x;
}
void vtkMRMLProtonBeamsNode::accessJaw_y(double newJaw_y)
{
	m_jaw_y = newJaw_y;
}
void vtkMRMLProtonBeamsNode::accessCouchRotation(double newCouchRotation)
{
	m_couchRotation = newCouchRotation;
}
void vtkMRMLProtonBeamsNode::accessCouchPitch(double newCouchPitch)
{
	m_couchPitch = newCouchPitch;
}
void vtkMRMLProtonBeamsNode::accessCouchRoll(double newCouchRoll)
{
	m_couchRoll = newCouchRoll;
}

// Reader definitions

std::string vtkMRMLProtonBeamsNode::readBeamName()
{
	return m_beamName;
}
double vtkMRMLProtonBeamsNode::readIsocenter_x()
{
	return m_isocenter_x;
}
double vtkMRMLProtonBeamsNode::readIsocenter_y()
{
	return m_isocenter_y;
}
double vtkMRMLProtonBeamsNode::readIsocenter_z()
{
	return m_isocenter_z;
}
double vtkMRMLProtonBeamsNode::readGantryAngle()
{
	return m_gantryAngle;
}
double vtkMRMLProtonBeamsNode::readCollimatorAngle()
{
	return m_collimatorAngle;
}
double vtkMRMLProtonBeamsNode::readRange()
{
	return m_range;
}
double vtkMRMLProtonBeamsNode::readMod()
{
	return m_mod;
}
double vtkMRMLProtonBeamsNode::readDistanceMin()
{
	return m_distance_min;
}
double vtkMRMLProtonBeamsNode::readDistanceMax()
{
	return m_distance_max;
}
double vtkMRMLProtonBeamsNode::readJaw_x()
{
	return m_jaw_x;
}
double vtkMRMLProtonBeamsNode::readJaw_y()
{
	return m_jaw_y;
}
double vtkMRMLProtonBeamsNode::readCouchRotation()
{
	return m_couchRotation;
}
double vtkMRMLProtonBeamsNode::readCouchPitch()
{
	return m_couchPitch;
}
double vtkMRMLProtonBeamsNode::readCouchRoll()
{
	return m_couchRoll;
}
double vtkMRMLProtonBeamsNode::readSourceX()
{
	return m_sourceX;
}

double vtkMRMLProtonBeamsNode::readSourceY()
{
	return m_sourceY;
}

double vtkMRMLProtonBeamsNode::readSourceZ()
{
	return m_sourceZ;
}

// Source position calculation in the patient reference (no roll pitch or collimator)

void vtkMRMLProtonBeamsNode::calculateSourcePosition()
{
	double PI = 3.14159265359;
	m_sourceX = m_isocenter_x + m_DSA*sin(m_gantryAngle*PI/180);
	m_sourceY = m_isocenter_y - m_DSA*cos(m_gantryAngle*PI/180);
	m_sourceZ = m_isocenter_z;
}
