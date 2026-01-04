# Plastimatch

Plastimatch is an open source software for image computation. Our main focus is high-performance volumetric registration of medical images, such as X-ray computed tomography (CT), magnetic resonance imaging (MRI), and positron emission tomography (PET). Software features include:

- B-spline method for deformable image registration (GPU and multicore accelerated)
- Demons method for deformable image registration (GPU accelerated)
- ITK-based algorithms for translation, rigid, affine, demons, and B-spline registration
- Pipelined, multi-stage registration framework with seamless conversion between most algorithms and transform types
- Landmark-based deformable registration using thin-plate splines for global registration
- Landmark-based deformable registration using radial basis functions for local corrections
- Broad support for 3D image file formats (using ITK), including DICOM, Nifti, NRRD, MetaImage, and Analyze
- DICOM and DICOM-RT import and export
- XiO import and export

[Download sample data](http://plastimatch.org/data_sources.html) to use with modules.

This work is part of the National Alliance for Medical Image Computing (NAMIC).
Author: Greg Sharp (gcsharp@partners.org)
Contributors: Jean-Christophe Fillion-Robin (Kitware)
Website: http://plastimatch.org/<br>
License: BSD

## Modules

- Modules from Plastimatch:
  - [Plastimatch Automatic deformable image registration](PlmBSplineDeformableRegistration.md)
  - [Plastimatch LANDWARP Landmark deformable registration](PlmLandWarp.md)

## References

- G Sharp, N Kandasamy, H Singh, M Folkert, "GPU-based streaming architectures for fast cone-beam CT image reconstruction and demons deformable registration," Physics in Medicine and Biology, 52(19), pp 5771-83, 2007.
- V Boldea, G Sharp, SB Jiang, D Sarrut, "4D-CT lung motion estimation with deformable registration: Quantification of motion nonlinearity and hysteresis," Medical Physics, 33(3), pp 1008-18, 2008.
- Z Wu, E Rietzel, V Boldea, D Sarrut, G Sharp, "Evaluation of deformable registration of patient lung 4DCT with sub-anatomical region segmentations," Medical Physics, 35(2), pp 775-81, 2008.
- G Sharp et al. "Plastimatch - An open source software suite for radiotherapy image processing," Proceedings of the XVIth International Conference on the use of Computers in Radiotherapy, May, 2010.
- N. Shusharina, G. Sharp "Landmark-based image registration with analytic regularization", IEEE Trans. Med. Imag., submitted, 2011.
