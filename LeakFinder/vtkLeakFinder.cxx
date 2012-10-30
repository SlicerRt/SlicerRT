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
class vtkStackWalkerStringOutput : public StackWalker, public vtkObject
{
public:
  static vtkStackWalkerStringOutput *New();

  vtkGetStringMacro(LastStackTraceString);

  virtual void OnOutput(LPCSTR buffer)
  {
    this->SetLastStackTraceString(buffer);
  };

protected:
  vtkStackWalkerStringOutput()
  {
    this->LastStackTraceString = NULL;
  };

  ~vtkStackWalkerStringOutput()
  {
    this->SetLastStackTraceString(NULL);
  };

  vtkSetStringMacro(LastStackTraceString);

protected:
  char* LastStackTraceString;
};

vtkStandardNewMacro(vtkStackWalkerStringOutput);



//----------------------------------------------------------------------------
class vtkLeakFinderObserver : public vtkDebugLeaksObserver
{
public:
  vtkLeakFinderObserver()
  {
    m_ObjectTraceEntries.clear();
    m_StackWalker = vtkStackWalkerStringOutput::New();
  };

  virtual ~vtkLeakFinderObserver()
  {
    m_ObjectTraceEntries.clear();

    if (m_StackWalker)
    {
      m_StackWalker->Delete();
      m_StackWalker = NULL;
    }
  };

  virtual void ConstructingObject(vtkObjectBase* o)
  {
    m_StackWalker->ShowCallstack();
    m_ObjectTraceEntries[o] = std::string(m_StackWalker->GetLastStackTraceString());
  };

  virtual void DestructingObject(vtkObjectBase* o)
  {
    m_ObjectTraceEntries.erase(o);
  };

  std::string GetLeakReport()
  {
    std::string report("");
    std::map<vtkObjectBase*, std::string>::iterator it;
    for (it=m_ObjectTraceEntries.begin(); it!=m_ObjectTraceEntries.end(); ++it)
    {
      std::stringstream ss;
      ss.setf(ios::hex,ios::basefield);
      ss << "Pointer: " << it->first << std::endl;
      ss << "Stack trace: " << std::endl << it->second << std::endl;
      report.append(ss.str());
    }
    return report;
  };

protected:
  vtkStackWalkerStringOutput* m_StackWalker;
  std::map<vtkObjectBase*, std::string> m_ObjectTraceEntries;
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
  vtkDebugLeaks::SetDebugLeaksObserver(this->Observer);
}

//----------------------------------------------------------------------------
void vtkLeakFinder::EndTracing()
{
  vtkDebugLeaks::SetDebugLeaksObserver(NULL);
}

//----------------------------------------------------------------------------
std::string vtkLeakFinder::GetLeakReport()
{
  if (vtkDebugLeaks::GetDebugLeaksObserver())
  {
    return "Cannot get report while running. EndTracing needs to be called first.";
  }

  return this->Observer->GetLeakReport();
}
