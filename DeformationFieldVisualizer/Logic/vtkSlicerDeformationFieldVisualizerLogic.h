
#ifndef __vtkSlicerDeformationFieldVisualizerLogic_h
#define __vtkSlicerDeformationFieldVisualizerLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>

#include "vtkSlicerDeformationFieldVisualizerModuleLogicExport.h"

class vtkMRMLDeformationFieldVisualizerParametersNode;
class vtkMRMLVectorVolumeNode;

/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class VTK_SLICER_DEFORMATIONFIELDVISUALIZER_MODULE_LOGIC_EXPORT vtkSlicerDeformationFieldVisualizerLogic : 
	public vtkSlicerModuleLogic
{
public:
	static vtkSlicerDeformationFieldVisualizerLogic *New();
	vtkTypeMacro(vtkSlicerDeformationFieldVisualizerLogic, vtkSlicerModuleLogic);
	void PrintSelf(ostream& os, vtkIndent indent);

	void SetAndObserveParameterNode(vtkMRMLDeformationFieldVisualizerParametersNode *node);
	vtkGetObjectMacro(ParameterNode, vtkMRMLDeformationFieldVisualizerParametersNode);

	void createVisualization(int);
	double GetFieldMaxNorm();
	
	vtkSmartPointer<vtkPolyData> glyphVisualization(vtkSmartPointer<vtkImageData>, int sourceOption);
	vtkSmartPointer<vtkPolyData> gridVisualization(vtkSmartPointer<vtkImageData>);
	vtkSmartPointer<vtkPolyData> contourVisualization(vtkSmartPointer<vtkImageData>);
	vtkSmartPointer<vtkPolyData> blockVisualization(vtkSmartPointer<vtkImageData>);
	vtkSmartPointer<vtkPolyData> glyphSliceVisualization(vtkSmartPointer<vtkImageData>, vtkSmartPointer<vtkMatrix4x4>); 
 	vtkSmartPointer<vtkPolyData> gridSliceVisualization(vtkSmartPointer<vtkImageData>, vtkSmartPointer<vtkMatrix4x4>);

	void generateTransformField();
  
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
	
	vtkSmartPointer<vtkImageData> transformField;
	int option;	
  
private:

	vtkSlicerDeformationFieldVisualizerLogic(const vtkSlicerDeformationFieldVisualizerLogic&); // Not implemented
	void operator=(const vtkSlicerDeformationFieldVisualizerLogic&);// Not implemented
	
protected:
	/// Parameter set MRML node
	vtkMRMLDeformationFieldVisualizerParametersNode* ParameterNode;
};

#endif
