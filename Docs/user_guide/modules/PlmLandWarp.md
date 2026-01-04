# Plastimatch LANDWARP Landmark deformable registration

This is the plastimatch landmark-based deformable image registration module. The intended application of this method is rapid, interactive correction of registration failures with a small number of mouse clicks. Compared to other landmark-based methods, the plastimatch registration method might offer:

1. both local and global registration
2. regularization of the deformation field

Examples of how this module is being used:
- Intra-subject registration for adaptive radiotherapy
- Inter-subject registration for automatic segmentation

Author: Greg Sharp (Department of Radiation Oncology, Massachusetts General Hospital), Julien Finet (Kitware)

# Tutorials

- [Download tutorial](https://github.com/SlicerRt/SlicerRtDoc/blob/master/tutorials/3D_Slicer_Plastimatch_Landmark_Registration_Tutorial.ppt)
- [Download tutorial data](https://github.com/SlicerRt/SlicerRtData/tree/master/landwarp-tutorial-images)

## Panels and their use

- Fixed Volume: Here you choose the "fixed image", which is the reference image.
- Moving Volume: Here you choose the "moving image", which will be warped to match the fixed image.
- Output Volume: Here you choose where to put the warped image. You can replace an existing image in the scene, or create a new image.
- Basis function: Here you can choose either tps (thin plate splines), or gauss (Gaussian RBF), or wendland (Wendland RBF).
- RBF radius: Here you can choose the radius of RBF.
- Number of clusters: Here you can choose the number of landmark clusters.
- Stiffness: Here you can choose the regularization parameter.
- Default Pixel Value: Here you can choose the value for pixels with unknown value.

## References

- G Sharp et al. "Plastimatch - An open source software suite for radiotherapy image processing," Proceedings of the XVIth International Conference on the use of Computers in Radiotherapy, May, 2010.
- N. Shusharina, G. Sharp "Landmark-based image registration with analytic regularization", IEEE Trans. Med. Imag., submitted, 2011.

## Acknowledgements

Acknowledgments:
- An Ira J Spiro translational research grant (2009)
- NIH / NCI 6-PO1 CA 21239
- The Federal share of program income earned by MGH on C06CA059267
- Progetto Rocca Foundation – A collaboration between MIT and Politecnico di Milano
- The National Alliance for Medical Image Computing (NAMIC), funded by the National Institutes of Health through the NIH Roadmap for Medical Research, Grant 2-U54-EB005149; information on the National Centers for Biomedical Computing can be obtained from http://nihroadmap.nih.gov/bioinformatics
- NSF ERC Innovation Award EEC-0946463

Webpage: www.plastimatch.org
