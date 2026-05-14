#ifndef __qSlicerDVHUtils_h
#define __qSlicerDVHUtils_h

#include <itkeigen/Eigen/Core>
#include <algorithm>
#include <vector>

/// Returns the dose value d such that the fraction of voxels with dose >= d
/// equals refVol.  Equivalent to matRad_calcInversDVH.
inline double qSlicerCalcInverseDVH(double refVol, const Eigen::VectorXd& dose)
{
  int n = static_cast<int>(dose.size());
  if (n == 0) return 0.0;

  std::vector<double> sorted(dose.data(), dose.data() + n);
  std::sort(sorted.begin(), sorted.end(), std::greater<double>());

  int idx = static_cast<int>(std::ceil(refVol * n)) - 1;
  idx = std::max(0, std::min(idx, n - 1));
  return sorted[idx];
}

#endif
