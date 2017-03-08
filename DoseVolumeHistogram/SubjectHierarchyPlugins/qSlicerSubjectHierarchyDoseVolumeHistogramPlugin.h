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

#ifndef __qSlicerSubjectHierarchyDoseVolumeHistogramPlugin_h
#define __qSlicerSubjectHierarchyDoseVolumeHistogramPlugin_h

// SlicerRt includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerDoseVolumeHistogramSubjectHierarchyPluginsExport.h"

class qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;
class vtkMRMLDoseVolumeHistogramNode;

/// \ingroup SlicerRt_QtModules_RtHierarchy
class Q_SLICER_DOSEVOLUMEHISTOGRAM_SUBJECT_HIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyDoseVolumeHistogramPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyDoseVolumeHistogramPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyDoseVolumeHistogramPlugin();

public:
  /// Determines if the actual plugin can handle a subject hierarchy item. The plugin with
  /// the highest confidence number will "own" the item in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param item Item to handle in the subject hierarchy tree
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   item, and 1 means that the plugin is the only one that can handle the item (by node type or identifier attribute)
  virtual double canOwnSubjectHierarchyItem(vtkIdType itemID)const;

  /// Get role that the plugin assigns to the subject hierarchy item.
  ///   Each plugin should provide only one role.
  Q_INVOKABLE virtual const QString roleForPlugin()const;

  /// Get icon of an owned subject hierarchy item
  /// \return Icon to set, empty icon if nothing to set
  virtual QIcon icon(vtkIdType itemID);

  /// Get visibility icon for a visibility state
  Q_INVOKABLE virtual QIcon visibilityIcon(int visible);

  /// Open module belonging to item and set inputs in opened module
  Q_INVOKABLE virtual void editProperties(vtkIdType itemID);

  /// Set display visibility of a owned subject hierarchy item
  Q_INVOKABLE virtual void setDisplayVisibility(vtkIdType itemID, int visible);

  /// Get display visibility of a owned subject hierarchy item
  /// \return Display visibility (0: hidden, 1: shown, 2: partially shown)
  Q_INVOKABLE virtual int getDisplayVisibility(vtkIdType itemID)const;

protected:
  /// Utility function for getting DVH parameter set node for DVH double array node
  vtkMRMLDoseVolumeHistogramNode* getDvhParameterSetNodeForDvhArray(vtkMRMLNode* dvhArrayNode)const;

  /// Utility function for getting chart subject hierarchy item for DVH array subject hierarchy item
  vtkIdType getChartShItemForDvhArray(vtkIdType dvhArrayShItemID)const;

protected:
  QScopedPointer<qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin);
};

#endif
