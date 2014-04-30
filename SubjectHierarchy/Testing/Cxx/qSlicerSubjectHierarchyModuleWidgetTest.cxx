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

#include "ctkTest.h"
#include "qSlicerSubjectHierarchyModuleWidget.h"
#include "ui_qSlicerSubjectHierarchyModule.h"

// SlicerRT includes
//#include "SlicerRtCommon.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerQt includes
#include <QApplication>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtksys/SystemTools.hxx>

// ITK includes
#include "itkFactoryRegistration.h"

// ----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_SubjectHierarchy
class qSlicerSubjectHierarchyModuleWidgetTester: public QObject
{
  Q_OBJECT

public:
//  qSlicerSubjectHierarchyModuleWidget* SlicerSubjectHierarchyModuleWidget;

public:
  int argc;
  char** argv;

protected:
  bool initialized;
  vtkMRMLScene* Scene;

private slots:
  void init();
  void cleanup();

  void testUI();
};

// ----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidgetTester::init()
{/*
  initialized = false;

  int argIndex = 1;
  std::ostream& outputStream = std::cout;
  std::ostream& errorStream = std::cerr;

  // TestSceneFile
  const char *testSceneFileName  = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TestSceneFile") == 0)
    {
      testSceneFileName = argv[argIndex+1];
      outputStream << "Test MRML scene file name: " << testSceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      testSceneFileName = "";
    }
  }
  else
  {
    errorStream << "Invalid arguments!" << std::endl;
    delete this->SlicerSubjectHierarchyModuleWidget;
    this->SlicerSubjectHierarchyModuleWidget = NULL;
    return;
  }

  const char *temporarySceneFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporarySceneFile") == 0)
    {
      temporarySceneFileName = argv[argIndex+1];
      outputStream << "Temporary scene file name: " << temporarySceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporarySceneFileName = "";
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    delete this->SlicerSubjectHierarchyModuleWidget;
    this->SlicerSubjectHierarchyModuleWidget = NULL;
    return;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  this->Scene = vtkMRMLScene::New();

  this->SlicerSubjectHierarchyModuleWidget = new qSlicerSubjectHierarchyModuleWidget;

  // Create logic
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> contoursLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  contoursLogic->SetMRMLScene(this->Scene);

  // TODO: Remove when subject hierarchy is integrated into Slicer core
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> subjectHierarchyLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  subjectHierarchyLogic->SetMRMLScene(this->Scene);

  // Load test scene into temporary scene
  this->Scene->SetURL(testSceneFileName);
  this->Scene->Import();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  this->Scene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  this->Scene->SetURL(temporarySceneFileName);
  this->Scene->Commit();

  this->SlicerSubjectHierarchyModuleWidget->testInit();
  this->SlicerSubjectHierarchyModuleWidget->setMRMLScene(this->Scene);
  this->SlicerSubjectHierarchyModuleWidget->enter();

  initialized = true;*/
}

// ----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidgetTester::cleanup()
{/*
  this->SlicerSubjectHierarchyModuleWidget->exit();
  QVERIFY(this->SlicerSubjectHierarchyModuleWidget != NULL);
  delete this->SlicerSubjectHierarchyModuleWidget;
  this->SlicerSubjectHierarchyModuleWidget = NULL;
  this->Scene->Delete();*/
}

// ----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidgetTester::testUI()
{/*
  if( this->initialized )
  {
    this->SlicerSubjectHierarchyModuleWidget->show();

    vtkMRMLContourNode* bodyNode = vtkMRMLContourNode::SafeDownCast(this->Scene->GetFirstNodeByName("BODY_Contour"));
    this->SlicerSubjectHierarchyModuleWidget->testSetContourNode(bodyNode);
    if( this->SlicerSubjectHierarchyModuleWidget->testGetCurrentContourNode() == NULL )
    {
      QFAIL("Unable to set contour node.");
    }

    // See if things work for index label map
    this->SlicerSubjectHierarchyModuleWidget->testSetTargetRepresentationType(vtkMRMLContourNode::IndexedLabelmap);

    if( !this->SlicerSubjectHierarchyModuleWidget->testGetDPointer()->MRMLNodeComboBox_ReferenceVolume->isVisible() )
    {
      QFAIL("Reference volume combobox is invisible when it should be visible.");
    }
    if( !this->SlicerSubjectHierarchyModuleWidget->testGetDPointer()->horizontalSlider_OversamplingFactor->isVisible() )
    {
      QFAIL("Oversampling factor widget is not visible.");
    }
    if( this->SlicerSubjectHierarchyModuleWidget->testGetDPointer()->pushButton_ApplyChangeRepresentation->isEnabled() )
    {
      QFAIL("Apply button is enabled when it shouldn't be.");
    }

    vtkMRMLScalarVolumeNode* doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->Scene->GetFirstNodeByName("Dose"));
    this->SlicerSubjectHierarchyModuleWidget->testSetReferenceVolumeNode(doseNode);
    if( this->SlicerSubjectHierarchyModuleWidget->testGetCurrentReferenceVolumeNode() != doseNode )
    {
      QFAIL("Selected reference volume is not \"Dose\".");
    }

    if( !this->SlicerSubjectHierarchyModuleWidget->testGetDPointer()->pushButton_ApplyChangeRepresentation->isEnabled() )
    {
      QFAIL("Apply button should be enabled after reference volume is set. It is not.");
    }

    this->SlicerSubjectHierarchyModuleWidget->hide();
  }*/
}

// ----------------------------------------------------------------------------
int qSlicerSubjectHierarchyModuleWidgetTest(int argc, char *argv[])
{
  QApplication app(argc, argv);
  qSlicerSubjectHierarchyModuleWidgetTester tc;
  tc.argc = argc;
  tc.argv = argv;
  argc = 1; // Truncate all remaining entries, run all tests
  return QTest::qExec(&tc, argc, argv);
}

#include "moc_qSlicerSubjectHierarchyModuleWidgetTest.cxx"
