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

#ifndef __vtkSlicerPatientHierarchyPluginHandler_h
#define __vtkSlicerPatientHierarchyPluginHandler_h

// VTK includes
#include <vtkObject.h>

// STD includes
#include <vector>

// SlicerRt includes
#include "vtkSlicerPatientHierarchyModulePluginsExport.h"

class vtkMRMLNode;
class vtkMRMLHierarchyNode;
class vtkSlicerPatientHierarchyPlugin;

/*!
  \class vtkSlicerPatientHierarchyPluginHandler 
  \brief Singleton class managing Patient Hierarchy plugins
  \ingroup SlicerRt_PatientHierarchy_Plugins
*/ 
class VTK_SLICER_PATIENTHIERARCHY_PLUGINS_EXPORT vtkSlicerPatientHierarchyPluginHandler: public vtkObject
{
public:
  static vtkSlicerPatientHierarchyPluginHandler *New();
  vtkTypeMacro(vtkSlicerPatientHierarchyPluginHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  typedef std::vector<vtkSlicerPatientHierarchyPlugin*> PatientHierarchyPluginListType;

  /// Instance getter for the singleton class
  /// \return Instance object
  static vtkSlicerPatientHierarchyPluginHandler* GetInstance();

  /// Allows cleanup of the singleton at application exit
  static void SetInstance(vtkSlicerPatientHierarchyPluginHandler* instance);

public:
  /// Register a plugin
  /// \return True if plugin registered successfully, false otherwise
  bool RegisterPlugin(vtkSlicerPatientHierarchyPlugin* plugin);

  /// Returns the registered plugin that can handle a node the best
  /// for adding it from outside the patient hierarchy to inside it
  /// \param node Node to be added to the hierarchy
  /// \param parent Prospective parent of the node to add.
  ///        Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return The plugin object if found, NULL otherwise
  vtkSlicerPatientHierarchyPlugin* GetPluginForAddToPatientHierarchyForNode(vtkMRMLNode* node, vtkMRMLHierarchyNode* parent=NULL);

  /// Returns the registered plugin that can handle a node the best
  /// for reparenting it inside the patient hierarchy
  /// \param node Node to be reparented in the hierarchy
  /// \param parent Prospective parent of the node to reparent.
  ///        Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return The plugin object if found, NULL otherwise
  vtkSlicerPatientHierarchyPlugin* GetPluginForReparentInsidePatientHierarchyForNode(vtkMRMLHierarchyNode* node, vtkMRMLHierarchyNode* parent=NULL);

protected:
  /// TODO
  PatientHierarchyPluginListType PluginList;
  
protected:
  vtkSlicerPatientHierarchyPluginHandler();
  virtual ~vtkSlicerPatientHierarchyPluginHandler();

private:
  vtkSlicerPatientHierarchyPluginHandler(const vtkSlicerPatientHierarchyPluginHandler&); // Not implemented
  void operator=(const vtkSlicerPatientHierarchyPluginHandler&); // Not implemented  

private:
  /// Instance of the singleton
  static vtkSlicerPatientHierarchyPluginHandler* Instance;
};

#endif
