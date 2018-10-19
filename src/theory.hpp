
#pragma once

#include <iostream>
#include <vector>

class Theory {
    public:
        using Theorem = long;
        using Model   = std::vector<std::pair<bool,Theorem>>;

        Theory();
        virtual ~Theory();

        /* Theorem I/O */
        virtual void read(Theorem id, std::ifstream& ifs) = 0;
        virtual void write(std::ofstream& ofs, Theorem theo) const = 0;
        virtual bool hasTheorem(Theorem theo) const = 0;
        virtual void remove(Theorem theo) = 0;

        /* Solver */
        virtual void add_clause(Theorem theo, bool polarity = true);
        virtual void remove_clause(Theorem theo);
        virtual void add_clauses(const Model& model) = 0;
        virtual void remove_clauses(const std::vector<Theorem>& model) = 0;
        /* Check if the actual model fails or not */
        virtual bool check() = 0;
        /* Get a minimal failing sub model of the actual model */
        virtual Model explain() = 0;
};
