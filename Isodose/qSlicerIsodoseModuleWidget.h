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

#ifndef __qSlicerIsodoseModuleWidget_h
#define __qSlicerIsodoseModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerIsodoseModuleExport.h"

class qSlicerIsodoseModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_Isodose
class Q_SLICER_QTMODULES_ISODOSE_EXPORT qSlicerIsodoseModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerIsodoseModuleWidget(QWidget *parent=0);
  virtual ~qSlicerIsodoseModuleWidget();

public slots:


protected:
  QScopedPointer<qSlicerIsodoseModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerIsodoseModuleWidget);
  Q_DISABLE_COPY(qSlicerIsodoseModuleWidget);
};

#endif
