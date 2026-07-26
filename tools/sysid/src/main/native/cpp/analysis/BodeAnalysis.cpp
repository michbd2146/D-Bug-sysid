// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "wpi/sysid/analysis/BodeAnalysis.hpp"

#include <cmath>
#include <numbers>

namespace sysid {

BodeAnalysisResult CalculateBodeAnalysis(double Kv, double Ka,
                                         double minFreqRadSec,
                                         double maxFreqRadSec,
                                         size_t numPoints) {
  BodeAnalysisResult result;

  if (!std::isfinite(Kv) || !std::isfinite(Ka) || Kv <= 0.0 || Ka <= 0.0) {
    result.isValid = false;
    return result;
  }

  result.isValid = true;
  result.timeConstantSec = Ka / Kv;
  result.bandwidthRadPerSec = Kv / Ka;
  result.bandwidthHz = result.bandwidthRadPerSec / (2.0 * std::numbers::pi);
  result.dcGainDb = 20.0 * std::log10(1.0 / Kv);
  result.poleLocation = -Kv / Ka;
  result.settlingTimeSec = 3.0 * result.timeConstantSec;

  result.points.reserve(numPoints);

  const double logMin = std::log10(minFreqRadSec);
  const double logMax = std::log10(maxFreqRadSec);
  const double step = (logMax - logMin) / static_cast<double>(numPoints - 1);

  for (size_t i = 0; i < numPoints; ++i) {
    const double w = std::pow(10.0, logMin + static_cast<double>(i) * step);
    const double freqHz = w / (2.0 * std::numbers::pi);

    // H(jw) = 1 / (Ka * jw + Kv)
    // |H(jw)| = 1 / sqrt(Kv^2 + (Ka * w)^2)
    const double magnitudeLinear = 1.0 / std::hypot(Kv, Ka * w);
    const double magnitudeDb = 20.0 * std::log10(magnitudeLinear);

    // phase = -atan2(Ka * w, Kv) in degrees
    const double phaseDeg = -std::atan2(Ka * w, Kv) * (180.0 / std::numbers::pi);

    result.points.push_back(BodePoint{w, freqHz, magnitudeDb, phaseDeg});
  }

  return result;
}

}  // namespace sysid
