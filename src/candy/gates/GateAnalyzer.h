#ifndef MGA
#define MGA

#include <cstdlib>
#include <algorithm>

#include <vector>
#include <set>

#include <climits>

#include "../core/CNFProblem.h"

#include "core/SolverTypes.h"
#include "core/Utilities.h"
#include "core/Solver.h"

namespace Candy {

typedef struct Gate {
  Lit out = Glucose::lit_Undef;
  For fwd, bwd;
  bool notMono = false;
  vector<Lit> inp;

  inline bool isDefined() { return out != Glucose::lit_Undef; }

  // Compatibility functions
  inline Lit getOutput() { return out; }
  inline For& getForwardClauses() { return fwd; }
  inline For& getBackwardClauses() { return bwd; }
  inline vector<Lit>& getInputs() { return inp; }
  inline bool hasNonMonotonousParent() { return notMono; }
  // End of compatibility functions
} Gate;


class GateAnalyzer {

public:
  GateAnalyzer(CNFProblem& dimacs, int tries = 0,
      bool patterns = true, bool semantic = true, bool holistic = true, bool lookahead = false, bool intensify = true, int lookahead_threshold = 10);

  // main analysis routine
  void analyze();

  // public getters
  int getGateCount() { return nGates; }
  Gate& getGate(Lit output) { return (*gates)[var(output)]; }


  void printGates() {
    vector<Lit> outputs;
    vector<bool> done(problem.nVars());
    for (Cl* root : roots) {
      outputs.insert(outputs.end(), root->begin(), root->end());
    }
    for (size_t i = 0; i < outputs.size(); i++) {
      Gate& gate = getGate(outputs[i]);

      if (gate.isDefined() && !done[var(outputs[i])]) {
        done[var(outputs[i])] = true;
        printf("Gate with output ");
        printLit(gate.getOutput());
        printf("Is defined by clauses ");
        printFormula(gate.getForwardClauses(), false);
        printFormula(gate.getBackwardClauses(), true);
        outputs.insert(outputs.end(), gate.getInputs().begin(), gate.getInputs().end());
      }
    }
  }

  const vector<Cl*> getRoots() const {
    return roots;
  }

private:
  // problem to analyze:
  CNFProblem problem;
  Glucose::Solver solver;

  // control structures:
  vector<For> index; // occurrence lists
  vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)

  // heuristic configuration:
  int maxTries = 0;
  bool usePatterns = false;
  bool useSemantic = false;
  bool useHolistic = false;
  bool useLookahead = false;
  bool useIntensification = false;
  int lookaheadThreshold = 10;

  // analyzer output:
  vector<Cl*> roots; // top-level clauses
  vector<Gate>* gates; // stores gate-struct for every output
  int nGates = 0;

  void printLit(Lit l) {
    printf("%s%i ", sign(l)?"-":"", var(l)+1);
  }
  void printClause(Cl* c, bool nl = false) {
    for (Lit l : *c) printLit(l);
    printf("; ");
    if (nl) printf("\n");
  }
  void printFormula(For& f, bool nl = false) {
    for (Cl* c : f) printClause(c, false);
    if (nl) printf("\n");
  }

  // main analysis routines
  void analyze(vector<Lit>& candidates);
  vector<Lit> analyze(vector<Lit>& candidates, bool pat, bool sem, bool dec);

  // clause selection heuristic
  Lit getRarestLiteral(vector<For>& index);

  // clause patterns of full encoding
  bool patternCheck(Lit o, For& fwd, For& bwd, set<Lit>& inputs);
  bool semanticCheck(Var o, For& fwd, For& bwd);

  // work in progress:
  bool isBlockedAfterVE(Lit o, For& f, For& g);

  // some helpers:
  bool isBlocked(Lit o, Cl& a, Cl& b) { // assert ~o \in a and o \in b
    for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
    return false;
  }

  bool isBlocked(Lit o, For& f, For& g) { // assert ~o \in f[i] and o \in g[j]
    for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) return false;
    return true;
  }

  bool isBlocked(Lit o, Cl* c, For& f) { // assert ~o \in c and o \in f[i]
    for (Cl* a : f) if (!isBlocked(o, *c, *a)) return false;
    return true;
  }

  bool fixedClauseSize(For& f, unsigned int n) {
    for (Cl* c : f) if (c->size() != n) return false;
    return true;
  }

  void removeFromIndex(vector<For>& index, Cl* clause) {
    for (Lit l : *clause) {
      For& h = index[l];
      h.erase(remove(h.begin(), h.end(), clause), h.end());
    }
  }

  void removeFromIndex(vector<For>& index, For& clauses) {
    For copy(clauses.begin(), clauses.end());
    for (Cl* c : copy) {
      removeFromIndex(index, c);
    }
  }

  vector<For> buildIndexFromClauses(For& f) {
    vector<For> index(2 * problem.nVars());
    for (Cl* c : f) for (Lit l : *c) {
      index[l].push_back(c);
    }
    return index;
  }

};

}
#endif