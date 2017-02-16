#include <zlib.h>
#include <sys/resource.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "core/CNFProblem.h"
#include "simp/SimpSolver.h"
#include "gates/GateAnalyzer.h"

using namespace Candy;

TEST (GateAnalyzerTestPatterns, countgates) {
  CNFProblem dimacs;
  dimacs.readDimacsFromFile("velev-vliw-uns-2.0-uq5.cnf");

  // create test out
  GateAnalyzer gates(dimacs, 0, true, false);
  gates.analyze();
  testing::internal::CaptureStdout();
  printf("%d", gates.getGateCount());
  std::string output = testing::internal::GetCapturedStdout();

  ASSERT_EQ (output, "14446");
}

//TEST (GateAnalyzerTestPatternSemantic, countgates) {
//  CNFProblem dimacs;
//  dimacs.readDimacsFromFile("velev-vliw-uns-2.0-uq5.cnf");
//
//  // create test out
//  GateAnalyzer gates(dimacs, 0, true, true, false, false, false);
//  gates.analyze();
//  testing::internal::CaptureStdout();
//  printf("%d", gates.getGateCount());
//  std::string output = testing::internal::GetCapturedStdout();
//
//  ASSERT_EQ (output, "70229");
//}
//TEST (GateAnalyzerTestPatternSemanticIntensification, countgates) {
//  CNFProblem dimacs;
//  dimacs.readDimacsFromFile("velev-vliw-uns-2.0-uq5.cnf");
//
//  // create test out
//  GateAnalyzer gates(dimacs, 0, true, true, false, false, true);
//  gates.analyze();
//  testing::internal::CaptureStdout();
//  printf("%d", gates.getGateCount());
//  std::string output = testing::internal::GetCapturedStdout();
//
//  ASSERT_EQ (output, "70229");
//}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
