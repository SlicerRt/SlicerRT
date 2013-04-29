
#ifndef __vtkSlicerDeformationFieldVisualizerLogic_h
#define __vtkSlicerDeformationFieldVisualizerLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>

#include "vtkSlicerDeformationFieldVisualizerModuleLogicExport.h"

class vtkMRMLDeformationFieldVisualizerNode;
class vtkMRMLVectorVolumeNode;

/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class VTK_SLICER_DEFORMATIONFIELDVISUALIZER_MODULE_LOGIC_EXPORT vtkSlicerDeformationFieldVisualizerLogic : 
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDeformationFieldVisualizerLogic *New();
  vtkTypeMacro(vtkSlicerDeformationFieldVisualizerLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /*!
   * TODO: Add description of the function itself and its arguments
   * \param option (it is named option in the source but it tells nothing, please rename)
   */
  void CreateVisualization(int option);
  
  double GetFieldMaxNorm();
  
  vtkSmartPointer<vtkPolyData> GlyphVisualization(vtkSmartPointer<vtkImageData>, int sourceOption);
  vtkSmartPointer<vtkPolyData> GridVisualization(vtkSmartPointer<vtkImageData>);
  vtkSmartPointer<vtkPolyData> ContourVisualization(vtkSmartPointer<vtkImageData>);
  vtkSmartPointer<vtkPolyData> BlockVisualization(vtkSmartPointer<vtkImageData>);
  vtkSmartPointer<vtkPolyData> GlyphSliceVisualization(vtkSmartPointer<vtkImageData>, vtkSmartPointer<vtkMatrix4x4>); 
  vtkSmartPointer<vtkPolyData> GridSliceVisualization(vtkSmartPointer<vtkImageData>, vtkSmartPointer<vtkMatrix4x4>);

  /*!
   * Issue when input data is changed without modifying node
   * Will either remake or account for scenario some other way
   */
  void GenerateTransformField();

public:
  void SetAndObserveDeformationFieldVisualizerNode(vtkMRMLDeformationFieldVisualizerNode *node);
  vtkGetObjectMacro(DeformationFieldVisualizerNode, vtkMRMLDeformationFieldVisualizerNode);
  
  //vtkSetObjectMacro(TransformField, vtkImageData);
  //vtkGetObjectMacro(TransformField, vtkImageData);
  
  vtkSmartPointer<vtkImageData> TransformField;

protected:
  vtkSlicerDeformationFieldVisualizerLogic();
  ~vtkSlicerDeformationFieldVisualizerLogic();

  virtual void RegisterNodes();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

protected:
  //vtkImageData *TransformField;
  //vtkSmartPointer<vtkImageData> TransformField
  
private:
  vtkSlicerDeformationFieldVisualizerLogic(const vtkSlicerDeformationFieldVisualizerLogic&);// Not implemented
  void operator=(const vtkSlicerDeformationFieldVisualizerLogic&);// Not implemented
  
protected:
  /// Parameter set MRML node
  vtkMRMLDeformationFieldVisualizerNode* DeformationFieldVisualizerNode;
};

#endif
