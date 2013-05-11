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

#ifndef __vtkMRMLProtonBeamsNode_h
#define __vtkMRMLProtonBeamsNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerProtonBeamsModuleLogicExport.h"

class VTK_SLICER_PROTONBEAMS_LOGIC_EXPORT vtkMRMLProtonBeamsNode : public vtkMRMLNode
{
public:
  static vtkMRMLProtonBeamsNode *New();
  vtkTypeMacro(vtkMRMLProtonBeamsNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "ProtonBeams";};

  /// Set and observe isocenter fiducial MRML Id 
  void SetAndObserveIsocenterFiducialNodeId(const char* id);

  /// Set and observe souce fiducial MRML Id 
  void SetAndObserveSourceFiducialNodeId(const char* id);

  /// Set and observe output beam model MRML Id 
  void SetAndObserveBeamModelNodeId(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

public:
  /// Get isocenter fiducial MRML Id 
  vtkGetStringMacro(IsocenterFiducialNodeId);

  /// Get source fiducial MRML Id 
  vtkGetStringMacro(SourceFiducialNodeId);

  /// Get output beam model MRML Id 
  vtkGetStringMacro(BeamModelNodeId);

  /// Set beams model opacity
  vtkSetMacro(BeamModelOpacity,double);
  /// Get beams model opacity
  vtkGetMacro(BeamModelOpacity,double);

  // accessor of the beam parameters
	void accessBeamName(std::string);
	void accessIsocenter_x(double);
	void accessIsocenter_y(double);
	void accessIsocenter_z(double);
	void accessGantryAngle(double);
	void accessCollimatorAngle(double);
	void accessRange(double);
	void accessMod(double);
	void accessDistanceMin(double);
	void accessDistanceMax(double);
	void accessJaw_x(double);
	void accessJaw_y(double);
	void accessCouchRotation(double);
	void accessCouchPitch(double);
	void accessCouchRoll(double);


	std::string readBeamName(); // reader of the parameters
	double readIsocenter_x();
	double readIsocenter_y();
	double readIsocenter_z();
	double readGantryAngle();
	double readCollimatorAngle();
	double readRange();
	double readMod();
	double readDistanceMin();
	double readDistanceMax();
	double readJaw_x();
	double readJaw_y();
	double readCouchRotation();
	double readCouchPitch();
	double readCouchRoll();
	double readSourceX();
	double readSourceY();
	double readSourceZ();

	void calculateSourcePosition(); // function to calculate the source position in the patient reference

  vtkMRMLProtonBeamsNode();
  ~vtkMRMLProtonBeamsNode();
  vtkMRMLProtonBeamsNode(const vtkMRMLProtonBeamsNode&);
  void operator=(const vtkMRMLProtonBeamsNode&);

protected:
  /// Set isocenter fiducial MRML Id 
  vtkSetStringMacro(IsocenterFiducialNodeId);

  /// Set source fiducial MRML Id 
  vtkSetStringMacro(SourceFiducialNodeId);

  /// Set output beam model MRML Id 
  vtkSetStringMacro(BeamModelNodeId);

protected:
  /// ID of the input isocenter fiducial node
  char* IsocenterFiducialNodeId;

  /// ID of the input source fiducial node
  char* SourceFiducialNodeId;

  /// ID of the output beam model node
  char* BeamModelNodeId;

  /// Opacity of the created beam model
  double BeamModelOpacity;

private:

  std::string m_beamName;
  double m_isocenter_x, m_isocenter_y, m_isocenter_z; // isocenter
  double m_gantryAngle, m_collimatorAngle; // rotations of the gantry
  double m_range, m_mod; // energy parameters of the beam
  double m_distance_min, m_distance_max;
  double m_jaw_x, m_jaw_y; // collimator parameters
  double m_couchRotation, m_couchPitch, m_couchRoll; // rotations of the couch
  double m_sourceX, m_sourceY, m_sourceZ; // x,y,z of the couch (DICOM)
  double m_DSA; // Distance Source Axis

};

#endif
