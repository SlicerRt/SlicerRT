/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __qSlicerDoseAccumulationModuleWidget_h
#define __qSlicerDoseAccumulationModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDoseAccumulationModuleExport.h"

class qSlicerDoseAccumulationModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_DoseAccumulation
class Q_SLICER_QTMODULES_DOSEACCUMULATION_EXPORT qSlicerDoseAccumulationModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDoseAccumulationModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDoseAccumulationModuleWidget();

public slots:


protected:
  QScopedPointer<qSlicerDoseAccumulationModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerDoseAccumulationModuleWidget);
  Q_DISABLE_COPY(qSlicerDoseAccumulationModuleWidget);
};

#endif
