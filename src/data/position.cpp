
#include "position.h"

#include "sat/variable_domain.h"
#include "util/log.h"

Position::Position() : _layer_idx(-1), _pos(-1) {}
void Position::setPos(int layerIdx, int pos) {_layer_idx = layerIdx; _pos = pos;}

void Position::addFact(const USignature& fact) {
    _facts.insert(fact);
}
void Position::addQFact(const USignature& qfact) {
    _qfacts[qfact._name_id];
    _qfacts[qfact._name_id].insert(qfact);
}
void Position::addTrueFact(const USignature& fact) {_true_facts.insert(fact);}
void Position::addFalseFact(const USignature& fact) {_false_facts.insert(fact);}
void Position::addDefinitiveFact(const Signature& fact) {(fact._negated ? _false_facts : _true_facts).insert(fact._usig);}

void Position::addFactSupport(const Signature& fact, const USignature& operation) {
    auto& supports = (fact._negated ? _neg_fact_supports : _pos_fact_supports);
    supports[fact._usig];
    supports[fact._usig].insert(operation);
}
void Position::touchFactSupport(const Signature& fact) {
    auto& supports = (fact._negated ? _neg_fact_supports : _pos_fact_supports);
    supports[fact._usig];
}
void Position::addQConstantTypeConstraint(const USignature& op, const TypeConstraint& c) {
    _q_constants_type_constraints[op];
    _q_constants_type_constraints[op].push_back(c);
}
void Position::addForbiddenSubstitution(const USignature& op, const std::vector<int>& src, const std::vector<int>& dest) {
    _forbidden_substitutions_per_op[op];
    _forbidden_substitutions_per_op[op].emplace(src, dest);
}

void Position::addAction(const USignature& action) {
    _actions[action];
    _actions[action]++;
    Log::d("+ACTION@(%i,%i) %s\n", _layer_idx, _pos, TOSTR(action));
}
void Position::addReduction(const USignature& reduction) {
    _reductions[reduction];
    _reductions[reduction]++;
    Log::d("+REDUCTION@(%i,%i) %s\n", _layer_idx, _pos, TOSTR(reduction));
}
void Position::addExpansion(const USignature& parent, const USignature& child) {
    _expansions[parent];
    _expansions[parent].insert(child);
}
void Position::addAxiomaticOp(const USignature& op) {
    _axiomatic_ops.insert(op);
}
void Position::addExpansionSize(int size) {_max_expansion_size = std::max(_max_expansion_size, size);}
void Position::setFactChanges(const USignature& op, const SigSet& factChanges) {
    _fact_changes[op] = factChanges;
}
const SigSet& Position::getFactChanges(const USignature& op) const {
    return _fact_changes.count(op) ? _fact_changes.at(op) : EMPTY_SIG_SET;
}

void Position::setMirroredFacts(Position& left) {
    if (left._mirrored_facts != nullptr) _mirrored_facts = left._mirrored_facts;
    else _mirrored_facts = &(left._facts);
    _facts.clear();
    _facts.reserve(0);
}

void Position::removeActionOccurrence(const USignature& action) {
    assert(_actions.count(action));
    _actions[action]--;
    if (_actions[action] == 0) {
        _actions.erase(action);
    }
}
void Position::removeReductionOccurrence(const USignature& reduction) {
    assert(_reductions.count(reduction));
    _reductions[reduction]--;
    if (_reductions[reduction] == 0) {
        _reductions.erase(reduction);
    }
}

int Position::encode(const USignature& sig) {
    
    // If the variable is already there, then it must be originally encoded to return the correct result
    assert(!_variables.count(sig) || isVariableOriginallyEncoded(sig));

    if (!_variables.count(sig)) {
        // introduce a new variable
        assert(!VariableDomain::isLocked() || Log::e("Unknown variable %s queried!\n", VariableDomain::varName(_layer_idx, _pos, sig).c_str()));
        _variables[sig] = VariableDomain::nextVar();
        VariableDomain::printVar(_variables[sig], _layer_idx, _pos, sig);
    }
    return _variables[sig];
}

void Position::setVariableReference(const USignature& sig, int priorPos) {
    assert(!_variables.count(sig));
    _variables[sig] = -priorPos;
}

int Position::getVariableOrReference(const USignature& sig) const {
    assert(_variables.count(sig));
    return _variables.at(sig);
}

bool Position::hasVariable(const USignature& sig) const {
    return _variables.count(sig);
}
bool Position::isVariableOriginallyEncoded(const USignature& sig) const {
    assert(_variables.count(sig));
    return _variables.at(sig) > 0;
}

bool Position::hasFact(const USignature& fact) const {
    if (_mirrored_facts != nullptr) return _mirrored_facts->count(fact);
    else return _facts.count(fact);
}
bool Position::hasQFact(const USignature& fact) const {return _qfacts.count(fact._name_id) && _qfacts.at(fact._name_id).count(fact);}
bool Position::hasAction(const USignature& action) const {return _actions.count(action);}
bool Position::hasReduction(const USignature& red) const {return _reductions.count(red);}

IntPair Position::getPos() const {return IntPair(_layer_idx, _pos);}

const USigSet& Position::getFacts() const {
    if (_mirrored_facts != nullptr) return *_mirrored_facts;
    return _facts;
}
const NodeHashMap<int, USigSet>& Position::getQFacts() const {return _qfacts;}
const USigSet& Position::getQFacts(int predId) const {return _qfacts.count(predId) ? _qfacts.at(predId) : EMPTY_USIG_SET;}
int Position::getNumQFacts() const {
    int sum = 0;
    for (const auto& entry : _qfacts) sum += entry.second.size();
    return sum;
}
const USigSet& Position::getTrueFacts() const {return _true_facts;}
const USigSet& Position::getFalseFacts() const {return _false_facts;}
const NodeHashMap<USignature, USigSet, USignatureHasher>& Position::getPosFactSupports() const {return _pos_fact_supports;}
const NodeHashMap<USignature, USigSet, USignatureHasher>& Position::getNegFactSupports() const {return _neg_fact_supports;}
const NodeHashMap<USignature, std::vector<TypeConstraint>, USignatureHasher>& Position::getQConstantsTypeConstraints() const {
    return _q_constants_type_constraints;
}
const NodeHashMap<USignature, NodeHashSet<Substitution, Substitution::Hasher>, USignatureHasher>& 
Position::getForbiddenSubstitutions() const {
    return _forbidden_substitutions_per_op;
}

const NodeHashMap<USignature, int, USignatureHasher>& Position::getActions() const {return _actions;}
const NodeHashMap<USignature, int, USignatureHasher>& Position::getReductions() const {return _reductions;}
const NodeHashMap<USignature, USigSet, USignatureHasher>& Position::getExpansions() const {return _expansions;}
const USigSet& Position::getAxiomaticOps() const {return _axiomatic_ops;}
int Position::getMaxExpansionSize() const {return _max_expansion_size;}

void Position::clearUnneeded() {
    _facts.clear();
    _facts.reserve(0);

    /*
    NodeHashMap<USignature, int, USignatureHasher> cleanedVars;
    for (const auto& r : _reductions) cleanedVars[r.first] = _variables[r.first];
    for (const auto& a : _actions) cleanedVars[a.first] = _variables[a.first];
    _variables = std::move(cleanedVars);
    */
}

void Position::clearFactChanges() {
    _qfacts.clear();
    _qfacts.reserve(0);
    _true_facts.clear();
    _true_facts.reserve(0);
    _false_facts.clear();
    _false_facts.reserve(0);
    _expansions.clear();
    _expansions.reserve(0);
    _axiomatic_ops.clear();
    _axiomatic_ops.reserve(0);
    _pos_fact_supports.clear();
    _pos_fact_supports.reserve(0);
    _neg_fact_supports.clear();
    _neg_fact_supports.reserve(0);
    _q_constants_type_constraints.clear();
    _q_constants_type_constraints.reserve(0);
    _forbidden_substitutions_per_op.clear();
    _forbidden_substitutions_per_op.reserve(0);
    _fact_changes.clear();
    _fact_changes.reserve(0);
}