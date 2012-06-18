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

#ifndef __qSlicerGammaDoseComparisonModuleWidget_h
#define __qSlicerGammaDoseComparisonModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerGammaDoseComparisonModuleExport.h"

class qSlicerGammaDoseComparisonModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_GammaDoseComparison
class Q_SLICER_QTMODULES_GAMMADOSECOMPARISON_EXPORT qSlicerGammaDoseComparisonModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerGammaDoseComparisonModuleWidget(QWidget *parent=0);
  virtual ~qSlicerGammaDoseComparisonModuleWidget();

public slots:


protected:
  QScopedPointer<qSlicerGammaDoseComparisonModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerGammaDoseComparisonModuleWidget);
  Q_DISABLE_COPY(qSlicerGammaDoseComparisonModuleWidget);
};

#endif
