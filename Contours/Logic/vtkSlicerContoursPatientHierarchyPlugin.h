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

#ifndef __vtkSlicerContoursPatientHierarchyPlugin_h
#define __vtkSlicerContoursPatientHierarchyPlugin_h

// SlicerRt includes
#include "vtkSlicerPatientHierarchyPlugin.h"

#include "vtkSlicerContoursModuleLogicExport.h"

class vtkMRMLNode;
class vtkMRMLHierarchyNode;
class vtkMRMLContourNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_QtModules_Contours
class VTK_SLICER_CONTOURS_LOGIC_EXPORT vtkSlicerContoursPatientHierarchyPlugin: public vtkSlicerPatientHierarchyPlugin
{
public:
  static vtkSlicerContoursPatientHierarchyPlugin *New();
  vtkTypeMacro(vtkSlicerContoursPatientHierarchyPlugin, vtkSlicerPatientHierarchyPlugin);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Determines if a non patient hierarchy node can be placed in the hierarchy, and gets a confidence
  ///   value for a certain MRML node (usually the type and possibly attributes are checked)
  /// \return Floating point number between 0 and 1, where 0 means that the plugin cannot
  ///   handle the node at all, and 1 means that the plugin was written exactly for this kind of node
  virtual double CanPluginAddNodeToPatientHierarchy(vtkMRMLNode* nodeToAdd);

  /// Add a node to patient hierarchy under a specified parent node. If added non patient hierarchy nodes
  ///   have certain steps to perform when adding them in Patient Hierarchy, those steps take place here
  /// \return True if added successfully, false otherwise
  virtual bool AddNodeToPatientHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLHierarchyNode* parentNode);

  /// Determines if a patient hierarchy node can be reparented in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// \return Floating point number between 0 and 1, where 0 means that the plugin cannot
  ///   handle the node at all, and 1 means that the plugin was written exactly for this kind of node.
  virtual double CanPluginReparentNodeInsidePatientHierarchy(vtkMRMLHierarchyNode* nodeToReparent);

  /// Reparent a node that was already in the patient hierarchy under a new parent.
  /// \return True if reparented successfully, false otherwise
  virtual bool ReparentNodeInsidePatientHierarchy(vtkMRMLHierarchyNode* nodeToReparent, vtkMRMLHierarchyNode* parentNode);

protected:
  /// Determines if the argument node is a representation object of a Contour node in the scene
  /// \return The found contour node whose representation the argument node is, NULL if node is not a representation
  vtkMRMLContourNode* IsNodeAContourRepresentation(vtkMRMLNode* node);

protected:
  vtkSlicerContoursPatientHierarchyPlugin();
  virtual ~vtkSlicerContoursPatientHierarchyPlugin();

private:
  vtkSlicerContoursPatientHierarchyPlugin(const vtkSlicerContoursPatientHierarchyPlugin&); // Not implemented
  void operator=(const vtkSlicerContoursPatientHierarchyPlugin&); // Not implemented  
};

//ETX

#endif
