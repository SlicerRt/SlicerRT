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

#ifndef __qMRMLContourSelectorWidget_h
#define __qMRMLContourSelectorWidget_h

// MRMLWidgets includes
#include "qMRMLWidget.h"

// Contours includes
#include "vtkMRMLContourNode.h"
#include "qSlicerContoursModuleWidgetsExport.h"

class qMRMLContourSelectorWidgetPrivate;

/// \ingroup SlicerRt_QtModules_Contours
/// \brief Widget for selecting contours as inputs. Automatically handles conversion to labelmap.
/// 
/// This widget consolidates the way contours are selected in modules as inputs. As in most cases
/// the indexed labelmap representation is requested, a reference volume selector is included
/// with the referenced volume (from DICOM) selected by default.
/// Note: The oversampling factor is 1 when using the default selection (the referenced volume)
///       Otherwise, the oversampling factor is 2, which means every voxel is divided to 8 voxels
class Q_SLICER_MODULE_CONTOURS_WIDGETS_EXPORT qMRMLContourSelectorWidget : public qMRMLWidget
{
  Q_OBJECT

public:
  typedef qMRMLWidget Superclass;
  qMRMLContourSelectorWidget(QWidget *parent=0);
  virtual ~qMRMLContourSelectorWidget();

  /// Set representation type required from the selected contour (e.g. if the used
  /// algorithm requires a labelmap, then IndexedLabelmap has to be set here)
  void setRequiredRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType);

  /// Get required representation type
  vtkMRMLContourNode::ContourRepresentationType requiredRepresentation();

  /// Set flag determining whether contour hierarchies are accepted or only individual contours
  void setAcceptContourHierarchies(bool acceptContourHierarchies);

  /// Get flag determining whether contour hierarchies are accepted or only individual contours
  bool acceptContourHierarchies();

  /// Get selected contour node. If this gives a valid contour node, then it is sure that
  /// the required representation can be got without problems
  /// \return The selected contour node if valid, NULL otherwise. Also returns NULL if 
  ///         hierarchies are accepted, in this case a warning is logged too.
  Q_INVOKABLE vtkMRMLContourNode* selectedContourNode();

  /// Get selected contour node list. If this gives a non-empty list, then it is sure that
  /// the required representation can be got from all contours without problems
  Q_INVOKABLE QList<vtkMRMLContourNode*> selectedContourNodes();

  /// Returns currently selected contour or contour hierarchy node
  Q_INVOKABLE vtkMRMLNode* currentNode();
  /// Returns currently selected contour or contour hierarchy node's ID
  Q_INVOKABLE QString currentNodeID();

  /// Set currently selected contour or contour hierarchy node by its ID
  Q_INVOKABLE void setCurrentNodeID(const QString& nodeID);

  /// Set currently selected contour or contour hierarchy node by pointer
  Q_INVOKABLE void setCurrentNode(vtkMRMLNode* newCurrentNode);

  /// Update widget state according to selection and set widget properties
  void updateWidgetState();

  /// Set forced reference volume node by ID.
  /// This means that there will be no search for a DICOM-based referenced volume, this will be used instead.
  /// Furthermore if an indexed labelmap has been created from a model representation using a different reference
  /// volume, it will be invalidated (the contour dose this when overriding the reference volume)
  void setForcedReferenceVolumeNodeID(const QString& nodeID);

  /// Get forced reference volume node by ID \sa setForcedReferenceVolumeNodeID
  QString forcedReferenceVolumeNodeID();

  /// Set if re-rasterization is enabled
  void setRerasterizationSupported(bool rerasterizationSupported);

  /// Get if re-rasterization is enabled
  bool getRerasterizationSupported();

  /// Add slave contour selector widget. This operation makes this instance a master instance and sets this instance's
  /// pointer as the master instance in the new slave object.
  void addSlaveContourSelectorWidget(qMRMLContourSelectorWidget* newSlaveInstance);

  /// Ungroup the grouped widget instances (empties the slave widget list for master and removes master widget pointers from slaves)
  void ungroup();

  /// Validate selection (sets IsSelectionValid flag) and update widgets
  bool validateSelection(QList<vtkMRMLContourNode*>& contours, bool slave);

  /// Returns true if selection is valid (the required representations can be got), false otherwise
  /// Takes occasional slave instances into account
  bool isSelectionValid();

  /// Programatically set the currently selected reference volume
  void setCurrentReferenceVolumeNodeID(const QString& nodeID);

  /// Get the currently selected reference volume node
  vtkMRMLNode* currentReferenceVolumeNode();
  /// Get the currently selected reference volume node's ID
  QString currentReferenceVolumeNodeID();

signals:
  /// Emitted if the currently selected contour or contour hierarchy node changed
  void currentNodeChanged(vtkMRMLNode*);

  /// Emitted if the currently selected reference volume node changed
  void currentReferenceVolumeNodeChanged(vtkMRMLNode*);

  /// Emitted if validity of the selection changed, to notify the parent widgets so that they can enable/disable their related widgets (such as the Apply button)
  void selectionValidityChanged();

protected:
  /// Find reference volume (if there is a forced reference, or otherwise if it is defined in DICOM) and set it to the given contours
  /// The forced reference of this instance is used, because this is called only from the master (moreover the forced reference
  /// is the same throughout the group).
  /// \param contours List of contours to search in. It should be either the only instance's selection or the unified selection of the group
  void setReferenceInSelection(QList<vtkMRMLContourNode*> contours);

  /// Set master instance pointer to indicate that this instance is a slave to that one
  /// This function is only called by the master instance
  void setMasterContourSelectorWidget(qMRMLContourSelectorWidget* masterInstance);

public slots:
  /// Set the MRML \a scene associated with the widget (overridden method that cleans up widget content)
  virtual void setMRMLScene(vtkMRMLScene* newScene);

protected slots:
  /// Handle change of selected contour node
  void contourNodeChanged(vtkMRMLNode* node);

  /// Handle change of selected reference volume node
  void referenceVolumeNodeChanged(vtkMRMLNode* node);

protected:
  QScopedPointer<qMRMLContourSelectorWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLContourSelectorWidget);
  Q_DISABLE_COPY(qMRMLContourSelectorWidget);
};

#endif
