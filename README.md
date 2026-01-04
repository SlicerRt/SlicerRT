# Introduction

SlicerRT is a radiation therapy toolkit for 3D Slicer, containing generic RT features for import/export, analysis, visualization, making 3D Slicer a powerful radiotherapy research platform. The SlicerRT extension incorporates components from [Plastimatch](Docs/user_guide/Plastimatch.md) toolkit.

- Project Website: https://slicerrt.org
- License: [3D Slicer license](https://spdx.github.io/license-list-data/3D-Slicer-1.0.html)
- Download/install: [download and install 3D Slicer](https://slicer.readthedocs.io/en/latest/user_guide/getting_started.html), start 3D Slicer, [install the SlicerRT extension](https://slicer.readthedocs.io/en/latest/user_guide/extensions.html#install-extensions).
- Contact: [Csaba Pinter](https://discourse.slicer.org/u/cpinter) (Ebatinca), [Andras Lasso](https://discourse.slicer.org/u/lassoan) (PerkLab, Queen's University)

# Modules

- [External Beam Planning (Treatment planning)](Docs/user_guide/modules/ExternalBeamPlanning.md)
- Dose analysis
  - [Dose volume histogram](Docs/user_guide/modules/DoseVolumeHistogram.md)
  - [Dose accumulation](Docs/user_guide/modules/DoseAccumulation.md)
  - [Dose comparison (Gamma dose similarity index)]Docs/user_guide/modules/DoseComparison.md)
  - [Isodose line and surface display](Docs/user_guide/modules/Isodose.md)
- Contour analysis
  - [Segment comparison (Dice Similarity Coefficient, Hausdorff distances)](Docs/user_guide/modules/SegmentComparison.md)
  - [Segment morphology (Add/remove margin, Unify, Intersect, etc.)](Docs/user_guide/modules/SegmentMorphology.md)
- I/O
  - [DICOM-RT import, export](Docs/user_guide/modules/DicomRtImport.md) (handles datasets of types RT Structure Set, RT Dose, RT Plan, RT Image)
  - [DICOM-SRO import/export](Docs/user_guide/modules/DicomSroImportExport.md) (handles DICOM Spatial Registration object, both rigid and deformable)
- [Batch processing scripts](https://github.com/SlicerRt/SlicerRT/tree/master/BatchProcessing) (currently only one is available for command-line conversion of RTSS to volume nodes)
- Modules from Plastimatch
  - [Plastimatch Automatic deformable image registration](Docs/user_guide/modules/PlmBSplineDeformableRegistration.md)
  - [Plastimatch LANDWARP Landmark deformable registration](Docs/user_guide/modules/PlmLandWarp.md)

# Use Cases

- Comparison of dose maps and dose volume histograms from various treatment planning systems
- Evaluation of the effect of different adaptive techniques (IGRT, image-based non-rigid patient motion compensation, etc.)
  - Calculate couch shift parameters for patient setup correction in IGRT
- Dose accumulation with motion compensation
- Testing of treatment planning algorithms
- Calculation of PTV margin
- Proton dose calculation
- Gel dosimetry analysis
- Tumor volume tracking
- Treatment plan similarity measurement in the cloud
- Batch structure set conversion

# Tutorials

## Comprehensive tutorials

- Image-guided radiation therapy tutorial 2019 (recommended)
  - [Slides including dataset](https://github.com/SlicerRt/SlicerRtDoc/blob/master/tutorials/SlicerRT_Tutorial_IGRT_4.11.pptx)
- World Congress 2015 tutorial
  - Tutorial presentation: [pptx](https://www.dropbox.com/s/b7qx3n10s52o5f8/SlicerRT_WorldCongress_TutorialIGRT.pptx?dl=0) [pdf](https://github.com/SlicerRt/SlicerRtDoc/raw/master/tutorials/SlicerRT_WorldCongress_TutorialIGRT.pdf)
  - Dataset: [download from MIDAS](http://slicer.kitware.com/midas3/download/item/205391/WC2015_Gel_Slicelet_Dataset.zip)
- Summer NA-MIC week 2013 tutorial
  - Tutorial presentation: [download from Slicer wiki](http://wiki.na-mic.org/Wiki/images/b/b0/SlicerRT_TutorialContestSummer2013.pdf)
  - Sample data: [download from MIDAS](http://slicer.kitware.com/midas3/download/folder/1345/SlicerRtTutorial_Namic2013June.zip)
- ECR 2013 - Medical University Vienna workshop
  - Workshop material: [download from NA-MIC.org](http://www.na-mic.org/Wiki/index.php/File:Pinter_MedUni2013_Workshop.pdf)
- RSNA 2012 tutorial
  - Tutorial description: SlicerRT wiki: Slicer tutorials at RSNA 2012
  - Sample data: [download](http://slicer.kitware.com/midas3/folder/859) SlicerRT ART dose verification data from Midas server

## Module tutorials

  - [External beam planning tutorial for orthovoltage RT (uses EGSnrc)](https://github.com/SlicerRt/SlicerRtDoc/blob/master/tutorials/SlicerRT_Tutorial_OrthovoltageDoseEngine.pptx)
  - [Dose surface histogram tutorial](https://github.com/SlicerRt/SlicerRtDoc/blob/master/tutorials/SlicerRT_Tutorial_DoseSurfaceHistogram.pptx)
  - [Isodose tutorial](https://github.com/SlicerRt/SlicerRtDoc/blob/master/tutorials/SlicerRT_Tutorial_Isodose.pptx)

# Information for Developers

[SlicerRT developers wiki page.](https://github.com/SlicerRt/SlicerRT/wiki/SlicerRt-developers-page)

# Related Extensions

- [Gel Dosimetry](https://www.slicer.org/wiki/Documentation/Nightly/Modules/GelDosimetry): Slicelet facilitating a streamlined workflow to perform true 3D gel dosimetry analysis for commissioning linacs and evaluating new dose calculation procedures
- [Film Dosimetry](https://www.slicer.org/wiki/Documentation/Nightly/Modules/FilmDosimetry): Slicelet supporting workflow to perform 2D film dosimetry analysis for commissioning new radiation techniques and to validate the accuracy of radiation treatment by enabling visual comparison of the planned dose to the delivered dose

# How to get help

You can write to the [3D Slicer Forum](https://discourse.slicer.org/) to get help on using 3D Slicer or SlicerRT extension. Use the `Support` category for user questions and `Developer` category for questions about using SlicerRT via using scripting or extending SlicerRT. Specify `slicerrt` tag to bring the topic to the attention of SlicerRT community members.

## How to report an error

If you encounter an error, please follow the following instructions to help us to find and fix it more quickly.

- Reproduce reliably with latest version
  - Make sure you use the latest Slicer and SlicerRT.
  - Try to reproduce the error and filter out what steps are really required for the error to occur
- Collect relevant information about the issue
  - Describe exactly what steps you made before the error occured
  - What happens exactly that is the unexpected behavior
  - Read the entries in the Error Log window, and send us the ones that seem to be related to SlicerRT
  - Send us the contents of the Python Interactor window
  - If needed for easier understanding, attach a screenshot. Use compressed format (like .png, .jpg, .gif).
  - If you have built your version yourself (you don't use the package), add the SlicerRT and Slicer core source code git hash. Otherwise, report the used Slicer and SlicerRT versions (stable/nightly, which version/day).
- Report the issue on the [3D Slicer Forum](https://discourse.slicer.org/)
- If the issue is confirmed to be a problem in SlicerRT then submit to the [SlicerRT project issue tracker](https://github.com/SlicerRt/SlicerRT/issues)

Thank you for helping to improve SlicerRT!

# How to cite

Please cite the following paper when referring to SlicerRt in your publication:

C. Pinter, A. Lasso, A. Wang, D. Jaffray and G. Fichtinger, ["SlicerRT – Radiation therapy research toolkit for 3D Slicer"](http://perk.cs.queensu.ca/sites/perk.cs.queensu.ca/files/Pinter2012_0.pdf), Med. Phys., 39(10) pp. 6332-6338, 2012

```
@ARTICLE{Pinter2012,
  author = {Pinter, C. and Lasso, A. and Wang, A. and Jaffray, D. and Fichtinger, G.},
  title = {SlicerRT – Radiation therapy research toolkit for 3D Slicer},
  journal = {Med. Phys.},
  year = {2012},
  volume = {39},
  number = {10},
  pages = {6332-6338},
}
```

# Acknowledgments

SlicerRT development is currently funded by CANARIE.
SlicerRT was originally created via funding by Cancer Care Ontario and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO) to provide free, open-source toolset for radiotherapy and related image-guided interventions.

Former SlicerRT modules integrated to Slicer core: Subject hierarchy, Transform visualizer, DICOM-RT export (as improved DICOM export function), Segmentations, Segment Editor.
