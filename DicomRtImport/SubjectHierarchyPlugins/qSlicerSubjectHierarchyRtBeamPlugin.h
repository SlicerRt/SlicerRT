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

#ifndef __qSlicerSubjectHierarchyRtBeamPlugin_h
#define __qSlicerSubjectHierarchyRtBeamPlugin_h

// SlicerRt includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerDicomRtImportSubjectHierarchyPluginsExport.h"

class qSlicerSubjectHierarchyRtBeamPluginPrivate;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;

// TODO: Move this to External Beam Planning (or other) module when beam
//   and plan nodes are unified: #425 #553

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_QtModules_DicomRtImport
class Q_SLICER_DICOMRTIMPORT_SUBJECT_HIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyRtBeamPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyRtBeamPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyRtBeamPlugin();

public:
  /// Determines if the actual plugin can handle a subject hierarchy node. The plugin with
  /// the highest confidence number will "own" the node in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param node Note to handle in the subject hierarchy tree
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const;

  /// Get role that the plugin assigns to the subject hierarchy node.
  ///   Each plugin should provide only one role.
  Q_INVOKABLE virtual const QString roleForPlugin()const;

  /// Get icon of an owned subject hierarchy node
  /// \return Icon to set, NULL if nothing to set
  virtual QIcon icon(vtkMRMLSubjectHierarchyNode* node);

  /// Get visibility icon for a visibility state
  virtual QIcon visibilityIcon(int visible);

  /// Open module belonging to node and set inputs in opened module
  virtual void editProperties(vtkMRMLSubjectHierarchyNode* node);

protected:
  QScopedPointer<qSlicerSubjectHierarchyRtBeamPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyRtBeamPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyRtBeamPlugin);
};

//ETX

#endif
