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

#ifndef __qMRMLContourSelectorWidget_h
#define __qMRMLContourSelectorWidget_h

// MRMLWidgets includes
#include "qMRMLWidget.h"

// Contours includes
#include "vtkMRMLContourNode.h"
#include "qSlicerContoursModuleWidgetsExport.h"

class qMRMLContourSelectorWidgetPrivate;

/// \brief Widget for selecting contours as inputs. Automatically handles conversion to labelmap.
/// \ingroup Slicer_QtModules_Contours
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
  vtkMRMLContourNode* selectedContourNode();

  /// Get selected contour node list. If this gives a non-empty list, then it is sure that
  /// the required representation can be got from all contours without problems
  std::vector<vtkMRMLContourNode*> selectedContourNodes();

protected:
  /// Update widget state according to selection and set widget properties
  void updateWidgetState();

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
