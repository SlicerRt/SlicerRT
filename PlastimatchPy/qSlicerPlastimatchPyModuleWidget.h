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

#ifndef __qSlicerPlastimatchPyModuleWidget_h
#define __qSlicerPlastimatchPyModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerPlastimatchPyModuleExport.h"

class qSlicerPlastimatchPyModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_PlastimatchPy
class Q_SLICER_QTMODULES_PLASTIMATCHPY_EXPORT qSlicerPlastimatchPyModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPlastimatchPyModuleWidget(QWidget *parent=0);
  virtual ~qSlicerPlastimatchPyModuleWidget();

public slots:


protected:
  QScopedPointer<qSlicerPlastimatchPyModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerPlastimatchPyModuleWidget);
  Q_DISABLE_COPY(qSlicerPlastimatchPyModuleWidget);
};

#endif
