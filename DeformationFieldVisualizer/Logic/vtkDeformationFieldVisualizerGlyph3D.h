#ifndef __vtkDeformationFieldVisualizerGlyph3D_h
#define __vtkDeformationFieldVisualizerGlyph3D_h

#include "vtkGlyph3D.h"
#include <vtkSmartPointer.h>
#include "vtkMinimalStandardRandomSequence.h"

#include "vtkSlicerDeformationFieldVisualizerModuleLogicExport.h"

class vtkMinimalStandardRandomSequence;

//------------------------------------------------------------------------------
class VTK_SLICER_DEFORMATIONFIELDVISUALIZER_MODULE_LOGIC_EXPORT vtkDeformationFieldVisualizerGlyph3D : public vtkGlyph3D
{
public:
  vtkTypeMacro(vtkDeformationFieldVisualizerGlyph3D,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDeformationFieldVisualizerGlyph3D *New();

  virtual int IsPointVisibleForListIndex(double vmag, vtkIdType ptId, int visibleListIndex);

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
  vtkDeformationFieldVisualizerGlyph3D();
  ~vtkDeformationFieldVisualizerGlyph3D() {};
  
  int PointMax;
  double ScaleFactor;
  bool ScaleDirectional;
  unsigned Seed;
  double MagnitudeMax;
  double MagnitudeMin;
  
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkDeformationFieldVisualizerGlyph3D(const vtkDeformationFieldVisualizerGlyph3D&);  // Not implemented.
  void operator=(const vtkDeformationFieldVisualizerGlyph3D&);  // Not implemented.
};

#endif
