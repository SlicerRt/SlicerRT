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

#ifndef __qSlicerSubjectHierarchyVolumesPlugin_h
#define __qSlicerSubjectHierarchyVolumesPlugin_h

// SlicerRt includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyModulePluginsExport.h"

class qSlicerSubjectHierarchyVolumesPluginPrivate;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSubjectHierarchyNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class Q_SLICER_SUBJECTHIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyVolumesPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyVolumesPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyVolumesPlugin();

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

  /// Set display visibility of a owned subject hierarchy node
  virtual void setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible);

  /// Get display visibility of a owned subject hierarchy node
  /// \return Display visibility (0: hidden, 1: shown, 2: partially shown)
  virtual int getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node);

  /// Get node context menu item actions to add to tree view
  virtual QList<QAction*> nodeContextMenuActions()const;

  /// Hide all context menu actions
  virtual void hideAllContextMenuActions();

  /// Show context menu actions valid for handling a given subject hierarchy node.
  /// "Handling" includes features that are applied to the node (e.g. transform, convert, etc.)
  /// This function is only called for a node's owner plugin and its dependent plugins.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForHandlingNode(vtkMRMLSubjectHierarchyNode* node);

protected:
  /// Show volume in slice viewers. The argument node becomes the background, and the previous
  /// background becomes the foreground with 50% transparency.
  void showVolume(vtkMRMLScalarVolumeNode* node, int visible=1);

  /// Update selection node based on current volumes visibility (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  void updateSelectionNodeBasedOnCurrentVolumesVisibility();
  /// Determine labelmap selection (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string getSelectedLabelmapVolumeNodeID();
  /// Determine background volume selection (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string getSelectedBackgroundVolumeNodeID();
  /// Determine foreground volume selection (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string getSelectedForegroundVolumeNodeID();

protected slots:
  /// Toggle between labelmap outline display in the slice views
  void toggleLabelmapOutlineDisplay(bool checked);

protected:
  QScopedPointer<qSlicerSubjectHierarchyVolumesPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyVolumesPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyVolumesPlugin);
};

//ETX

#endif
