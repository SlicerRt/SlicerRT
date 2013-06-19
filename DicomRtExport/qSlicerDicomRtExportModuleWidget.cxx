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

public:
  // Description:
  void SetImageVolumeNodeID(char* id)
  {
    this->ImageVolumeNodeID = id;
  }
  void SetDoseVolumeNodeID(char* id)
  {
    this->DoseVolumeNodeID = id;
  }
  void SetContourHierarchyNodeID(char* id)
  {
    this->ContourHierarchyNodeID = id;
  }
  void SetCurrentOutputPath(const char* path)
  {
    this->CurrentOutputPath = const_cast<char*>(path);
  }

protected:
  char *ImageVolumeNodeID;
  char *DoseVolumeNodeID;
  char *ContourHierarchyNodeID;
  char *CurrentOutputPath;
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidgetPrivate::qSlicerDicomRtExportModuleWidgetPrivate(qSlicerDicomRtExportModuleWidget& object)
  : q_ptr(&object)
{
  this->ImageVolumeNodeID = NULL;
  this->DoseVolumeNodeID = NULL;
  this->ContourHierarchyNodeID = NULL;
  this->CurrentOutputPath = NULL;
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

  // Make connections
  this->connect( d->MRMLNodeComboBox_ImageVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
           this, SLOT( onImageVolumeNodeChanged(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), 
           this, SLOT( onDoseVolumeNodeChanged(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_ContourHierarchy, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), 
           this, SLOT( onContourHierarchyNodeChanged(vtkMRMLNode*) ) );
  this->connect( d->DirectoryButton_OutputDirectory, SIGNAL( directoryChanged(const QString &) ), 
           this, SLOT( onCurrentPathChanged(const QString &) ) );

  this->connect( d->pushButton_SaveDicomRT, SIGNAL(clicked()), this, SLOT(onSaveClicked()) );

  // Handle scene change event if occurs
  //qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onImageVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  if (!this->mrmlScene() || !node)
  {
    return;
  }
  d->SetImageVolumeNodeID(node->GetID());

}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  if (!this->mrmlScene() || !node)
  {
    return;
  }

  d->SetDoseVolumeNodeID(node->GetID());

}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onContourHierarchyNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  if (!this->mrmlScene() || !node)
  {
    return;
  }

  d->SetContourHierarchyNodeID(node->GetID());
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onCurrentPathChanged(const QString &path)
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }
  // try to use unix style escape charactor but does not work
  QString myUnixPath = QDir::fromNativeSeparators(path);
  std::string current_locale_text = myUnixPath.toLocal8Bit().constData();
  d->SetCurrentOutputPath(current_locale_text.c_str());

  // use windows style and does not work
  //std::string current_locale_text = path.toLocal8Bit().constData();
  //d->SetCurrentOutputPath(current_locale_text.c_str());

  // this does not work
  //QByteArray byteArray = path.toUtf8();
  //d->SetCu rrentOutputPath(byteArray.constData());
  
  // this works
  d->SetCurrentOutputPath("c:\\wangk\\tmp\\testexport2");

  // this works
  //d->SetCurrentOutputPath("c:/wangk/tmp/testexport2");
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::onSaveClicked()
{
  Q_D(qSlicerDicomRtExportModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the isodose surface for the selected dose volume
  d->logic()->SaveDicomRTStudy(d->ImageVolumeNodeID, d->DoseVolumeNodeID, d->ContourHierarchyNodeID, d->CurrentOutputPath);

  QApplication::restoreOverrideCursor();
}
