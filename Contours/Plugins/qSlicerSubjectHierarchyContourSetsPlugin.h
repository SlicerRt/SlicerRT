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

#ifndef __qSlicerSubjectHierarchyContourSetsPlugin_h
#define __qSlicerSubjectHierarchyContourSetsPlugin_h

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerContoursModulePluginsExport.h"

class qSlicerSubjectHierarchyContourSetsPluginPrivate;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;
class vtkMRMLContourNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_QtModules_Contours
class Q_SLICER_CONTOURS_PLUGINS_EXPORT qSlicerSubjectHierarchyContourSetsPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyContourSetsPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyContourSetsPlugin();

public:
  /// Determines if a non subject hierarchy node can be placed in the hierarchy, and gets a confidence
  ///   value for a certain MRML node (usually the type and possibly attributes are checked)
  /// \param node Node to be added to the hierarchy
  /// \param parent Prospective parent of the node to add.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by type or identifier attribute)
  virtual double canAddNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parent=NULL);

  /// Add a node to subject hierarchy under a specified parent node. If added non subject hierarchy nodes
  ///   have certain steps to perform when adding them in subject hierarchy, those steps take place here
  /// \return True if added successfully, false otherwise
  virtual bool addNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parentNode);

  /// Determines if a subject hierarchy node can be reparented in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// \param node Node to be reparented in the hierarchy
  /// \param parent Prospective parent of the node to reparent.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by type or identifier attribute)
  virtual double canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parent);

  /// Reparent a node that was already in the subject hierarchy under a new parent.
  /// \return True if reparented successfully, false otherwise
  virtual bool reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parentNode);

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

protected:
  /// Determine if the argument node is a representation object of a Contour node in the scene
  /// \return The found contour node whose representation the argument node is, NULL if node is not a representation
  vtkMRMLContourNode* isNodeAContourRepresentation(vtkMRMLNode* node);

  /// Add the color of a contour to the corresponding color table (in the same contour set)
  /// Also repaint the occasional labelmap representation to the new color index
  bool addContourColorToCorrespondingColorTable(vtkMRMLContourNode* contourNode, QString colorName);

protected slots:
  /// Create contour set node under the current node
  void createChildContourSetForCurrentNode();

protected:
  QScopedPointer<qSlicerSubjectHierarchyContourSetsPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyContourSetsPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyContourSetsPlugin);
};

//ETX

#endif
