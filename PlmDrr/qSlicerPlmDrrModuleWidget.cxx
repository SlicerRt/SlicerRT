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
#include <QButtonGroup>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QProgressDialog>

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerCoreIOManager.h>
#include <qSlicerIO.h>

#include "qSlicerPlmDrrModuleWidget.h"
#include "ui_qSlicerPlmDrrModuleWidget.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

#include "vtkMRMLPlmDrrNode.h"

// RTBeam includes
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTIonBeamNode.h>

// Logic includes
#include "vtkSlicerPlmDrrLogic.h"

#define INPUT_VOLUME_FILE "inputVolumeFile"
#define OUTPUT_VOLUME_FILE "Out0000"
#define INPUT_MHA_FILE (INPUT_VOLUME_FILE ".mha")
#define OUTPUT_MHD_FILE (OUTPUT_VOLUME_FILE ".mhd")
#define OUTPUT_RAW_FILE (OUTPUT_VOLUME_FILE ".raw")

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlmDrrModuleWidgetPrivate: public Ui_qSlicerPlmDrrModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerPlmDrrModuleWidget);
protected:
  qSlicerPlmDrrModuleWidget* const q_ptr;
public:
  qSlicerPlmDrrModuleWidgetPrivate(qSlicerPlmDrrModuleWidget& object);
  virtual ~qSlicerPlmDrrModuleWidgetPrivate();
  vtkSlicerPlmDrrLogic* logic() const;

public:
  vtkMRMLPlmDrrNode* ParameterNode;
  vtkMRMLRTBeamNode* RtBeamNode;
  vtkMRMLScalarVolumeNode* ReferenceVolumeNode;
  bool ModuleWindowInitialized;
  QProcess* m_PlastimatchProcess;
  QString m_ReferenceVolumeFile;
  QProgressDialog* m_ProgressDialog;
  std::list< std::string > PlastimatchArgs;
};

//-----------------------------------------------------------------------------
// qSlicerLoadableModuleTemplateModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlmDrrModuleWidgetPrivate::qSlicerPlmDrrModuleWidgetPrivate(qSlicerPlmDrrModuleWidget &object)
  :
  q_ptr(&object),
  ParameterNode(nullptr),
  RtBeamNode(nullptr),
  ReferenceVolumeNode(nullptr),
  ModuleWindowInitialized(false),
  m_PlastimatchProcess(nullptr),
  m_ProgressDialog(nullptr)
{
}

//-----------------------------------------------------------------------------
qSlicerPlmDrrModuleWidgetPrivate::~qSlicerPlmDrrModuleWidgetPrivate()
{
  if (m_PlastimatchProcess)
  {
    m_PlastimatchProcess->close();
    delete m_PlastimatchProcess;
    m_PlastimatchProcess = nullptr;
  }
  if (m_ProgressDialog)
  {
    m_ProgressDialog->hide();
    delete m_ProgressDialog;
    m_ProgressDialog = nullptr;
  }
}

//-----------------------------------------------------------------------------
vtkSlicerPlmDrrLogic*
qSlicerPlmDrrModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerPlmDrrModuleWidget);
  return vtkSlicerPlmDrrLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerPlmDrrModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPlmDrrModuleWidget::qSlicerPlmDrrModuleWidget(QWidget* _parent)
  :
  Superclass(_parent),
  d_ptr(new qSlicerPlmDrrModuleWidgetPrivate(*this))
{
//  Q_D(qSlicerPlmDrrModuleWidget);
}

//-----------------------------------------------------------------------------
qSlicerPlmDrrModuleWidget::~qSlicerPlmDrrModuleWidget()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  QSettings* settings = qSlicerCoreApplication::application()->settings();

  // Get plastimatch application path
  QString plastimatchPath = d->LineEdit_PlastimatchAppPath->text();

  // saving up the plastimatch path
  settings->setValue( "SlicerRT/PlastimatchApplicationPath", plastimatchPath);
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::setup()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  d->setupUi(this);

  this->Superclass::setup();

  QStringList plmDrrNodes;
  plmDrrNodes.push_back("vtkMRMLPlmDrrNode");
  d->MRMLNodeComboBox_ParameterNode->setNodeTypes(plmDrrNodes);

  QStringList rtBeamNodes;
  rtBeamNodes.push_back("vtkMRMLRTBeamNode");
  d->MRMLNodeComboBox_RtBeam->setNodeTypes(rtBeamNodes);

  QStringList volumeNodes;
  volumeNodes.push_back("vtkMRMLScalarVolumeNode");
  d->MRMLNodeComboBox_ReferenceVolume->setNodeTypes(volumeNodes);

  // Nodes
  connect( d->MRMLNodeComboBox_RtBeam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onRTBeamNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onReferenceVolumeNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_ParameterNode, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onParameterNodeChanged(vtkMRMLNode*)));

  // Sliders
  connect( d->SliderWidget_IsocenterImagerDistance, SIGNAL(valueChanged(double)), 
    this, SLOT(onIsocenterImagerDistanceValueChanged(double)));

  // Coordinates widgets
  connect( d->CoordinatesWidget_ImagePixelDimention, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImageDimentionChanged(double*)));
  connect( d->CoordinatesWidget_ImagePixelSpacing, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImageSpacingChanged(double*)));
  connect( d->CoordinatesWidget_ImageWindow, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImageWindowCoordinatesChanged(double*)));
  connect( d->RangeWidget_ImageWindowColumns, SIGNAL(valuesChanged( double, double)), 
    this, SLOT(onImageWindowColumnsValuesChanged( double, double)));
  connect( d->RangeWidget_ImageWindowRows, SIGNAL(valuesChanged( double, double)), 
    this, SLOT(onImageWindowRowsValuesChanged( double, double)));

  // Buttons
  connect( d->PushButton_SelectPlastimatchAppPath, SIGNAL(clicked()), this, SLOT(onSelectPlastimatchAppPathClicked()));
  connect( d->PushButton_ComputeDrr, SIGNAL(clicked()), this, SLOT(onComputeDrrClicked()));
  connect( d->CheckBox_ShowDrrMarkups, SIGNAL(toggled(bool)), this, SLOT(onShowMarkupsToggled(bool)));
  connect( d->CheckBox_UseImageWindow, SIGNAL(toggled(bool)), this, SLOT(onUseImageWindowToggled(bool)));
  connect( d->CheckBox_UseExponentialMapping, SIGNAL(toggled(bool)), this, SLOT(onUseExponentialMappingToggled(bool)));
  connect( d->CheckBox_AutoscalePixelsRange, SIGNAL(toggled(bool)), this, SLOT(onAutoscalePixelsRangeToggled(bool)));

  // Button groups
  connect( d->ButtonGroup_ReconstructionAlgorithm, SIGNAL(buttonClicked(int)), this, SLOT(onReconstructionAlgorithmChanged(int)));
  connect( d->ButtonGroup_Threading, SIGNAL(buttonClicked(int)), this, SLOT(onThreadingChanged(int)));
  connect( d->ButtonGroup_HUConversion, SIGNAL(buttonClicked(int)), this, SLOT(onHUConversionChanged(int)));

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT(onLogicModified()));

  // Load default the plastimatch application path
  QSettings* settings = qSlicerCoreApplication::application()->settings();
  QString plastimatchPath;

  // set up the plastimatch path
  if (settings->value( "SlicerRT/PlastimatchApplicationPath", "") == "")
  {
    plastimatchPath = QString("./plastimatch");
    qCritical() << Q_FUNC_INFO << ": No Plastimatch path in settings.  Using \"" << qPrintable(plastimatchPath.toUtf8()) << "\".\n";
  }
  else
  {
    plastimatchPath = settings->value( "SlicerRT/PlastimatchApplicationPath", "").toString();
  }
  d->LineEdit_PlastimatchAppPath->setText(plastimatchPath);
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerPlmDrrModuleWidget);
  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));

  // Find parameters node or create it if there is none in the scene
  if (scene)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass( 0, "vtkMRMLPlmDrrNode");
    if (d->MRMLNodeComboBox_ParameterNode->currentNode())
    {
      this->setParameterNode(d->MRMLNodeComboBox_ParameterNode->currentNode());
    }
    else if (node)
    {
      this->setParameterNode(node);
    }
    else
    {
      vtkNew<vtkMRMLPlmDrrNode> newNode;
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}

/// RTBeam Node (RTBeam or RTIonBeam) changed
void qSlicerPlmDrrModuleWidget::onRTBeamNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(node);
  Q_UNUSED(ionBeamNode);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  d->ParameterNode->DisableModifiedEventOn();
  d->ParameterNode->SetAndObserveBeamNode(beamNode);
  d->ParameterNode->DisableModifiedEventOff();

  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onReferenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerPlmDrrModuleWidget);
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!volumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid reference volume node";
    return;
  }

  d->ReferenceVolumeNode = volumeNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onParameterNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerPlmDrrModuleWidget);
  vtkMRMLPlmDrrNode* paramNode = vtkMRMLPlmDrrNode::SafeDownCast(node);
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  setParameterNode(paramNode);
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onComputeDrrClicked()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (d->ReferenceVolumeNode && d->ReferenceVolumeNode->AddDefaultStorageNode())
  {
    if (d->m_ProgressDialog)
    {
      delete d->m_ProgressDialog;
    }
    d->m_ProgressDialog = new QProgressDialog( tr("Saving files to the Slicer temporary directory..."), 
      tr("Stop"), 0, 0, this);
    connect( d->m_ProgressDialog, SIGNAL(canceled()), this, SLOT(onCancelPlastimatchDrrProcessClicked()));
    d->m_ProgressDialog->setWindowTitle("DRR computation progress");
    d->m_ProgressDialog->setWindowModality(Qt::WindowModal);

    qSlicerCoreIOManager* coreIOManager = qSlicerCoreApplication::application()->coreIOManager();
    qSlicerIO::IOProperties fileParameters;
    QDir dir(qSlicerCoreApplication::application()->temporaryPath());
    QString drrVolumeFileName = dir.absoluteFilePath(INPUT_MHA_FILE);

    fileParameters["nodeID"] = d->ReferenceVolumeNode->GetID();
    fileParameters["fileName"] = drrVolumeFileName;
    // Saving reference volume into temporary mha file and starting the process
    if (coreIOManager->saveNodes( "VolumeFile", fileParameters))
    {
      d->m_ReferenceVolumeFile = drrVolumeFileName;

      d->m_PlastimatchProcess = new QProcess();
      connect( d->m_PlastimatchProcess, SIGNAL(started()), 
        this, SLOT(onPlastimatchDrrProcessStarted()));
      connect( d->m_PlastimatchProcess, SIGNAL(finished( int, QProcess::ExitStatus)), 
        this, SLOT(onPlastimatchDrrProcessFinished( int, QProcess::ExitStatus)));

      QStringList arguments;
      for (const std::string& arg : d->PlastimatchArgs)
      {
        arguments << QString::fromStdString(arg);
      }
      arguments << d->m_ReferenceVolumeFile;

      d->m_PlastimatchProcess->setWorkingDirectory(qSlicerCoreApplication::application()->temporaryPath());
      d->m_PlastimatchProcess->start( d->LineEdit_PlastimatchAppPath->text(), arguments);
    }
    else
    {
      d->m_ReferenceVolumeFile.clear();
      qCritical() << Q_FUNC_INFO << ": Unable to save reference volume into temporary mha file";
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onPlastimatchDrrProcessStarted()
{
  Q_D(qSlicerPlmDrrModuleWidget);
  qDebug() << Q_FUNC_INFO << ": Process of DRR RTImage calculation has been started";
  if (d->m_ProgressDialog)
  {
    d->m_ProgressDialog->setLabelText(tr("Plastimatch process..."));
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onPlastimatchDrrProcessFinished( int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  QProcess::ProcessError err = d->m_PlastimatchProcess->error();

  delete d->m_PlastimatchProcess;
  d->m_PlastimatchProcess = nullptr;

  if (exitCode == EXIT_SUCCESS && exitStatus == QProcess::NormalExit)
  {
    if (d->m_ProgressDialog)
    {
      d->m_ProgressDialog->setLabelText(tr("Loading and processing DRR image..."));
    }

    // load resulted raw DRR image file
    QDir dir(qSlicerCoreApplication::application()->temporaryPath());
    QStringList filters;
    filters << "*.raw";
    QStringList tmpRawImages = dir.entryList(filters);

    // Create *.mhd file for that one image
    std::string mhdName;
    QString rawName;
    for (const QString& fileName : tmpRawImages)
    {
      QFileInfo fileInfo(fileName);
      QString baseName = fileInfo.baseName();
      QString mhdFileName = baseName + ".mhd";

      if (mhdFileName == OUTPUT_MHD_FILE)
      {
        fileInfo.setFile( dir, mhdFileName);
        rawName = fileName;
        mhdName = fileInfo.absoluteFilePath().toStdString();
        std::ofstream ofs(mhdName.c_str());

        int res[2];
        d->ParameterNode->GetImageDimention(res);
        int window[4];
        d->ParameterNode->GetImageWindow(window);
        double spacing[2];
        d->ParameterNode->GetImageSpacing(spacing);

        ofs << "NDims = 3\n";
        if (!d->CheckBox_UseImageWindow->isChecked())
        {
          ofs << "DimSize = " << res[1] << " " << res[0] << " 1\n";
        }
        else
        {
          ofs << "DimSize = " << window[3] - window[1] + 1 << " " << window[2] - window[0] + 1 << " 1\n";
        }
        ofs << "ElementSpacing = " << spacing[1] << " " << spacing[0] << " 1\n";
        ofs << "Position = 0 0 0\n";
        ofs << "BinaryData = True\n";
        ofs << "ElementByteOrderMSB = False\n";
        ofs << "ElementType = MET_LONG\n";
        ofs << "ElementDataFile = " << fileName.toStdString() << '\n';
        ofs.close();
      }
    }

    if (mhdName.empty())
    {
      qCritical() << Q_FUNC_INFO << ": MetaImageHeader file name of DRR raw imahe is empty!";
      return;
    }

    // Load DRR RtImage from raw file 
    if (d->logic()->LoadRtImage( d->ParameterNode, mhdName))
    {
      // delete temporary files
      QFile drrReferenceVolumeFile(d->m_ReferenceVolumeFile);
      QFile drrMhdFile(mhdName.c_str());
      QFile drrRawFile(mhdName.c_str());
        
      drrReferenceVolumeFile.remove();
      drrMhdFile.remove();
      drrRawFile.remove();
    }
    else
    {
      qCritical() << Q_FUNC_INFO << ": DRR scalar volume node hasn't been loaded";
    }
  }
  else
  {    
    QString errorMessage;
    switch (err)
    {
    case QProcess::FailedToStart:
      errorMessage = tr("Failed to start, Either the invoked program is missing, " \
        "or you may have insufficient permissions to invoke the program.");
      break;
    case QProcess::Crashed:
      errorMessage = tr("Crashed some time after starting successfully.");
      break;
    case QProcess::Timedout:
      errorMessage = tr("The last waitFor...() function timed out, no response.");
      break;
    case QProcess::WriteError:
      errorMessage = tr("An error occurred when attempting to write to the process.");
      break;    
    case QProcess::ReadError:
      errorMessage = tr("An error occurred when attempting to read from the process.");
      break;
    case QProcess::UnknownError:
    default:
      errorMessage = tr("An unknown error occurred.");
      break;
    }
    qCritical() << Q_FUNC_INFO << ": DRR calculation process has been finished with an error: " << errorMessage;
  }
  if (d->m_ProgressDialog)
  {
    d->m_ProgressDialog->hide();
    delete d->m_ProgressDialog;
    d->m_ProgressDialog = nullptr;
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onCancelPlastimatchDrrProcessClicked()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (d->m_PlastimatchProcess)
  {
    d->m_PlastimatchProcess->close();
    delete d->m_PlastimatchProcess;
    d->m_PlastimatchProcess = nullptr;
  }
  if (d->m_ProgressDialog)
  {
    d->m_ProgressDialog->hide();
    delete d->m_ProgressDialog;
    d->m_ProgressDialog = nullptr;
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onSelectPlastimatchAppPathClicked()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  QFileDialog* dialog = new QFileDialog( this, tr("Select plastimatch application"));
  QStringList fileNames;
  QString fileName;
  QString filter;

  dialog->setAcceptMode(QFileDialog::AcceptOpen);
  dialog->setFileMode(QFileDialog::ExistingFile);

  QStringList filters;

  filters << tr("All files (*)");

  QFileInfo fileInfo(d->LineEdit_PlastimatchAppPath->text());
  dialog->setDirectory(fileInfo.absoluteFilePath());

  if (dialog->exec())
  {
    fileNames = dialog->selectedFiles();

    if (!fileNames.isEmpty())
    {
      fileName = fileNames[0];
      filter = dialog->selectedNameFilter();
    }
  }
  if (!fileName.isEmpty())
  {
    d->LineEdit_PlastimatchAppPath->setText(fileName);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Wrong path to plastimatch";
  }
  
  delete dialog;
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  d->ParameterNode = vtkMRMLPlmDrrNode::SafeDownCast(d->MRMLNodeComboBox_ParameterNode->currentNode());

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->ParameterNode || !d->ModuleWindowInitialized)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (!d->ParameterNode->GetBeamNode())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced parameter's beam node";
    return;
  }

  if (!d->ReferenceVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }

  // Update widgets info from parameter node and update plastimatch drr command
  d->MRMLNodeComboBox_RtBeam->setCurrentNode(d->ParameterNode->GetBeamNode());
  d->SliderWidget_IsocenterImagerDistance->setValue(d->ParameterNode->GetIsocenterImagerDistance());

  int imageDimInteger[2] = {};
  double imageDim[2] = {};
  d->ParameterNode->GetImageDimention(imageDimInteger);
  imageDim[0] = static_cast<double>(imageDimInteger[0]);
  imageDim[1] = static_cast<double>(imageDimInteger[1]);
  d->CoordinatesWidget_ImagePixelDimention->setCoordinates(imageDim);
  d->CoordinatesWidget_ImagePixelSpacing->setCoordinates(d->ParameterNode->GetImageSpacing());
    
  int imageWindowInteger[4] = {};
  double imageWindow[4] = {};
  d->ParameterNode->GetImageWindow(imageWindowInteger);
  imageWindow[0] = static_cast<double>(imageWindowInteger[0]);
  imageWindow[1] = static_cast<double>(imageWindowInteger[1]);
  imageWindow[2] = static_cast<double>(imageWindowInteger[2]);
  imageWindow[3] = static_cast<double>(imageWindowInteger[3]);
  d->CoordinatesWidget_ImageWindow->setCoordinates(imageWindow);

  d->CheckBox_UseImageWindow->setChecked(d->ParameterNode->GetImageWindowFlag());

  d->CheckBox_UseExponentialMapping->setChecked(d->ParameterNode->GetExponentialMappingFlag());
  d->CheckBox_AutoscalePixelsRange->setChecked(d->ParameterNode->GetAutoscaleFlag());

  switch (d->ParameterNode->GetAlgorithmReconstuction())
  {
    case vtkMRMLPlmDrrNode::AlgorithmReconstuctionType::EXACT:
      d->RadioButton_Exact->setChecked(true);
      break;
    case vtkMRMLPlmDrrNode::AlgorithmReconstuctionType::UNIFORM:
      d->RadioButton_Exact->setChecked(true);
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetHUConversion())
  {
    case vtkMRMLPlmDrrNode::HounsfieldUnitsConversionType::PREPROCESS:
      d->RadioButton_Preprocess->setChecked(true);
      break;
    case vtkMRMLPlmDrrNode::HounsfieldUnitsConversionType::INLINE:
      d->RadioButton_Inline->setChecked(true);
      break;
    case vtkMRMLPlmDrrNode::HounsfieldUnitsConversionType::NONE:
      d->RadioButton_None->setChecked(true);
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetThreading())
  {
    case vtkMRMLPlmDrrNode::ThreadingType::CPU:
      d->RadioButton_CPU->setChecked(true);
      break;
    case vtkMRMLPlmDrrNode::ThreadingType::CUDA:
      d->RadioButton_CUDA->setChecked(true);
      break;
    case vtkMRMLPlmDrrNode::ThreadingType::OPENCL:
      d->RadioButton_OpenCL->setChecked(true);
      break;
    default:
      break;
  }

  // Update DRR arguments
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::enter()
{
  this->Superclass::enter();
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::exit()
{
  this->Superclass::exit();
  this->qvtkDisconnectAll();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onEnter()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic";
    return;
  }

  if (!d->ParameterNode)
  {
    // Try to find one in the scene
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass( 0, "vtkMRMLPlmDrrNode");
    if (node)
    {
      d->ParameterNode = vtkMRMLPlmDrrNode::SafeDownCast(node);
    }
    else 
    {
      qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
      return;
    }
  }

  if (!d->ParameterNode->GetBeamNode())
  {
    // Try to find one in the scene
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass( 0, "vtkMRMLRTBeamNode");
    if (node)
    {
      d->RtBeamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
      d->ParameterNode->SetAndObserveBeamNode(d->RtBeamNode);
    }
    else 
    {
      d->RtBeamNode = nullptr;
    }
  }

  d->logic()->CreateMarkupsNodes(d->ParameterNode);

  d->ModuleWindowInitialized = true;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  d->ParameterNode = vtkMRMLPlmDrrNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterNode->setCurrentNode(d->ParameterNode);

  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect( d->ParameterNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
  
  // Set selected MRML nodes in comboboxes in the parameter set if it was nullptr there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (d->ParameterNode)
  {
    if (!d->ParameterNode->GetBeamNode())
    {
      vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
      qvtkConnect( beamNode, vtkMRMLRTBeamNode::BeamGeometryModified, this, SLOT(onUpdateImageWindowFromBeamJaws()));
      d->ParameterNode->SetAndObserveBeamNode(beamNode);
      d->ParameterNode->Modified();
    }
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onIsocenterImagerDistanceValueChanged(double value)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  d->ParameterNode->SetIsocenterImagerDistance(value);

  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onImageSpacingChanged(double* spacing)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  double s[2] = { spacing[0], spacing[1] };
  d->ParameterNode->SetImageSpacing(s);

  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onShowMarkupsToggled(bool toggled)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  // Update imager and image markups, DRR arguments
  d->logic()->ShowMarkupsNodes( d->ParameterNode, toggled);
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onImageDimentionChanged(double* dimention)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int dim[2] = { static_cast<int>(dimention[0]), static_cast<int>(dimention[1]) }; // x, y

  d->ParameterNode->SetImageDimention(dim);

  d->RangeWidget_ImageWindowColumns->setMaximum(dim[0] - 1);
  d->RangeWidget_ImageWindowRows->setMaximum(dim[1] - 1);
  d->CoordinatesWidget_ImageWindow->setMaximum(std::max( dim[0] - 1, dim[1] - 1));

  d->RangeWidget_ImageWindowColumns->setMinimum(0);
  d->RangeWidget_ImageWindowColumns->setMaximum(dim[0] - 1);
  d->RangeWidget_ImageWindowRows->setMinimum(0);
  d->RangeWidget_ImageWindowRows->setMaximum(dim[1] - 1);

  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onImageWindowCoordinatesChanged(double* window)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4] = { 
    static_cast<int>(window[0]), // c1 = x1
    static_cast<int>(window[1]), // r1 = y1
    static_cast<int>(window[2]), // c2 = x2
    static_cast<int>(window[3]) }; // r2 = y2

  d->ParameterNode->SetImageWindow(imageWindow);

  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onImageWindowRowsValuesChanged( double r1, double r2)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  d->ParameterNode->GetImageWindow(imageWindow);
  imageWindow[1] = static_cast<int>(r1); // r1
  imageWindow[3] = static_cast<int>(r2); // r2

  double imageWin[4];
  const double* coord = d->CoordinatesWidget_ImageWindow->coordinates();
  imageWin[0] = coord[0];
  imageWin[1] = r1;
  imageWin[2] = coord[2];
  imageWin[3] = r2;
  d->CoordinatesWidget_ImageWindow->setCoordinates(imageWin);

  d->ParameterNode->SetImageWindow(imageWindow);
  
  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onImageWindowColumnsValuesChanged( double c1, double c2)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  d->ParameterNode->GetImageWindow(imageWindow);
  imageWindow[0] = static_cast<int>(c1); // c1
  imageWindow[2] = static_cast<int>(c2); // c2

  double imageWin[4];
  const double* coord = d->CoordinatesWidget_ImageWindow->coordinates();
  imageWin[0] = c1;
  imageWin[1] = coord[1];
  imageWin[2] = c2;
  imageWin[3] = coord[3];
  d->CoordinatesWidget_ImageWindow->setCoordinates(imageWin);

  d->ParameterNode->SetImageWindow(imageWindow);

  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onUseImageWindowToggled(bool value)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  if (value)
  {
    const double* window = d->CoordinatesWidget_ImageWindow->coordinates();
    imageWindow[0] = static_cast<int>(window[0]); // c1 = x1
    imageWindow[1] = static_cast<int>(window[1]); // r1 = y1
    imageWindow[2] = static_cast<int>(window[2]); // c2 = x2
    imageWindow[3] = static_cast<int>(window[3]); // r2 = y2
  }
  else
  {
    const double* window = d->CoordinatesWidget_ImagePixelDimention->coordinates();
    imageWindow[0] = 0;
    imageWindow[1] = 0;
    imageWindow[2] = static_cast<int>(window[0]); // column = x
    imageWindow[3] = static_cast<int>(window[1]); // row = y
  }

  d->ParameterNode->SetImageWindowFlag(value);
  d->ParameterNode->SetImageWindow(imageWindow);
  
  // Update imager and image markups, DRR arguments
  d->logic()->UpdateMarkupsNodes(d->ParameterNode);
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onUseExponentialMappingToggled(bool value)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  d->ParameterNode->SetExponentialMappingFlag(value);
  // Update DRR arguments
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onAutoscalePixelsRangeToggled(bool value)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  d->ParameterNode->SetAutoscaleFlag(value);
  // Update DRR arguments
  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onUpdateImageWindowFromBeamJaws()
{
  Q_D(qSlicerPlmDrrModuleWidget);
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onUpdatePlmDrrArgs()
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  std::string args = d->ParameterNode->GenerateArguments(d->PlastimatchArgs);
  if (!args.empty())
  {
    // all nodes are valid, enable compute button and update plastimatch drr parameters
    d->PushButton_ComputeDrr->setEnabled(true); 
    d->plainTextEdit_PlmDrrArgs->setPlainText(QString::fromStdString(args));
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onReconstructionAlgorithmChanged(int button_id)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  QAbstractButton* button = d->ButtonGroup_ReconstructionAlgorithm->button(button_id);
  QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);

  if (rbutton == d->RadioButton_Exact)
  {
    d->ParameterNode->SetAlgorithmReconstuction(vtkMRMLPlmDrrNode::AlgorithmReconstuctionType::EXACT);
  }
  else if (rbutton == d->RadioButton_Uniform)
  {
    d->ParameterNode->SetAlgorithmReconstuction(vtkMRMLPlmDrrNode::AlgorithmReconstuctionType::UNIFORM);
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Invalid reconstruction algorithm button id";
    return;
  }

  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onThreadingChanged(int button_id)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  QAbstractButton* button = d->ButtonGroup_Threading->button(button_id);
  QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);

  if (rbutton == d->RadioButton_CPU)
  {
    d->ParameterNode->SetThreading(vtkMRMLPlmDrrNode::ThreadingType::CPU);
  }
  else if (rbutton == d->RadioButton_CUDA)
  {
    d->ParameterNode->SetThreading(vtkMRMLPlmDrrNode::ThreadingType::CUDA);
  }
  else if (rbutton == d->RadioButton_OpenCL)
  {
    d->ParameterNode->SetThreading(vtkMRMLPlmDrrNode::ThreadingType::OPENCL);
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Invalid threading button id";
    return;
  }

  this->onUpdatePlmDrrArgs();
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModuleWidget::onHUConversionChanged(int button_id)
{
  Q_D(qSlicerPlmDrrModuleWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  QAbstractButton* button = d->ButtonGroup_HUConversion->button(button_id);
  QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);

  if (rbutton == d->RadioButton_None)
  {
    d->ParameterNode->SetHUConversion(vtkMRMLPlmDrrNode::HounsfieldUnitsConversionType::NONE);
  }
  else if (rbutton == d->RadioButton_Inline)
  {
    d->ParameterNode->SetHUConversion(vtkMRMLPlmDrrNode::HounsfieldUnitsConversionType::INLINE);
  }
  else if (rbutton == d->RadioButton_Preprocess)
  {
    d->ParameterNode->SetHUConversion(vtkMRMLPlmDrrNode::HounsfieldUnitsConversionType::PREPROCESS);
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Invalid Hounsfield units conversion button id";
    return;
  }

  this->onUpdatePlmDrrArgs();
}
