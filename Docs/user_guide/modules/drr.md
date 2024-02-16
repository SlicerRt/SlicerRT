## DRR calculation 

A digitally reconstructed radiograph (DRR) is a synthetic radiograph which 
can be generated from a computed tomography (CT) scan. It is used as a reference 
image for verifying the correct setup position of a patient prior to radiation treatment.

The process of DRR calculation consists of two modules: command line interface module and 
graphical user interface loadable module.

### Command Line Interface module (CLI)

![image](https://user-images.githubusercontent.com/3785912/107616458-2c8c0980-6c5f-11eb-923e-fafb55a0e61f.png)

CLI module "slicer_plastimatch_drr" takes a CT image as input, and generates one 
DRR image according to the parameters of detector orientation, source distance,
isocenter position and image resolution and spacing. Most of these parameters are 
based on [plastimatch drr](http://www.plastimatch.org/drr.html) ones.

#### Input and output volumes parameters
1. **CT Input Volume**: Input CT data
2. **DRR Output Volume**: Output DRR image

#### Source parameters
3. **SAD**: Distance between the x-ray source and an axis of rotation (isocenter) in mm
4. **SID**: Distance between the x-ray source and the detector in mm (SID >= SAD)

#### Detector orientation parameters
5. **View-Up vector**: View up vector to the first row of the detector in LPS coordinate system
6. **Normal vector**: Detector surface normal vector in LPS coordinate system

#### CT scan parameter
7. **Isocenter position**: Isocenter position in LPS coordinate system

#### Detector and image parameters
8. **Resolution**: Detector resolution in pixels (columns, rows)
9. **Spacing**: Detector spacing in mm (columns, rows)
10. **Use image window**: whether use image subwindow or a whole detector
11. **Image window**: Limit DRR output to a subwindow (column1, row1, column2, row2)

#### Processing parameters
12. **Automatically rescale intensity**: Automatically rescale intensity range
13. **Autoscale range**: Range used for autoscale in form (min, max)
14. **Exponential mapping**: Apply exponential mapping of output values
15. **Threading type**: Type of parallelism of computation
16. **HU conversion type**: Type of Hounsfield Units conversion during computation
17. **Exposure type**: Type of exposure algorithm
18. **Output format**: Type of output file formal ("raw" for 3DSlicer)
19. **Invert intensity**: Apply intensity inversion during data post-processing.
20. **Threshold below**: HU values below threshold are counted as -1000 (Air value)

### Graphical User Interface loadable module (GUI)

![image](https://private-user-images.githubusercontent.com/3785912/294439297-c315f0db-9ab7-4a91-9eaa-10a4698d7cd8.png?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MDQ0NDQ3NzMsIm5iZiI6MTcwNDQ0NDQ3MywicGF0aCI6Ii8zNzg1OTEyLzI5NDQzOTI5Ny1jMzE1ZjBkYi05YWI3LTRhOTEtOWVhYS0xMGE0Njk4ZDdjZDgucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI0MDEwNSUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNDAxMDVUMDg0NzUzWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9MmYyNGIzYmNmYjM2ZDI0YWU0NGFiM2RiYjg0ZGE4ZDQ3NGIyMjFiYWNkMmQ2Y2Y4MzlmY2I2OWI0MGUzZDI3YyZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QmYWN0b3JfaWQ9MCZrZXlfaWQ9MCZyZXBvX2lkPTAifQ.J_kchHVITj00Qsd6Z84Wujp8kEXvknmQYZ9SoiWgqbA)

Loadable GUI module "DRR Image Computation" uses CLI module's logic and nodes data to calculate
and visualize DRR image. It also shows basic detector elements such as: detector boundary,
detector normal vector, detector view-up vector, detector image origin (0,0) pixel,
image subwindow boundary as markups data on a slice and 3D views.

This module _partially_ supports calculation of DRR for orientated CT volumes,
if IJKToRASDirection matrix of the volume isn't as below.

| -1  0 0 0 |
|  0 -1 0 0 |
|  0  0 1 0 |
|  0  0 0 1 |

The markups data is only for WYSIWYG purpose.

#### Reference input nodes

![image](https://private-user-images.githubusercontent.com/3785912/294439383-a0718f87-81bf-4a9f-880b-34876a6152ed.png?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MDQ0NDQ3NzMsIm5iZiI6MTcwNDQ0NDQ3MywicGF0aCI6Ii8zNzg1OTEyLzI5NDQzOTM4My1hMDcxOGY4Ny04MWJmLTRhOWYtODgwYi0zNDg3NmE2MTUyZWQucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI0MDEwNSUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNDAxMDVUMDg0NzUzWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9NzNlYWI1MDk2M2Q0MzQ3NWQ5MzZhMWNiMzlmNmVmNmQ0NzFkNDQ5Y2U1YjBmNDE5NzZlNTYzM2E1NGEyYjY3NSZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QmYWN0b3JfaWQ9MCZrZXlfaWQ9MCZyZXBvX2lkPTAifQ.ZrUVivDARk6G1kLMgdk36E3r-z0UzOV7SLK2OUBZzbg)

1. **CT volume**: Input CT data
2. **RT beam**: Input RT beam (vtkMRMLRTBeamNode) for source and detector orientation parameters
2. **Camera**: Camera node (vtkMRMLCameraNode) to update if needed beam geometry and transformation
3. **Update beam**: Update beam geometry and transformation using camera node data 
4. **Volume direction to LPS transform matrix**: The matrix widget shows elements of the transformation matrix R
    The volume direction to LPS transform matrix _R_
    can be applied to the volume, to make IJKToRASDirectionMatrix of the volume a LPS orientated
    after hardening the transform.

    If IJKToRASDirection matrix _A_ isn't LPS transform

          | a11 a12 a13 a14 |    | -1  0 0 0 |
    _A_ = | a21 a22 a23 a24 | != |  0 -1 0 0 |
          | a31 a32 a33 a34 |    |  0  0 1 0 |
          | a41 a42 a43 a44 |    |  0  0 0 1 |

    the volume direction to LPS transform matrix _R_ can be applied to the volume,
    by clicking on `Apply transform to volume` button, to make
    the IJKToRASDirection matrix a LPS orientated after hardening the transform.
    the matrix transformation:

                   | -1  0 0 0 |
    _R_ = _A_^-1 * |  0 -1 0 0 |
                   |  0  0 1 0 |
                   |  0  0 0 1 |

    if matrix _A_ is LPS orientated => matrix _R_ is an identity matrix. 

5. **Show DRR markups**: Show or hide detector markups

CT data is represented by vtkMRMLScalarVolumeNode object. RT beam data is
represented by vtkMRMLRTBeamNode object. Camera data is represented by vtkMRMLCameraNode object.

#### Geometry basic parameters

![image](https://user-images.githubusercontent.com/3785912/107616499-40d00680-6c5f-11eb-9d45-c6cccc9e12cd.png)

6. **Isocenter to imager distance**: Distance from isocenter to detector center in mm
7. **Imager resolution (columns, rows)**: Detector resolution in pixels
8. **Imager spacing in mm (columns, rows)**: Detector spacing in mm
9. **Image window parameters**:  Use and setup image subwindow or a whole detector

#### Image Window Parameters
10. **Columns**: Number of image columns in subwindow 
11. **Rows**: Number of image rows in subwindow

#### Markups perspective projection on the imager plane

![image](https://private-user-images.githubusercontent.com/3785912/294439414-d1bbb6ac-4838-47a7-88fc-9ec76bfe19b2.png?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MDQ0NDQ3NzMsIm5iZiI6MTcwNDQ0NDQ3MywicGF0aCI6Ii8zNzg1OTEyLzI5NDQzOTQxNC1kMWJiYjZhYy00ODM4LTQ3YTctODhmYy05ZWM3NmJmZTE5YjIucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI0MDEwNSUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNDAxMDVUMDg0NzUzWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9M2I4NzQ3ZWU0ZWFlOTExZTllZTM1YTMzZjE0OTI2N2VkOWQ0MWEwOTdmYzQ0MGFhMTU0MWFiY2Y0NjdmYTRkNSZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QmYWN0b3JfaWQ9MCZrZXlfaWQ9MCZyZXBvX2lkPTAifQ.yUYlwYdj9JdqD7CUyS-zFfZ4tV0Yezux__UI35jEUlg)

Calculates projection of fiducial markups points in imager plane.

#### Plastimatch DRR image processing

![image](https://user-images.githubusercontent.com/3785912/107617306-b38db180-6c60-11eb-9dd1-b2751b23a314.png)

12. **Use exponential mapping**: Apply exponential mapping of output values
13. **Autoscale**: Automatically rescale intensity
14. **Invert**: Invert image intensity
15. **Range**: Range intensity in form (min, max)
16. **Reconstruction algorithm**: Type of reconstruction algorithm (Type of exposure algorithm in CLI module)
17. **Hounsfield units conversion**: Type of Hounsfield Units conversion during computation
18. **Threading**: Type of parallelism of computation
19. **Hounsfield units threshold**: HU values below threshold are counted as -1000 (Air value)

#### Plastimatch DRR command arguments (read only)

![image](https://user-images.githubusercontent.com/3785912/96576928-8c8b2880-12db-11eb-875e-a06df31fd792.png)

Arguments for [plastimatch drr](http://www.plastimatch.org/drr.html) program are generated using loadable module parameters
for testing and debugging purposes.

## How to compute a DRR image using python in 3D Slicer?

One must have a CT volume (mandatory) and RTSTRUCT or segmentation (optional).

#### Example 1 (CT Volume with segmentation, beam is set manually, isocenter is a center of ROI)
```
# Create dummy RTPlan
rtImagePlan = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTPlanNode', 'rtImagePlan')
# Create RTImage dummy beam
rtImageBeam = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTBeamNode', 'rtImageBeam')
# Add beam to the plan
rtImagePlan.AddBeam(rtImageBeam)
# Set required beam parameters 
rtImageBeam.SetGantryAngle(90.)
rtImageBeam.SetCouchAngle(12.)
# Get CT volume
ctVolume = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLScalarVolumeNode')
# Get Segmentation (RTSTRUCT)
ctSegmentation = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLSegmentationNode')
# Set and observe CT volume by the plan
rtImagePlan.SetAndObserveReferenceVolumeNode(ctVolume)
# Set and observe Segmentation by the plan
rtImagePlan.SetAndObserveSegmentationNode(ctSegmentation)
# Set isocenter position as a center of ROI
rtImagePlan.SetIsocenterSpecification(slicer.vtkMRMLRTPlanNode.CenterOfTarget)
# Set required segment ID (for example 'PTV')
rtImagePlan.SetTargetSegmentID('PTV')
rtImagePlan.SetIsocenterToTargetCenter()
# Create DRR image computation node for user imager parameters
drrParameters = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLDrrImageComputationNode', 'rtImageBeamParams')
# Set and observe RTImage beam by the DRR node
drrParameters.SetAndObserveBeamNode(rtImageBeam)
# Get DRR computation logic
drrLogic = slicer.modules.drrimagecomputation.logic()
# Update imager markups for the 3D view and slice views (optional)
drrLogic.UpdateMarkupsNodes(drrParameters)
# Update imager normal and view-up vectors (mandatory)
drrLogic.UpdateNormalAndVupVectors(drrParameters) # REQUIRED
# Compute DRR image
drrLogic.ComputePlastimatchDRR( drrParameters, ctVolume)

```
#### Example 2 (CT Volume with segmentation, beam updated according to the 3D camera orientation, isocenter is a center of ROI)
```
# Create dummy plan
rtImagePlan = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTPlanNode', 'rtImagePlan')
# Create RTImage dummy beam
rtImageBeam = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTBeamNode', 'rtImageBeam')
# Add beam to the plan
rtImagePlan.AddBeam(rtImageBeam)
# Get CT volume
ctVolume = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLScalarVolumeNode')
# Set and observe CT volume by the plan
rtImagePlan.SetAndObserveReferenceVolumeNode(ctVolume)
# Get Segmentation (RTSTRUCT)
ctSegmentation = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLSegmentationNode')
# Set and observe CT volume by the plan
rtImagePlan.SetAndObserveReferenceVolumeNode(ctVolume)
# Set and observe Segmentation by the plan
rtImagePlan.SetAndObserveSegmentationNode(ctSegmentation)
# Set isocenter position as a center of ROI
rtImagePlan.SetIsocenterSpecification(slicer.vtkMRMLRTPlanNode.CenterOfTarget)
# Set name of target segment from segmentation (for example 'PTV')
rtImagePlan.SetTargetSegmentID('PTV')
rtImagePlan.SetIsocenterToTargetCenter()
# Get 3D camera
threeDcamera = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLCameraNode')
# Create DRR image computation node for user imager parameters
rtImageParameters = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLDrrImageComputationNode', 'rtImageBeamParams')
# Set and observe RTImage beam by the DRR node
rtImageParameters.SetAndObserveBeamNode(rtImageBeam)
# Set and observe camera node by the DRR node
rtImageParameters.SetAndObserveCameraNode(threeDcamera)
# Set required DRR parameters
rtImageParameters.SetHUThresholdBelow(120)
# Get DRR computation logic
drrLogic = slicer.modules.drrimagecomputation.logic()

# Update beam according to the 3D camera orientation (mandatory)
if (drrLogic.UpdateBeamFromCamera(rtImageParameters)): # REQUIRED
  print('Beam orientation updated according to the 3D camera orientation')

# Update imager markups for the 3D view and slice views (optional)
drrLogic.UpdateMarkupsNodes(rtImageParameters)
# Update imager normal and view-up vectors (mandatory)
drrLogic.UpdateNormalAndVupVectors(rtImageParameters) # REQUIRED
# Compute DRR image
drrLogic.ComputePlastimatchDRR( rtImageParameters, ctVolume)

```

#### Example 3 (CT Volume only, beam is set manually, isocenter is set manually)
```
# Create dummy plan
rtImagePlan = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTPlanNode', 'rtImagePlan')
# Create RTImage dummy beam
rtImageBeam = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTBeamNode', 'rtImageBeam')
# Set required beam parameters 
rtImageBeam.SetGantryAngle(90.)
rtImageBeam.SetCouchAngle(12.)
# Add beam to the plan
rtImagePlan.AddBeam(rtImageBeam)
# Get CT volume
ctVolume = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLScalarVolumeNode')
# Set and observe CT volume by the plan
rtImagePlan.SetAndObserveReferenceVolumeNode(ctVolume)

# Set required isocenter position as a point
rtImagePlan.SetIsocenterSpecification(slicer.vtkMRMLRTPlanNode.ArbitraryPoint)
isocenterPosition = [ -1., -2., -3. ]
if (rtImagePlan.SetIsocenterPosition(isocenterPosition)):
  print('New isocenter position is set')

# Create DRR image computation node for user imager parameters
rtImageParameters = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLDrrImageComputationNode', 'rtImageBeamParams')
# Set and observe RTImage beam by the DRR node
rtImageParameters.SetAndObserveBeamNode(rtImageBeam)
# Set required DRR parameters
rtImageParameters.SetHUThresholdBelow(50)
# Get DRR computation logic
drrLogic = slicer.modules.drrimagecomputation.logic()
# Update imager markups for the 3D view and slice views (optional)
drrLogic.UpdateMarkupsNodes(rtImageParameters)
# Update imager normal and view-up vectors (mandatory)
drrLogic.UpdateNormalAndVupVectors(rtImageParameters) # REQUIRED
# Compute DRR image
drrLogic.ComputePlastimatchDRR( rtImageParameters, ctVolume)

```

#### Example 4 (CT Volume only, beam updated according to the 3D camera orientation, isocenter is set manually)
```
# Create dummy plan
rtImagePlan = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTPlanNode', 'rtImagePlan')
# Create RTImage dummy beam
rtImageBeam = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTBeamNode', 'rtImageBeam')
# Add beam to the plan
rtImagePlan.AddBeam(rtImageBeam)
# Get CT volume
ctVolume = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLScalarVolumeNode')
# Set and observe CT volume by the plan
rtImagePlan.SetAndObserveReferenceVolumeNode(ctVolume)

# Set required isocenter position as a point
rtImagePlan.SetIsocenterSpecification(slicer.vtkMRMLRTPlanNode.ArbitraryPoint)
isocenterPosition = [ -1., -2., -3. ]
if (rtImagePlan.SetIsocenterPosition(isocenterPosition)):
  print('New isocenter position is set')

# Get 3D camera
threeDcamera = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLCameraNode')
# Create DRR image computation node for user imager parameters
rtImageParameters = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLDrrImageComputationNode', 'rtImageBeamParams')
# Set and observe RTImage beam by the DRR node
rtImageParameters.SetAndObserveBeamNode(rtImageBeam)
# Set and observe camera node by the DRR node
rtImageParameters.SetAndObserveCameraNode(threeDcamera)
# Set required DRR parameters
rtImageParameters.SetHUThresholdBelow(-500)
# Get DRR computation logic
drrLogic = slicer.modules.drrimagecomputation.logic()

# Update beam according to the 3D camera orientation (mandatory)
if (drrLogic.UpdateBeamFromCamera(rtImageParameters)): # REQUIRED
  print('Beam orientation updated according to the 3D camera orientation')

# Update imager markups for the 3D view and slice views (optional)
drrLogic.UpdateMarkupsNodes(rtImageParameters)
# Update imager normal and view-up vectors (mandatory)
drrLogic.UpdateNormalAndVupVectors(rtImageParameters) # REQUIRED
# Compute DRR image
drrLogic.ComputePlastimatchDRR( rtImageParameters, ctVolume)

```
