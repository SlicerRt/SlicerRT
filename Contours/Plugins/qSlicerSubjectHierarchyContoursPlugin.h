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

#ifndef __qSlicerSubjectHierarchyContoursPlugin_h
#define __qSlicerSubjectHierarchyContoursPlugin_h

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerContoursModulePluginsExport.h"

class qSlicerSubjectHierarchyContoursPluginPrivate;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;
class vtkMRMLContourNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_QtModules_Contours
class Q_SLICER_CONTOURS_PLUGINS_EXPORT qSlicerSubjectHierarchyContoursPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyContoursPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyContoursPlugin();

public:
  /// Determines if the actual plugin can handle a subject hierarchy node. The plugin with
  /// the highest confidence number will "own" the node in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param node Note to handle in the subject hierarchy tree
  /// \param role Output argument for the role that the plugin assigns to the subject hierarchy node
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node, QString &role=QString());

  /// Set icon of a owned subject hierarchy node
  /// \return Flag indicating whether setting an icon was successful
  virtual bool setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item);

  /// Set visibility icon of a owned subject hierarchy node
  virtual void setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item);

  /// Get node context menu item actions to add to tree view
  virtual QList<QAction*> nodeContextMenuActions()const;

  /// Hide all context menu actions
  virtual void hideAllContextMenuActions();

  /// Show context menu actions valid for handling a given subject hierarchy node.
  /// "Handling" includes features that are applied to the node (e.g. transform, convert, etc.)
  /// This function is only called for a node's owner plugin and its dependent plugins.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForHandlingNode(vtkMRMLSubjectHierarchyNode* node);

  /// Show context menu actions valid for creating a child for a given subject hierarchy node.
  /// This function is called for all plugins, not just a node's owner plugin and its dependents,
  /// because it's not the node's ownership that determines what kind of children can be created
  /// to it, but the properties (level etc.) of the node
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForCreatingChildForNode(vtkMRMLSubjectHierarchyNode* node);

  /// Get the list of plugin dependencies
  virtual QStringList dependencies()const;

protected slots:
  /// Create supported child for the current node (selected in the tree)
  void createChildContourForCurrentNode();

  /// Delete current (selected) contour and add its representation to the scene
  void convertCurrentNodeContourToRepresentation();

  /// Called when a node's owner plugin has changed
  /// Note: Also called when this plugin was the owner from which it has been changed!
  void onNodeClaimed(vtkObject* node, void* callData);

protected:
  QScopedPointer<qSlicerSubjectHierarchyContoursPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyContoursPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyContoursPlugin);
};

//ETX

#endif
