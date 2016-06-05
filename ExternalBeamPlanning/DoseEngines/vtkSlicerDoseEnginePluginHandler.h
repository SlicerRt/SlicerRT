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

#ifndef __vtkSlicerDoseEnginePluginHandler_h
#define __vtkSlicerDoseEnginePluginHandler_h

#include "vtkSlicerExternalBeamPlanningDoseEnginesExport.h"

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

// STD includes
#include <vector>

class vtkSlicerAbstractDoseEngine;

/// \ingroup SegmentationCore
/// \brief Class that can create vtkSegmentationConverter instances.
///
/// This singleton class is a repository of all segmentation converter rules.
/// Singleton pattern adopted from vtkEventBroker class.
class VTK_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT vtkSlicerDoseEnginePluginHandler : public vtkObject
{
public:
  typedef std::vector< vtkSmartPointer<vtkSlicerAbstractDoseEngine> > DoseEngineListType;

  vtkTypeMacro(vtkSlicerDoseEnginePluginHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Add a dose engine.
  /// The handler will keep a reference to this dose engine object,
  /// and it will not be deleted until all these referring classes are deleted.
  void RegisterDoseEngine(vtkSlicerAbstractDoseEngine* rule);

  /// Remove a dose engine from the factory.
  /// This does not affect dose engines that have already been created.
  void UnregisterDoseEngine(vtkSlicerAbstractDoseEngine* rule);

  /// Get dose engine by its name
  vtkSlicerAbstractDoseEngine* GetDoseEngineByName(const char* name);

  /// Get all registered dose engines
  const DoseEngineListType& GetDoseEngines();

public:
  /// Return the singleton instance with no reference counting.
  static vtkSlicerDoseEnginePluginHandler* GetInstance();
  
  /// This is a singleton pattern New.  There will only be ONE
  /// reference to a vtkSlicerDoseEnginePluginHandler object per process.  Clients that
  /// call this must call Delete on the object so that the reference
  /// counting will work. The single instance will be unreferenced when
  /// the program exits.
  static vtkSlicerDoseEnginePluginHandler* New();

protected:
  vtkSlicerDoseEnginePluginHandler();
  ~vtkSlicerDoseEnginePluginHandler();
  vtkSlicerDoseEnginePluginHandler(const vtkSlicerDoseEnginePluginHandler&);
  void operator=(const vtkSlicerDoseEnginePluginHandler&);

  // Singleton management functions.
  static void classInitialize();
  static void classFinalize();

  friend class vtkSlicerDoseEnginePluginHandlerInitialize;
  typedef vtkSlicerDoseEnginePluginHandler Self;

  /// Registered converter rules
  DoseEngineListType DoseEngines;
};

/// Utility class to make sure qSlicerModuleManager is initialized before it is used.
class VTK_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT vtkSlicerDoseEnginePluginHandlerInitialize
{
public:
  typedef vtkSlicerDoseEnginePluginHandlerInitialize Self;

  vtkSlicerDoseEnginePluginHandlerInitialize();
  ~vtkSlicerDoseEnginePluginHandlerInitialize();
private:
  static unsigned int Count;
};

/// This instance will show up in any translation unit that uses
/// vtkSlicerDoseEnginePluginHandler.  It will make sure vtkSlicerDoseEnginePluginHandler is initialized
/// before it is used.
static vtkSlicerDoseEnginePluginHandlerInitialize vtkSlicerDoseEnginePluginHandlerInitializer;

#endif
