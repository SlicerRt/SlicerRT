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

// SlicerRt includes
#include "vtkSlicerPatientHierarchyPluginHandler.h"
#include "vtkSlicerPatientHierarchyPlugin.h"

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

//----------------------------------------------------------------------------
vtkSlicerPatientHierarchyPluginHandler *vtkSlicerPatientHierarchyPluginHandler::Instance = NULL;

//----------------------------------------------------------------------------
class vtkSlicerPatientHierarchyPluginHandlerCleanup
{
public:
  inline void Use()
  {
  }

  ~vtkSlicerPatientHierarchyPluginHandlerCleanup()
  {
    if( vtkSlicerPatientHierarchyPluginHandler::GetInstance() )
    {
      vtkSlicerPatientHierarchyPluginHandler::SetInstance(NULL);
    }
  }
};
static vtkSlicerPatientHierarchyPluginHandlerCleanup vtkSlicerPatientHierarchyPluginHandlerCleanupGlobal;

//-----------------------------------------------------------------------------
vtkSlicerPatientHierarchyPluginHandler* vtkSlicerPatientHierarchyPluginHandler::New()
{
  return vtkSlicerPatientHierarchyPluginHandler::GetInstance();
}

//-----------------------------------------------------------------------------
vtkSlicerPatientHierarchyPluginHandler* vtkSlicerPatientHierarchyPluginHandler::GetInstance()
{
  if(!vtkSlicerPatientHierarchyPluginHandler::Instance) 
  {
    if(!vtkSlicerPatientHierarchyPluginHandler::Instance) 
    {
      vtkSlicerPatientHierarchyPluginHandlerCleanupGlobal.Use();

      // Need to call vtkObjectFactory::CreateInstance method because this
      // registers the class in the vtkDebugLeaks::MemoryTable.
      // Without this we would get a "Deleting unknown object" VTK warning on application exit
      // (in debug mode, with debug leak checking enabled).
      vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSlicerPatientHierarchyPluginHandler");
      if(ret)
      {
        vtkSlicerPatientHierarchyPluginHandler::Instance = static_cast<vtkSlicerPatientHierarchyPluginHandler*>(ret);
      }
      else
      {
        vtkSlicerPatientHierarchyPluginHandler::Instance = new vtkSlicerPatientHierarchyPluginHandler();   
      }

    }
  }
  // Return the instance
  return vtkSlicerPatientHierarchyPluginHandler::Instance;
}

//-----------------------------------------------------------------------------
void vtkSlicerPatientHierarchyPluginHandler::SetInstance(vtkSlicerPatientHierarchyPluginHandler* instance)
{
  if (vtkSlicerPatientHierarchyPluginHandler::Instance==instance)
  {
    return;
  }
  // Preferably this will be NULL
  if (vtkSlicerPatientHierarchyPluginHandler::Instance)
  {
    vtkSlicerPatientHierarchyPluginHandler::Instance->Delete();
  }
  vtkSlicerPatientHierarchyPluginHandler::Instance = instance;
  if (!instance)
  {
    return;
  }
  // User will call ->Delete() after setting instance
  instance->Register(NULL);
}

//-----------------------------------------------------------------------------
vtkSlicerPatientHierarchyPluginHandler::vtkSlicerPatientHierarchyPluginHandler()
{
  this->PluginList.clear();
}

//-----------------------------------------------------------------------------
vtkSlicerPatientHierarchyPluginHandler::~vtkSlicerPatientHierarchyPluginHandler()
{
  PatientHierarchyPluginListType::iterator pluginIt;
  for (pluginIt = this->PluginList.begin(); pluginIt != this->PluginList.end(); ++pluginIt)
  {
    (*pluginIt)->UnRegister(this);
  }
  this->PluginList.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerPatientHierarchyPluginHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
bool vtkSlicerPatientHierarchyPluginHandler::RegisterPlugin(vtkSlicerPatientHierarchyPlugin* plugin)
{
  if (plugin == NULL)
  {
    return false;
  }

  // Check if the same plugin has already been registered
  PatientHierarchyPluginListType::iterator pluginIt;
  for (pluginIt = this->PluginList.begin(); pluginIt != this->PluginList.end(); ++pluginIt)
  {
    if (plugin->GetName() && strcmp(plugin->GetName(), (*pluginIt)->GetName()) == 0)
    {
      vtkWarningWithObjectMacro(plugin, "RegisterPlugin: PatientHierarchy plugin " << plugin->GetName() << " is already registered");
      return false;
    }
  }

  if (plugin->GetName() == NULL)
  {
    vtkErrorWithObjectMacro(plugin, "RegisterPlugin: PatientHierarchy plugin cannot be registered with empty name!");
    return false;
  }

  // Make sure the plugin instance does not get deleted before the handler is destructed
  plugin->Register(this);

  // Add the plugin to the list
  this->PluginList.push_back(plugin);

  return true;
}

//---------------------------------------------------------------------------
vtkSlicerPatientHierarchyPlugin* vtkSlicerPatientHierarchyPluginHandler::GetPluginForAddToPatientHierarchyForNode(vtkMRMLNode* node)
{
  PatientHierarchyPluginListType::iterator pluginIt;
  vtkSlicerPatientHierarchyPlugin* mostSuitablePlugin = NULL;
  double bestConfidence = 0.0;
  for (pluginIt = this->PluginList.begin(); pluginIt != this->PluginList.end(); ++pluginIt)
  {
    double currentConfidence = (*pluginIt)->CanPluginAddNodeToPatientHierarchy(node);
    if (currentConfidence > bestConfidence)
    {
      bestConfidence = currentConfidence;
      mostSuitablePlugin = (*pluginIt);
    }
  }

  return mostSuitablePlugin;
}

//---------------------------------------------------------------------------
vtkSlicerPatientHierarchyPlugin* vtkSlicerPatientHierarchyPluginHandler::GetPluginForReparentInsidePatientHierarchyForNode(vtkMRMLHierarchyNode* node)
{
  PatientHierarchyPluginListType::iterator pluginIt;
  vtkSlicerPatientHierarchyPlugin* mostSuitablePlugin = NULL;
  double bestConfidence = 0.0;
  for (pluginIt = this->PluginList.begin(); pluginIt != this->PluginList.end(); ++pluginIt)
  {
    double currentConfidence = (*pluginIt)->CanPluginReparentNodeInsidePatientHierarchy(node);
    if (currentConfidence > bestConfidence)
    {
      bestConfidence = currentConfidence;
      mostSuitablePlugin = (*pluginIt);
    }
  }

  return mostSuitablePlugin;
}
