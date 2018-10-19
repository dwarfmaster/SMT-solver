
#include <iostream>
#include "SMT.hpp"

int main() {
    SMT smt(nullptr);
    smt.addClause({1,4});
    smt.addClause({1,-3,-8});
    smt.addClause({1,8,12});
    smt.addClause({2,11});
    smt.addClause({-7,-3,9});
    smt.addClause({-7,8,-9});
    smt.addClause({7,8,-10});
    smt.addClause({7,10,-12});
    std::cout << smt.solve() << std::endl;
    return 0;
}

