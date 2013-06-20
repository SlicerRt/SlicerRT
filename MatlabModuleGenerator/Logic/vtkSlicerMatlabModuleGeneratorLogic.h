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

// .NAME vtkSlicerMatlabModuleGeneratorLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMatlabModuleGeneratorLogic_h
#define __vtkSlicerMatlabModuleGeneratorLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerMatlabModuleGeneratorModuleLogicExport.h"


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_MATLABMODULEGENERATOR_MODULE_LOGIC_EXPORT vtkSlicerMatlabModuleGeneratorLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMatlabModuleGeneratorLogic *New();
  vtkTypeMacro(vtkSlicerMatlabModuleGeneratorLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Get the directory where the generated Matlab script (and corresponding proxy shell script and XML description) files are stored
  const char* GetMatlabScriptDirectory();
  
  /// Get/set path to Matlab.exe
  vtkGetStringMacro(MatlabExecutablePath);
  vtkSetStringMacro(MatlabExecutablePath);

  /// Generates and installs a Matlab module and returns the status
  vtkStdString GenerateModule(const char* moduleName);

protected:
  vtkSlicerMatlabModuleGeneratorLogic();
  virtual ~vtkSlicerMatlabModuleGeneratorLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// return true if successful
  bool CreateFileFromTemplate(const vtkStdString& templateFilename, const vtkStdString& targetFilename, const vtkStdString& originalString, const vtkStdString& modifiedString, vtkStdString &result);

  vtkSetStringMacro(MatlabScriptDirectory);

private:

  char* MatlabScriptDirectory;
  char* MatlabExecutablePath;

  vtkSlicerMatlabModuleGeneratorLogic(const vtkSlicerMatlabModuleGeneratorLogic&); // Not implemented
  void operator=(const vtkSlicerMatlabModuleGeneratorLogic&);               // Not implemented
};

#endif
