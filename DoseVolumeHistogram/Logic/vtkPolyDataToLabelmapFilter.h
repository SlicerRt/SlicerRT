// .NAME vtkPolyDataToLabelmapFilter - Converts PolyData model to Labelmap image data
// .SECTION Description
// !!! Copied from ModelToLabelMap CLI module !!! TODO: Make the DVH plugin use that module and depend on it instead of having this class (or both use this class)


#ifndef __vtkPolyDataToLabelmapFilter_h
#define __vtkPolyDataToLabelmapFilter_h

// VTK includes
#include <vtkDataSetToImageFilter.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>

// ITK includes
#include <itkImage.h>
typedef itk::Image<unsigned char, 3> LabelImageType;

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkPolyDataToLabelmapFilter :
  public vtkDataSetToImageFilter
{
public:

  static vtkPolyDataToLabelmapFilter *New();
  vtkTypeMacro(vtkPolyDataToLabelmapFilter, vtkDataSetToImageFilter );
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(vtkDataSet *input);
  virtual vtkImageData* GetOutput();

  virtual void Update();

protected:
  LabelImageType::Pointer BinaryDilateFilter3D( LabelImageType::Pointer & img, unsigned int ballsize );
  LabelImageType::Pointer BinaryErodeFilter3D( LabelImageType::Pointer & img, unsigned int ballsize );
  LabelImageType::Pointer BinaryClosingFilter3D( LabelImageType::Pointer & img, unsigned int ballsize );
  int ConvertModelToLabelUsingFloodFill(vtkPolyData* inputPolyData, LabelImageType::Pointer outputLabel, double sampleDistance, unsigned char labelValue);

protected:
  vtkSetObjectMacro(InputPolyData, vtkPolyData);
  vtkSetObjectMacro(OutputLabelmap, vtkImageData);
  vtkSetMacro(SampleDistance, double);
  vtkSetMacro(LabelValue, unsigned char);
  vtkSetVector3Macro(OutputLabelmapSize, unsigned long);

protected:
  vtkPolyData* InputPolyData;
  vtkImageData* OutputLabelmap;
  double SampleDistance;
  unsigned char LabelValue;
  unsigned long OutputLabelmapSize[3];

protected:
  vtkPolyDataToLabelmapFilter();
  virtual ~vtkPolyDataToLabelmapFilter();

private:

  vtkPolyDataToLabelmapFilter(const vtkPolyDataToLabelmapFilter&); // Not implemented
  void operator=(const vtkPolyDataToLabelmapFilter&);               // Not implemented
};

#endif
