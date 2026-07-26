// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <vector>

namespace sysid {

/**
 * Single frequency response sample point.
 */
struct BodePoint {
  double frequencyRadPerSec;  // Frequency in radians per second (omega)
  double frequencyHz;         // Frequency in Hertz
  double magnitudeDb;         // Transfer function magnitude in decibels 20*log10(|H(jw)|)
  double phaseDeg;            // Transfer function phase angle in degrees
};

/**
 * System stability, time constant, and frequency response analysis.
 */
struct BodeAnalysisResult {
  std::vector<BodePoint> points;
  double timeConstantSec;   // System time constant tau = Ka / Kv
  double bandwidthRadPerSec; // Cutoff frequency omega_c = 1 / tau = Kv / Ka
  double bandwidthHz;        // Cutoff frequency in Hz
  double dcGainDb;           // DC gain in dB (20*log10(1/Kv))
  double poleLocation;       // Continuous pole location s = -Kv / Ka
  double settlingTimeSec;    // 95% settling time estimate (3 * tau)
  bool isValid = false;
};

/**
 * Calculates the frequency response (Bode magnitude & phase curves) and system dynamics
 * metrics from feedforward gains Kv and Ka.
 *
 * @param Kv Velocity gain (V / (unit/s))
 * @param Ka Acceleration gain (V / (unit/s^2))
 * @param minFreqRadSec Starting frequency in rad/s (default 0.1)
 * @param maxFreqRadSec Ending frequency in rad/s (default 1000.0)
 * @param numPoints Number of logarithmically spaced frequency points (default 200)
 * @return BodeAnalysisResult containing magnitude/phase curves and system poles
 */
BodeAnalysisResult CalculateBodeAnalysis(double Kv, double Ka,
                                         double minFreqRadSec = 0.1,
                                         double maxFreqRadSec = 1000.0,
                                         size_t numPoints = 200);

}  // namespace sysid
