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

#include <vtkMRMLNode.h>

// SlicerRT includes
#include "SlicerRtCommon.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DicomRtExport
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
void qSlicerDicomRtExportModuleWidget::setup()
{
  Q_D(qSlicerDicomRtExportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Filter out nodes that are not image volume nodes
  d->MRMLNodeComboBox_ImageVolume->addAttribute( QString("vtkMRMLScalarVolumeNode"), QString("LabelMap"), 0 );

  // Filter out nodes that are not dose volume nodes
  d->MRMLNodeComboBox_DoseVolume->addAttribute( QString("vtkMRMLScalarVolumeNode"), QString(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );

  // Filter out hierarchy nodes that are not contour hierarchy nodes
  d->MRMLNodeComboBox_ContourHierarchy->addAttribute( QString("vtkMRMLSubjectHierarchyNode"), QString(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );

  // Make connections
  this->connect( d->pushButton_SaveDicomRT, SIGNAL(clicked()), this, SLOT(onSaveClicked()) );
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onSaveClicked()
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  d->logic()->SaveDicomRTStudy(
    d->MRMLNodeComboBox_ImageVolume->currentNodeId().toLatin1().constData(),
    d->MRMLNodeComboBox_DoseVolume->currentNodeId().toLatin1().constData(),
    d->MRMLNodeComboBox_ContourHierarchy->currentNodeId().toLatin1().constData(),
    d->DirectoryButton_OutputDirectory->directory().toLatin1().constData() );

  QApplication::restoreOverrideCursor();
}
