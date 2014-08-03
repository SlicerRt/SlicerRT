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

#ifndef __qSlicerSubjectHierarchyContoursPlugin_h
#define __qSlicerSubjectHierarchyContoursPlugin_h

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerContoursSubjectHierarchyPluginsExport.h"

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

protected slots:
  /// Create supported child for the current node (selected in the tree)
  void createChildContourForCurrentNode();

  /// Delete current (selected) contour and add its representation to the scene
  void convertCurrentNodeContourToRepresentation();

  /// Switch to Colors module with the corresponding color table selected
  void changeColorForCurrentNode();

  /// Called when a node's owner plugin has changed
  /// Note: Also called when this plugin was the owner from which it has been changed!
  void onNodeClaimed(vtkObject* node, void* callData);

  /// Hide/show the ribbon model
  void hideShowRibbonModel();

  /// Hide/show the labelmap
  void hideShowLabelmap();

  /// Hide/show the closed surface model
  void hideShowClosedSurfaceModel();

  /// Create the ribbon model representation
  void createRibbonModelRepresentation();

  /// Create the labelmap representation
  void createLabelmapRepresentation();

  /// Create the closed surface model representation
  void createClosedSurfaceModelRepresentation();

  /// Extract a labelmap from a contour
  void extractLabelmapFromContour();

protected:
  QScopedPointer<qSlicerSubjectHierarchyContoursPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyContoursPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyContoursPlugin);
};

//ETX

#endif
