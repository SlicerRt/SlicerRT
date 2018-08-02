#ifndef __vtkPolyDataDistanceHistogramFilter_h
#define __vtkPolyDataDistanceHistogramFilter_h

#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>

#include "vtkSlicerSegmentComparisonModuleLogicExport.h"


/// \class vtkPolyDataDistanceHistogramFilter
/// \brief Compute a histogram of distances from one poly data to another.
///
/// vtkPolyDataDistanceHistogramFilter is an algorithm that outputs a histogram
/// of the distances from one input vtkPolyData to another. This filter
/// is based on the following classes from vtk: vtkPolyDataPointSampler,
/// vtkImplicitPolyDataDistance. The histogram is output  as a vtkTable 
/// object. The user can also access the raw distances directly as a 
/// vtkDoubleArray using GetOutputDistances().
///
/// This class CANNOT be a part of the VTK pipeline (as a filter) because
/// it uses the pipeline internally. Creating such a "mini-pipeline" may
/// result in unexpected requests being sent up the pipeline and other
/// associated unexpected behaviour.
class VTK_SLICER_SEGMENTCOMPARISON_MODULE_LOGIC_EXPORT vtkPolyDataDistanceHistogramFilter : public vtkObject
{
public:
  static const int INPUT_PORT_REFERENCE_POLYDATA;
  static const int INPUT_PORT_COMPARE_POLYDATA;
  static const int OUTPUT_PORT_HISTOGRAM;
  //static const int OUTPUT_PORT_DISTANCES;

public:
  vtkTypeMacro(vtkPolyDataDistanceHistogramFilter,vtkObject);
 
  /// Instantiate object with all settings turned on (set to 1)
  /// except for SamplePolyDataEdges and SamplePolyDataFaces.
  static vtkPolyDataDistanceHistogramFilter *New();
  
  /// Set the reference vtkPolyData object used as an input to generate the distances
  /// (from the compare vtkPolyData to the reference vtkPolyData)
  void SetInputReferencePolyData(vtkPolyData*);
  /// Get the reference vtkPolyData object used as an input to generate the distances
  /// (from the compare vtkPolyData to the reference vtkPolyData)
  vtkPolyData* GetInputReferencePolyData();

  /// Set the compare vtkPolyData object used as an input to generate the distances
  /// (from the compare vtkPolyData to the reference vtkPolyData)
  void SetInputComparePolyData(vtkPolyData*);
  /// Get the compare vtkPolyData object used as an input to generate the distances
  /// (from the compare vtkPolyData to the reference vtkPolyData)
  vtkPolyData* GetInputComparePolyData();

  /// Get the distance values in the form of a histogram.
  vtkTable* GetOutputHistogram();

  /// Get the minimum of the distances from each point of the compare mesh to the reference mesh
  /// Contains as many distance values as there are samples (points, etc.) in the compare mesh
  vtkDoubleArray* GetOutputDistances();
  
  /// Get maximum of the absolute of the minimum distances \sa GetOutputDistances from the compare mesh to the reference mesh.
  /// This is what is traditionally called Hausdorff distance.
  double GetMaximumHausdorffDistance();
  
  /// Get average of the absolute of the minimum distances \sa GetOutputDistances from the compare mesh to the reference mesh.
  /// (this corresponds to the 'average Hausdorff distance' in plastimatch: http://plastimatch.org/doxygen/classHausdorff__distance.html )
  double GetAverageHausdorffDistance();

  /// Get standard deviation of the absolute of the minimum distances \sa GetOutputDistances from the compare mesh to the reference mesh.
  double GetStandardDeviationHausdorffDistance();
  
  /// Get 95th percentile of the absolute of the minimum distances \sa GetOutputDistances from the compare mesh to the reference mesh.
  /// (this corresponds to the 'percent Hausdorff distance' in plastimatch: http://plastimatch.org/doxygen/classHausdorff__distance.html )
  double GetPercent95HausdorffDistance();

  // Get the Nth percentile of the absolute of the minimum distances \sa GetOutputDistances from the compare mesh to the reference mesh.
  /// (this corresponds to the 'percent Hausdorff distance' in plastimatch: http://plastimatch.org/doxygen/classHausdorff__distance.html )
  double GetNthPercentileHausdorffDistance(double n);
  
  /// Set whether the filter should sample on the vertices of the input vtkPolyData objects.
  vtkSetMacro(SamplePolyDataVertices, int);
  /// Get whether the filter should sample on the vertices of the input vtkPolyData objects.
  vtkGetMacro(SamplePolyDataVertices, int);
  /// Set whether the filter should sample on the vertices of the input vtkPolyData objects.
  vtkBooleanMacro(SamplePolyDataVertices,int);
    
  /// Set whether the filter should sample on the edges of the input vtkPolyData objects.
  vtkSetMacro(SamplePolyDataEdges, int);
  /// Get whether the filter should sample on the edges of the input vtkPolyData objects.
  vtkGetMacro(SamplePolyDataEdges, int);
  /// Set whether the filter should sample on the edges of the input vtkPolyData objects.
  vtkBooleanMacro(SamplePolyDataEdges,int);
    
  /// Set whether the filter should sample on the faces of the input vtkPolyData objects.
  vtkSetMacro(SamplePolyDataFaces, int);
  /// Get whether the filter should sample on the faces of the input vtkPolyData objects.
  vtkGetMacro(SamplePolyDataFaces, int);
  /// Set whether the filter should sample on the faces of the input vtkPolyData objects.
  vtkBooleanMacro(SamplePolyDataFaces,int);

  /// Set the sampling distance for points on edges or faces of the input vtkPolyData objects.
  vtkSetMacro(SamplingDistance, double);
  /// Get the sampling distance for points on edges or faces of the input vtkPolyData objects.
  vtkGetMacro(SamplingDistance, double);

  /// Set the histogram minimum (left-most value).
  vtkSetMacro(HistogramMinimum, double);
  /// Get the histogram minimum (left-most value).
  vtkGetMacro(HistogramMinimum, double);

  /// Set the histogram maximum (right-most value).
  vtkSetMacro(HistogramMaximum, double);
  /// Get the histogram maximum (right-most value).
  vtkGetMacro(HistogramMaximum, double);

  /// Set the histogram spacing (width of the bins).
  vtkSetMacro(HistogramSpacing, double);
  /// Get the histogram spacing (width of the bins).
  vtkGetMacro(HistogramSpacing, double);
  
  /// Compute distances an histogram
  void Update();

protected:
  vtkPolyDataDistanceHistogramFilter();
  ~vtkPolyDataDistanceHistogramFilter();

// Pipeline-related functions that were deactivated. See details in source file
protected:
  //int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  //int FillInputPortInformation(int portNumber, vtkInformation *);
  //int FillOutputPortInformation(int port, vtkInformation* info);

private:
  /// This method measures the raw distances from points on comparePolyData to referencePolyData, and stores them in distanceArray.
  /// \param referencePolyData The reference vtkPolyData on which to compute the distances. Distances are measured from points on the comparePolyData to the referencePolyData.
  /// \param comparePolyData The compare vtkPolyData on which to compute the distances. Distances are measured from points on the comparePolyData to the referencePolyData.
  /// \param distanceArray The array in which to store the raw distances.
  void ComputeDistances(vtkPolyData* referencePolyData, vtkPolyData* comparePolyData, vtkDoubleArray* distanceArray);
  
protected:
  /// Compare polydata, one of the inputs to generate the distances (from the compare vtkPolyData to the reference vtkPolyData)
  vtkPolyData* InputComparePolyData;
  /// Reference polydata, one of the inputs to generate the distances (from the compare vtkPolyData to the reference vtkPolyData)
  vtkPolyData* InputReferencePolyData;

  /// Output histogram of the distances
  vtkTable* OutputHistogram;
  /// Output distances for each reference vertex in an array
  vtkDoubleArray* OutputDistances;

  /// Flag determining  whether the filter should sample on the vertices of the input vtkPolyData objects.
  /// All vertices from the vtkPolyData will be used, regardless of the sampling distance.
  /// Default is 1 (on).
  int SamplePolyDataVertices;
  /// Flag determining whether the filter should sample on the edges of the input vtkPolyData objects.
  /// The user can control the sampling distance using \sa SetSamplingDistance
  /// Default is 0 (off).
  int SamplePolyDataEdges;
  /// Flag determining whether the filter should sample on the faces of the input vtkPolyData objects.
  /// The user can control the sampling distance using\sa/ SetSamplingDistance.
  /// Default is 0 (off).
  int SamplePolyDataFaces;

  /// Sampling distance for points on edges or faces of the input vtkPolyData objects.
  /// Default is 0.01.
  double SamplingDistance;
  /// Histogram minimum (left-most value).
  /// Default is -10.
  double HistogramMinimum;
  /// Histogram maximum (right-most value).
  /// Default is 10.
  double HistogramMaximum;
  /// Histogram spacing (width of the bins).
  /// Default is 0.1.
  double HistogramSpacing;
  
private:
  vtkPolyDataDistanceHistogramFilter(const vtkPolyDataDistanceHistogramFilter&);  // Not implemented.
  void operator=(const vtkPolyDataDistanceHistogramFilter&);  // Not implemented.
};
 
#endif
