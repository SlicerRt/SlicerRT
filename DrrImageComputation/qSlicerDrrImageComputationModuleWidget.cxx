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

// qSlicer includes
#include "qSlicerDrrImageComputationModuleWidget.h"
#include "qSlicerSimpleMarkupsWidget.h"
#include "ui_qSlicerDrrImageComputationModuleWidget.h"

// Slicer MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLTableNode.h>

// SlicerRT MRML Beams includes
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTIonBeamNode.h>

// SlicerRT MRML DrrImageComputation includes
#include "vtkMRMLDrrImageComputationNode.h"

// Logic includes
#include "vtkSlicerDrrImageComputationLogic.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkCamera.h>
#include <vtkTable.h>

namespace {

constexpr int PROJECTED_POINT_LABEL_COLUMN = 0;
constexpr int PROJECTED_POINT_X_COLUMN = 1;
constexpr int PROJECTED_POINT_Y_COLUMN = 2;
constexpr int PROJECTED_POINT_Z_COLUMN = 3;
constexpr int PROJECTED_POINT_WIDTH_COLUMN = 4;
constexpr int PROJECTED_POINT_HEIGHT_COLUMN = 5;
constexpr int PROJECTED_POINT_COLUMN_COLUMN = 6;
constexpr int PROJECTED_POINT_ROW_COLUMN = 7;
constexpr int PROJECTED_POINT_STATUS_COLUMN = 8;
constexpr int PROJECTED_POINT_COLUMNS = 9;

};

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DrrImageComputation
class qSlicerDrrImageComputationModuleWidgetPrivate: public Ui_qSlicerDrrImageComputationModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerDrrImageComputationModuleWidget);
protected:
  qSlicerDrrImageComputationModuleWidget* const q_ptr;
public:
  qSlicerDrrImageComputationModuleWidgetPrivate(qSlicerDrrImageComputationModuleWidget &object);
  virtual ~qSlicerDrrImageComputationModuleWidgetPrivate();
  vtkSlicerDrrImageComputationLogic* logic() const;
  bool CheckPointWithinVolumeBounds(vtkMRMLScalarVolumeNode* volumeNode, const double pointRAS[3]) const;
  bool GetMarkupsWidgetRowList(std::list< int >& list);
};

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModuleWidgetPrivate::qSlicerDrrImageComputationModuleWidgetPrivate(qSlicerDrrImageComputationModuleWidget &object)
  :
  q_ptr(&object)
{
}

qSlicerDrrImageComputationModuleWidgetPrivate::~qSlicerDrrImageComputationModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerDrrImageComputationLogic* qSlicerDrrImageComputationModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDrrImageComputationModuleWidget);
  return vtkSlicerDrrImageComputationLogic::SafeDownCast(q->logic());
}

//------------------------------------------------------------------------------
bool qSlicerDrrImageComputationModuleWidgetPrivate::CheckPointWithinVolumeBounds(vtkMRMLScalarVolumeNode* volumeNode,
  const double pointRAS[3]) const
{
  if (!volumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Volume node is invalid";
    return false;
  }
  double bounds[6] = {};
  volumeNode->GetRASBounds(bounds);

  return (pointRAS[0] > bounds[0] && pointRAS[0] < bounds[1] &&
  pointRAS[1] > bounds[2] && pointRAS[1] < bounds[3] &&
  pointRAS[2] > bounds[4] && pointRAS[2] < bounds[5]);
}

//------------------------------------------------------------------------------
bool qSlicerDrrImageComputationModuleWidgetPrivate::GetMarkupsWidgetRowList(std::list< int >& list)
{
  Q_Q(qSlicerDrrImageComputationModuleWidget);
  QTableWidget* markupsTableWidget = this->SimpleMarkupsWidget_PointCoordinates->tableWidget();
  if (this->RadioButton_ProjectSelectedControlPoints->isChecked())
  {
    for (int i = 0; i < markupsTableWidget->rowCount(); ++i)
    {
      QTableWidgetItem* item = markupsTableWidget->item( i, 0);
      if (item && item->isSelected())
      {
        list.push_back(i);
      }
    }
  }
  else if (this->RadioButton_ProjectAllControlPoints->isChecked())
  {
    if (markupsTableWidget->rowCount() > 0)
    {
      list.resize(markupsTableWidget->rowCount());
      std::iota(list.begin(), list.end(), 0);
    }
  }
  return (list.size() > 0);
}

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModuleWidget::qSlicerDrrImageComputationModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDrrImageComputationModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModuleWidget::~qSlicerDrrImageComputationModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::setup()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->TableWidget_ProjectedPointsCoordinates->setColumnCount( PROJECTED_POINT_COLUMNS );
  d->TableWidget_ProjectedPointsCoordinates->setHorizontalHeaderLabels( QStringList() << tr("Original label") << tr("R") << tr("A") << tr("S") \
    << tr("Width") << tr("Height") << tr("Column") << tr("Row") << tr("Status") );
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);

  // Reduce row height to minimum necessary
  d->TableWidget_ProjectedPointsCoordinates->setWordWrap(true);
  d->TableWidget_ProjectedPointsCoordinates->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  // Nodes
  connect( d->MRMLNodeComboBox_RtBeam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onRTBeamNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_CtVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onCtVolumeNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onParameterNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_Camera, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onCameraNodeChanged(vtkMRMLNode*)));

  // Sliders
  connect( d->SliderWidget_IsocenterImagerDistance, SIGNAL(valueChanged(double)), 
    this, SLOT(onIsocenterImagerDistanceValueChanged(double)));

  // Coordinates widgets
  connect( d->CoordinatesWidget_ImagerResolution, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImagerResolutionChanged(double*)));
  connect( d->CoordinatesWidget_ImagerSpacing, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImagerSpacingChanged(double*)));
  connect( d->RangeWidget_ImageWindowColumns, SIGNAL(valuesChanged( double, double)), 
    this, SLOT(onImageWindowColumnsValuesChanged( double, double)));
  connect( d->RangeWidget_ImageWindowRows, SIGNAL(valuesChanged( double, double)), 
    this, SLOT(onImageWindowRowsValuesChanged( double, double)));

  // Widgets
  connect( d->SimpleMarkupsWidget_PointCoordinates, SIGNAL(currentMarkupsControlPointSelectionChanged(int)), 
    this, SLOT(onMarkupsControlPointSelectionChanged(int)));
  connect( d->SimpleMarkupsWidget_PointCoordinates, SIGNAL(markupsNodeChanged()), 
    this, SLOT(onMarkupsNodeChanged()));

  // Buttons
  connect( d->PushButton_ComputeDrr, SIGNAL(clicked()), this, SLOT(onComputeDrrClicked()));
  connect( d->CheckBox_ShowDrrMarkups, SIGNAL(toggled(bool)), this, SLOT(onShowMarkupsToggled(bool)));
  connect( d->GroupBox_ImageWindowParameters, SIGNAL(toggled(bool)), this, SLOT(onUseImageWindowToggled(bool)));
  connect( d->PushButton_UpdateBeamFromCamera, SIGNAL(clicked()), this, SLOT(onUpdateBeamFromCameraClicked()));
  connect( d->PushButton_ProjectControlPoints, SIGNAL(clicked()), this, SLOT(onProjectMarkupsControlPointsClicked()));
  connect( d->PushButton_ClearProjectedTableWidget, SIGNAL(clicked()), this, SLOT(onClearProjectedTableClicked()));

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT(onLogicModified()));
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));
  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndCloseEvent, this, SLOT(onSceneClosedEvent()));

  // Find parameters node or create it if there is none in the scene
  if (scene)
  {
    if (d->MRMLNodeComboBox_ParameterSet->currentNode())
    {
      this->setParameterNode(d->MRMLNodeComboBox_ParameterSet->currentNode());
    }
    else if (vtkMRMLNode* node = scene->GetFirstNodeByClass("vtkMRMLDrrImageComputationNode"))
    {
      this->setParameterNode(node);
    }
    else
    {
      vtkMRMLNode* newNode = scene->AddNewNodeByClass("vtkMRMLDrrImageComputationNode");
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(node);

  // Set parameter node to children widgets (PlastimatchParameters)
  d->PlastimatchParametersWidget->setParameterNode(node);
 
  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect( parameterNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set selected MRML nodes in comboboxes in the parameter set if it was nullptr there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (parameterNode)
  {
    if (!parameterNode->GetBeamNode())
    {
      vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
//      qvtkConnect( beamNode, vtkMRMLRTBeamNode::BeamTransformModified, this, SLOT(updateNormalAndVupVectors()));
      parameterNode->SetAndObserveBeamNode(beamNode);
    }
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  // Enable widgets
  d->CheckBox_ShowDrrMarkups->setEnabled(parameterNode);
  d->CollapsibleButton_ReferenceInput->setEnabled(parameterNode);
  d->CollapsibleButton_GeometryBasicParameters->setEnabled(parameterNode);
  d->PlastimatchParametersWidget->setEnabled(parameterNode);
  d->PushButton_ComputeDrr->setEnabled(parameterNode);
  
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (parameterNode->GetBeamNode())
  {
    d->MRMLNodeComboBox_RtBeam->setCurrentNode(parameterNode->GetBeamNode());
  }

  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CtVolume->currentNode());
  if (!ctVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }

  // Update widgets info from parameter node
  d->SliderWidget_IsocenterImagerDistance->setValue(parameterNode->GetIsocenterImagerDistance());

  int imagerResolution[2] = {};
  double imagerRes[2] = {};
  parameterNode->GetImagerResolution(imagerResolution);
  imagerRes[0] = static_cast<double>(imagerResolution[0]);
  imagerRes[1] = static_cast<double>(imagerResolution[1]);
  d->CoordinatesWidget_ImagerResolution->setCoordinates(imagerRes);
  d->CoordinatesWidget_ImagerSpacing->setCoordinates(parameterNode->GetImagerSpacing());

  d->RangeWidget_ImageWindowColumns->setMinimum(0.);
  d->RangeWidget_ImageWindowColumns->setMaximum(double(imagerResolution[0] - 1));
  d->RangeWidget_ImageWindowRows->setMinimum(0.);
  d->RangeWidget_ImageWindowRows->setMaximum(double(imagerResolution[1] - 1));

  bool useImageWindow = parameterNode->GetImageWindowFlag();
  int imageWindow[4] = {};
  parameterNode->GetImageWindow(imageWindow);

  d->GroupBox_ImageWindowParameters->setChecked(useImageWindow);
  if (!useImageWindow)
  {
    d->RangeWidget_ImageWindowColumns->setValues( 0., double(imagerResolution[0] - 1));
    d->RangeWidget_ImageWindowRows->setValues( 0., double(imagerResolution[1] - 1));
  }
  else
  {
    d->RangeWidget_ImageWindowColumns->setValues( 
      static_cast<double>(std::max<int>( 0, imageWindow[0])),
      static_cast<double>(std::min<int>( imagerResolution[0] - 1, imageWindow[2])));
    d->RangeWidget_ImageWindowRows->setValues( 
      static_cast<double>(std::max<int>( 0, imageWindow[1])), 
      static_cast<double>(std::min<int>( imagerResolution[1] - 1, imageWindow[3])));
  }
  
  // update RT beam from camera button
  vtkMRMLCameraNode* cameraNode = vtkMRMLCameraNode::SafeDownCast(d->MRMLNodeComboBox_Camera->currentNode());
  d->PushButton_UpdateBeamFromCamera->setEnabled(cameraNode);

  // Update Markups imager plan intersection widgets

  // Plastimatch Intrinsic Matrix
  vtkNew<vtkMatrix4x4> mat;
  bool res = d->logic()->GetPlastimatchIntrinsicMatrix(parameterNode, mat);
  d->PlastimatchParametersWidget->setEnabled(res);
  d->PlastimatchParametersWidget->setIntrinsicMatrix(mat);
  // Plastimatch Extrinsic Matrix
  res = d->logic()->GetPlastimatchExtrinsicMatrix(parameterNode, mat);
  d->PlastimatchParametersWidget->setEnabled(res);
  d->PlastimatchParametersWidget->setExtrinsicMatrix(mat);
  // Plastimatch Projection Matrix
  res = d->logic()->GetPlastimatchProjectionMatrix(parameterNode, mat);
  d->PlastimatchParametersWidget->setEnabled(res);
  d->PlastimatchParametersWidget->setProjectionMatrix(mat);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onSceneClosedEvent()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  this->updateWidgetFromMRML();
}

/// RTBeam Node (RTBeam or RTIonBeam) changed
void qSlicerDrrImageComputationModuleWidget::onRTBeamNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

//  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
//  if (!shNode)
//  {
//    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
//    return;
//  }

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    parameterNode->SetAndObserveBeamNode(nullptr); // Disable imager and image markups update, VUP and normal vectors recalculation
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(node);
  Q_UNUSED(ionBeamNode);

  parameterNode->SetAndObserveBeamNode(beamNode); // Update imager and image markups, DRR arguments in logic
  parameterNode->Modified();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::enter()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  this->Superclass::enter();
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::exit()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  this->Superclass::exit();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onCtVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!volumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid reference CT volume node";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onParameterNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  this->setParameterNode(parameterNode);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onCameraNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  vtkMRMLCameraNode* cameraNode = vtkMRMLCameraNode::SafeDownCast(node);
  if (!cameraNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid camera node";
    d->PushButton_UpdateBeamFromCamera->setEnabled(false);
    parameterNode->SetAndObserveCameraNode(nullptr);
    return;
  }

  parameterNode->SetAndObserveCameraNode(cameraNode);
  d->PushButton_UpdateBeamFromCamera->setEnabled(node);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onEnter()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

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

  vtkMRMLDrrImageComputationNode* parameterNode = nullptr; 
  // Try to find one in the scene
  if (vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass( 0, "vtkMRMLDrrImageComputationNode"))
  {
    parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);
  }

  if (parameterNode && parameterNode->GetBeamNode())
  {
    // First thing first: update normal and vup vectors for parameter node
    // in case observed beam node transformation has been modified
    d->logic()->UpdateNormalAndVupVectors(parameterNode);
  }

  // Create DRR markups nodes
  d->logic()->CreateMarkupsNodes(parameterNode);

  // All required data for GUI is initiated
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onIsocenterImagerDistanceValueChanged(double value)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  parameterNode->SetIsocenterImagerDistance(value); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onImagerSpacingChanged(double* spacing)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  double s[2] = { spacing[0], spacing[1] }; // columns, rows
  parameterNode->SetImagerSpacing(s); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onShowMarkupsToggled(bool toggled)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  // Show/hide imager and image markups
  d->logic()->ShowMarkupsNodes(toggled);
}

/// \brief Setup imager resolution (dimention)
/// \param dimention: dimention[0] = columns, dimention[1] = rows
void qSlicerDrrImageComputationModuleWidget::onImagerResolutionChanged(double* res)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imagerResolution[2] = { static_cast<int>(res[0]), static_cast<int>(res[1]) }; // x, y

  parameterNode->SetImagerResolution(imagerResolution); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onUpdateBeamFromCameraClicked()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  // Update DRR RT beam parameters from observed 3D camera node data
  if (d->logic()->UpdateBeamFromCamera(parameterNode))
  {
    d->logic()->UpdateMarkupsNodes(parameterNode); // Update imager and image markups, DRR arguments
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onImageWindowColumnsValuesChanged(double start_column, double end_column)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  parameterNode->GetImageWindow(imageWindow);
  
  imageWindow[0] = start_column;
  imageWindow[2] = end_column;

  parameterNode->SetImageWindow(imageWindow); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onImageWindowRowsValuesChanged( double start_row, double end_row)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  parameterNode->GetImageWindow(imageWindow);
  
  imageWindow[1] = start_row;
  imageWindow[3] = end_row;

  parameterNode->SetImageWindow(imageWindow); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onUseImageWindowToggled(bool value)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (value)
  {
    int imagerResolution[2] = {};
    int imageWindow[4] = {};
    parameterNode->GetImagerResolution(imagerResolution);

    double columns[2], rows[2];
    d->RangeWidget_ImageWindowColumns->values( columns[0], columns[1]);
    d->RangeWidget_ImageWindowRows->values( rows[0], rows[1]);

    imageWindow[0] = static_cast<int>(columns[0]); // c1 = x1
    imageWindow[1] = static_cast<int>(rows[0]); // r1 = y1
    imageWindow[2] = static_cast<int>(columns[1]); // c2 = x2
    imageWindow[3] = static_cast<int>(rows[1]); // r2 = y2

    parameterNode->SetImageWindow(imageWindow);
  }
  else
  {
//    const double* window = d->CoordinatesWidget_ImagerResolution->coordinates();
//    imageWindow[0] = 0; // c1 = x1
//    imageWindow[1] = 0; // r1 = y1
//    imageWindow[2] = static_cast<int>(window[0] - 1.); // c2 = x2
//    imageWindow[3] = static_cast<int>(window[1] - 1.); // r2 = y2
  }

  parameterNode->SetImageWindowFlag(value); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onComputeDrrClicked()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CtVolume->currentNode());

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (!parameterNode->GetBeamNode())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced parameter's beam node";
    return;
  }

  if (!ctVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }
  
  QApplication::setOverrideCursor(Qt::WaitCursor);

  vtkMRMLScalarVolumeNode* drrImageNode = d->logic()->ComputePlastimatchDRR( parameterNode, ctVolumeNode);
  if (drrImageNode)
  {
    // node is OK
    QApplication::restoreOverrideCursor();
    return;
  }
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onMarkupsControlPointSelectionChanged(int index)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(d->SimpleMarkupsWidget_PointCoordinates->currentNode());
  if (!markupsNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid markups fiducial node";
    return;
  }

  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CtVolume->currentNode());
  if (!ctVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }

  double pointPos[3] = {};
  markupsNode->GetNthControlPointPosition(index, pointPos);

  if (!d->CheckPointWithinVolumeBounds(ctVolumeNode, pointPos))
  {
    qCritical() << Q_FUNC_INFO << ": Point is out of volume bounds!";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onMarkupsNodeChanged()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(d->SimpleMarkupsWidget_PointCoordinates->currentNode());

  d->TableWidget_ProjectedPointsCoordinates->clear();
  d->TableWidget_ProjectedPointsCoordinates->setHorizontalHeaderLabels( QStringList() << tr("Original label") << tr("R") << tr("A") << tr("S") \
    << tr("Width") << tr("Height") << tr("Column") << tr("Row") << tr("Status") );

  d->PushButton_ClearProjectedTableWidget->setEnabled(false);
  if (markupsNode)
  {
    d->PushButton_ProjectControlPoints->setEnabled(markupsNode->GetNumberOfControlPoints());
  }
  else
  {
    d->PushButton_ProjectControlPoints->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onProjectMarkupsControlPointsClicked()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(d->SimpleMarkupsWidget_PointCoordinates->currentNode());
  if (!markupsNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid markups fiducial node";
    return;
  }

  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CtVolume->currentNode());
  if (!ctVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }

  d->TableWidget_ProjectedPointsCoordinates->clear();

  std::list<int> list;
  bool res = d->GetMarkupsWidgetRowList(list);
  if (res)
  {
    d->PushButton_ProjectControlPoints->setEnabled(false);
    d->PushButton_ClearProjectedTableWidget->setEnabled(true);
  }

  vtkMRMLMarkupsFiducialNode* projectedPointsNode = nullptr;
  if (d->CheckBox_CreateValidProjectionsMarkupsNode->isChecked())
  {
    std::string name = std::string(markupsNode->GetName()) + "_Projected";
    projectedPointsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLMarkupsFiducialNode", name.c_str()));
  }

  vtkMRMLTableNode* projectedPointsTableNode = nullptr;
  vtkTable* projectionTable = nullptr;
  if (d->CheckBox_CreateValidProjectionsTableNode->isChecked())
  {
    projectedPointsTableNode = d->logic()->CreateProjectionsTableNode(parameterNode, ctVolumeNode);
  }
  if (projectedPointsTableNode)
  {
    projectionTable = projectedPointsTableNode->GetTable();
    if (list.size())
    {
      projectionTable->SetNumberOfRows(list.size());
    }
  }

  if (d->CheckBox_CreateValidProjectionsTableNode->isChecked() && !projectionTable)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid vtkTable to fill projection data";
    return;
  }

  d->TableWidget_ProjectedPointsCoordinates->setColumnCount(PROJECTED_POINT_COLUMNS);
  d->TableWidget_ProjectedPointsCoordinates->setRowCount(list.size());
  d->TableWidget_ProjectedPointsCoordinates->setHorizontalHeaderLabels( QStringList() << tr("Original label") << tr("R") << tr("A") << tr("S") \
    << tr("Width") << tr("Height") << tr("Column") << tr("Row") << tr("Status") );
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
  d->TableWidget_ProjectedPointsCoordinates->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);


  int projectedRowCount = 0;
  for (int cpIndex : list)
  {
    double pointPos[3] = {};
    markupsNode->GetNthControlPointPosition( cpIndex, pointPos);

    if (!d->CheckPointWithinVolumeBounds(ctVolumeNode, pointPos))
    {
      d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_STATUS_COLUMN, new QTableWidgetItem(tr("Point is out of volume bounds!")));
      if (projectionTable)
      {
        projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_STATUS_COLUMN, "Point is out of volume bounds!");
      }
      projectedRowCount++;
      continue;
    }
    double pointImagerIntersection[3] = {};
    double offsetFromOrigin[2] = {};
    double offsetRowColumn[2] = {};

    res = d->logic()->GetRayIntersectWithImagerPlane(parameterNode, pointPos, pointImagerIntersection);
    QString msg = res ? tr("Intersection found") : tr("No intersection");

    if (res && d->logic()->GetPointOffsetFromImagerOrigin(parameterNode, pointPos, offsetFromOrigin, offsetRowColumn))
    {
      msg = tr("Projection is valid");
    }
    else
    {
      d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_STATUS_COLUMN, new QTableWidgetItem(msg));
      if (projectionTable)
      {
        std::string strMsg = msg.toStdString(); 
        projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_STATUS_COLUMN, strMsg.c_str());
      }
      projectedRowCount++;
      continue;
    }

    if (projectedPointsNode)
    {
      projectedPointsNode->AddControlPoint(pointImagerIntersection, markupsNode->GetNthControlPointLabel(cpIndex));
    }

    QTableWidgetItem* itemLabel = new QTableWidgetItem(QString::fromStdString(markupsNode->GetNthControlPointLabel(cpIndex)));
    QTableWidgetItem* itemR = new QTableWidgetItem(QString::number(pointImagerIntersection[0], 'g', 4));
    QTableWidgetItem* itemA = new QTableWidgetItem(QString::number(pointImagerIntersection[1], 'g', 4));
    QTableWidgetItem* itemS = new QTableWidgetItem(QString::number(pointImagerIntersection[2], 'g', 4));
    QTableWidgetItem* itemWidth = new QTableWidgetItem(QString::number(offsetFromOrigin[0], 'g', 4));
    QTableWidgetItem* itemHeight = new QTableWidgetItem(QString::number(offsetFromOrigin[1], 'g', 4));
    QTableWidgetItem* itemRow = new QTableWidgetItem(QString::number(offsetRowColumn[0], 'g', 4));
    QTableWidgetItem* itemColumn = new QTableWidgetItem(QString::number(offsetRowColumn[1], 'g', 4));
    QTableWidgetItem* itemStatus = new QTableWidgetItem(msg);

    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_LABEL_COLUMN, itemLabel);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_X_COLUMN, itemR);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_Y_COLUMN, itemA);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_Z_COLUMN, itemS);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_WIDTH_COLUMN, itemWidth);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_HEIGHT_COLUMN, itemHeight);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_ROW_COLUMN, itemRow);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_COLUMN_COLUMN, itemColumn);
    d->TableWidget_ProjectedPointsCoordinates->setItem(projectedRowCount, PROJECTED_POINT_STATUS_COLUMN, itemStatus);

    if (projectionTable)
    {
      std::string strMsg = msg.toStdString();
      vtkVariant strLabel(markupsNode->GetNthControlPointLabel(cpIndex));
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_LABEL_COLUMN, strLabel);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_X_COLUMN, pointImagerIntersection[0]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_Y_COLUMN, pointImagerIntersection[1]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_Z_COLUMN, pointImagerIntersection[2]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_WIDTH_COLUMN, offsetFromOrigin[0]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_HEIGHT_COLUMN, offsetFromOrigin[1]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_ROW_COLUMN, offsetRowColumn[0]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_COLUMN_COLUMN, offsetRowColumn[1]);
      projectionTable->SetValue(projectedRowCount, PROJECTED_POINT_STATUS_COLUMN, strMsg.c_str());
    }
    projectedRowCount++;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onClearProjectedTableClicked()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  d->TableWidget_ProjectedPointsCoordinates->clear();
  d->TableWidget_ProjectedPointsCoordinates->setRowCount(0);
  d->TableWidget_ProjectedPointsCoordinates->setHorizontalHeaderLabels( QStringList() << tr("Original label") << tr("R") << tr("A") << tr("S") \
    << tr("Width") << tr("Height") << tr("Column") << tr("Row") << tr("Status") );
  d->PushButton_ProjectControlPoints->setEnabled(true);
  d->PushButton_ClearProjectedTableWidget->setEnabled(false);
}
