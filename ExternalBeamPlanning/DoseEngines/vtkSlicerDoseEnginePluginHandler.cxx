/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Andras Lasso, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "vtkSlicerDoseEnginePluginHandler.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkDataObject.h>

// ExternalBeamPlanning includes
#include "vtkSlicerAbstractDoseEngine.h"

//----------------------------------------------------------------------------
// The dose engine manager singleton.
// This MUST be default initialized to zero by the compiler and is
// therefore not initialized here.  The ClassInitialize and ClassFinalize methods handle this instance.
static vtkSlicerDoseEnginePluginHandler* vtkSlicerDoseEnginePluginHandlerInstance;

//----------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is necessary.
unsigned int vtkSlicerDoseEnginePluginHandlerInitialize::Count;

//----------------------------------------------------------------------------
// Implementation of vtkSlicerDoseEnginePluginHandlerInitialize class.
//----------------------------------------------------------------------------
vtkSlicerDoseEnginePluginHandlerInitialize::vtkSlicerDoseEnginePluginHandlerInitialize()
{
  if (++Self::Count == 1)
    {
    vtkSlicerDoseEnginePluginHandler::classInitialize();
    }
}

//----------------------------------------------------------------------------
vtkSlicerDoseEnginePluginHandlerInitialize::~vtkSlicerDoseEnginePluginHandlerInitialize()
{
  if (--Self::Count == 0)
    {
    vtkSlicerDoseEnginePluginHandler::classFinalize();
    }
}

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkSlicerDoseEnginePluginHandler);

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Up the reference count so it behaves like New
vtkSlicerDoseEnginePluginHandler* vtkSlicerDoseEnginePluginHandler::New()
{
  vtkSlicerDoseEnginePluginHandler* ret = vtkSlicerDoseEnginePluginHandler::GetInstance();
  ret->Register(NULL);
  return ret;
}

//----------------------------------------------------------------------------
// Return the single instance of the vtkSlicerDoseEnginePluginHandler
vtkSlicerDoseEnginePluginHandler* vtkSlicerDoseEnginePluginHandler::GetInstance()
{
  if(!vtkSlicerDoseEnginePluginHandlerInstance)
    {
    // Try the factory first
    vtkSlicerDoseEnginePluginHandlerInstance = (vtkSlicerDoseEnginePluginHandler*)vtkObjectFactory::CreateInstance("vtkSlicerDoseEnginePluginHandler");
    // if the factory did not provide one, then create it here
    if(!vtkSlicerDoseEnginePluginHandlerInstance)
      {
      vtkSlicerDoseEnginePluginHandlerInstance = new vtkSlicerDoseEnginePluginHandler;
      }
    }
  // return the instance
  return vtkSlicerDoseEnginePluginHandlerInstance;
}

//----------------------------------------------------------------------------
vtkSlicerDoseEnginePluginHandler::vtkSlicerDoseEnginePluginHandler()
{
}

//----------------------------------------------------------------------------
vtkSlicerDoseEnginePluginHandler::~vtkSlicerDoseEnginePluginHandler()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDoseEnginePluginHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseEnginePluginHandler::classInitialize()
{
  // Allocate the singleton
  vtkSlicerDoseEnginePluginHandlerInstance = vtkSlicerDoseEnginePluginHandler::GetInstance();
}

//----------------------------------------------------------------------------
void vtkSlicerDoseEnginePluginHandler::classFinalize()
{
  vtkSlicerDoseEnginePluginHandlerInstance->Delete();
  vtkSlicerDoseEnginePluginHandlerInstance = 0;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseEnginePluginHandler::RegisterDoseEngine(vtkSlicerAbstractDoseEngine* engine)
{
  if (!engine)
  {
    vtkErrorMacro("RegisterDoseEngine failed: invalid input engine");
    return;
  }

  this->DoseEngines.push_back(engine);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseEnginePluginHandler::UnregisterDoseEngine(vtkSlicerAbstractDoseEngine* engine)
{
  for (DoseEngineListType::iterator engineIt = this->DoseEngines.begin(); engineIt != this->DoseEngines.end(); ++engineIt)
  {
    if (engineIt->GetPointer() == engine)
    {
      // Found
      this->DoseEngines.erase(engineIt);
      return;
    }
  }
  vtkWarningMacro("vtkSlicerDoseEnginePluginHandler::UnregisterDoseEngine failed: engine not found");
}

//----------------------------------------------------------------------------
vtkSlicerAbstractDoseEngine* vtkSlicerDoseEnginePluginHandler::GetDoseEngineByName(const char* name)
{
  for (DoseEngineListType::iterator engineIt = this->DoseEngines.begin(); engineIt != this->DoseEngines.end(); ++engineIt)
  {
    if (!engineIt->GetPointer()->GetName())
    {
      vtkErrorMacro("GetDoseEngineByName: Undefined name for dose engine " << engineIt->GetPointer()->GetClassName());
      continue;
    }

    if (!strcmp(engineIt->GetPointer()->GetName(), name))
    {
      return engineIt->GetPointer();
    }
  }

  vtkErrorMacro("GetDoseEngineByName: No dose engine found with name " << name);
  return NULL;
}

//----------------------------------------------------------------------------
const vtkSlicerDoseEnginePluginHandler::DoseEngineListType& vtkSlicerDoseEnginePluginHandler::GetDoseEngines()
{
  return this->DoseEngines;
}
