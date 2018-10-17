
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
        void setTheorems(const std::unordered_map<long,Theory::Theorem>& theorems);
        /* Check if all literals have an associated theorem */
        bool isValid() const;

        /* Final solution can be extracted from the theory */
        bool solve();

    private:
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
                using LitIterator = std::vector<Literal>::const_iterator;

                ClauseView();

                void next();
                bool end();

                LitIterator begin() const;
                LitIterator end() const;
                std::pair<Literal,Literal>& watched();

            private:
                using Iterator = std::vector<InternalClause>::const_iterator;
                bool m_learned;
                Iterator m_iterator;
        };

        /* Theory */
        Theory* m_theory;
        std::unordered_map<long,Theory::Theorem> m_theorems;
        std::unordered_map<Theory::Theorem,long> m_theoremsLits;

        /* Variables */
        double m_alpha;
        double m_beta;
        /* Contains only variables that are yet to be assigned */
        std::set<std::pair<double,long>> m_literalsScores;
        std::vector<bool> m_literalAssignation;
        struct Assignation {
            Literal assigned;
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
};

