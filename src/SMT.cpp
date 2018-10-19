
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
        auto watched = view.watched();
        if(std::abs(watched.first) == std::abs(l)) {
            if(watched.first != l) {
                LitIterator res = lookup_nonfalse(view.begin(), view.end(), l);
                if(res != view.end()) watched.first = *res;
                else if(can_assign(watched.second)) {
                    ass.propagated.push_back(watched.second);
                    std::cout << "Propagating " << watched.second << std::endl;
                    propagate(watched.second, ass);
                } else return view;
            }
        }

        if(std::abs(watched.second) == std::abs(l)
                && lit_val(std::abs(watched.first)) != LIT_TRUE) {
            if(watched.first != l) {
                LitIterator res = lookup_nonfalse(view.begin(), view.end(), l);
                if(res != view.end()) watched.second = *res;
                else if(can_assign(watched.first)) {
                    ass.propagated.push_back(watched.first);
                    std::cout << "Propagating " << watched.first << std::endl;
                    propagate(watched.first, ass);
                } else return view;
            } else std::swap(watched.first, watched.second);
        }
    }

    return view;
}

SMT::LitIterator SMT::lookup_nonfalse(LitIterator beg, LitIterator end, Literal lit) {
    for(LitIterator it = beg; it != end; ++it) {
        if(std::abs(*it) == std::abs(lit)) continue;
        if(lit_val(std::abs(*it)) != LIT_FALSE) return it;
    }
    return end;
}

SMT::LitValues SMT::lit_val(Literal lit) {
         if(m_literalFree[lit])        return LIT_UNSET;
    else if(m_literalAssignation[lit]) return LIT_TRUE;
    else                               return LIT_FALSE;
}

bool SMT::lit_sgn(Literal lit) {
    return (lit >= 0);
}

bool SMT::can_assign(Literal l) {
    return (lit_val(l) == LIT_TRUE  &&  lit_sgn(l))
        || (lit_val(l) == LIT_FALSE && !lit_sgn(l))
        || (lit_val(l) == LIT_UNSET);
}

SMT::ClauseView::ClauseView(SMT* smt)
    : m_learned(false), m_id(0), m_smt(smt)
    { }

void SMT::ClauseView::next() {
    ++m_id;
    if(!m_learned && m_id >= m_smt->m_clauses.size()) {
        m_id = 0;
        m_learned = true;
    }
}

bool SMT::ClauseView::ended() {
    return m_learned && m_id == m_smt->m_learnedClauses.size();
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
    /* TODO in case of conflict learn clause and VSIDS update */
    m_decisionStack.push_back(ass);
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
                return true;
            }

            long lit = choose();
            std::cout << "Choosing " << lit << std::endl;
            step(-lit); /* Default polarity : negative */
        } else {
            if(ass.learned < (size_t)-1) {
                /* TODO backjump */
                /* TODO maybe restart */
                unfold(); /* backtrack */
                m_decisionStack.back().success = false;
                continue;
            }

            if(ass.first) {
                unfold(true);
                std::cout << std::abs(ass.assigned)
                    << " -> " << (ass.assigned > 0 ? "0" : "1")
                    << std::endl;
                step(-ass.assigned);
                m_decisionStack.back().first = false;
            } else {
                unfold();
                m_decisionStack.back().success = false;
            }
        }
    }
}

