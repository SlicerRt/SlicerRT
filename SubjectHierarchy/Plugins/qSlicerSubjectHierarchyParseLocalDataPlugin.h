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

#ifndef __qSlicerSubjectHierarchyParseLocalDataPlugin_h
#define __qSlicerSubjectHierarchyParseLocalDataPlugin_h

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyModulePluginsExport.h"

class qSlicerSubjectHierarchyParseLocalDataPluginPrivate;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class Q_SLICER_SUBJECTHIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyParseLocalDataPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyParseLocalDataPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyParseLocalDataPlugin();

public:
  /// Determines if a non subject hierarchy node can be placed in the hierarchy, and gets a confidence
  ///   value for a certain MRML node (usually the type and possibly attributes are checked)
  /// \param node Node to be added to the hierarchy
  /// \param parent Prospective parent of the node to add.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by type or identifier attribute)
  virtual double canAddNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parent=NULL);

  /// Determines if a subject hierarchy node can be reparented in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// \param node Node to be reparented in the hierarchy
  /// \param parent Prospective parent of the node to reparent.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by type or identifier attribute)
  virtual double canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parent);

  /// Determines if the actual plugin can handle a subject hierarchy node. The plugin with
  /// the highest confidence number will "own" the node in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param node Note to handle in the subject hierarchy tree
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by type or identifier attribute)
  virtual double canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node);

  /// Set icon of a owned subject hierarchy node
  /// \return Flag indicating whether setting an icon was successful
  virtual bool setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item);

  /// Set visibility icon of a owned subject hierarchy node
  virtual void setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item);

  /// Get scene context menu item actions to add to tree view
  /// Separate method is needed for the scene, as its actions are set to the
  /// tree by a different method \sa nodeContextMenuActions
  virtual QList<QAction*> sceneContextMenuActions()const;

  /// Hide all context menu actions
  virtual void hideAllContextMenuActions();

  /// Get the list of plugin dependencies
  virtual QStringList dependencies()const;

  /// Show context menu actions valid for creating a child for a given subject hierarchy node.
  /// This function is called for all plugins, not just a node's owner plugin and its dependents,
  /// because it's not the node's ownership that determines what kind of children can be created
  /// to it, but the properties (level etc.) of the node
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForCreatingChildForNode(vtkMRMLSubjectHierarchyNode* node);

protected slots:
  /// Create subject hierarchy from loaded loacl directories.
  /// Adds all nodes in the potential subject hierarchy nodes list that is storable and has a valid storage node with a file
  /// name (meaning it has been loaded from local disk). Creates patient/study/series/subseries hierarchies according to the
  /// paths of the loaded files, ignoring the part that is identical (if everything has been loaded from the same directory,
  /// then only creates subject hierarchy nodes for the directories within that directory).
  void createHierarchyFromLoadedLocalDirectories();

protected:
  QScopedPointer<qSlicerSubjectHierarchyParseLocalDataPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyParseLocalDataPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyParseLocalDataPlugin);
};

//ETX

#endif
