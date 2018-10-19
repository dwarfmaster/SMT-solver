
#pragma once

#include "theory.hpp"
#include <unordered_map>
#include <set>

class SMT {
    public:
        using Literal = signed long;
        using Clause = std::vector<Literal>;
        struct InternalClause {
            size_t start;
            size_t size;
        };

        SMT() = delete;
        SMT(Theory* theory, double alpha = 0.95, double beta = 1);

        void addClause(const Clause& clause);

        /* Final solution can be extracted from the theory */
        bool solve();

    private:
        using LitIterator = std::vector<Literal>::const_iterator;
        enum LitValues { LIT_TRUE, LIT_FALSE, LIT_UNSET };

        /* Clauses */
        std::vector<Literal> m_clauseContent;
        std::vector<InternalClause> m_clauses;
        std::vector<std::pair<Literal,Literal>> m_watched;

        /* Learning */
        std::vector<Literal> m_learnedClauseContent;
        std::vector<InternalClause> m_learnedClauses;
        std::vector<std::pair<Literal,Literal>> m_learnedWatched;

        /* View all clauses in one go */
        class ClauseView {
            public:
                ClauseView();
                ClauseView(SMT* smt);

                void next();
                bool ended();

                LitIterator begin() const;
                LitIterator end() const;
                std::pair<Literal,Literal>& watched();

            private:
                bool m_learned;
                size_t m_id;
                SMT* m_smt;
        };

        /* Theory */
        Theory* m_theory;

        /* Variables */
        double m_alpha;
        double m_beta;
        /* Contains only variables that are yet to be assigned */
        std::set<std::pair<double,long>> m_literalsScoresQueue;
        std::vector<double> m_literalsScores;
        std::vector<bool> m_literalAssignation;
        std::vector<bool> m_literalFree;
        std::vector<ClauseView> m_assignationSource;
        std::vector<bool> m_literalPropagated;
        struct Assignation {
            Literal assigned;
            bool first;
            bool success;
            size_t learned;
            std::vector<Literal> propagated;
        };
        std::vector<Assignation> m_decisionStack;

        /* Statistics */
        using llong = unsigned long long;
        llong ms_restarts;
        llong ms_conflicts;
        llong ms_conflictsSR; /* SR means since last restart */
        llong ms_badModels;
        llong ms_badModelsSR;
        llong ms_decisions;
        llong ms_decisionsSR;
        llong ms_propagations;
        llong ms_propagationsSR;
        llong ms_learnedClauses;
        llong ms_learnedClausesSR;

        /* Utils */
        void resizeToLiteral(long lit);
        long choose();
        ClauseView propagate(Literal l, Assignation& ass);
                                         /* Index of the clause in case of conflict
                                          * (the view has ended if no conflict) */
        LitIterator lookup_nonfalse(LitIterator beg, LitIterator end,
                Literal lit1, Literal lit2);
        LitValues lit_val(Literal lit);
        bool lit_sgn(Literal lit);
        bool can_assign(Literal l);
        void step(Literal asgn);
        void unfold(bool reentrant = false);
        size_t learn_clause(ClauseView clause);
};

