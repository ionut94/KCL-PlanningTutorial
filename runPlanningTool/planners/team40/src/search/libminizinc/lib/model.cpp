/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
 *  Main authors:
 *     Guido Tack <guido.tack@monash.edu>
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <minizinc/model.hh>
#include <minizinc/flatten_internal.hh>
#include <minizinc/astexception.hh>
#include <minizinc/prettyprinter.hh>

#undef MZN_DEBUG_FUNCTION_REGISTRY

namespace MiniZinc {
  
  Model::Model(void) : _parent(NULL), _solveItem(NULL), _outputItem(NULL) {
    GC::add(this);
  }

  Model::~Model(void) {
    for (unsigned int j=0; j<_items.size(); j++) {
      Item* i = _items[j];
      if (IncludeI* ii = i->dyn_cast<IncludeI>()) {
        if (ii->own() && ii->m()) {
          delete ii->m();
          ii->m(NULL);
        }
      }
    }
    GC::remove(this);
  }

  VarDeclIterator
  Model::begin_vardecls(void) {
    return VarDeclIterator(this, begin());
  }
  VarDeclIterator
  Model::end_vardecls(void) {
    return VarDeclIterator(this, end());
  }
  ConstraintIterator
  Model::begin_constraints(void) {
    return ConstraintIterator(this, begin());
  }
  ConstraintIterator
  Model::end_constraints(void) {
    return ConstraintIterator(this, end());
  }
  
  SolveI*
  Model::solveItem() {
    return _solveItem;
  }
  
  OutputI*
  Model::outputItem() {
    return _outputItem;
  }

  void
  Model::addItem(Item* i) {
    _items.push_back(i);
    if (i->isa<SolveI>()) {
      Model* m = this;
      while (m->_parent)
        m = m->_parent;
      m->_solveItem = i->cast<SolveI>();
    } else if (i->isa<OutputI>()) {
      Model* m = this;
      while (m->_parent)
        m = m->_parent;
      m->_outputItem = i->cast<OutputI>();
    }
  }

  void
  Model::setOutputItem(OutputI* oi) {
    Model* m = this;
    while (m->_parent)
      m = m->_parent;
    m->_outputItem = oi;
  }
  
  void
  Model::registerFn(EnvI& env, FunctionI* fi) {
    Model* m = this;
    while (m->_parent)
      m = m->_parent;
    FnMap::iterator i_id = m->fnmap.find(fi->id());
    if (i_id == m->fnmap.end()) {
      // new element
      std::vector<FunctionI*> v; v.push_back(fi);
      m->fnmap.insert(std::pair<ASTString,std::vector<FunctionI*> >(fi->id(),v));
    } else {
      // add to list of existing elements
      std::vector<FunctionI*>& v = i_id->second;
      for (unsigned int i=0; i<v.size(); i++) {
        if (v[i]->params().size() == fi->params().size()) {
          bool alleq=true;
          for (unsigned int j=0; j<fi->params().size(); j++) {
            Type t1 = v[i]->params()[j]->type();
            Type t2 = fi->params()[j]->type();
            t1.enumId(0);
            t2.enumId(0);
            if (t1 != t2) {
              alleq=false; break;
            }
          }
          if (alleq) {
            if (v[i]->e() && fi->e()) {
              throw TypeError(env, fi->loc(),
                              "function with the same type already defined in "
                              +v[i]->loc().toString());
              
            } else {
              if (fi->e())
                v[i] = fi;
              return;
            }
          }
        }
      }
      v.push_back(fi);
    }
  }

  FunctionI*
  Model::matchFn(EnvI& env, const ASTString& id,
                 const std::vector<Type>& t,
                 bool strictEnums) {
    if (id==constants().var_redef->id())
      return constants().var_redef;
    Model* m = this;
    while (m->_parent)
      m = m->_parent;
    FnMap::iterator i_id = m->fnmap.find(id);
    if (i_id == m->fnmap.end()) {
      return NULL;
    }
    std::vector<FunctionI*>& v = i_id->second;
    for (unsigned int i=0; i<v.size(); i++) {
      FunctionI* fi = v[i];
#ifdef MZN_DEBUG_FUNCTION_REGISTRY
      std::cerr << "try " << *fi;
#endif
      if (fi->params().size() == t.size()) {
        bool match=true;
        for (unsigned int j=0; j<t.size(); j++) {
          if (!env.isSubtype(t[j],fi->params()[j]->type(),strictEnums)) {
#ifdef MZN_DEBUG_FUNCTION_REGISTRY
            std::cerr << t[j].toString(env) << " does not match "
            << fi->params()[j]->type().toString(env) << "\n";
#endif
            match=false;
            break;
          }
        }
        if (match) {
          return fi;
        }
      }
    }
    return NULL;
  }

  void
  Model::mergeStdLib(EnvI &env, Model *m) const {
    for (FnMap::const_iterator it=fnmap.begin(); it != fnmap.end(); ++it) {
      for (std::vector<FunctionI*>::const_iterator cit = it->second.begin(); cit != it->second.end(); ++cit) {
        if ((*cit)->from_stdlib()) {
          m->registerFn(env, *cit);
        }
      }
    }
  }
  
  namespace {
    class FunSort {
    public:
      bool operator()(FunctionI* x, FunctionI* y) const {
        if (x->params().size() < y->params().size())
          return true;
        if (x->params().size() == y->params().size()) {
          for (unsigned int i=0; i<x->params().size(); i++) {
            switch (x->params()[i]->type().cmp(y->params()[i]->type())) {
            case -1: return true;
            case 1: return false;
            }
          }
        }
        return false;
      }
    };
  }

  void
  Model::sortFn(void) {
    Model* m = this;
    while (m->_parent)
      m = m->_parent;
    FunSort funsort;
    for (FnMap::iterator it=m->fnmap.begin(); it!=m->fnmap.end(); ++it) {
      std::sort(it->second.begin(),it->second.end(),funsort);
    }
  }

  void
  Model::checkFnOverloading(EnvI& env) {
    Model* m = this;
    while (m->_parent)
      m = m->_parent;
    for (FnMap::iterator it=m->fnmap.begin(); it!=m->fnmap.end(); ++it) {
      std::vector<FunctionI*>& fs = it->second;
      for (unsigned int i=0; i<fs.size()-1; i++) {
        FunctionI* cur = fs[i];
        for (unsigned int j=i+1; j<fs.size(); j++) {
          FunctionI* cmp = fs[j];
          if (cur->params().size() != cmp->params().size())
            break;
          bool allEqual = true;
          for (unsigned int i=0; i<cur->params().size(); i++) {
            Type t1 = cur->params()[i]->type();
            Type t2 = cmp->params()[i]->type();
            t1.enumId(0);
            t2.enumId(0);
            if (t1!=t2) {
              allEqual = false;
              break;
            }
          }
          if (allEqual)
            throw TypeError(env,cur->loc(),
                            "unsupported type of overloading. \nFunction/predicate with equivalent signature defined in "+cmp->loc().toString());
        }
      }
    }
  }
  
  FunctionI*
  Model::matchFn(EnvI& env, const ASTString& id,
                 const std::vector<Expression*>& args,
                 bool strictEnums) const {
    if (id==constants().var_redef->id())
      return constants().var_redef;
    const Model* m = this;
    while (m->_parent)
      m = m->_parent;
    FnMap::const_iterator it = m->fnmap.find(id);
    if (it == m->fnmap.end()) {
      return NULL;
    }
    const std::vector<FunctionI*>& v = it->second;
    std::vector<FunctionI*> matched;
    Expression* botarg = NULL;
    for (unsigned int i=0; i<v.size(); i++) {
      FunctionI* fi = v[i];
#ifdef MZN_DEBUG_FUNCTION_REGISTRY
      std::cerr << "try " << *fi;
#endif
      if (fi->params().size() == args.size()) {
        bool match=true;
        for (unsigned int j=0; j<args.size(); j++) {
          if (!env.isSubtype(args[j]->type(),fi->params()[j]->type(),strictEnums)) {
#ifdef MZN_DEBUG_FUNCTION_REGISTRY
            std::cerr << args[j]->type().toString(env) << " does not match "
            << fi->params()[j]->type().toString(env) << "\n";
#endif
            match=false;
            break;
          }
          if (args[j]->type().isbot() && fi->params()[j]->type().bt()!=Type::BT_TOP) {
            botarg = args[j];
          }
        }
        if (match) {
          if (botarg)
            matched.push_back(fi);
          else
            return fi;
        }
      }
    }
    if (matched.empty())
      return NULL;
    if (matched.size()==1)
      return matched[0];
    Type t = matched[0]->ti()->type();
    t.ti(Type::TI_PAR);
    for (unsigned int i=1; i<matched.size(); i++) {
      if (!env.isSubtype(t,matched[i]->ti()->type(),strictEnums))
        throw TypeError(env, botarg->loc(), "ambiguous overloading on return type of function");
    }
    return matched[0];
  }
  
  FunctionI*
  Model::matchFn(EnvI& env, Call* c, bool strictEnums) const {
    if (c->id()==constants().var_redef->id())
      return constants().var_redef;
    const Model* m = this;
    while (m->_parent)
      m = m->_parent;
    FnMap::const_iterator it = m->fnmap.find(c->id());
    if (it == m->fnmap.end()) {
      return NULL;
    }
    const std::vector<FunctionI*>& v = it->second;
    std::vector<FunctionI*> matched;
    Expression* botarg = NULL;
    for (unsigned int i=0; i<v.size(); i++) {
      FunctionI* fi = v[i];
#ifdef MZN_DEBUG_FUNCTION_REGISTRY
      std::cerr << "try " << *fi;
#endif
      if (fi->params().size() == c->args().size()) {
        bool match=true;
        for (unsigned int j=0; j<c->args().size(); j++) {
          if (!env.isSubtype(c->args()[j]->type(),fi->params()[j]->type(),strictEnums)) {
#ifdef MZN_DEBUG_FUNCTION_REGISTRY
            std::cerr << c->args()[j]->type().toString(env) << " does not match "
            << fi->params()[j]->type().toString(env) << "\n";
            std::cerr << "Wrong argument is " << *c->args()[j];
#endif
            match=false;
            break;
          }
          if (c->args()[j]->type().isbot() && fi->params()[j]->type().bt()!=Type::BT_TOP) {
            botarg = c->args()[j];
          }
        }
        if (match) {
          if (botarg)
            matched.push_back(fi);
          else
            return fi;
        }
      }
    }
    if (matched.empty())
      return NULL;
    if (matched.size()==1)
      return matched[0];
    Type t = matched[0]->ti()->type();
    t.ti(Type::TI_PAR);
    for (unsigned int i=1; i<matched.size(); i++) {
      if (!env.isSubtype(t,matched[i]->ti()->type(),strictEnums))
        throw TypeError(env, botarg->loc(), "ambiguous overloading on return type of function");
    }
    return matched[0];
  }

  Item*&
  Model::operator[] (int i) { assert(i < _items.size()); return _items[i]; }
  const Item*
  Model::operator[] (int i) const { assert(i < _items.size()); return _items[i]; }
  unsigned int
  Model::size(void) const { return _items.size(); }
  
  std::vector<Item*>::iterator
  Model::begin(void) { return _items.begin(); }

  std::vector<Item*>::const_iterator
  Model::begin(void) const { return _items.begin(); }

  std::vector<Item*>::iterator
  Model::end(void) { return _items.end(); }

  std::vector<Item*>::const_iterator
  Model::end(void) const { return _items.end(); }
  
  void
  Model::compact(void) {
    struct { bool operator() (const Item* i) {
      return i->removed();
    }} isremoved;
    _items.erase(remove_if(_items.begin(),_items.end(),isremoved),
                 _items.end());
  }
  
}
