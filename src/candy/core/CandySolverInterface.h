/*
 * CandySolverInterface.h
 *
 *  Created on: Nov 26, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_
#define SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_

#include "candy/core/SolverTypes.h"

#include <vector>

namespace Candy {

class CandySolverInterface {
public:
	virtual ~CandySolverInterface() {}

    virtual void setCertificate(Certificate& certificate) = 0;
    virtual void setVerbosities(unsigned int verbEveryConflicts, unsigned int verbosity) = 0;
    virtual void enablePreprocessing() = 0;
    virtual void disablePreprocessing() = 0;

    virtual Certificate* getCertificate() = 0;
    virtual unsigned int getVerbosity() = 0;

    virtual Var newVar() = 0;

    virtual void addClauses(const CNFProblem& problem) = 0;
    virtual bool addClause(const std::vector<Lit>& lits) = 0;
    virtual bool addClause(std::initializer_list<Lit> lits) = 0;

    virtual bool simplify() = 0; // remove satisfied clauses
    virtual bool strengthen() = 0; // remove false literals from clauses
    virtual bool eliminate() = 0;  // Perform variable elimination based simplification.
    virtual bool eliminate(bool use_asymm, bool use_elim) = 0;  // Perform variable elimination based simplification.
    virtual bool isEliminated(Var v) const = 0;
    virtual void setFrozen(Var v, bool freeze) = 0;

	virtual lbool solve() = 0;
	virtual lbool solve(std::initializer_list<Lit> assumps) = 0;
	virtual lbool solve(const std::vector<Lit>& assumps) = 0;

	virtual void setConfBudget(uint64_t x) = 0;
	virtual void setPropBudget(uint64_t x) = 0;
	virtual void setInterrupt(bool value) = 0;
	virtual void budgetOff() = 0;

	virtual void printDIMACS() = 0;

	// The value of a variable in the last model. The last call to solve must have been satisfiable.
	virtual lbool modelValue(Var x) const = 0;
	virtual lbool modelValue(Lit p) const = 0;
    virtual Cl getModel() = 0;

	// true means solver is in a conflicting state
	virtual bool isInConflictingState() const = 0;
	virtual std::vector<Lit>& getConflict() = 0;

	virtual size_t nClauses() const = 0;
	virtual size_t nLearnts() const = 0;
	virtual size_t nVars() const = 0;
    virtual size_t getConflictCount() const = 0;
    virtual size_t getPropagationCount() const = 0;

	virtual bool isSelector(Var v) = 0;

	// Incremental mode
	virtual void setIncrementalMode() = 0;
	virtual void initNbInitialVars(int nb) = 0;
	virtual bool isIncremental() = 0;
};

}
#endif /* SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_ */
