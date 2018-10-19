
#include "SMT.hpp"
#include <iterator>
#include <algorithm>

SMT::SMT(Theory* theory, double alpha, double beta)
    : m_theory(theory)
    , m_alpha(alpha), m_beta(beta)
    , ms_restarts(0), ms_conflicts(0), ms_conflictsSR(0)
    , ms_badModels(0), ms_badModelsSR(0)
    , ms_decisions(0), ms_decisionsSR(0)
    , ms_propagations(0), ms_propagationsSR(0)
    , ms_learnedClauses(0), ms_learnedClausesSR(0)
{
    m_literalAssignation.resize(1);
}

void SMT::resizeToLiteral(long lit) {
    if(m_literalAssignation.size() > (size_t)lit) return;

    m_literalsScores.resize(lit + 1);
    m_literalFree.resize(lit + 1);
    m_literalPropagated.resize(lit + 1);
    m_assignationSource.resize(lit + 1);
    for(long toadd = m_literalAssignation.size(); toadd <= lit; ++toadd) {
        m_literalsScoresQueue.insert(std::make_pair(1, toadd));
        m_literalsScores[toadd] = 1;
        m_literalFree[toadd] = true;
    }
    m_literalAssignation.resize(lit + 1, false);
}

/* Assumes clause size is at least two */
void SMT::addClause(const Clause& clause) {
    for(auto lit : clause) resizeToLiteral(std::abs(lit));

    InternalClause iclause;
    iclause.start = m_clauseContent.size();
    iclause.size  = clause.size();
    m_clauses.push_back(iclause);
    std::copy(clause.begin(), clause.end(), std::back_inserter(m_clauseContent));

    std::pair<Literal,Literal> towatch;
    towatch.first  = clause[0];
    towatch.second = clause[1];
    m_watched.push_back(towatch);
}

long SMT::choose() {
    auto it = m_literalsScoresQueue.begin();
    long ret = it->second;
    m_literalsScoresQueue.erase(it);
    ++ms_decisions;
    ++ms_decisionsSR;
    return ret;
}

SMT::ClauseView SMT::propagate(Literal l, Assignation& ass) {
    long lit = std::abs(l);
    m_literalFree[lit] = false;
    m_literalAssignation[lit] = lit_sgn(l);

    ClauseView view(this);
    ++ms_propagations;
    ++ms_propagationsSR;

    for(; !view.ended(); view.next()) {
        auto& watched = view.watched();
        if(std::abs(watched.first) == std::abs(l)) {
            if(watched.first != l) {
                LitIterator res = lookup_nonfalse(view.begin(), view.end(),
                        l, watched.second);
                if(res != view.end()) {
                    watched.first = *res;
                } else if(can_assign(watched.second)) {
                    ass.propagated.push_back(watched.second);
                    m_literalPropagated[watched.second] = true;
                    m_assignationSource[watched.second] = view;
                    ClauseView ret = propagate(watched.second, ass);
                    if(!ret.ended()) return ret;
                } else return view;
            }
        }

        if(std::abs(watched.second) == std::abs(l)
                && lit_val(std::abs(watched.first)) != LIT_TRUE) {
            if(watched.second != l) {
                LitIterator res = lookup_nonfalse(view.begin(), view.end(),
                        l, watched.first);
                if(res != view.end()) {
                    watched.second = *res;
                } else if(can_assign(watched.first)) {
                    ass.propagated.push_back(watched.first);
                    m_literalPropagated[watched.first] = true;
                    m_assignationSource[watched.first] = view;
                    ClauseView ret = propagate(watched.first, ass);
                    if(!ret.ended()) return ret;
                } else return view;
            } else std::swap(watched.first, watched.second);
        }
    }

    return view;
}

SMT::LitIterator SMT::lookup_nonfalse(LitIterator beg, LitIterator end,
        Literal lit1, Literal lit2) {
    for(LitIterator it = beg; it != end; ++it) {
        if(std::abs(*it) == std::abs(lit1)) continue;
        if(std::abs(*it) == std::abs(lit2)) continue;
        if(lit_val(std::abs(*it)) != LIT_FALSE) return it;
    }
    return end;
}

SMT::LitValues SMT::lit_val(Literal lit) {
    lit = std::abs(lit);
         if(m_literalFree[lit])        return LIT_UNSET;
    else if(m_literalAssignation[lit]) {
        if(lit > 0) return LIT_TRUE;
        else        return LIT_FALSE;
    } else {
        if(lit > 0) return LIT_FALSE;
        else        return LIT_TRUE;
    }
}

bool SMT::lit_sgn(Literal lit) {
    return (lit >= 0);
}

bool SMT::can_assign(Literal l) {
    return (lit_val(l) == LIT_TRUE  &&  lit_sgn(l))
        || (lit_val(l) == LIT_FALSE && !lit_sgn(l))
        || (lit_val(l) == LIT_UNSET);
}
                
SMT::ClauseView::ClauseView()
    : m_learned(true), m_id((size_t)-1), m_smt(nullptr)
    { }

SMT::ClauseView::ClauseView(SMT* smt)
    : m_learned(false), m_id(0), m_smt(smt)
    { }

SMT::ClauseView::ClauseView(SMT* smt, bool learned, size_t id)
    : m_learned(learned), m_id(id), m_smt(smt)
    { }

void SMT::ClauseView::next() {
    ++m_id;
    if(!m_learned && m_id >= m_smt->m_clauses.size()) {
        m_id = 0;
        m_learned = true;
    }
}

bool SMT::ClauseView::ended() {
    return m_learned && m_id >= m_smt->m_learnedClauses.size();
}

SMT::LitIterator SMT::ClauseView::begin() const {
    if(m_learned) {
        return m_smt->m_learnedClauseContent.begin() + m_smt->m_learnedClauses[m_id].start;
    } else {
        return m_smt->m_clauseContent.begin() + m_smt->m_clauses[m_id].start;
    }
}

SMT::LitIterator SMT::ClauseView::end() const {
    if(m_learned) {
        return m_smt->m_learnedClauseContent.begin()
            + m_smt->m_learnedClauses[m_id].start
            + m_smt->m_learnedClauses[m_id].size;
    } else {
        return m_smt->m_clauseContent.begin()
            + m_smt->m_clauses[m_id].start
            + m_smt->m_clauses[m_id].size;
    }
}

std::pair<SMT::Literal,SMT::Literal>& SMT::ClauseView::watched() {
    if(m_learned) return m_smt->m_learnedWatched[m_id];
    else          return m_smt->m_watched[m_id];
}

void SMT::step(Literal asgn) {
    Assignation ass;
    ass.first = true;
    ass.assigned = asgn;
    ClauseView clause = propagate(asgn, ass);

    ass.success = clause.ended();
    ass.learned = (size_t)-1;
    if(!ass.success) ass.learned = learn_clause(clause);
    m_decisionStack.push_back(ass);

    for(auto lit : ass.propagated) {
        auto it = std::find_if(m_literalsScoresQueue.begin(),
                m_literalsScoresQueue.end(),
                [&] (auto& p) { return p.second == std::abs(lit); });
        if(it != m_literalsScoresQueue.end()) {
            m_literalsScoresQueue.erase(it);
        } else break;
    }
}
        
size_t SMT::learn_clause(ClauseView clause) {
    std::set<Literal> temp(clause.begin(), clause.end());
    std::vector<bool> seen(m_literalsScores.size(), false);
    InternalClause learned;
    learned.start = m_learnedClauseContent.size();
    learned.size = 0;

    while(!temp.empty()) {
        Literal lit = *temp.begin();
        long l = std::abs(lit);
        temp.erase(temp.begin());
        if(seen[l]) continue;
        seen[l] = true;
        /* TODO VSIDS update */

        if(!m_literalPropagated[l]) {
            m_learnedClauseContent.push_back(lit);
            ++learned.size;
            continue;
        }

        ClauseView cause = m_assignationSource[l];
        std::copy_if(cause.begin(), cause.end(),
                std::inserter(temp, temp.begin()),
                [&seen] (Literal l) { return !seen[std::abs(l)]; });
    }

    if(learned.size == 1) {
        m_learnedClauseContent.pop_back();
        return (size_t)-1;
    }

    m_learnedClauses.push_back(learned);
    m_learnedWatched.push_back(std::make_pair(
                *m_learnedClauseContent.rbegin(),
                *(m_learnedClauseContent.rbegin() + 1)));
    return m_learnedClauses.size() - 1;
}

void SMT::unfold(bool reentrant) {
    Assignation ass = m_decisionStack.back();
    m_decisionStack.pop_back();

    ass.assigned = std::abs(ass.assigned);
    m_literalFree[ass.assigned] = true;
    if(!reentrant) {
        m_literalsScoresQueue.insert(
                std::make_pair(m_literalsScores[ass.assigned], ass.assigned));
    }
    for(auto lit : ass.propagated) {
        lit = std::abs(lit);
        m_literalFree[lit] = true;
        m_literalsScoresQueue.insert(
                std::make_pair(m_literalsScores[lit], lit));
    }
}

bool SMT::solve() {
    Assignation ass;

    goto start;
    while(true) {
        if(m_decisionStack.empty()) return false;

        ass = m_decisionStack.back();

        if(ass.success) {
start:
            if(m_literalsScoresQueue.empty()) {
                /* TODO use theory */
                for(size_t i = 1; i < m_literalAssignation.size(); ++i) {
                    std::cout << (m_literalAssignation[i] ? "+" : "-")
                        << i << " ";
                }
                std::cout << std::endl;
                return true;
            }

            long lit = choose();
            step(-lit); /* Default polarity : negative */
        } else {
            if(ass.learned < (size_t)-1) {
                ClauseView cl(this, true, ass.learned);
                while(clause_val(cl) == LIT_FALSE) unfold();

                /* TODO restart */
                m_decisionStack.back().success = false;
                continue;
            }

            if(ass.first) {
                unfold(true);
                step(-ass.assigned);
                m_decisionStack.back().first = false;
            } else {
                unfold();
                m_decisionStack.back().success = false;
            }
        }
    }
}

SMT::LitValues SMT::clause_val(ClauseView clause) {
    LitValues result = LIT_FALSE;
    for(auto it = clause.begin(); it != clause.end(); ++it) {
        LitValues nval = lit_val(*it);
        if(nval == LIT_TRUE) return LIT_TRUE;
        else if(nval == LIT_UNSET) result = LIT_UNSET;
    }
    return result;
}

