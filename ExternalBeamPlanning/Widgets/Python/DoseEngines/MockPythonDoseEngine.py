from DoseEngines import *

class MockPythonDoseEngine(AbstractScriptedDoseEngine):
  """ Mock python dose engine to test python interface for External Beam Planning
  """

  def __init__(self, scriptedEngine):
    scriptedEngine.name = 'Mock python'
    AbstractScriptedDoseEngine.__init__(self, scriptedEngine)

  def defineBeamParameters(self):
    self.scriptedEngine.addBeamParameterSpinBox(
    "Mock python dose", "NoiseRange", "Noise range (% of Rx):", "Range of noise added to the prescription dose (+- half of the percentage of the Rx dose)",
    0.0, 99.99, 10.0, 1.0, 2 )

  def calculateDoseUsingEngine(self, beamNode, resultDoseVolumeNode):
    import qSlicerExternalBeamPlanningModuleWidgetsPythonQt as engines
    mockEngine = engines.qSlicerMockDoseEngine()

    # Set parameter for C++ mock engine so that it is used with this beam node
    mockEngine.setParameter( beamNode, "NoiseRange", self.scriptedEngine.doubleParameter(beamNode, "NoiseRange") )

    # Call C++ mock engine to calculate mock dose
    return mockEngine.calculateDoseUsingEngine(beamNode, resultDoseVolumeNode)
