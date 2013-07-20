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
class vtkMRMLScene;

/// \ingroup Slicer_QtModules_Contours
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
  /// the required representation can be got without having to do anything else
  vtkMRMLContourNode* selectedContourNode();

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
