## "External Beam Planning" module description

The ExternalBeamPlanning module is a generic, extensible module for forward
planning of external beam radiation therapy treatments. 

The "External Beam Planning" module is responsible for creating a beam plan or ion beam plan
according to DICOM RT standard. Only a few basic features are supported.
Each plan consists of a number of beams: static or dynamic. Each plan supports
only one isocenter point, multiple isocenters per a single plan are not allowed.

This work was in part funded by An Applied Cancer Research Unit of Cancer Care Ontario
with funds provided by the Ministry of Health and Long-Term Care and the Ontario Consortium
for Adaptive Interventions in Radiation Oncology (OCAIRO) to provide free, open-source toolset
for radiotherapy and related image-guided interventions.

Author: Csaba Pinter (PerkLab, Queen's University), Greg Sharp (Massachusetts General Hospital)
Contact: Csaba Pinter, <email>csaba.pinter@queensu.ca</email>

![image](https://www.slicer.org/w/img_auth.php/3/3f/LogoCco.png)
![image](https://www.slicer.org/w/img_auth.php/2/27/LogoOCAIRO.jpg)

### Use Cases
- Proton dose calculation
- Any dose engine can be integrated (C++, Python, Matlab)

### Tutorials
[Orthovoltage RT treatment planning tutorial](https://github.com/SlicerRt/SlicerRtDoc/blob/master/tutorials/SlicerRT_Tutorial_OrthovoltageDoseEngine.pptx) (uses EGSnrc)

### Graphical User Interface loadable module (GUI). Panels and their use

![image](https://user-images.githubusercontent.com/3785912/183899734-d4795313-fd39-4c40-8df6-e84f0469c3b0.png)

Loadable GUI module "External Beam Planning" is used to setup beam parameters
for beams or ion beams, or modify already created beams.
Static RT beam is setup by: Isocenter position, SAD, SID, Jaws borders, MLC positions. Static beam is fixed.

Dynamic RT and Ion RT beams are setup by: initial and final angles of rotation around gantry axis,
direction of rotation, isocenter position, SAD, SID, Jaws borders, MLC positions, scanning spot map, etc.
Dynamic beam changes it position over time by means of rotation or (and) changing the position of phase space of the beam.

#### Active RT plan

![image](https://user-images.githubusercontent.com/3785912/183899795-762b8b13-4ad5-4112-bcb6-e3f5fbd21fe2.png)

1. **Active RT plan**: Selected RT Plan
2. **Ion plan**: flag to generate RTIonPlan instead of RTPlan

#### Plan parameters

![image](https://user-images.githubusercontent.com/3785912/183899832-fb178c71-900f-48d8-97a5-35c4d532bfb3.png)

1. **Reference volume**: Reference volume node data (usually CT)
2. **Structure set**: Segmentation node data
3. **Points of interest**: Markups fiducial to be used as isocenter
4. **Isocenter**:  Isocenter position in RAS
5. **Center views**:  Center slice views on isocenter
6. **Target volume**:  Select targer ROI
7. **Isocenter at target center**:  Set isocenter in the center of the targer ROI
8. **Dose engine**:  Select a dose engine calculator **Not implemented**
9. **Rx Dose (Gy)**:  Setup an amount of dose in Gy

#### Output

![image](https://user-images.githubusercontent.com/3785912/183899869-580d00a9-aa86-405d-b22c-d11a35f2f6df.png)

1. **Output dose volume**: Setup output dose volume
2. **Clear dose**: Clear dose volume
3. **Calculate WED**: Calculate water-equivalent distance **Not implemented**
4. **Calculate Dose**: Calculate dose according to the engine parameters **Not implemented**

#### Beams

![image](https://user-images.githubusercontent.com/3785912/183899904-6f8e76fd-1574-462e-bf31-21f2cf5fa740.png)

1. **Arc beam sequence**: Activate dynamic beam arc movement around gantry rotation axis
2. **Initial angle**: Initial angle position in degrees
3. **Final angle**: Final angle position in degrees
4. **Angle step**: Angle step
5. **Rotation direction**: Clockwise (CW) of CounterClockwise (CCW) rotation 
5. **Add beam**: Add static or dynamic arc beam to the plan
6. **Remove beam**: Remove selected beam from the plan

### Arc beam

The beam movement around rotation axis could be used to calculate a filtered back-projection reconstruction for cone-beam geometries.
For fast forward and back projection reconstruction the [RTK](https://www.openrtk.org) library can be used.

### References

1. Sharp, G., Pinter, C., Unkelbach, J., Fichtinger, G. (2017). Open Source Proton Treatment Planning in 3D Slicer: Status Update. Proceedings to the 56 Annual Meeting of the Particle Therapy Cooperative Group (PTCOG), 8-13 May 2017. International Journal of Particle Therapy: Summer 2017, Vol. 4, No. 1, pp. 14-83.

### Information for Developers

- Sample C++ dose engine: [https://github.com/SlicerRt/SlicerRT/blob/master/ExternalBeamPlanning/Widgets/qSlicerMockDoseEngine.h](https://github.com/SlicerRt/SlicerRT/blob/master/ExternalBeamPlanning/Widgets/qSlicerMockDoseEngine.h)
- Sample Python dose engine: [https://github.com/SlicerRt/SlicerRT/blob/master/ExternalBeamPlanning/Widgets/Python/MockPythonDoseEngine.py](https://github.com/SlicerRt/SlicerRT/blob/master/ExternalBeamPlanning/Widgets/Python/MockPythonDoseEngine.py)
- Future plans
  - Inverse planning capabilities
  - Matlab plugin adapter
  - Ports module (apertures, MLC, target volume)
  - Beam groups (common parameters for a group of beams)

