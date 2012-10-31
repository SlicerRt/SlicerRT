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
class StackWalkerStringOutput : public StackWalker
{
public:
  StackWalkerStringOutput()
  {
    this->m_LastStackTraceString = "";
  };

  std::string GetLastStackTraceString()
  {
    return this->m_LastStackTraceString;
  };

  virtual void OnOutput(LPCSTR buffer)
  {
    m_LastStackTraceString = std::string(buffer);
  };

protected:
  std::string m_LastStackTraceString;
};



//----------------------------------------------------------------------------
class vtkLeakFinderObserver : public vtkDebugLeaksObserver
{
public:
  vtkLeakFinderObserver()
  {
    m_ObjectTraceEntries.clear();
    m_StackWalker = new StackWalkerStringOutput();
    m_OldDebugLeakObserver = NULL;
  };

  virtual ~vtkLeakFinderObserver()
  {
    m_ObjectTraceEntries.clear();
    m_OldDebugLeakObserver = NULL;

    if (m_StackWalker)
    {
      delete m_StackWalker;
      m_StackWalker = NULL;
    }
  };

  virtual void ConstructingObject(vtkObjectBase* o)
  {
    m_StackWalker->ShowCallstack();
    m_ObjectTraceEntries[o] = std::string(m_StackWalker->GetLastStackTraceString());

    if (m_OldDebugLeakObserver)
    {
      m_OldDebugLeakObserver->ConstructingObject(o);
    }
  };

  virtual void DestructingObject(vtkObjectBase* o)
  {
    m_ObjectTraceEntries.erase(o);

    if (m_OldDebugLeakObserver)
    {
      m_OldDebugLeakObserver->DestructingObject(o);
    }
  };

  std::string GetLeakReport()
  {
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
  };

  void SetOldDebugLeakObserver(vtkDebugLeaksObserver* oldObserver)
  {
    m_OldDebugLeakObserver = oldObserver;
  };

  vtkDebugLeaksObserver* GetOldDebugLeakObserver()
  {
    return m_OldDebugLeakObserver;
  };

protected:
  StackWalkerStringOutput* m_StackWalker;
  std::map<vtkObjectBase*, std::string> m_ObjectTraceEntries;
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
  vtkDebugLeaks::SetDebugLeaksObserver(this->Observer->GetOldDebugLeakObserver());
  this->Observer->SetOldDebugLeakObserver(NULL);
}

//----------------------------------------------------------------------------
std::string vtkLeakFinder::GetLeakReport()
{
  if (vtkDebugLeaks::GetDebugLeaksObserver())
  {
    return "Cannot get report while running. EndTracing needs to be called first.";
  }

  return this->Observer->GetLeakReport();//.c_str();
}
