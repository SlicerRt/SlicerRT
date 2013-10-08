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

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "SlicerRtCommon.h"
#include "ctkTest.h"
#include "qSlicerContoursModuleWidget.h"
#include <QApplication>
#include <vtkMRMLContourNode.h>
#include <vtkMRMLModelNode.h>
#include <vtksys/SystemTools.hxx>

// ITK includes
#if ITK_VERSION_MAJOR > 3
#include "itkFactoryRegistration.h"
#endif

// ----------------------------------------------------------------------------
class qSlicerContoursModuleWidgetTester: public QObject
{
  Q_OBJECT

public:
  int argc;
  char** argv;

protected:
  bool initialized;

private slots:
  void init();
  void cleanup();

  void testUI();
};

// ----------------------------------------------------------------------------
void qSlicerContoursModuleWidgetTester::init()
{
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
    return;
  }

  // Make sure NRRD reading works
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  initialized = true;
}

// ----------------------------------------------------------------------------
void qSlicerContoursModuleWidgetTester::cleanup()
{
}

// ----------------------------------------------------------------------------
void qSlicerContoursModuleWidgetTester::testUI()
{
  
}

// ----------------------------------------------------------------------------
int qSlicerContoursModuleWidgetTest(int argc, char *argv[])
{
  QApplication app(argc, argv);
  qSlicerContoursModuleWidgetTester tc;
  tc.argc = argc;
  tc.argv = argv;
  argc = 1; // Truncate all remaining entries, run all tests
  return QTest::qExec(&tc, argc, argv);
}

#include "moc_qSlicerContoursModuleWidgetTest.cxx"