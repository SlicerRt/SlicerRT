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
    /// Only show name and allow multiple selection, no header is visible
    SimpleListMode,
    /// Only show name, do not allow selection, no header is visible
    RepresentationMode,
    /// Show name and color, allow single selection
    EditorMode
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
  /// 3. RepresentationMode: Not selectable list with only names, no header is visible
  /// 4. EditorMode: Table with single selection, showing name and color
  void setMode(SegmentTableMode mode);
  /// Set segment table mode. Python compatibility function.
  void setMode(int mode);

  /// Get segment ID of selected segments
  Q_INVOKABLE QStringList selectedSegmentIDs();
  /// Select segments with specified IDs
  Q_INVOKABLE void setSelectedSegmentIDs(QStringList segmentIDs);
  /// Clear segment selection
  Q_INVOKABLE void clearSelection();

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
  void onVisibilityButtonClicked();
  void onVisibility3DActionToggled(bool visible);
  void onVisibility2DFillActionToggled(bool visible);
  void onVisibility2DOutlineActionToggled(bool visible);

  /// Populate segment table according to the segmentation node
  void populateSegmentTable();

  /// Update from segmentation node state (invoked when segment count stays the same)
  void updateWidgetFromMRML();

protected:
  /// Convenience function to set segment visibility options from event handlers
  /// \sa onVisibilityButtonToggled \sa onVisibility3DActionToggled \sa onVisibility2DFillActionToggled \sa onVisibility2DOutlineActionToggled
  /// \param visible Visibility of the segment referenced from senderObject. If 0, then hide, if 1 then show, otherwise don't change
  /// \param visible3D Visibility of the segment referenced from senderObject in 3D. If 0, then hide, if 1 then show, otherwise don't change
  /// \param visible2DFill Visibility of the segment referenced from senderObject for 2D fill. If 0, then hide, if 1 then show, otherwise don't change
  /// \param visible2DOutline Visibility of the segment referenced from senderObject for 2D outline. If 0, then hide, if 1 then show, otherwise don't change
  void setSegmentVisibility(QObject* senderObject, int visible, int visible3D, int visible2DFill, int visible2DOutline);

  /// To prevent accidentally moving out of the widget when pressing up/down arrows
  virtual bool eventFilter(QObject* target, QEvent* event);

protected:
  QScopedPointer<qMRMLSegmentsTableViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSegmentsTableView);
  Q_DISABLE_COPY(qMRMLSegmentsTableView);
};

#endif
