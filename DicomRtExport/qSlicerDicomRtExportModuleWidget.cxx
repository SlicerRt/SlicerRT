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

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerDicomRtExportModuleWidget.h"
#include "ui_qSlicerDicomRtExportModule.h"
#include "vtkSlicerDicomRtExportModuleLogic.h"

#include "vtkMRMLNode.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomRtExport
class qSlicerDicomRtExportModuleWidgetPrivate: public Ui_qSlicerDicomRtExportModule
{
  Q_DECLARE_PUBLIC(qSlicerDicomRtExportModuleWidget);
protected:
  qSlicerDicomRtExportModuleWidget* const q_ptr;
public:
  qSlicerDicomRtExportModuleWidgetPrivate(qSlicerDicomRtExportModuleWidget &object);
  ~qSlicerDicomRtExportModuleWidgetPrivate();

  vtkSlicerDicomRtExportModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidgetPrivate::qSlicerDicomRtExportModuleWidgetPrivate(qSlicerDicomRtExportModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidgetPrivate::~qSlicerDicomRtExportModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerDicomRtExportModuleLogic* qSlicerDicomRtExportModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDicomRtExportModuleWidget);
  return vtkSlicerDicomRtExportModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidget::qSlicerDicomRtExportModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDicomRtExportModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidget::~qSlicerDicomRtExportModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  this->Superclass::setMRMLScene(scene);
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::setup()
{
  Q_D(qSlicerDicomRtExportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onSaveClicked()
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  d->logic()->SaveDicomRTStudy(
    d->MRMLNodeComboBox_ImageVolume->currentNodeID().toLatin1().constData(),
    d->MRMLNodeComboBox_DoseVolume->currentNodeID().toLatin1().constData(),
    d->MRMLNodeComboBox_ContourHierarchy->currentNodeID().toLatin1().constData(),
    d->DirectoryButton_OutputDirectory->directory().toLatin1().constData() );

  QApplication::restoreOverrideCursor();
}
