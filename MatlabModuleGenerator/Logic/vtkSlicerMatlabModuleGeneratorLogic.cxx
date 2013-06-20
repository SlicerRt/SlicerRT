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

// MatlabModuleGenerator Logic includes
#include "vtkSlicerMatlabModuleGeneratorLogic.h"

// MRML includes

// VTK includes
#include <vtkNew.h>
#include <vtksys/SystemTools.hxx>

// STD includes
#include <cassert>

// Slicer includes
#include "vtkSlicerConfigure.h" // For Slicer_CLIMODULES_SUBDIR

static const std::string TEMPLATE_NAME="MatlabModuleTemplate";
static const std::string MODULE_PROXY_TEMPLATE_EXTENSION=".bat";
static const std::string MODULE_SCRIPT_TEMPLATE_EXTENSION=".m";
static const std::string MODULE_DEFINITION_TEMPLATE_EXTENSION=".xml";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMatlabModuleGeneratorLogic);

//----------------------------------------------------------------------------
vtkSlicerMatlabModuleGeneratorLogic::vtkSlicerMatlabModuleGeneratorLogic()
: MatlabScriptDirectory(NULL)
, MatlabExecutablePath(NULL)
{
}

//----------------------------------------------------------------------------
vtkSlicerMatlabModuleGeneratorLogic::~vtkSlicerMatlabModuleGeneratorLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMatlabModuleGeneratorLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMatlabModuleGeneratorLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMatlabModuleGeneratorLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMatlabModuleGeneratorLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMatlabModuleGeneratorLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerMatlabModuleGeneratorLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{  
}

//---------------------------------------------------------------------------
const char* vtkSlicerMatlabModuleGeneratorLogic
::GetMatlabScriptDirectory()
{
  // Find out where the MatlabCommander CLI is (as the scripts must be in the same director as the MatlabCommander CLI)
  // Retrieve the bin directory from the known path of the share directory 
  std::string matlabScriptDirBase=this->GetModuleShareDirectory()+"/../../../../"+Slicer_CLIMODULES_BIN_DIR;
  matlabScriptDirBase=vtksys::SystemTools::CollapseFullPath(matlabScriptDirBase.c_str());
  std::string matlabScriptDir;
  // If Debug or Release subdirectory exists below lib/Slicer-4.2/cli-modules directory then assume
  // it is the build tree in windows (where executables are in a Debug or Release subdirectory)
  // otherwise just use the parent (lib/Slicer-4.2/cli-modules) directory.
#ifdef _DEBUG
  matlabScriptDir+=matlabScriptDirBase+"/Debug";
#else
  matlabScriptDir+=matlabScriptDirBase+"/Release";
#endif
  if (!vtksys::SystemTools::FileExists(matlabScriptDir.c_str(),false))
  {
    // Debug or Release subdirectory doesn't exist, so it must be an installed module
    matlabScriptDir=matlabScriptDirBase;
  }
  SetMatlabScriptDirectory(matlabScriptDir.c_str());
  return this->MatlabScriptDirectory;
}

//---------------------------------------------------------------------------
vtkStdString vtkSlicerMatlabModuleGeneratorLogic
::GenerateModule(const char* moduleName)
{  
  vtkStdString overallResult;

  vtkStdString result;

  CreateFileFromTemplate(this->GetModuleShareDirectory()+"/"+TEMPLATE_NAME+MODULE_PROXY_TEMPLATE_EXTENSION,
    std::string(this->GetMatlabScriptDirectory())+"/"+moduleName+MODULE_PROXY_TEMPLATE_EXTENSION, TEMPLATE_NAME, moduleName, result);
  overallResult+=result+"\n";
  
  CreateFileFromTemplate(this->GetModuleShareDirectory()+"/"+TEMPLATE_NAME+MODULE_SCRIPT_TEMPLATE_EXTENSION,
    std::string(this->GetMatlabScriptDirectory())+"/"+moduleName+MODULE_SCRIPT_TEMPLATE_EXTENSION, TEMPLATE_NAME, moduleName, result);
  overallResult+=result+"\n";

  CreateFileFromTemplate(this->GetModuleShareDirectory()+"/"+TEMPLATE_NAME+MODULE_DEFINITION_TEMPLATE_EXTENSION,
    std::string(this->GetMatlabScriptDirectory())+"/"+moduleName+MODULE_DEFINITION_TEMPLATE_EXTENSION, TEMPLATE_NAME, moduleName, result);
  overallResult+=result+"\n";

  return overallResult;
}

//#include <iostream>
//#include <fstream>

void vtkSlicerMatlabModuleGeneratorLogic
::CreateFileFromTemplate(const vtkStdString& templateFilename, const vtkStdString& targetFilename, const vtkStdString& originalString, const vtkStdString& modifiedString, vtkStdString &result)
{
  result.clear();

  // Open input file
  fstream templateFile;
  templateFile.open(templateFilename.c_str(), fstream::in);
  if (!templateFile.is_open())
  {
    result="Template file not found:\n "+templateFilename;
    return;
  }

  // Open output file
  std::ofstream targetFile;
  targetFile.open(targetFilename.c_str() );
  if (!targetFile.is_open())
  {
    result="Target file cannot be opened for writing:\n "+targetFilename;
    templateFile.close(); 
    return;
  }  

  // Copy line-by-line while replacing the original string with the modified string
  for (std::string line; std::getline(templateFile, line); ) 
  {
    if (line.length()>0 && originalString.length())
    {
      // search and replace
      size_t idx = 0;
      for (;;) 
      {
        idx = line.find( originalString, idx);
        if (idx == std::string::npos)  break;
        line.replace( idx, originalString.length(), modifiedString);
        idx += modifiedString.length();
      }
    }
    targetFile << line << std::endl;
  }

  // Close input and output files
  templateFile.close(); 
  targetFile.close(); 

  result="File created:\n "+targetFilename;
}
