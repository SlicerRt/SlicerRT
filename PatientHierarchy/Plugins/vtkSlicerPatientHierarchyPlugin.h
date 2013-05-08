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

#ifndef __vtkSlicerPatientHierarchyPlugin_h
#define __vtkSlicerPatientHierarchyPlugin_h

// VTK includes
#include <vtkObject.h>

// SlicerRt includes
#include "vtkSlicerPatientHierarchyModulePluginsExport.h"

class vtkMRMLNode;
class vtkMRMLHierarchyNode;

/// \ingroup SlicerRt_PatientHierarchy_Plugins
class VTK_SLICER_PATIENTHIERARCHY_PLUGINS_EXPORT vtkSlicerPatientHierarchyPlugin: public vtkObject
{
public:
  vtkTypeMacro(vtkSlicerPatientHierarchyPlugin, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) { this->Superclass::PrintSelf(os, indent); };

public:
  /// Determines if a non patient hierarchy node can be placed in the hierarchy, and gets a confidence
  ///   value for a certain MRML node (usually the type and possibly attributes are checked)
  /// \return Floating point number between 0 and 1, where 0 means that the plugin cannot
  ///   handle the node at all, and 1 means that the plugin was written exactly for this kind of node
  virtual double CanPluginAddNodeToPatientHierarchy(vtkMRMLNode*) = 0;

  /// Add a node to patient hierarchy under a specified parent node. If added non patient hierarchy nodes
  ///   have certain steps to perform when adding them in Patient Hierarchy, those steps take place here
  /// \return True if added successfully, false otherwise
  virtual bool AddNodeToPatientHierarchy(vtkMRMLNode*, vtkMRMLHierarchyNode*) = 0;

public:
  /// Get plugin name
  vtkGetStringMacro(Name);

protected:
  /// Set plugin name
  vtkSetStringMacro(Name);

protected:
  /// Name of the plugin
  char* Name;

protected:
  vtkSlicerPatientHierarchyPlugin() { this->Name = NULL; };
  virtual ~vtkSlicerPatientHierarchyPlugin() { this->SetName(NULL); };

private:
  vtkSlicerPatientHierarchyPlugin(const vtkSlicerPatientHierarchyPlugin&); // Not implemented
  void operator=(const vtkSlicerPatientHierarchyPlugin&); // Not implemented  
};

#endif
