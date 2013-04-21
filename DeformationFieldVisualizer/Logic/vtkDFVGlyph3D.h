#ifndef __vtkDFVGlyph3D_h
#define __vtkDFVGlyph3D_h

#include "vtkGlyph3D.h"
#include <vtkSmartPointer.h>
#include "vtkMinimalStandardRandomSequence.h"

#include "vtkSlicerDeformationFieldVisualizerModuleLogicExport.h"

class vtkMinimalStandardRandomSequence;

//------------------------------------------------------------------------------
class VTK_SLICER_DEFORMATIONFIELDVISUALIZER_MODULE_LOGIC_EXPORT vtkDFVGlyph3D : public vtkGlyph3D
{
public:
  vtkTypeMacro(vtkDFVGlyph3D,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDFVGlyph3D *New();

  virtual int IsPointVisible(double vmag, vtkIdType ptId, int visibleListIndex);

  vtkSetMacro(PointMax,int);
  vtkGetMacro(PointMax,int);  
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);
  vtkSetMacro(ScaleDirectional,bool);
  vtkGetMacro(ScaleDirectional,bool);
  vtkSetMacro(Seed,double);
  vtkGetMacro(Seed,double);
  vtkSetMacro(MagnitudeMax,double);
  vtkGetMacro(MagnitudeMax,double);
  vtkSetMacro(MagnitudeMin,double);
  vtkGetMacro(MagnitudeMin,double);
  
protected:
  vtkDFVGlyph3D();
  ~vtkDFVGlyph3D() {};
  
  int PointMax;
  double ScaleFactor;
  bool ScaleDirectional;
  unsigned Seed;
  double MagnitudeMax;
  double MagnitudeMin;
  
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkDFVGlyph3D(const vtkDFVGlyph3D&);  // Not implemented.
  void operator=(const vtkDFVGlyph3D&);  // Not implemented.
};

#endif
