#ifndef __qSlicerDeformationFieldVisualizerModuleWidget_h
#define __qSlicerDeformationFieldVisualizerModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDeformationFieldVisualizerModuleExport.h"

class qSlicerDeformationFieldVisualizerModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class Q_SLICER_QTMODULES_DEFORMATIONFIELDVISUALIZER_EXPORT qSlicerDeformationFieldVisualizerModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT
  
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDeformationFieldVisualizerModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDeformationFieldVisualizerModuleWidget();
  
  virtual void enter();

public slots:
  virtual void setMRMLScene(vtkMRMLScene*);
  void onSceneImportedEvent();
  void setDeformationFieldVisualizerParametersNode(vtkMRMLNode *node);
  void updateWidgetFromMRML();

  void visualize();

protected slots:
  void onLogicModified();
  void updateSourceOptions(int);
  
  void inputVolumeChanged(vtkMRMLNode*);
  void referenceVolumeChanged(vtkMRMLNode*);
  void outputModelChanged(vtkMRMLNode*);
  
  // Parameters
  // Glyph Parameters
  void setGlyphPointMax(double);
  void setGlyphScale(double);
  void setGlyphScaleDirectional(bool);
  void setGlyphScaleIsotropic(bool);
  void setGlyphThreshold(double, double);
  void setGlyphSeed(int);
  void setSeed();
  void setGlyphSourceOption(int);
  // Arrow Parameters
  void setGlyphArrowTipLength(double);
  void setGlyphArrowTipRadius(double);
  void setGlyphArrowShaftRadius(double);
  void setGlyphArrowResolution(double);
  // Cone Parameters
  void setGlyphConeHeight(double);
  void setGlyphConeRadius(double);
  void setGlyphConeResolution(double);
  // Sphere Parameters
  void setGlyphSphereResolution(double);
    
  // Grid Parameters
  void setGridScale(double);
  void setGridDensity(double);
  
  // Block Parameters
  void setBlockScale(double);
  void setBlockDisplacementCheck(int);
    
  // Contour Parameters
  void setContourNumber(double);
  void setContourRange(double, double);

  // Glyph Slice Parameters
  void setGlyphSliceNode(vtkMRMLNode*);
  void setGlyphSlicePointMax(double);
  void setGlyphSliceThreshold(double, double);
  void setGlyphSliceScale(double);
  void setGlyphSliceSeed(int);
  void setSeed2();
  
  // Grid Slice Parameters
  void setGridSliceNode(vtkMRMLNode*);
  void setGridSliceScale(double);
  void setGridSliceDensity(double);
  
protected:
  QScopedPointer<qSlicerDeformationFieldVisualizerModuleWidgetPrivate> d_ptr;

  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerDeformationFieldVisualizerModuleWidget);
  Q_DISABLE_COPY(qSlicerDeformationFieldVisualizerModuleWidget);
};

#endif
