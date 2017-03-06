/***************************************************************************************[Solver.cc]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 LRI  - Univ. Paris Sud, France (2009-2013)
 Labri - Univ. Bordeaux, France

 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 Labri - Univ. Bordeaux, France

 Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
 Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it
 is based on. (see below).

 Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
 version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
 without restriction, including the rights to use, copy, modify, merge, publish, distribute,
 sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 - The above and below copyrights notices and this permission notice shall be included in all
 copies or substantial portions of the Software;
 - The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
 be used in any competitive event (sat competitions/evaluations) without the express permission of
 the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
 using Glucose Parallel as an embedded SAT engine (single core or not).


 --------------- Original Minisat Copyrights

 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include <math.h>

#include "candy/utils/System.h"
#include "candy/core/Solver.h"
#include "candy/core/Constants.h"
#include "candy/core/SolverTypes.h"

using namespace Glucose;

//=================================================================================================
// Options:

static const char* _cat = "CORE";
static const char* _cr = "CORE -- RESTART";
static const char* _cred = "CORE -- REDUCE";
static const char* _cm = "CORE -- MINIMIZE";

static DoubleOption opt_K(_cr, "K", "The constant used to force restart", 0.8, DoubleRange(0, false, 1, false));
static DoubleOption opt_R(_cr, "R", "The constant used to block restart", 1.4, DoubleRange(1, false, 5, false));
static IntOption opt_size_lbd_queue(_cr, "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT32_MAX));
static IntOption opt_size_trail_queue(_cr, "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT32_MAX));

static IntOption opt_first_reduce_db(_cred, "firstReduceDB", "The number of conflicts before the first reduce DB", 2000, IntRange(0, INT32_MAX));
static IntOption opt_inc_reduce_db(_cred, "incReduceDB", "Increment for reduce DB", 300, IntRange(0, INT32_MAX));
static IntOption opt_spec_inc_reduce_db(_cred, "specialIncReduceDB", "Special increment for reduce DB", 1000, IntRange(0, INT32_MAX));
static IntOption opt_lb_lbd_frozen_clause(_cred, "minLBDFrozenClause", "Protect clauses if their LBD decrease and is lower than (for one turn)", 30,
        IntRange(0, INT32_MAX));

static IntOption opt_lb_size_minimzing_clause(_cm, "minSizeMinimizingClause", "The min size required to minimize clause", 30, IntRange(3, INT32_MAX));
static IntOption opt_lb_lbd_minimzing_clause(_cm, "minLBDMinimizingClause", "The min LBD required to minimize clause", 6, IntRange(3, INT32_MAX));

static DoubleOption opt_var_decay(_cat, "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
static DoubleOption opt_max_var_decay(_cat, "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
static DoubleOption opt_clause_decay(_cat, "cla-decay", "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));
static DoubleOption opt_random_var_freq(_cat, "rnd-freq", "The frequency with which the decision heuristic tries to choose a random variable", 0,
        DoubleRange(0, true, 1, true));
static DoubleOption opt_random_seed(_cat, "rnd-seed", "Used by the random variable selection", 91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption opt_ccmin_mode(_cat, "ccmin-mode", "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption opt_phase_saving(_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption opt_rnd_init_act(_cat, "rnd-init", "Randomize the initial activity", false);
static DoubleOption opt_garbage_frac(_cat, "gc-frac", "The fraction of wasted memory allowed before a garbage collection is triggered", 0.20,
        DoubleRange(0, false, HUGE_VAL, false));

static IntOption opt_sonification_delay("SONIFICATION", "sonification-delay", "ms delay after each event to improve realtime sonification", 0,
        IntRange(0, INT32_MAX));

//=================================================================================================
// Constructor/Destructor:

Solver::Solver() :
                verbosity(0), verbEveryConflicts(10000), showModel(0), K(opt_K), R(opt_R), sizeLBDQueue(opt_size_lbd_queue), sizeTrailQueue(
                                opt_size_trail_queue), incReduceDB(opt_inc_reduce_db), specialIncReduceDB(opt_spec_inc_reduce_db), lbLBDFrozenClause(
                                opt_lb_lbd_frozen_clause), lbSizeMinimizingClause(opt_lb_size_minimzing_clause), lbLBDMinimizingClause(
                                opt_lb_lbd_minimzing_clause), var_decay(opt_var_decay), max_var_decay(opt_max_var_decay), clause_decay(opt_clause_decay), random_var_freq(
                                opt_random_var_freq), random_seed(opt_random_seed), ccmin_mode(opt_ccmin_mode), phase_saving(opt_phase_saving), rnd_pol(false), rnd_init_act(
                                opt_rnd_init_act), garbage_frac(opt_garbage_frac), certifiedOutput(NULL), certifiedUNSAT(false),
                // Statistics: (formerly in 'SolverStats')
                //
                sumDecisionLevels(0), nbRemovedClauses(0), nbReducedClauses(0), nbDL2(0), nbBin(0), nbUn(0), nbReduceDB(0), solves(0), starts(
                                0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0), conflictsRestarts(0), nbstopsrestarts(0), nbstopsrestartssame(
                                0), lastblockatrestart(0), dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0), curRestart(
                                1),

                ok(true), cla_inc(1), var_inc(1), watches(WatcherDeleted()), watchesBin(WatcherDeleted()), trail_size(0), qhead(
                                0), simpDB_assigns(-1), simpDB_props(0), order_heap(VarOrderLt(activity)), remove_satisfied(true), reduceOnSize(false), reduceOnSizeSize(
                                12) /* constant to use on size reduction */, nbclausesbeforereduce(opt_first_reduce_db), sumLBD(0), MYFLAG(0),
                // Resource constraints:
                //
                conflict_budget(-1), propagation_budget(-1), asynch_interrupt(false), incremental(false), nbVarsInitialFormula(INT32_MAX), totalTime4Sat(0.), totalTime4Unsat(
                                0.), nbSatCalls(0), nbUnsatCalls(0),
                // Added since Candy
                sonification(), termCallbackState(nullptr), termCallback(nullptr), status(l_Undef) {
    lbdQueue.initSize(sizeLBDQueue);
    trailQueue.initSize(sizeTrailQueue);
}

Solver::~Solver() {
}

/****************************************************************
 Set the incremental mode
 ****************************************************************/

// This function set the incremental mode to true.
// You can add special code for this mode here.
void Solver::setIncrementalMode() {
    incremental = true;
}

// Number of variables without selectors
void Solver::initNbInitialVars(int nb) {
    nbVarsInitialFormula = nb;
}

bool Solver::isIncremental() {
    return incremental;
}

//=================================================================================================
// Minor methods:

// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar) {
    int v = nVars();
    watches.init(mkLit(v, false));
    watches.init(mkLit(v, true));
    watchesBin.init(mkLit(v, false));
    watchesBin.init(mkLit(v, true));
    assigns.push_back(l_Undef);
    vardata.push_back(mkVarData(nullptr, 0));
    activity.push_back(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen.push_back(0);
    permDiff.push_back(0);
    polarity.push_back(sign);
    decision.push_back(0);
    trail.resize(v + 1);
    setDecisionVar(v, dvar);
    return v;
}

void Solver::addClauses(Candy::CNFProblem dimacs) {
    int max = nVars();
    assigns.resize(dimacs.nVars(), l_Undef);
    vardata.resize(dimacs.nVars(), mkVarData(nullptr, 0));
    seen.resize(dimacs.nVars(), 0);
    decision.resize(dimacs.nVars(), 0);
    permDiff.resize(dimacs.nVars(), 0);
    trail.resize(dimacs.nVars(), lit_Undef);
    for (Var v = max; v < (int)dimacs.nVars(); v++) {
        watches.init(mkLit(v, false));
        watches.init(mkLit(v, true));
        watchesBin.init(mkLit(v, false));
        watchesBin.init(mkLit(v, true));
        activity.push_back(dimacs.getRelativeOccurence(mkLit(v, false)) + dimacs.getRelativeOccurence(mkLit(v, true)));
        //polarity.push_back(dimacs.getRelativeOccurence(mkLit(v, false)) - dimacs.getRelativeOccurence(mkLit(v, true)) > 0 ? true : false);
        polarity.push_back(true);
        setDecisionVar(v, true);
    }
    for (vector<Lit>* clause : dimacs.getProblem()) {
        addClause(*clause);
    }
}

bool Solver::addClause_(vector<Lit>& ps) {
    assert(decisionLevel() == 0);
    if (!ok)
        return false;

    std::sort(ps.begin(), ps.end());

    vector<Lit> oc;
    if (certifiedUNSAT) {
        auto pos = find_if(ps.begin(), ps.end(), [this](Lit lit) { return value(lit) == l_True || value(lit) == l_False; });
        if (pos != ps.end()) oc.insert(oc.end(), ps.begin(), ps.end());
    }

    Lit p;
    unsigned int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        if (value(ps[i]) == l_True || ps[i] == ~p) {
            return true;
        }
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];
        }
    }
    ps.resize(j);

    if (oc.size() > 0 && certifiedUNSAT) {
        printCertificateLearnt(ps);
        printCertificateRemoved(oc);
    }

    if (ps.size() == 0) {
        return ok = false;
    } else if (ps.size() == 1) {
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == nullptr);
    } else {
        Candy::Clause* cr = new (ps.size()) Candy::Clause(ps, false);
        clauses.push_back(cr);
        sort(cr->begin(), cr->end(), [this](Lit lit1, Lit lit2) { return activity[var(lit1)] < activity[var(lit2)]; });
        attachClause(cr);
    }

    return true;
}

void Solver::attachClause(Candy::Clause* cr) {
    assert(cr->size() > 1);
    Candy::Clause& c = *cr;

    if (c.size() == 2) {
        watchesBin[~c[0]].push_back(Watcher(cr, c[1]));
        watchesBin[~c[1]].push_back(Watcher(cr, c[0]));
    } else {
        watches[~c[0]].push_back(Watcher(cr, c[1]));
        watches[~c[1]].push_back(Watcher(cr, c[0]));
    }

    if (c.isLearnt()) {
        learnts_literals += c.size();
    } else {
        clauses_literals += c.size();
    }
}

void Solver::detachClause(Candy::Clause* cr, bool strict) {
    assert(cr->size() > 1);
    Candy::Clause& c = *cr;

    if (c.isLearnt()) {
        learnts_literals -= c.size();
    } else {
        clauses_literals -= c.size();
    }

    if (c.size() == 2) {
        if (strict) {
            vector<Watcher>& list0 = watchesBin[~c[0]];
            vector<Watcher>& list1 = watchesBin[~c[1]];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [cr](Watcher w){ return w.cref == cr; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [cr](Watcher w){ return w.cref == cr; }), list1.end());
        } else {
            // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
            watchesBin.smudge(~c[0]);
            watchesBin.smudge(~c[1]);
        }
    } else {
        if (strict) {
            vector<Watcher>& list0 = watches[~c[0]];
            vector<Watcher>& list1 = watches[~c[1]];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [cr](Watcher w){ return w.cref == cr; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [cr](Watcher w){ return w.cref == cr; }), list1.end());
        } else {
            // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
            watches.smudge(~c[0]);
            watches.smudge(~c[1]);
        }
    }
}

void Solver::removeClause(Candy::Clause* cr) {
    Candy::Clause& c = *cr;
    if (certifiedUNSAT) {
        printCertificateRemoved(cr);
    }
    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(cr))
        vardata[var(c[0])].reason = nullptr;
    c.setDeleted();
}

bool Solver::satisfied(const Candy::Clause& c) const {
    return std::any_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) == l_True; });
}

/************************************************************
 * Compute LBD functions
 *************************************************************/
inline unsigned int Solver::computeLBD(const vector<Lit>& lits, int end) {
    int nblevels = 0;
    MYFLAG++;

    if (incremental) { // ----------------- INCREMENTAL MODE
        if (end == -1)
            end = lits.size();
        int nbDone = 0;
        for (unsigned int i = 0; i < lits.size() && nbDone < end; i++) {
            if (isSelector(var(lits[i]))) {
                continue;
            } else {
                nbDone++;
            }
            int l = level(var(lits[i]));
            if (permDiff[l] != MYFLAG) {
                permDiff[l] = MYFLAG;
                nblevels++;
            }
        }
    } else { // -------- DEFAULT MODE. NOT A LOT OF DIFFERENCES... BUT EASIER TO READ
        for (unsigned int i = 0; i < lits.size(); i++) {
            int l = level(var(lits[i]));
            if (permDiff[l] != MYFLAG) {
                permDiff[l] = MYFLAG;
                nblevels++;
            }
        }
    }

    if (!reduceOnSize)
        return nblevels;
    if ((int) lits.size() < reduceOnSizeSize)
        return lits.size(); // See the XMinisat paper
    return lits.size() + nblevels;
}

inline unsigned int Solver::computeLBD(const Candy::Clause &c) {
    int nblevels = 0;
    MYFLAG++;

    if (incremental) { // ----------------- INCREMENTAL MODE
        for (unsigned int i = 0; i < c.size(); i++) {
            if (isSelector(var(c[i]))) {
                continue;
            }
            int l = level(var(c[i]));
            if (permDiff[l] != MYFLAG) {
                permDiff[l] = MYFLAG;
                nblevels++;
            }
        }
    } else { // -------- DEFAULT MODE. NOT A LOT OF DIFFERENCES... BUT EASIER TO READ
        for (unsigned int i = 0; i < c.size(); i++) {
            int l = level(var(c[i]));
            if (permDiff[l] != MYFLAG) {
                permDiff[l] = MYFLAG;
                nblevels++;
            }
        }
    }

    if (!reduceOnSize)
        return nblevels;
    if ((int)c.size() < reduceOnSizeSize)
        return c.size(); // See the XMinisat paper
    return c.size() + nblevels;

}

/******************************************************************
 * Minimisation with binary reolution
 ******************************************************************/
void Solver::minimisationWithBinaryResolution(vector<Lit> &out_learnt) {
    // Find the LBD measure
    unsigned int lbd = computeLBD(out_learnt);
    Lit p = ~out_learnt[0];

    if (lbd <= lbLBDMinimizingClause) {
        MYFLAG++;

        for (unsigned int i = 1; i < out_learnt.size(); i++) {
            permDiff[var(out_learnt[i])] = MYFLAG;
        }

        vector<Watcher>& wbin = watchesBin[p];
        int nb = 0;
        for (Watcher& watcher : wbin) {
            if (permDiff[var(watcher.blocker)] == MYFLAG && value(watcher.blocker) == l_True) {
                nb++;
                permDiff[var(watcher.blocker)] = MYFLAG - 1;
            }
        }
        if (nb > 0) {
            nbReducedClauses++;
            for (unsigned int i = 1, l = out_learnt.size()-1; i < out_learnt.size() - nb; i++) {
                if (permDiff[var(out_learnt[i])] != MYFLAG) {
                    std::swap(out_learnt[i], out_learnt[l]);
                    l--;
                    i--;
                }
            }
            out_learnt.resize(out_learnt.size() - nb);
        }
    }
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if ((int)decisionLevel() > level) {
        for (int c = trail_size - 1; c >= trail_lim[level]; c--) {
            Var x = var(trail[c]);
            assigns[x] = l_Undef;
            if (phase_saving > 1 || ((phase_saving == 1) && c > trail_lim.back())) {
                polarity[x] = sign(trail[c]);
            }
            insertVarOrder(x);
        }
        qhead = trail_lim[level];
        trail_size = trail_lim[level];
        trail_lim.resize(level);
    }
}

//=================================================================================================
// Major methods:

Lit Solver::pickBranchLit() {
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty()) {
        next = order_heap[irand(random_seed, order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++;
    }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        } else {
            next = order_heap.removeMin();
        }

    return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}

/*_________________________________________________________________________________________________
 |
 |  analyze : (confl : Clause*) (out_learnt : vector<Lit>&) (out_btlevel : int&)  ->  [void]
 |
 |  Description:
 |    Analyze conflict and produce a reason clause.
 |
 |    Pre-conditions:
 |      * 'out_learnt' is assumed to be cleared.
 |      * Current decision level must be greater than root level.
 |
 |    Post-conditions:
 |      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
 |      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
 |        rest of literals. There may be others from the same level though.
 |
 |________________________________________________________________________________________________@*/
void Solver::analyze(Candy::Clause* confl, vector<Lit>& out_learnt, int& out_btlevel, unsigned int &lbd) {
    int pathC = 0;
    Lit asslit = lit_Undef;
    vector<Lit> selectors;

    // Generate conflict clause:
    out_learnt.push_back(Glucose::lit_Undef); // (leave room for the asserting literal)
    int index = trail_size - 1;
    do {
        assert(confl != nullptr); // (otherwise should be UIP)
        Candy::Clause& c = *confl;

        // Special case for binary clauses: The first one has to be SAT
        if (asslit != lit_Undef && c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            c.swap(0, 1);
        }

        if (c.isLearnt()) {
            claBumpActivity(c);
        }

        // DYNAMIC NBLEVEL trick (see competition'09 companion paper)
        if (c.isLearnt() && c.getLBD() > 2) {
            unsigned int nblevels = computeLBD(c);
            if (nblevels + 1 < c.getLBD()) { // improve the LBD
                if (c.getLBD() <= lbLBDFrozenClause) {
                    c.setFrozen(true);
                }
                // seems to be interesting : keep it for the next round
                c.setLBD(nblevels); // Update it
            }
        }

        for (unsigned int j = (asslit == lit_Undef) ? 0 : 1; j < c.size(); j++) {
            Lit lit = c[j];

            if (!seen[var(lit)] && level(var(lit)) != 0) {
                varBumpActivity(var(lit));
                seen[var(lit)] = 1;
                if (level(var(lit)) >= (int)decisionLevel()) {
                    pathC++;
                    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
                    if ((reason(var(lit)) != nullptr) && reason(var(lit))->isLearnt()) {
                        lastDecisionLevel.push_back(lit);
                    }
                } else {
                    if (isSelector(var(lit))) {
                        assert(value(lit) == l_False);
                        selectors.push_back(lit);
                    } else {
                        out_learnt.push_back(lit);
                    }
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index])]) {
            index--;
        }

        asslit = trail[index];
        confl = reason(var(asslit));
        seen[var(asslit)] = 0;
        pathC--;
    } while (pathC > 0);

    out_learnt[0] = ~asslit;

    // Simplify conflict clause:
    out_learnt.insert(out_learnt.end(), selectors.begin(), selectors.end());
    max_literals += out_learnt.size();

    analyze_toclear.clear();
    analyze_toclear.insert(analyze_toclear.end(), out_learnt.begin(), out_learnt.end());

    if (ccmin_mode == 2) {
        uint32_t abstract_level = 0;
        for (unsigned int i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        auto end = remove_if(out_learnt.begin()+1, out_learnt.end(),
                [this, abstract_level] (Lit lit) {
                    return reason(var(lit)) != nullptr && litRedundant(lit, abstract_level);
                });
        out_learnt.erase(end, out_learnt.end());
    }
    else if (ccmin_mode == 1) {
        auto end = remove_if(out_learnt.begin()+1, out_learnt.end(),
                [this] (Lit lit) {
                    return reason(var(lit)) != nullptr && seenAny(*reason(var(lit)));
                });
        out_learnt.erase(end, out_learnt.end());
    }

    assert(out_learnt[0] == ~asslit);

    tot_literals += out_learnt.size();

    /* ***************************************
     Minimisation with binary clauses of the asserting clause
     First of all : we look for small clauses
     Then, we reduce clauses with small LBD.
     Otherwise, this can be useless */
    if (!incremental && (int)out_learnt.size() <= lbSizeMinimizingClause) {
        minimisationWithBinaryResolution(out_learnt);
    }

    // Find correct backtrack level:
    if (out_learnt.size() == 1) {
        out_btlevel = 0;
    }
    else {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (unsigned int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1] = p;
        out_btlevel = level(var(p));
    }

    // Compute LBD
    lbd = computeLBD(out_learnt, out_learnt.size() - selectors.size());

    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
    if (lastDecisionLevel.size() > 0) {
        for (Lit lit : lastDecisionLevel) {
            if (reason(var(lit))->getLBD() < lbd)
                varBumpActivity(var(lit));
        }
        lastDecisionLevel.clear();
    }

    // clear seen[]
    for_each(analyze_toclear.begin(), analyze_toclear.end(), [this] (Lit lit) { seen[var(lit)] = 0; });
}

bool Solver::seenAny(Candy::Clause& c) {
    Candy::Clause::iterator begin = (c.size() == 2) ? c.begin() : c.begin() + 1;
    return std::none_of(begin, c.end(), [this] (Lit clit) { return !seen[var(clit)] && level(var(clit)) > 0; });
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels) {
    analyze_stack.clear();
    analyze_stack.push_back(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        assert(reason(var(analyze_stack.back())) != nullptr);
        Candy::Clause& c = *reason(var(analyze_stack.back()));
        analyze_stack.pop_back(); //
        if (c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            c.swap(0, 1);
        }

        for (Lit p : c) {
            if (!seen[var(p)] && level(var(p)) > 0) {
                if (reason(var(p)) != nullptr && (abstractLevel(var(p)) & abstract_levels) != 0) {
                    seen[var(p)] = 1;
                    analyze_stack.push_back(p);
                    analyze_toclear.push_back(p);
                } else {
                    for (unsigned int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.resize(top);
                    return false;
                }
            }
        }
    }

    return true;
}

/*_________________________________________________________________________________________________
 |
 |  analyzeFinal : (p : Lit)  ->  [void]
 |
 |  Description:
 |    Specialized analysis procedure to express the final conflict in terms of assumptions.
 |    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
 |    stores the result in 'out_conflict'.
 |________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vector<Lit>& out_conflict) {
    out_conflict.clear();
    out_conflict.push_back(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail_size - 1; i >= trail_lim[0]; i--) {
        Var x = var(trail[i]);
        if (seen[x]) {
            if (reason(x) == nullptr) {
                assert(level(x) > 0);
                out_conflict.push_back(~trail[i]);
            } else {
                Candy::Clause& c = *reason(x);
                //                for (int j = 1; j < c.size(); j++) Minisat (glucose 2.0) loop
                // Bug in case of assumptions due to special data structures for Binary.
                // Many thanks to Sam Bayless (sbayless@cs.ubc.ca) for discover this bug.
                for (unsigned int j = ((c.size() == 2) ? 0 : 1); j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }

            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::uncheckedEnqueue(Lit p, Candy::Clause* from) {
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail[trail_size++] = p;
}

/*_________________________________________________________________________________________________
 |
 |  propagate : [void]  ->  [Clause*]
 |
 |  Description:
 |    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
 |    otherwise CRef_Undef.
 |
 |    Post-conditions:
 |      * the propagation queue is empty, even if there was a conflict.
 |________________________________________________________________________________________________@*/
Candy::Clause* Solver::propagate() {
    Candy::Clause* confl = nullptr;

    int num_props = trail_size - qhead;
    if (num_props > 0) {
        propagations += num_props;
        simpDB_props -= num_props;
    } else {
        return nullptr;
    }

    // Must remain for now (SimpSolver smudges clauses and propagates)
    // TODO: check is smudge has any runtime benefits
    watches.cleanAll();
    watchesBin.cleanAll();

    while (qhead < trail_size) {
        Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.

        // Propagate binary clauses
        vector<Watcher>& wbin = watchesBin[p];
        for (auto watcher : wbin) {
            if (value(watcher.blocker) == l_False) {
                return watcher.cref;
            }
            if (value(watcher.blocker) == l_Undef) {
                uncheckedEnqueue(watcher.blocker, watcher.cref);
            }
        }

        // Propagate other 2-watched clauses
        vector<Watcher>& ws = watches[p];
        vector<Watcher>::iterator keep = ws.begin();
        for (vector<Watcher>::iterator watcher = ws.begin(); watcher != ws.end();) {
            // Try to avoid inspecting the clause:
            Lit blocker = watcher->blocker;
            if (value(blocker) == l_True) {
                *keep++ = *watcher++;
                continue;
            }

            // Make sure the false literal is data[1]:
            Candy::Clause* cr = watcher->cref;
            if (cr->first() == ~p) {
                cr->swap(0, 1);
            }

            watcher++;
            // If 0th watch is true, then clause is already satisfied.
            Watcher w = Watcher(cr, cr->first());
            if (cr->first() != blocker && value(cr->first()) == l_True) {
                *keep++ = w;
                continue;
            }

            if (incremental) { // INCREMENTAL MODE
                Candy::Clause& c = *cr;
                for (unsigned int k = 2; k < c.size(); k++) {
                    if (value(c[k]) != l_False) {
                        if (decisionLevel() > assumptions.size() || value(c[k]) == l_True || !isSelector(var(c[k]))) {
                            c.swap(1, k);
                            watches[~c[1]].push_back(w);
                            goto NextClause;
                        }
                    }
                }
            }
            else { // DEFAULT MODE (NOT INCREMENTAL)
                Candy::Clause& c = *cr;
                for (unsigned int k = 2; k < c.size(); k++) {
                    if (value(c[k]) != l_False) {
                        c.swap(1, k);
                        watches[~c[1]].push_back(w);
                        goto NextClause;
                    }
                }
            }

            // Did not find watch -- clause is unit under assignment:
            *keep++ = w;
            if (value(cr->first()) == l_False) {
                confl = cr;
                qhead = trail_size;
                while (watcher != ws.end()) { // Copy the remaining watches
                    *keep++ = *watcher++;
                }
            }
            else {
                uncheckedEnqueue(cr->first(), cr);
            }

            NextClause: ;
        }
        ws.erase(keep, ws.end());
    }

    return confl;
}

/*_________________________________________________________________________________________________
 |
 |  reduceDB : ()  ->  [void]
 |
 |  Description:
 |    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
 |    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
 |________________________________________________________________________________________________@*/

void Solver::reduceDB() {
    nbReduceDB++;
    std::sort(learnts.begin(), learnts.end(), reduceDB_lt());

    // We have a lot of "good" clauses, it is difficult to compare them. Keep more !
    if (learnts[learnts.size() / RATIOREMOVECLAUSES]->getLBD() <= 3)
        nbclausesbeforereduce += specialIncReduceDB;
    // Useless :-)
    if (learnts.back()->getLBD() <= 5)
        nbclausesbeforereduce += specialIncReduceDB;

    // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
    // Keep clauses which seem to be useful (their lbd was reduce during this sequence)
    unsigned int limit = learnts.size() / 2;
    for (unsigned int i = 0; i < limit; i++) {
        Candy::Clause& c = *learnts[i];
        //if (c.isFrozen()) break; // frozen clauses area reached early (see reduceDB_lt)
        if (!c.isFrozen() && c.getLBD() > 2 && c.size() > 2 && !locked(learnts[i])) {
            removeClause(learnts[i]);
        }
    }
    watches.cleanAll();
    watchesBin.cleanAll();
    nbRemovedClauses += freeMarkedClauses(learnts);
    for (Candy::Clause* c : learnts) { // "unfreeze" remaining clauses
        c->setFrozen(false);
    }
}

/**
 * Make sure all references are cleared before space is handed back to ClauseAllocator
 */
unsigned int Solver::freeMarkedClauses(vector<Candy::Clause*>& list) {
    auto new_end = std::remove_if(list.begin(), list.end(),
            [this](Candy::Clause* c) {
                if (c->isDeleted()) {
                    delete c;
                    return true;
                } else {
                    return false;
                }
            });
    int nRemoved = std::distance(new_end, list.end());
    list.erase(new_end, list.end());
    return nRemoved;
}

void Solver::rebuildOrderHeap() {
    vector<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push_back(v);
    order_heap.build(vs);
}

/*_________________________________________________________________________________________________
 |
 |  simplify : [void]  ->  [bool]
 |
 |  Description:
 |    Simplify the clause database according to the current top-level assignment. Currently, the only
 |    thing done here is the removal of satisfied clauses, but more things can be put here.
 |________________________________________________________________________________________________@*/
bool Solver::simplify() {
    assert(decisionLevel() == 0);

    if (!ok || propagate() != nullptr) {
        return ok = false;
    }

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0)) {
        return true;
    }

    // Remove satisfied clauses:
    std::for_each(learnts.begin(), learnts.end(), [this] (Candy::Clause* c) { if (satisfied(*c)) removeClause(c); } );
    if (remove_satisfied) { // Can be turned off.
        std::for_each(clauses.begin(), clauses.end(), [this] (Candy::Clause* c) { if (satisfied(*c)) removeClause(c); } );
    }
    watches.cleanAll();
    watchesBin.cleanAll();
    freeMarkedClauses(learnts);
    freeMarkedClauses(clauses);

    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props = clauses_literals + learnts_literals; // (shouldn't depend on stats really, but it will do for now)

    return true;
}

/*_________________________________________________________________________________________________
 |
 |  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
 |
 |  Description:
 |    Search for a model the specified number of conflicts.
 |    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
 |
 |  Output:
 |    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
 |    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
 |    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
 |________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts) {
    assert(ok);
    int backtrack_level;
    vector<Lit> learnt_clause;
    unsigned int nblevels;
    bool blocked = false;
    starts++;
    for (;;) {
        sonification.decisionLevel(decisionLevel(), opt_sonification_delay);

        Candy::Clause* confl = propagate();

        sonification.assignmentLevel(nAssigns());

        if (confl != nullptr) {
            sonification.conflictLevel(decisionLevel());

            sumDecisionLevels += decisionLevel();
            // CONFLICT
            conflicts++;
            conflictsRestarts++;
            if (conflicts % 5000 == 0 && var_decay < max_var_decay)
                var_decay += 0.01;

            if (verbosity >= 1 && conflicts % verbEveryConflicts == 0) {
                printf("c | %8d   %7d    %5d | %7d %8d %8d | %5d %8d   %6d %8d | %6.3f %% |\n", (int) starts, (int) nbstopsrestarts, (int) (conflicts / starts),
                        (int) dec_vars - (int) (trail_lim.size() == 0 ? trail_size : trail_lim[0]), nClauses(), (int) clauses_literals, (int) nbReduceDB,
                        nLearnts(), (int) nbDL2, (int) nbRemovedClauses, -1.0);
            }
            if (decisionLevel() == 0) {
                return l_False;
            }

            trailQueue.push(trail_size);

            // BLOCK RESTART (CP 2012 paper)
            if (conflictsRestarts > LOWER_BOUND_FOR_BLOCKING_RESTART && lbdQueue.isvalid() && trail_size > R * trailQueue.getavg()) {
                lbdQueue.fastclear();
                nbstopsrestarts++;
                if (!blocked) {
                    lastblockatrestart = starts;
                    nbstopsrestartssame++;
                    blocked = true;
                }
            }

            learnt_clause.clear();

            analyze(confl, learnt_clause, backtrack_level, nblevels);

            sonification.learntSize(learnt_clause.size());

            lbdQueue.push(nblevels);
            sumLBD += nblevels;

            cancelUntil(backtrack_level);

            if (certifiedUNSAT) {
                printCertificateLearnt(learnt_clause);
            }

            if (learnt_clause.size() == 1) {
                uncheckedEnqueue(learnt_clause[0]);
                nbUn++;
            } else {
                Candy::Clause* cr = new (learnt_clause.size()) Candy::Clause(learnt_clause, true);
                cr->setLBD(nblevels);
                if (nblevels <= 2)
                    nbDL2++; // stats
                if (cr->size() == 2)
                    nbBin++; // stats
                learnts.push_back(cr);
                sort(cr->begin()+1, cr->end(), [this](Lit lit1, Lit lit2) { return activity[var(lit1)] < activity[var(lit2)]; });
                attachClause(cr);
                claBumpActivity(*cr);
                uncheckedEnqueue(learnt_clause[0], cr);
            }
            varDecayActivity();
            claDecayActivity();
        } else {
            // Our dynamic restart, see the SAT09 competition compagnion paper
            if ((lbdQueue.isvalid() && ((lbdQueue.getavg() * K) > (sumLBD / conflictsRestarts)))) {
                lbdQueue.fastclear();
                int bt = 0;
                if (incremental) // DO NOT BACKTRACK UNTIL 0.. USELESS
                    bt = (decisionLevel() < assumptions.size()) ? decisionLevel() : (int) assumptions.size();
                cancelUntil(bt);
                return l_Undef;
            }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify()) {
                return l_False;
            }
            // Perform clause database reduction !
            if (conflicts >= ((unsigned int) curRestart * nbclausesbeforereduce)) {
                if (learnts.size() > 0) {
                    curRestart = (conflicts / nbclausesbeforereduce) + 1;
                    reduceDB();
                    nbclausesbeforereduce += incReduceDB;
                }
            }

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True) {
                    // Dummy decision level:
                    newDecisionLevel();
                } else if (value(p) == l_False) {
                    analyzeFinal(~p, conflict);
                    return l_False;
                } else {
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef) {
                // New variable decision:
                decisions++;
                next = pickBranchLit();
                if (next == lit_Undef) {
                    // Model found:
                    return l_True;
                }
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
    return l_Undef; // not reached
}

void Solver::printIncrementalStats() {

    printf("c---------- Glucose Stats -------------------------\n");
    printf("c restarts              : %" PRIu64"\n", starts);
    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);

    printf("c conflicts             : %" PRIu64"\n", conflicts);
    printf("c decisions             : %" PRIu64"\n", decisions);
    printf("c propagations          : %" PRIu64"\n", propagations);

    printf("\nc SAT Calls             : %d in %g seconds\n", nbSatCalls, totalTime4Sat);
    printf("c UNSAT Calls           : %d in %g seconds\n", nbUnsatCalls, totalTime4Unsat);

    printf("c--------------------------------------------------\n");
}

// NOTE: assumptions passed in member-variable 'assumptions'.
// Parameters are useless in core but useful for SimpSolver....
lbool Solver::solve_(bool do_simp, bool turn_off_simp) {
    if (incremental && certifiedUNSAT) {
        printf("Can not use incremental and certified unsat in the same time\n");
        exit(-1);
    }

    model.clear();
    conflict.clear();
    if (!ok)
        return l_False;
    double curTime = cpuTime();

    sonification.start(nVars(), nClauses());

    solves++;

    status = l_Undef;
    if (!incremental && verbosity >= 1) {
        printf("c ========================================[ MAGIC CONSTANTS ]==============================================\n");
        printf("c | Constants are supposed to work well together :-)                                                      |\n");
        printf("c | however, if you find better choices, please let us known...                                           |\n");
        printf("c |-------------------------------------------------------------------------------------------------------|\n");
        printf("c |                                |                                |                                     |\n");
        printf("c | - Restarts:                    | - Reduce Clause DB:            | - Minimize Asserting:               |\n");
        printf("c |   * LBD Queue    : %6d      |   * First     : %6d         |    * size < %3d                     |\n", lbdQueue.maxSize(),
                nbclausesbeforereduce, lbSizeMinimizingClause);
        printf("c |   * Trail  Queue : %6d      |   * Inc       : %6d         |    * lbd  < %3d                     |\n", trailQueue.maxSize(), incReduceDB,
                lbLBDMinimizingClause);
        printf("c |   * K            : %6.2f      |   * Special   : %6d         |                                     |\n", K, specialIncReduceDB);
        printf("c |   * R            : %6.2f      |   * Protected :  (lbd)< %2d     |                                     |\n", R, lbLBDFrozenClause);
        printf("c |                                |                                |                                     |\n");
        printf("c ==================================[ Search Statistics (every %6d conflicts) ]=========================\n", verbEveryConflicts);
        printf("c |                                                                                                       |\n");

        printf("c |          RESTARTS           |          ORIGINAL         |              LEARNT              | Progress |\n");
        printf("c |       NB   Blocked  Avg Cfc |    Vars  Clauses Literals |   Red   Learnts    LBD2  Removed |          |\n");
        printf("c =========================================================================================================\n");
    }

    // Search:
    int curr_restarts = 0;
    while (status == l_Undef) {
        sonification.restart();
        status = search(0); // the parameter is useless in glucose, kept to allow modifications
        if (!withinBudget())
            break;
        curr_restarts++;
    }

    if (!incremental && verbosity >= 1)
        printf("c =========================================================================================================\n");

    if (certifiedUNSAT) { // Want certified output
        if (status == l_False)
            fprintf(certifiedOutput, "0\n");
        fclose(certifiedOutput);
    }

    if (status == l_True) {
        // Extend & copy model:
        if ((int) model.size() < nVars())
            model.resize(nVars());
        for (int i = 0; i < nVars(); i++)
            model[i] = value(i);
    } else if (status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);

    double finalTime = cpuTime();
    if (status == l_True) {
        nbSatCalls++;
        totalTime4Sat += (finalTime - curTime);
    }
    if (status == l_False) {
        nbUnsatCalls++;
        totalTime4Unsat += (finalTime - curTime);
    }

    sonification.stop(status == l_True ? 1 : status == l_False ? 0 : -1);

    return status;
}
