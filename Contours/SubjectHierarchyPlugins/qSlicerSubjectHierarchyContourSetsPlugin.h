/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

#include "qSlicerContoursSubjectHierarchyPluginsExport.h"

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
  virtual double canAddNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parent=NULL)const;

  /// Determines if a subject hierarchy node can be reparented in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// \param node Node to be reparented in the hierarchy
  /// \param parent Prospective parent of the node to reparent.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by type or identifier attribute)
  virtual double canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parent)const;

  /// Reparent a node that was already in the subject hierarchy under a new parent.
  /// \return True if reparented successfully, false otherwise
  virtual bool reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parentNode);

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

  /// Get help text for plugin to be added in subject hierarchy module widget help box
  virtual const QString helpText()const;

  /// Get icon of an owned subject hierarchy node
  /// \return Icon to set, NULL if nothing to set
  virtual QIcon icon(vtkMRMLSubjectHierarchyNode* node);

  /// Get visibility icon for a visibility state
  virtual QIcon visibilityIcon(int visible);

  /// Open module belonging to node and set inputs in opened module
  virtual void editProperties(vtkMRMLSubjectHierarchyNode* node);

  /// Get node context menu item actions to add to tree view
  Q_INVOKABLE virtual QList<QAction*> nodeContextMenuActions()const;

  /// Show context menu actions valid for  given subject hierarchy node.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node);

protected:
  /// Add the color of a contour to the corresponding color table (in the same contour set)
  /// Also repaint the occasional labelmap representation to the new color index
  bool addContourColorToCorrespondingColorTable(vtkMRMLContourNode* contourNode, QString colorName);

signals:
  /// Emitted when the user wants to create a contour from a representation
  /// \param Contour set node to use as group for new contour
  void createContourFromRepresentationClicked(QString);

protected slots:
  /// Create contour set node under the current node
  void createChildContourSetForCurrentNode();

  /// Create contour set node under the current node
  void convertRepresentationAction();

  /// Jump to Colors module to edit the color table belonging to the current contour set node
  void onEditColorTable();

protected:
  QScopedPointer<qSlicerSubjectHierarchyContourSetsPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyContourSetsPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyContourSetsPlugin);
};

//ETX

#endif
