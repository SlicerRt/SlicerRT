import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

class PolyDataDistanceHistogramFilterTest(unittest.TestCase):
  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  #------------------------------------------------------------------------------
  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()

    self.assertIsNotNone( slicer.modules.segmentcomparison )

    sphereSource1 = vtk.vtkSphereSource()
    sphereSource1.SetRadius(1.0)
    sphereSource1.SetCenter([0,0,0])
    sphereSource1.Update()

    sphereSource2 = vtk.vtkSphereSource()
    sphereSource2.SetRadius(1.0)
    sphereSource2.SetCenter([0.5,0,0])
    sphereSource2.Update()

    polyDataDistanceHistogramFilter = slicer.vtkPolyDataDistanceHistogramFilter()
    polyDataDistanceHistogramFilter.SetInputReferencePolyData(sphereSource1.GetOutput())
    polyDataDistanceHistogramFilter.SetInputComparePolyData(sphereSource2.GetOutput())
    polyDataDistanceHistogramFilter.SetSamplePolyDataVertices(1)
    polyDataDistanceHistogramFilter.SetSamplePolyDataEdges(1)
    polyDataDistanceHistogramFilter.SetSamplePolyDataFaces(1)
    polyDataDistanceHistogramFilter.SetSamplingDistance(0.01)
    polyDataDistanceHistogramFilter.SetHistogramMinimum(-0.5)
    polyDataDistanceHistogramFilter.SetHistogramMaximum(0.5)
    polyDataDistanceHistogramFilter.SetHistogramSpacing(0.05)
    polyDataDistanceHistogramFilter.Update()

    # "ground truth" may be a bit misleading. These values were collected
    # by running this code through the Python interactor on Oct 5 2017.
    groundTruthFrequencies = [94,330,136,240,180,172,640,392,576,58534,97079,3660,2258,2840,2024,1882,1736,971,1214,702]
    histogramTable = polyDataDistanceHistogramFilter.GetOutputHistogram()
    for i in xrange(0,len(groundTruthFrequencies)):
      computedFrequecy = histogramTable.GetValue(i,1)
      self.assertEqual(computedFrequecy, groundTruthFrequencies[i])

    logging.info('Test finished')

