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

#ifndef __qMRMLBeamParametersTabWidget_h
#define __qMRMLBeamParametersTabWidget_h

// Beams includes
#include "qSlicerBeamsModuleWidgetsExport.h"

// Qt includes
#include <QTabWidget>

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class vtkMRMLNode;
class qMRMLBeamParametersTabWidgetPrivate;

/// \ingroup SlicerRt_QtModules_Beams_Widgets
class Q_SLICER_MODULE_BEAMS_WIDGETS_EXPORT qMRMLBeamParametersTabWidget : public QTabWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef QTabWidget Superclass;
  /// Constructor
  explicit qMRMLBeamParametersTabWidget(QWidget* parent = 0);
  /// Destructor
  virtual ~qMRMLBeamParametersTabWidget();

  /// Get RT beam MRML node
  Q_INVOKABLE vtkMRMLNode* beamNode();

public slots:
  /// Set RT beam MRML node
  void setBeamNode(vtkMRMLNode* node);

signals:

protected slots:
  /// Update from beam node state
  void updateWidgetFromMRML();

  // Geometry page
  void mlcPositionDoubleArrayNodeChanged(vtkMRMLNode* node);
  void sourceDistanceChanged(double);
  void xJawsPositionValuesChanged(double, double);
  void yJawsPositionValuesChanged(double, double);
  void collimatorAngleChanged(double);
  void gantryAngleChanged(double);
  void couchAngleChanged(double);

  // Visualization page
  void updateDRRClicked();
  void beamEyesViewClicked(bool);
  void contoursInBEWClicked(bool);

protected:
  QScopedPointer<qMRMLBeamParametersTabWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLBeamParametersTabWidget);
  Q_DISABLE_COPY(qMRMLBeamParametersTabWidget);
};

#endif 