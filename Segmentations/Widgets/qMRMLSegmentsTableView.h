/*==============================================================================

  Program: 3D Slicer

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

#ifndef __qMRMLSegmentsTableView_h
#define __qMRMLSegmentsTableView_h

// Qt includes
#include <QWidget>

// MRMLWidgets includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class vtkMRMLNode;
class qMRMLSegmentsTableViewPrivate;
class QTableWidgetItem;
class QItemSelection;

/// \ingroup SlicerRt_QtModules_Segmentations_Widgets
class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qMRMLSegmentsTableView : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT

  enum SegmentTableItemDataRole
    {
    /// Unique ID of the item. For nodes, it is the node ID.
    IDRole = Qt::UserRole + 1,
    /// Integer that contains the visibility property of a node.
    /// It is closely related to the item icon.
    VisibilityRole,
    };

public:
  enum SegmentTableMode
    {
    /// Show visibility options and allow multiple selections
    VisibilityOptionsMode = 0,
    /// Only show segment name and allow multiple selection, no header is visible
    SimpleListMode,
    /// Only show name, do not allow selection, no header is visible
    RepresentationMode
    };

  Q_PROPERTY(int mode READ mode WRITE setMode)

public:
  /// Constructor
  explicit qMRMLSegmentsTableView(QWidget* parent = 0);
  /// Destructor
  virtual ~qMRMLSegmentsTableView();

  /// Get segmentation MRML node
  vtkMRMLNode* segmentationNode();

  /// Get representation MRML node (model or labelmap volume MRML node for import/export)
  vtkMRMLNode* representationNode();

  /// Return number of segments (rows) in the table
  int segmentCount() const;

  /// Get mode
  int mode()const;
  /// Set mode of segment table. There are two modes:
  /// 1. VisibilityOptionsMode: Not selectable table with visibility options
  /// 2. SelectableSimpleListMode: Selectable list with only segment names, no header is visible
  void setMode(SegmentTableMode mode);
  /// Set segment table mode. Python compatibility function.
  void setMode(int mode);

  /// Get segment ID of selected segments
  Q_INVOKABLE QStringList selectedSegmentIDs();
  /// Get segment ID of selected segments
  Q_INVOKABLE void setSelectedSegmentIDs(QStringList segmentIDs);

public slots:
  /// Set segmentation MRML node
  void setSegmentationNode(vtkMRMLNode* node);

  /// Set representation MRML node (model or labelmap volume MRML node for import/export)
  void setRepresentationNode(vtkMRMLNode* node);

signals:
  /// Emitted if selection changes
  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected slots:
  /// Handles changing of values in a cell (segment name, visibility, color, opacity)
  void onSegmentTableItemChanged(QTableWidgetItem* changedItem);

  /// Handles clicks on a table cell (visibility)
  void onSegmentTableItemClicked(QTableWidgetItem* item);

  /// Populate segment table according to the segmentation node
  void populateSegmentTable();

protected:
  QScopedPointer<qMRMLSegmentsTableViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSegmentsTableView);
  Q_DISABLE_COPY(qMRMLSegmentsTableView);
};

#endif
