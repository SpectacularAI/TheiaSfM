// Copyright (C) 2015 The Regents of the University of California (Regents).
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents or University of California nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Chris Sweeney (cmsweeney@cs.ucsb.edu)

#include "theia/sfm/estimators/estimate_calibrated_absolute_pose.h"

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <vector>

#include "theia/sfm/create_and_initialize_ransac_variant.h"
#include "theia/sfm/estimators/feature_correspondence_2d_3d.h"
#include "theia/sfm/pose/perspective_three_point.h"
#include "theia/solvers/estimator.h"
#include "theia/solvers/sample_consensus_estimator.h"
#include "theia/util/util.h"

namespace theia {
namespace {

using Eigen::Matrix3d;
using Eigen::Vector3d;

// An estimator for computing the absolute pose from 3 feature
// correspondences. The feature correspondences should be normalized by the
// focal length with the principal point at (0, 0).
class CalibratedAbsolutePoseEstimator
    : public Estimator<FeatureCorrespondence2D3D, CalibratedAbsolutePose> {
 private:
    // note: not "fixed-size-vectorizable" so no need to use aligned_allocator
    // https://eigen.tuxfamily.org/dox/group__TopicFixedSizeVectorizable.html
    std::vector<Eigen::Matrix3d> rotations;
    std::vector<Eigen::Vector3d> translations;
 public:
  CalibratedAbsolutePoseEstimator() {}

  // 3 correspondences are needed to determine the absolute pose.
  double SampleSize() const { return 3; }

  // Estimates candidate absolute poses from correspondences.
  bool EstimateModel(
      const std::vector<FeatureCorrespondence2D3D>& correspondences,
      std::vector<CalibratedAbsolutePose>* absolute_poses) const {
    const Eigen::Vector2d features[3] = {correspondences[0].feature,
                                         correspondences[1].feature,
                                         correspondences[2].feature};
    const Eigen::Vector3d world_points[3] = {correspondences[0].world_point,
                                             correspondences[1].world_point,
                                             correspondences[2].world_point};

    // avoid massive reallocation if called frequently
    auto mutable_this = const_cast<CalibratedAbsolutePoseEstimator&>(*this);
    auto &rotations = mutable_this.rotations;
    auto &translations = mutable_this.translations;
    rotations.clear();
    translations.clear();
    if (!PoseFromThreePoints(features,
                             world_points,
                             &rotations,
                             &translations)) {
      return false;
    }

    for (int i = 0; i < rotations.size(); i++) {
      CalibratedAbsolutePose pose;
      pose.rotation = rotations[i];
      pose.position = -pose.rotation.transpose() * translations[i];
      absolute_poses->emplace_back(pose);
    }

    return absolute_poses->size() > 0;
  }

  // The error for a correspondences given an absolute pose. This is the squared
  // reprojection error.
  double Error(const FeatureCorrespondence2D3D& correspondence,
               const CalibratedAbsolutePose& absolute_pose) const {
    // The reprojected point is computed as R * (X - c) where R is the camera
    // rotation, c is the position, and X is the 3D point.
    // 2022-07-13 MSVC optimized this very poorly unless each operation was done on separate lines
    const Eigen::Vector3d translated = correspondence.world_point - absolute_pose.position;
    const Eigen::Vector3d rotated = absolute_pose.rotation * translated;
    const Eigen::Vector2d reprojected_feature = rotated.hnormalized();
    const Eigen::Vector2d error = reprojected_feature - correspondence.feature;
    return error.squaredNorm();
  }
};

class ReusableCalibratedAbsolutePoseEstimatorImpl : public ReusableCalibratedAbsolutePoseEstimator {
private:
  CalibratedAbsolutePoseEstimator absolute_pose_estimator;
  std::unique_ptr<SampleConsensusEstimator<CalibratedAbsolutePoseEstimator> > ransac;

public:
  ReusableCalibratedAbsolutePoseEstimatorImpl(
    const RansacParameters& ransac_params,
    const RansacType& ransac_type)
  {
    ransac = CreateAndInitializeRansacVariant(ransac_type, ransac_params, absolute_pose_estimator);
  }

  bool estimate(
      const std::vector<FeatureCorrespondence2D3D>& normalized_correspondences,
      CalibratedAbsolutePose* absolute_pose,
      RansacSummary* ransac_summary) final {
      // Estimate the absolute pose.
      return ransac->Estimate(normalized_correspondences, absolute_pose, ransac_summary);
  }
};
}  // namespace

std::unique_ptr<ReusableCalibratedAbsolutePoseEstimator> ReusableCalibratedAbsolutePoseEstimator::build(
      const RansacParameters& ransac_params,
      const RansacType& ransac_type) {
  return std::make_unique<ReusableCalibratedAbsolutePoseEstimatorImpl>(ransac_params, ransac_type);
}

ReusableCalibratedAbsolutePoseEstimator::~ReusableCalibratedAbsolutePoseEstimator() = default;

bool EstimateCalibratedAbsolutePose(
    const RansacParameters& ransac_params,
    const RansacType& ransac_type,
    const std::vector<FeatureCorrespondence2D3D>& normalized_correspondences,
    CalibratedAbsolutePose* absolute_pose,
    RansacSummary* ransac_summary)
{
  return ReusableCalibratedAbsolutePoseEstimatorImpl(ransac_params, ransac_type)
    .estimate(normalized_correspondences, absolute_pose, ransac_summary);
}
}  // namespace theia
