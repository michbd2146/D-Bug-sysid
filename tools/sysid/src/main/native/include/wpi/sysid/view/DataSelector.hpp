// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <functional>
#include <future>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "wpi/datalog/DataLogReaderThread.hpp"
#include "wpi/glass/View.hpp"
#include "wpi/sysid/analysis/Storage.hpp"

namespace wpi::glass {
class Storage;
}  // namespace wpi::glass

namespace wpi {
namespace log {
class DataLogReaderEntry;
class DataLogReaderThread;
}  // namespace log
namespace util {
class Logger;
}  // namespace util
}  // namespace wpi

namespace sysid {
/**
 * Helps with loading datalog files.
 */
class DataSelector : public wpi::glass::View {
 public:
  /**
   * Represents a SysId routine detected in a log file. Carries the
   * matched log entries for the test state, voltage, position, and velocity.
   */
  struct DetectedRoutine {
    /// Human-readable routine name (the {logName} part from
    /// "sysid-test-state-{logName}").
    std::string name;
    const wpi::log::DataLogReaderEntry* testStateEntry = nullptr;
    const wpi::log::DataLogReaderEntry* voltageEntry = nullptr;
    const wpi::log::DataLogReaderEntry* positionEntry = nullptr;
    const wpi::log::DataLogReaderEntry* velocityEntry = nullptr;
  };

  /**
   * Creates a data selector widget
   *
   * @param storage Glass Storage
   * @param logger The program logger
   */
  explicit DataSelector(wpi::glass::Storage& storage, wpi::util::Logger& logger)
      : m_logger{logger} {}

  /**
   * Displays the log loader window.
   */
  void Display() override;

  /**
   * Resets view. Must be called whenever the DataLogReader goes away, as this
   * class keeps references to DataLogReaderEntry objects.
   */
  void Reset();

  /**
   * Scans the given DataLogReaderThread for SysId routine entries and
   * populates m_detectedRoutines. Call this whenever a new log is loaded.
   *
   * @param reader The log reader to scan. May be nullptr to clear state.
   */
  void SetReader(wpi::log::DataLogReaderThread* reader);

  /**
   * Called when new test data is loaded.
   */
  std::function<void(TestData)> testdata;
  std::vector<std::string> m_missingTests;

 private:
  wpi::util::Logger& m_logger;
  using Runs = std::vector<std::pair<int64_t, int64_t>>;
  using State = std::map<std::string, Runs, std::less<>>;   // full name
  using Tests = std::map<std::string, State, std::less<>>;  // e.g. "dynamic"
  std::future<Tests> m_testsFuture;
  Tests m_tests;
  std::string m_selectedTest;
  const wpi::log::DataLogReaderEntry* m_testStateEntry = nullptr;
  const wpi::log::DataLogReaderEntry* m_velocityEntry = nullptr;
  const wpi::log::DataLogReaderEntry* m_positionEntry = nullptr;
  const wpi::log::DataLogReaderEntry* m_voltageEntry = nullptr;
  double m_velocityScale = 1.0;
  double m_positionScale = 1.0;
  int m_selectedUnit = 0;
  int m_selectedAnalysis = 0;
  std::future<TestData> m_testdataFuture;
  std::vector<std::string> m_testdataStats;
  std::set<std::string> kValidTests = {"quasistatic-forward",
                                       "quasistatic-reverse", "dynamic-forward",
                                       "dynamic-reverse"};
  std::set<std::string> m_executedTests;
  bool m_testCountValidated = false;

  // Auto-detected SysId routines
  std::vector<DetectedRoutine> m_detectedRoutines;
  int m_selectedRoutine = 0;

  static Tests LoadTests(const wpi::log::DataLogReaderEntry& testStateEntry);
  TestData BuildTestData();

  /// Displays the auto-detection section.
  void DisplayAutoDetect();
  /// Loads a specific auto-detected routine by index into the manual fields.
  void ApplyDetectedRoutine(int index);
};
}  // namespace sysid
