
#include "theory.hpp"

Theory::Theory() { }

Theory::~Theory() { }

void Theory::add_clause(Theory::Theorem theo, bool polarity) {
    Model clauses(1, std::make_pair(polarity, theo));
    add_clauses(clauses);
}

void Theory::remove_clause(Theory::Theorem theo) {
    std::vector<Theorem> clauses(1, theo);
    remove_clauses(clauses);
}

