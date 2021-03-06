/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#ifndef X_66055FCA_E0CE_46B3_9E4C_7F093C37330B_BRANCHINGHEURISTICS_H
#define X_66055FCA_E0CE_46B3_9E4C_7F093C37330B_BRANCHINGHEURISTICS_H

#include <candy/core/SolverTypes.h>
#include <candy/core/Branch.h>
#include <candy/rsil/ImplicitLearningAdvice.h>
#include <candy/randomsimulation/Conjectures.h>
#include <candy/utils/FastRand.h>
#include <candy/utils/Attributes.h>
#include <candy/rsar/Heuristics.h>
#include "RSARHeuristicsFilter.h"

#include <type_traits>

namespace Candy {
    /**
     * \defgroup RS_ImplicitLearning
     */
    
    /**
     * \class CandyDefaultSolverTypes
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief Solver datastructure types for Candy's core solver.
     */
    struct CandyDefaultSolverTypes {
        /// The type of the underlying solver's trail data structure
        using TrailType = std::vector<Lit>;
        
        /// The type of the underlying solver's trail limits data structure
        using TrailLimType = std::vector<uint32_t>;
        
        /// The type of the underlying solver's container marking decision variables
        using DecisionType = std::vector<char>;
        
        /// The type of the underlying solver's assignment data structure
        using AssignsType = std::vector<lbool>;
    };
    
    /**
     * \class RSILBranchingHeuristic
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief A PickBranchLit type for branching decisions using implicit learning.
     *
     * A PickBranchLitT type (see candy/core/Solver.h) for plain random-simulation-based
     * implicit learning solvers. A pickBranchLit implementation is provided for the
     * RSILBranchingHeuristic PickBranchLitT type.
     *
     * \tparam AdviceType    The advice type used by the branching heuristic. May be any
     *                       type having the same interface as AdviceType defined in the
     *                       ImplicitLearningAdvice.h file. Advices are used as compact
     *                       storages of conjectures about literal equivalencies and may
     *                       carry further information about these conjectures, e.g. usage
     *                       budgets.
     *
     * \tparam SolverTypes   The types of solver data structures used for RSILBranchingHeuristic.
     *                       SolverTypes must provide the following types:
     *                         - SolverTypes::TrailType
     *                         - SolverTypes::TrailLimType
     *                         - SolverTypes::DecisionType
     *                         - SolverTypes::AssignsType<br>
     *                       All four types must provide operator[], a type size_type (being
     *                       the argument type of operator[]), and a method size_type size().<br>
     *                         - TrailType is the type of the solver's trail data structure type (with
     *                           Lit& operator[](n) returning the n'th assigned literal)
     *                         - TrailLimType is the solver's trail limits data structure type (with
     *                           operator[](n) returning the begin index of the n'th decision level
     *                           in the solver's trail)
     *                         - DecisionType is the solver's decision variable marker data structure type
     *                           (with operator[](v) being an integral value which is 0 iff variable v may
     *                           not be a branching variable)),
     *                         - AssignsType is the solver's variable assignment data structure type
     *                           (with operator[](v) being an lbool value representing the current assignment
     *                           of v).
     */
    template<class AdviceType,
             typename SolverTypes = CandyDefaultSolverTypes>
    class RSILBranchingHeuristic {
        static_assert(std::is_class<typename AdviceType::BasicType>::value,
          "AdviceType must have an inner type BasicType");
        
    public:
        /**
         * A few definitions to make that compatible with the new direct design
         */
        Branch defaultBranchingHeuristic;

        Lit pickBranchLit(Trail& trail);

    	void setDecisionVar(Var v, bool b) {
    		defaultBranchingHeuristic.setDecisionVar(v, b);
    	}

    	void initFrom(const CNFProblem& problem) {
    		defaultBranchingHeuristic.initFrom(problem);
    	}

    	void grow() {
    		defaultBranchingHeuristic.grow();
    	}

    	void grow(size_t size) {
    		defaultBranchingHeuristic.grow(size);
    	}

    	void notify_conflict(vector<Var>& involved_variables, Trail& trail, unsigned int learnt_lbd, uint64_t nConflicts) {
    		defaultBranchingHeuristic.notify_conflict(involved_variables, trail, learnt_lbd, nConflicts);
    	}

    	void notify_backtracked(vector<Lit> lits) {
    		defaultBranchingHeuristic.notify_backtracked(lits);
    	}

    	void notify_restarted(Trail& trail) {
    		defaultBranchingHeuristic.notify_restarted(trail);
    	}

        /// The type of the underlying solver's trail data structure
        using TrailType = typename SolverTypes::TrailType;
        
        /// The type of the underlying solver's trail limits data structure
        using TrailLimType = typename SolverTypes::TrailLimType;
        
        /// The type of the underlying solver's container marking decision variables
        using DecisionType = typename SolverTypes::DecisionType;
        
        /// The type of the underlying solver's assignment data structure
        using AssignsType = typename SolverTypes::AssignsType;
        
        /// A type used for recognition of RSILBranchingHeuristic types in template metaprogramming.
        /// This type is independent of the template argument AdviceType.
        using BasicType = RSILBranchingHeuristic<AdviceEntry<3>>;
        
        /**
         * \brief RSILBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The conjectures to be used for implicit learning, e.g. obtained via random simulation.
            const Conjectures& conjectures;
            
            /// Iff true, conjectures about backbone variables are taken into consideration for implicit learning.
            const bool backbonesEnabled;
            
            /// Iff true, the RSARHeuristic given by this parameter struct is used to filter the conjectures
            /// before running RSIL.
            const bool filterByRSARHeuristic;
            
            /// An RSARHeuristic used to filter the conjectures before running RSIL. May contain nullptr if
            /// filterByRSARHeuristic == false.
            std::shared_ptr<RefinementHeuristic> RSARHeuristic;
            
            /// Iff true, the RSARHeuristic is only used to filter backbone conjectures.
            const bool filterOnlyBackbones;
            
            Parameters(const Conjectures& conjectures_,
                       bool backbonesEnabled_ = false,
                       bool filterByRSARHeuristic_ = false,
                       std::shared_ptr<RefinementHeuristic> RSARHeuristic_ = nullptr,
                       bool filterOnlyBackbones_ = false)
            : conjectures(conjectures_),
            backbonesEnabled(backbonesEnabled_),
            filterByRSARHeuristic(filterByRSARHeuristic_),
            RSARHeuristic(RSARHeuristic_),
            filterOnlyBackbones(filterOnlyBackbones_) {
            }
            
            Parameters() = default;
        };
        
        /**
         * \brief Constructs a new RSILBranchingHeuristic instance with a default configuration
         *        and no literal-equivalency/backbone conjectures.
         *
         * An instance constructed this way does not provide implicit learning advice, i.e.
         * getAdvice(...) always returns lit_Undef and getSignAdvice(L) returns L.
         *
         */
        RSILBranchingHeuristic();
        
        /**
         * \brief Constructs a new RSILBranchingHeuristic instance using the given parameters.
         *
         * \param params    The RSIL parameter object used to configure the constructed object.
         */
        explicit RSILBranchingHeuristic(const Parameters& params);
        RSILBranchingHeuristic(RSILBranchingHeuristic&& other) = default;
        RSILBranchingHeuristic& operator=(RSILBranchingHeuristic&& other) = default;
        
        /**
         * \brief Gets a branching literal advice using implicit learning.
         *
         * If possible, a branching decision literal is proposed with implicit learning using
         * the literals occurring on the trail's most recent decision level. If no such literal
         * can be picked, lit_Undef is returned. Note that getAdvice(...) does not return literals
         * conjectured to belong to the problem's backbone.
         *
         * \param trail         The sequence of current variable assignments, ordered by the
         *                      time of assignment.
         * \param trailSize     The size of the initial segment of \p trail containing the current
         *                      sequence of variable assignments.
         * \param trailLimits   A sequence of indices of \p trail partitioning \p trail into assignments
         *                      occurring on different decision levels.
         * \param assigns       The underlying solver's variable assignments. For a variable \p v,
         *                      \p assigns[v] must be the lbool representing its current assignment.
         * \param decision      A data structure marking variables eligible to be used for branching
         *                      decisions. A variable \p v is considered eligible iff \p decision[v]
         *                      is not 0.
         *
         * \returns a literal chosen via implicit learning or lit_Undef.
         */
        Lit getAdvice(const TrailType& trail,
                      uint64_t trailSize,
                      const TrailLimType& trailLimits,
                      const AssignsType& assigns,
                      const DecisionType& decision) noexcept;
        
        /**
         * \brief Gets a branching literal sign advice using implicit learning.
         *
         * For a branching literal L having already been picked for a branching decision, this method
         * returns a literal with the same variable as L, but a sign chosen via implicit learning. Note
         * that with this method, implicit learning is only used for backbone variable conjectures.
         *
         * \returns a literal L having the same variable as \p literal.
         */
        Lit getSignAdvice(Lit literal) noexcept;
        
        /**
         * \brief Gets the pseudorandom number generator used by the heuristic.
         *
         * This method exposes the heuristic's internal PRNG to allow more efficient implementations
         * of this heuristic's variants. Clients may use and modify the PRNG.
         *
         * \returns a PRNG.
         */
        FastRandomNumberGenerator& getRandomNumberGenerator() noexcept;
        
    protected:
        /// The conjectures about literal equivalencies and backbones.
        ImplicitLearningAdvice<AdviceType> m_advice;
        
    private:
        /// The heuristic's pseudorandom number generator.
        FastRandomNumberGenerator m_rng;
        
        /// Iff true, backbones are taken into consideration for implicit learning.
        bool m_backbonesEnabled;
    };
    
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILBranchingHeuristic<n> with n=3.
     */
    using RSILBranchingHeuristic3 = RSILBranchingHeuristic<AdviceEntry<3>>;
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILBranchingHeuristic<n> with n=2.
     */
    using RSILBranchingHeuristic2 = RSILBranchingHeuristic<AdviceEntry<2>>;
    
    
    /**
     * \class RSILBudgetBranchingHeuristic
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief A PickBranchLit type for branching decisions using implicit learning, limited by budgets.
     *
     * This is a RSILBranchingHeuristic tailored for implicit learning with implication
     * budgets. Each implication is assigned an initial budget, and each time it is used
     * for implicit learning, the budget is decreased by one. If an implication's budget
     * is 0, it is not further used for implicit learning.
     *
     * \tParam tAdviceSize      The maximum of size equivalence conjectures taken into account
     * \tParam SolverTypes      See the SolverTypes template parameter of RSILBranchingHeuristic
     */
    template<unsigned int tAdviceSize, typename SolverTypes = CandyDefaultSolverTypes>
    class RSILBudgetBranchingHeuristic : public RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>, SolverTypes> {
        static_assert(tAdviceSize > 1, "advice size must be >= 2");
    public:
        /**
         * A few definitions to make that compatible with the new direct design
         */
        Branch defaultBranchingHeuristic;

        Lit pickBranchLit(Trail& trail);

    	void setDecisionVar(Var v, bool b) {
    		defaultBranchingHeuristic.setDecisionVar(v, b);
    	}

    	void initFrom(const CNFProblem& problem) {
    		defaultBranchingHeuristic.initFrom(problem);
    	}

    	void grow() {
    		defaultBranchingHeuristic.grow();
    	}

    	void grow(size_t size) {
    		defaultBranchingHeuristic.grow(size);
    	}

    	void notify_conflict(vector<Var>& involved_variables, Trail& trail, unsigned int learnt_lbd, uint64_t nConflicts) {
    		defaultBranchingHeuristic.notify_conflict(involved_variables, trail, learnt_lbd, nConflicts);
    	}

    	void notify_backtracked(vector<Lit> lits) {
    		defaultBranchingHeuristic.notify_backtracked(lits);
    	}

    	void notify_restarted(Trail& trail) {
    		defaultBranchingHeuristic.notify_restarted(trail);
    	}

        /// A type used for recognition of RSILBudgetBranchingHeuristic types in template metaprogramming.
        /// This type is independent of the template argument AdviceType.
        using BasicType = RSILBudgetBranchingHeuristic<3>;
        
        /// The type of the extended heuristic, used for parameter arguments.
        using UnderlyingHeuristicType = RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>;
        
        /**
         * \class
         * \brief RSILBudgetBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The parameters for the underlying RSILBranchingHeuristic.
            typename RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>::Parameters rsilParameters;
            
            /// The initial implication budget.
            uint64_t initialBudget;
            
            Parameters() = default;
            Parameters(const typename UnderlyingHeuristicType::Parameters& rsilParameters_,
                       uint64_t initialBudget_ = 10000ull)
            : rsilParameters(rsilParameters_),
            initialBudget(initialBudget_) {
            }
            
            Parameters(const Conjectures& conjectures_,
                                uint64_t initialBudget_ = 10000ull)
            : rsilParameters(typename UnderlyingHeuristicType::Parameters{conjectures_}),
            initialBudget(initialBudget_) {
            }
        };
        
        RSILBudgetBranchingHeuristic();
        explicit RSILBudgetBranchingHeuristic(const Parameters& params);
        RSILBudgetBranchingHeuristic(RSILBudgetBranchingHeuristic&& other) = default;
        RSILBudgetBranchingHeuristic& operator=(RSILBudgetBranchingHeuristic&& other) = default;
    };
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILBudgetBranchingHeuristic<n> with n=3.
     */
    using RSILBudgetBranchingHeuristic3 = RSILBudgetBranchingHeuristic<3>;
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILBudgetBranchingHeuristic<n> with n=2.
     */
    using RSILBudgetBranchingHeuristic2 = RSILBudgetBranchingHeuristic<2>;
    
    /**
     * \class RSILVanishingBranchingHeuristic
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief A PickBranchLit type for branching decisions using implicit learning, limited by the amount of performed decisions.
     *
     * A PickBranchLitT type (see candy/core/Solver.h) for plain random-simulation-based
     * implicit learning solvers. This heuristic produces results with decreased probability,
     * configured via the parameter \p probHalfLife : every \p probHalfLife decisions, the
     * probability of possibly overriding the solver's internal decision heuristic is halved.
     *
     * A pickBranchLit implementation is provided for the RSILVanishingBranchingHeuristic
     * PickBranchLitT type.
     *
     * \tParam AdviceType       See the AdviceType template parameter of RSILBranchingHeuristic
     * \tParam SolverTypes      See the SolverTypes template parameter of RSILBranchingHeuristic
     */
    template<class AdviceType, typename SolverTypes = CandyDefaultSolverTypes>
    class RSILVanishingBranchingHeuristic {
    public:
        /**
         * A few definitions to make that compatible with the new direct design
         */
        Branch defaultBranchingHeuristic;

        Lit pickBranchLit(Trail& trail);

    	void setDecisionVar(Var v, bool b) {
    		defaultBranchingHeuristic.setDecisionVar(v, b);
    	}

    	void initFrom(const CNFProblem& problem) {
    		defaultBranchingHeuristic.initFrom(problem);
    	}

    	void grow() {
    		defaultBranchingHeuristic.grow();
    	}

    	void grow(size_t size) {
    		defaultBranchingHeuristic.grow(size);
    	}

    	void notify_conflict(vector<Var>& involved_variables, Trail& trail, unsigned int learnt_lbd, uint64_t nConflicts) {
    		defaultBranchingHeuristic.notify_conflict(involved_variables, trail, learnt_lbd, nConflicts);
    	}

    	void notify_backtracked(vector<Lit> lits) {
    		defaultBranchingHeuristic.notify_backtracked(lits);
    	}

    	void notify_restarted(Trail& trail) {
    		defaultBranchingHeuristic.notify_restarted(trail);
    	}

        // See the documentation of RSILBranchingHeuristic for the following 4 types.
        using TrailType = typename RSILBranchingHeuristic<AdviceType>::TrailType;
        using TrailLimType = typename RSILBranchingHeuristic<AdviceType>::TrailLimType;
        using DecisionType = typename RSILBranchingHeuristic<AdviceType>::DecisionType;
        using AssignsType = typename RSILBranchingHeuristic<AdviceType>::AssignsType;
        
        
        /// A type used for recognition of RSILVanishingBranchingHeuristic types in template metaprogramming.
        /// This type is independent of the template argument AdviceType.
        using BasicType = RSILVanishingBranchingHeuristic<AdviceEntry<3>>;
        
        /// The type of the extended heuristic, used for parameter arguments.
        using UnderlyingHeuristicType = RSILBranchingHeuristic<AdviceType>;
        
        /**
         * RSILVanishingBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The parameters for the underlying RSILBranchingHeuristic.
            typename RSILBranchingHeuristic<AdviceType>::Parameters rsilParameters;
            
            /// The intervention probability half-life.
            uint64_t probHalfLife;
            
            Parameters() = default;
            
            explicit Parameters(const Conjectures& conjectures_)
            : rsilParameters(typename UnderlyingHeuristicType::Parameters{conjectures_}), probHalfLife(100ull) {
            }
            
            Parameters(const Conjectures& conjectures_, uint64_t probHalfLife_)
            : rsilParameters(typename UnderlyingHeuristicType::Parameters{conjectures_}),
            probHalfLife(probHalfLife_) {}
            
            Parameters(const typename UnderlyingHeuristicType::Parameters& rsilParameters_,
                       uint64_t probHalfLife_)
            : rsilParameters(rsilParameters_),
            probHalfLife(probHalfLife_) {
            }
        };
        
        RSILVanishingBranchingHeuristic();
        explicit RSILVanishingBranchingHeuristic(const Parameters& params);
        RSILVanishingBranchingHeuristic(RSILVanishingBranchingHeuristic&& other) = default;
        RSILVanishingBranchingHeuristic& operator=(RSILVanishingBranchingHeuristic&& other) = default;
        
        /**
         * \brief Gets a branching literal advice using implicit learning.
         *
         * If possible, a branching decision literal is proposed with implicit learning using
         * the literals occurring on the trail's most recent decision level. If no such literal
         * can be picked, lit_Undef is returned. Note that getAdvice(...) does not return literals
         * conjectured to belong to the problem's backbone.
         *
         * \param trail         The sequence of current variable assignments, ordered by the
         *                      time of assignment.
         * \param trailSize     The size of the initial segment of \p trail containing the current
         *                      sequence of variable assignments.
         * \param trailLimits   A sequence of indices of \p trail partitioning \p trail into assignments
         *                      occurring on different decision levels.
         * \param assigns       The underlying solver's variable assignments. For a variable \p v,
         *                      \p assigns[v] must be the lbool representing its current assignment.
         * \param decision      A data structure marking variables eligible to be used for branching
         *                      decisions. A variable \p v is considered eligible iff \p decision[v]
         *                      is not 0.
         *
         * \returns a literal chosen via implicit learning or lit_Undef.
         */
        Lit getAdvice(const TrailType& trail,
                      uint64_t trailSize,
                      const TrailLimType& trailLimits,
                      const AssignsType& assigns,
                      const DecisionType& decision) noexcept;
        
        /**
         * \brief Gets a branching literal sign advice using implicit learning.
         *
         * For a branching literal L having already been picked for a branching decision, this method
         * returns a literal with the same variable as L, but a sign chosen via implicit learning. Note
         * that with this method, implicit learning is only used for backbone variable conjectures.
         *
         * \returns a literal L having the same variable as \p literal.
         */
        Lit getSignAdvice(Lit literal) noexcept;
        
        
    private:
        /**
         * \brief Decides if implicit learning can used for this decision.
         *
         * \returns true iff implicit learning can used for this decision.
         */
        bool isRSILEnabled() noexcept;
        
        /**
         * \brief Updates the counter used for isRSILEnabled().
         *
         * Increases m_callCounter by one. If the counter exceeds the probability half-life,
         * the probability of using implicit learning is halved and the counter is reset to 0.
         */
        void updateCallCounter() noexcept;
        
        /// The counter used to keep track of the number of branching decisions modulo the
        /// intervention probability half-life.
        uint64_t m_callCounter;
        
        /// The mask used on a pseudorandom number for deciding whether to use implicit learning:
        /// If the current probability of using IL is 2^{-P}, the P lowest bits of m_mask are set
        /// to 1. To decide whether to use IL, a pseudorandom number X is generated; if X & m_mask
        /// is 0, implicit learning is used, otherwise not.
        fastnextrand_state_t m_mask;
        
        /// The intervention probability half-life (in terms of overall performed decisions)
        uint64_t m_probHalfLife;
        
        /// The underlying RSIL branching heuristic used for computing implicit learning advice.
        RSILBranchingHeuristic<AdviceType> m_rsilHeuristic;
    };
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILVanishingBranchingHeuristic<n> with n=3.
     */
    using RSILVanishingBranchingHeuristic3 = RSILVanishingBranchingHeuristic<AdviceEntry<3>>;
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILVanishingBranchingHeuristic<n> with n=2.
     */
    using RSILVanishingBranchingHeuristic2 = RSILVanishingBranchingHeuristic<AdviceEntry<2>>;
    
    
    
    
    
    //******* Implementation ************************************************************
     
    namespace BranchingHeuristicsImpl {
        static inline Var getMaxVar(const Conjectures& conj) {
            Var result = 0;
            for (auto& c : conj.getBackbones()) {
                result = std::max(result, var(c.getLit()));
            }
            for (auto& c : conj.getEquivalences()) {
                for (auto literal : c) {
                    result = std::max(result, var(literal));
                }
            }
            return result;
        }
        
        // The following provides an implementation of usedAdvice() and canUseAdvice() for each supported advice entry
        // type, via SFINAE. It is required that ...AdviceEntry<n>::BasicType == ...AdviceEntry<m>::BasicType for all
        // admissible values of n and m. Therefore, it suffices to compare types with ...AdviceEntry<2>::BasicType.
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, AdviceEntry<2>::BasicType>::value, void>::type
        usedAdvice(AdviceType& advice, size_t index) noexcept {
            (void)advice;
            (void)index;
        }
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, AdviceEntry<2>::BasicType>::value, bool>::type
        canUseAdvice(AdviceType& advice, size_t index) noexcept {
            (void)advice;
            (void)index;
            return true;
        }
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, BudgetAdviceEntry<2>::BasicType>::value, void>::type
        usedAdvice(AdviceType& advice, size_t index) noexcept {
            assert (index < advice.getSize());
            assert (advice.getBudget(index) > 0);
            advice.setBudget(index, advice.getBudget(index)-1);
        }
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, BudgetAdviceEntry<2>::BasicType>::value, bool>::type
        canUseAdvice(AdviceType& advice, size_t index) noexcept {
            assert (index < advice.getSize());
            return advice.getBudget(index) > 0;
        }
    }
    
    
    
    template<class AdviceType, typename SolverTypes>
    RSILBranchingHeuristic<AdviceType, SolverTypes>::RSILBranchingHeuristic()
    : defaultBranchingHeuristic(), m_advice(),
    m_rng(0xFFFF),
    m_backbonesEnabled(false) {
    }
    
    template<class AdviceType, typename SolverTypes>
    RSILBranchingHeuristic<AdviceType, SolverTypes>::RSILBranchingHeuristic(const Parameters& params)
    : defaultBranchingHeuristic(), m_advice(params.conjectures,
               BranchingHeuristicsImpl::getMaxVar(params.conjectures)),
    m_rng(0xFFFF),
    m_backbonesEnabled(params.backbonesEnabled) {
        if (params.filterByRSARHeuristic) {
            assert (params.RSARHeuristic.get() != nullptr);
            std::vector<RefinementHeuristic*> heuristics;
            heuristics.push_back(params.RSARHeuristic.get());
            filterWithRSARHeuristics(heuristics, m_advice, params.filterOnlyBackbones);
        }
    }
    
    template<class AdviceType, typename SolverTypes>
    inline FastRandomNumberGenerator& RSILBranchingHeuristic<AdviceType, SolverTypes>::getRandomNumberGenerator() noexcept {
        return m_rng;
    }
    
    template<class AdviceType, typename SolverTypes>
    ATTR_ALWAYSINLINE
    inline Lit RSILBranchingHeuristic<AdviceType, SolverTypes>::getAdvice(const TrailType& trail,
                                                                          uint64_t trailSize,
                                                                          const TrailLimType& trailLimits,
                                                                          const AssignsType& assigns,
                                                                          const DecisionType& decision) noexcept {
        if (trailLimits.size() == 0) {
            return lit_Undef;
        }
        
        auto trailStart = trailLimits.back();
        auto scanLen = trailSize - trailStart;
        auto randomNumber = trailLimits.size(); // decision level "randomness" suffices here
        
        for (decltype(scanLen) j = 0; j < scanLen; ++j) {
            auto i = trailStart + j;
            
            Lit cursor = trail[i];
            Var variable = var(cursor);
            
            if (!m_advice.hasPotentialAdvice(variable)) {
                continue;
            }
            
            auto& advice = m_advice.getAdvice(variable);
            
            if (advice.isBackbone()) {
                continue;
            }
            
            size_t adviceSize = advice.getSize();
            for (decltype(adviceSize) a = 0; a < adviceSize; ++a) {
                auto idx = (randomNumber + a) % adviceSize;
                
                auto advisedLit = advice.getLiteral(idx);
                if (assigns[var(advisedLit)] == l_Undef
                    && decision[var(advisedLit)]
                    && BranchingHeuristicsImpl::canUseAdvice(advice, idx)) {
                    BranchingHeuristicsImpl::usedAdvice(advice, idx);
                    auto result = sign(cursor) ? ~advisedLit : advisedLit;
                    return result;
                }
            }
        }
        
        return lit_Undef;
    }
    
    template<class AdviceType, typename SolverTypes>
    ATTR_ALWAYSINLINE
    inline Lit RSILBranchingHeuristic<AdviceType, SolverTypes>::getSignAdvice(Lit literal) noexcept {
        if (m_backbonesEnabled) {
            Var decisionVariable = var(literal);
            if (m_advice.hasPotentialAdvice(decisionVariable)) {
                auto& advice = m_advice.getAdvice(decisionVariable);
                
                if (advice.isBackbone()
                    && BranchingHeuristicsImpl::canUseAdvice(advice, 0)
                    && advice.getSize() == 1) {
                    assert(advice.getLiteral(0) != lit_Undef);
                    BranchingHeuristicsImpl::usedAdvice(advice, 0);
                    return ~(advice.getLiteral(0));
                }
            }
        }
        return literal;
    }
    
    
    template<unsigned int tAdviceSize, typename SolverTypes>
    RSILBudgetBranchingHeuristic<tAdviceSize, SolverTypes>::RSILBudgetBranchingHeuristic()
    : defaultBranchingHeuristic(), RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>() {
    }
    
    template<unsigned int tAdviceSize, typename SolverTypes>
    RSILBudgetBranchingHeuristic<tAdviceSize, SolverTypes>::RSILBudgetBranchingHeuristic(const Parameters& params)
    : defaultBranchingHeuristic(), RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>(params.rsilParameters) {
        for (Var i = 0; this->m_advice.hasPotentialAdvice(i); ++i) {
            auto& advice = this->m_advice.getAdvice(i);
            for (size_t j = 0; j < advice.getSize(); ++j) {
                advice.setBudget(j, params.initialBudget);
            }
        }
    }
    
    
    // Note: The RSILBudgetBranchingHeuristic implementation itself is not concerned with
    // actually updating the AdviceEntry objects, and does not directly alter the behaviour
    // of getAdvice(). Any changes to the behaviour of getAdvice() (selecting implications,
    // updating advice entries) are realized by specializations of usedAdvice() and
    // canUseAdvice() for BudgetAdviceEntry - see the BranchingHeuristicsImpl namespace
    // for further details.
    
    template<class AdviceType, typename SolverTypes>
    RSILVanishingBranchingHeuristic<AdviceType, SolverTypes>::RSILVanishingBranchingHeuristic()
    : defaultBranchingHeuristic(), m_callCounter(1ull),
    m_mask(0ull),
    m_probHalfLife(1ull),
    m_rsilHeuristic(){
    }
    
    template<class AdviceType, typename SolverTypes>
    RSILVanishingBranchingHeuristic<AdviceType, SolverTypes>::RSILVanishingBranchingHeuristic(const Parameters& params)
    : defaultBranchingHeuristic(), m_callCounter(params.probHalfLife),
    m_mask(0ull),
    m_probHalfLife(params.probHalfLife),
    m_rsilHeuristic(params.rsilParameters) {
        assert(m_probHalfLife > 0);
    }
    
    template<class AdviceType, typename SolverTypes>
    inline bool RSILVanishingBranchingHeuristic<AdviceType, SolverTypes>::isRSILEnabled() noexcept {
        auto randomNumber = m_rsilHeuristic.getRandomNumberGenerator()();
        return (randomNumber & m_mask) == 0ull;
    }
    
    template<class AdviceType, typename SolverTypes>
    inline void RSILVanishingBranchingHeuristic<AdviceType, SolverTypes>::updateCallCounter() noexcept {
        assert(m_callCounter > 0);
        --m_callCounter;
        if (m_callCounter == 0) {
            m_mask = (m_mask << 1) | 1ull;
            m_callCounter = m_probHalfLife;
        }
    }
    
    template<class AdviceType, typename SolverTypes>
    ATTR_ALWAYSINLINE
    inline Lit RSILVanishingBranchingHeuristic<AdviceType, SolverTypes>::getAdvice(const TrailType& trail,
                                                                                   uint64_t trailSize,
                                                                                   const TrailLimType& trailLimits,
                                                                                   const AssignsType& assigns,
                                                                                   const DecisionType& decision) noexcept {
        auto result = isRSILEnabled() ? m_rsilHeuristic.getAdvice(trail, trailSize, trailLimits, assigns, decision) : lit_Undef;
        updateCallCounter();
        return result;
    }
    
    template<class AdviceType, typename SolverTypes>
    ATTR_ALWAYSINLINE
    inline Lit RSILVanishingBranchingHeuristic<AdviceType, SolverTypes>::getSignAdvice(Lit literal) noexcept {
        auto result = isRSILEnabled() ? m_rsilHeuristic.getSignAdvice(literal) : literal;
        updateCallCounter();
        return result;
    }
}

#endif
