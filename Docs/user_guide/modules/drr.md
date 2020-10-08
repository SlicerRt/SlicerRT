## DRR calculation 

A digitally reconstructed radiograph (DRR) is a synthetic radiograph which 
can be generated from a computed tomography (CT) scan. It is used as a reference 
image for verifying the correct setup position of a patient prior to radiation treatment.

The process of DRR calculation consists of two modules: command line interface module and 
graphical user interface loadable module.

### Command Line Interface module (CLI)

![image](https://user-images.githubusercontent.com/3785912/96350796-25ddf300-10c0-11eb-9389-3ea0068616c3.png)

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

### Graphical User Interface loadable module (GUI)

![image](https://user-images.githubusercontent.com/3785912/97528819-a6093000-19bf-11eb-969b-3eaa38bf98c4.png)

Loadable GUI module "DRR Image Computation" uses CLI module's logic and nodes data to calculate
and visualize DRR image. It also shows basic detector elements such as: detector boundary,
detector normal vector, detector view-up vector, detector image origin (0,0) pixel,
image subwindow boundary as markups data on a slice and 3D views.

Markups data is only for WYSIWYG purpose.

#### Reference input nodes

![image](https://user-images.githubusercontent.com/3785912/96576548-01aa2e00-12db-11eb-9a4a-6ed445d6fc4f.png)

1. **CT volume**: Input CT data
2. **RT beam**: Input RT beam (vtkMRMLRTBeamNode) for source and detector orientation parameters
3. **Show DRR markups**: Show or hide detector markups

CT data is represented by vtkMRMLScalarVolumeNode object. RT beam data is
represented by vtkMRMLRTBeamNode object.

#### Geometry basic parameters

![image](https://user-images.githubusercontent.com/3785912/96576643-23a3b080-12db-11eb-85c0-187d364a2e33.png)

4. **Isocenter to imager distance**: Distance from isocenter to detector center in mm
5. **Imager resolution (columns, rows)**: Detector resolution in pixels
6. **Imager spacing in mm (columns, rows)**: Detector spacing in mm
7. **Use image window**:  whether use image subwindow or a whole detector

#### Image Window Parameters
8. **Columns**: Number of image columns in subwindow 
9. **Rows**: Number of image rows in subwindow


#### Plastimatch DRR image processing

![image](https://user-images.githubusercontent.com/3785912/97527533-99cfa380-19bc-11eb-8875-42598847f246.png)

10. **Use exponential mapping**: Apply exponential mapping of output values
11. **Autoscale**: Automatically rescale intensity
12. **Invert**: Invert image intensity
13. **Range**: Range intensity in form (min, max)
14. **Reconstruction algorithm**: Type of reconstruction algorithm (Type of exposure algorithm in CLI module)
15. **Hounsfield units conversion**: Type of Hounsfield Units conversion during computation
16. **Threading**: Type of parallelism of computation

#### Plastimatch DRR command arguments (read only)

![image](https://user-images.githubusercontent.com/3785912/96576928-8c8b2880-12db-11eb-875e-a06df31fd792.png)

Arguments for [plastimatch drr](http://www.plastimatch.org/drr.html) program are generated using loadable module parameters
for testing and debugging purposes.
