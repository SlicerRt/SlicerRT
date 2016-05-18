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

#ifndef __qMRMLBeamsTableView_h
#define __qMRMLBeamsTableView_h

// Qt includes
#include <QWidget>

// MRMLWidgets includes
#include "qSlicerBeamsModuleWidgetsExport.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class qMRMLBeamsTableViewPrivate;
class vtkMRMLNode;
class QTableWidgetItem;
class QItemSelection;

/// \ingroup SlicerRt_QtModules_Beams_Widgets
class Q_SLICER_MODULE_BEAMS_WIDGETS_EXPORT qMRMLBeamsTableView : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT

  enum BeamTableItemDataRole
  {
    /// Unique ID of the item. For nodes, it is the node ID.
    IDRole = Qt::UserRole + 1
  };

public:
  /// Constructor
  explicit qMRMLBeamsTableView(QWidget* parent = 0);
  /// Destructor
  virtual ~qMRMLBeamsTableView();

  /// Get plan MRML node
  vtkMRMLNode* planNode();

  /// Return number of beams (rows) in the table
  int beamCount() const;

  /// Return selected beam node ID
  QString selectedBeamNodeID();

public slots:
  /// Set plan MRML node
  void setPlanNode(vtkMRMLNode* node);

signals:
  /// Emitted if selection changes
  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected slots:
  /// Handle changing of values in a cell
  void onBeamTableItemChanged(QTableWidgetItem* changedItem);

  /// Handle edit button click. Switches to Beams module and selects beam
  void onEditButtonClicked();

  /// Populate beam table according to the plan node
  void populateBeamTable();

  /// To prevent accidentally moving out of the widget when pressing up/down arrows
  virtual bool eventFilter(QObject* target, QEvent* event);

protected:
  QScopedPointer<qMRMLBeamsTableViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLBeamsTableView);
  Q_DISABLE_COPY(qMRMLBeamsTableView);
};

#endif
