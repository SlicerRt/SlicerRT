#include "vtkLeakFinder.h"

#include "StackWalker.h"

// VTK includes
#include "vtkObjectFactory.h"
#include "vtkDebugLeaks.h"

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLeakFinder);

//----------------------------------------------------------------------------
/*!
 * \brief Class that extracts the call stack and saves it in a string.
 */
class StackWalkerStringOutput : public StackWalker
{
public:
  StackWalkerStringOutput()
  {
    m_LastStackTraceString = "";
  }

  std::string GetLastStackTraceString()
  {
    return m_LastStackTraceString;
  }

  /// Resets the output string. Has to be called after getting the output and before the next ShowCallstack call
  void ResetLastStackTraceString()
  {
    m_LastStackTraceString = "";
  }

  /// Overridden function that appends the call stack element to the output string
  virtual void OnOutput(LPCSTR buffer)
  {
    m_LastStackTraceString.append(buffer);
  }

protected:
  /// Output string containing the call stack
  std::string m_LastStackTraceString;
};



//----------------------------------------------------------------------------
/*!
 * \brief Debug leaks observer variant that keeps a record of the created objects and the call stacks at the point of creation
 */
class vtkLeakFinderObserver : public vtkDebugLeaksObserver
{
public:
  vtkLeakFinderObserver()
  {
    m_ObjectTraceEntries.clear();
    m_StackWalker = new StackWalkerStringOutput();
    m_OldDebugLeakObserver = NULL;
  }

  virtual ~vtkLeakFinderObserver()
  {
    m_ObjectTraceEntries.clear();
    m_OldDebugLeakObserver = NULL;

    if (m_StackWalker)
    {
      delete m_StackWalker;
      m_StackWalker = NULL;
    }
  }

  /// Callback function that is called every time a VTK class is instantiated
  virtual void ConstructingObject(vtkObjectBase* o)
  {
    m_StackWalker->ShowCallstack();
    m_ObjectTraceEntries[o] = m_StackWalker->GetLastStackTraceString();
    m_StackWalker->ResetLastStackTraceString();

    if (m_OldDebugLeakObserver)
    {
      m_OldDebugLeakObserver->ConstructingObject(o);
    }
  }

  /// Callback function that is called every time a VTK class is deleted
  virtual void DestructingObject(vtkObjectBase* o)
  {
    m_ObjectTraceEntries.erase(o);

    if (m_OldDebugLeakObserver)
    {
      m_OldDebugLeakObserver->DestructingObject(o);
    }
  }

  /// Callback function that is called at the last possible moment before the application exits.
  /// This function ends the tracing and writes the leak report to a file.
  /// Note: This callback function is only called when a patch is applied to VTK defining the signature of this
  ///  function in the vtkDebugLeaksObserver abstract class, and calling it in vtkDebugLeaks::ClassFinalize()
  virtual void Finalizing()
  {
    this->RestoreOldObserver();

    std::string report = GetLeakReport();

    std::ofstream f;
    f.open("D:\\trace.log", ios::trunc);
    f << this->GetLeakReport();
    f.close();
  }

  /// Returns a string containing information (pointer, type and call stack at the point of creation)
  /// of objects that have been created but not deleted between registering and unregistering of this observer class
  std::string GetLeakReport()
  {
    if (vtkDebugLeaks::GetDebugLeaksObserver() != m_OldDebugLeakObserver)
    {
      return "Cannot get report while running. Observer needs to be disconnected (vtkLeakFinder::EndTracing called) first.";
    }

    std::string report("");
    std::map<vtkObjectBase*, std::string>::iterator it;
    for (it=m_ObjectTraceEntries.begin(); it!=m_ObjectTraceEntries.end(); ++it)
    {
      std::stringstream ss;
      ss.setf(ios::hex,ios::basefield);
      ss << "Pointer: " << it->first << " (type: " << it->first->GetClassName() << ")" << std::endl;
      ss << "Stack trace: " << std::endl << it->second << std::endl << std::endl;
      report.append(ss.str());
    }
    return report;
  }

  /// Saves the previously set observer (its callbacks are also called)
  void SetOldDebugLeakObserver(vtkDebugLeaksObserver* oldObserver)
  {
    m_OldDebugLeakObserver = oldObserver;
  }

  /// Release this observer and restore the previous one that was saved
  void RestoreOldObserver() 
  {
    vtkDebugLeaks::SetDebugLeaksObserver(m_OldDebugLeakObserver);
    this->SetOldDebugLeakObserver(NULL);
  }

protected:
  /// Stack walker object that extracts the call stack
  StackWalkerStringOutput* m_StackWalker;

  /// Map containing the constructed VTK objects and the call stacks at the point of their creation
  std::map<vtkObjectBase*, std::string> m_ObjectTraceEntries;

  /// Previously set debug leaks observer
  vtkDebugLeaksObserver* m_OldDebugLeakObserver;
};



//----------------------------------------------------------------------------
vtkLeakFinder::vtkLeakFinder()
{
  this->Observer = NULL;
  this->Observer = new vtkLeakFinderObserver();
}

//----------------------------------------------------------------------------
vtkLeakFinder::~vtkLeakFinder()
{
  if (this->Observer)
  {
    delete this->Observer;
    this->Observer = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkLeakFinder::StartTracing()
{
  if (vtkDebugLeaks::GetDebugLeaksObserver())
  {
    this->Observer->SetOldDebugLeakObserver(vtkDebugLeaks::GetDebugLeaksObserver());
  }

  vtkDebugLeaks::SetDebugLeaksObserver(this->Observer);
}

//----------------------------------------------------------------------------
void vtkLeakFinder::EndTracing()
{
  this->Observer->RestoreOldObserver();
}

//----------------------------------------------------------------------------
std::string vtkLeakFinder::GetLeakReport()
{
  return this->Observer->GetLeakReport();
}
