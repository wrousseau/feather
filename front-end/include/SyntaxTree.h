#ifndef SyntaxTreeH
#define SyntaxTreeH

#include <iostream>
#include "assert.h"
#include <map>
#include <vector>
#include <list>
#include <set>
#include <memory>

namespace std {

  template <class Type> class shared_ptr {
  private:
    int* m_count;
    Type* m_object;

  public:
    shared_ptr() : m_count(NULL), m_object(NULL) {}
    explicit shared_ptr(Type* object)
    :  m_count(NULL), m_object(object)
    {  if (m_object) {
      m_count = new int;
      *m_count = 0;
      ++(*m_count);
    };
  }
  shared_ptr(const shared_ptr<Type>& source)
  :  m_count(source.m_count), m_object(source.m_object)
  {  if (m_count) ++(*m_count); }
  shared_ptr<Type>& operator=(const shared_ptr<Type>& source)
  {  if (this != &source) {
    clear();
    m_count = source.m_count;
    m_object = source.m_object;
    if (m_count) ++(*m_count);
  };
  return *this;
}
void reset(Type* ptObjectSource)
{  clear();
  m_object = ptObjectSource;
  if (m_object) {
    m_count = new int;
    *m_count = 0;
    ++(*m_count);
  };
}
~shared_ptr()
{  if (m_count && m_object) {
  if (--(*m_count) == 0) {
    delete m_count;
    delete m_object;
  };
  m_count = NULL;
  m_object = NULL;
}
}
void clear()
{  if (m_count && m_object) {
  if (--(*m_count) == 0) {
    delete m_count;
    delete m_object;
  };
};
m_count = NULL;
m_object = NULL;
}
Type& operator*() const { assert(m_object); return *m_object; }
Type* operator->() const { assert(m_object); return m_object; }
Type* get() const { return m_object; }
};

} // end of namespace std

/***************************************/
/* Définition de la table des symboles */
/***************************************/

class VirtualType {
public:
  VirtualType() {}
  VirtualType(const VirtualType& source) {}

  virtual ~VirtualType() {}
  virtual int getSize() const = 0;
  virtual VirtualType* clone() const = 0;
  virtual void print(std::ostream& out) const = 0;
};

class Scope;
class Function;
class SymbolTable {
private:
  std::map<std::string, int> m_localIndexes;
  std::vector<VirtualType*> m_types;
  std::shared_ptr<SymbolTable> m_parent;

public:
  SymbolTable() {}
  ~SymbolTable()
  {  for (std::vector<VirtualType*>::iterator iIter = m_types.begin();
    iIter != m_types.end(); ++iIter)
    delete *iIter;
    m_types.clear();
  }

  void addDeclaration(const std::string& name, VirtualType* type)
  {  m_localIndexes.insert(std::pair<std::string, int>(name, m_types.size()));
    m_types.push_back(type);
  }
  bool contain(const std::string& name) const { return m_localIndexes.find(name) != m_localIndexes.end(); }
  int localIndex(const std::string& name) const { return m_localIndexes.find(name)->second; }
  const VirtualType& getType(int uIndex) const { return *m_types[uIndex]; }
  friend class Scope;
  void printAsDeclarations(std::ostream& out) const
  {  for (std::map<std::string, int>::const_iterator iIter = m_localIndexes.begin();
    iIter != m_localIndexes.end(); ++iIter)
    out << m_types[iIter->second] << ' ' << iIter->first
    << " := " << iIter->second << '\t';
  }
  int count() const { return m_types.size(); }
};

class Scope {
private:
  std::shared_ptr<SymbolTable> m_last;

  Scope(const std::shared_ptr<SymbolTable>& lastSource) : m_last(lastSource) {}

public:
  Scope() : m_last(new SymbolTable()) {}
  Scope(const Scope& source) : m_last(source.m_last) {}

  void push()
  {  std::shared_ptr<SymbolTable> parent = m_last;
    m_last.reset(new SymbolTable());
    m_last->m_parent = parent;
  }
  void pop() { m_last = m_last->m_parent; }
  enum FindResult { FRNotFound, FRLocal, FRGlobal, FRType };

  void addDeclaration(const std::string& name, VirtualType* type)
  {  m_last->addDeclaration(name, type); }
  FindResult find(const std::string& name, int& local, Scope& scope) const;
  int getFunctionIndex(int local) const;
  void printAsDeclarations(std::ostream& out) const
  {  m_last->printAsDeclarations(out); }
  int count() const { return m_last->count(); }
  const VirtualType& getType(int uIndex) const { return m_last->getType(uIndex); }
};

/************************/
/* D�finition des types */
/************************/

class BaseType : public VirtualType {
public:
  enum Value { VUndefined, VInt, VChar };

private:
  Value m_value;

public:
  BaseType() : m_value(VUndefined) {}
  BaseType(Value valueSource) : m_value(valueSource) {}
  BaseType(const BaseType& source) : VirtualType(source), m_value(source.m_value) {}
  virtual int getSize() const
  {  assert(m_value != VUndefined); return (m_value == VInt) ? 4 : 1; }
  virtual VirtualType* clone() const { return new BaseType(*this); }
  virtual void print(std::ostream& out) const
  {  out << ((m_value == VInt) ? "int" : ((m_value == VChar) ? "char" : "no-type")); }
};

class PointerType : public VirtualType {
private:
  std::auto_ptr<VirtualType> m_subType;

public:
  PointerType(VirtualType* subType) : m_subType(subType) {}
  PointerType(const PointerType& source)
  : VirtualType(source), m_subType(source.m_subType->clone()) {}

  virtual int getSize() const { return 4; }
  virtual VirtualType* clone() const { return new PointerType(*this); }
  VirtualType* getSubType() { return m_subType.release(); }
  virtual void print(std::ostream& out) const
  {  m_subType->print(out);
    out << '*';
  }
};

/*****************************************/
/* D�finition des it�rations de parcours */
/*****************************************/

enum Comparison { CLess, CEqual, CGreater, CNonComparable };

class VirtualInstruction;
class WorkList;
class VirtualTask {
private:
  VirtualInstruction* m_instruction;

protected:
  static Comparison fcompare(int uFst, int uSnd)
  {  return uFst < uSnd ? CLess : ((uFst > uSnd) ? CGreater : CEqual); }

public:
  VirtualTask() : m_instruction(NULL) {}
  VirtualTask(const VirtualTask& source) : m_instruction(source.m_instruction) {}

  virtual Comparison compare(const VirtualTask& source) const;

  virtual ~VirtualTask() {}
  virtual VirtualTask* clone() const { return new VirtualTask(*this); }
  virtual int getType() const { return 0; }
  virtual bool mergeWith(VirtualTask& source) { return true; }
  virtual void applyOn(VirtualInstruction& instruction, WorkList& continuations) {}

  bool operator==(const VirtualTask& source) const { return compare(source) == CEqual; }
  bool operator<(const VirtualTask& source) const { return compare(source) == CLess; }
  bool operator>(const VirtualTask& source) const { return compare(source) == CGreater; }
  bool operator<=(const VirtualTask& source) const
  {  Comparison cResult = compare(source); return cResult == CLess || cResult == CEqual; }
  bool operator>=(const VirtualTask& source) const
  {  Comparison cResult = compare(source); return cResult == CGreater || cResult == CEqual; }
  bool operator!=(const VirtualTask& source) const
  {  Comparison cResult = compare(source); return cResult == CGreater || cResult == CLess; }

  bool hasInstruction() const { return m_instruction; }
  VirtualInstruction& getInstruction() const { assert(m_instruction); return *m_instruction; }
  VirtualTask& setInstruction(const VirtualInstruction& instruction)
  {  m_instruction = &const_cast<VirtualInstruction&>(instruction); return *this; }
  VirtualTask& clearInstruction() { m_instruction = NULL; return *this; }
};

class WorkList {
private:
  std::list<VirtualTask*> m_tasks;

public:
  WorkList() {}
  virtual ~WorkList()
  {  for (std::list<VirtualTask*>::iterator iIter = m_tasks.begin();
    iIter != m_tasks.end(); ++iIter)
    if (*iIter) delete (*iIter);
    m_tasks.clear();
  }

  void addNewAsFirst(VirtualTask* task) { m_tasks.push_front(task); }
  void addNewAsLast(VirtualTask* task) { m_tasks.push_back(task); }
  void addNewSorted(VirtualTask* task);
  VirtualTask* extractFirst() { VirtualTask* result = m_tasks.front(); m_tasks.pop_front(); return result; }
  VirtualTask* extractLast() { VirtualTask* result = m_tasks.back(); m_tasks.pop_back(); return result; }
  virtual void markInstructionWith(VirtualInstruction& instruction, VirtualTask& task);

  void execute();

  class Reusability {
  private:
    bool m_reuse;
    bool m_sorted;

  public:
    Reusability() : m_reuse(false), m_sorted(false) {}
    Reusability(const Reusability& source) : m_reuse(source.m_reuse), m_sorted(source.m_sorted) {}

    Reusability& setReuse() { m_reuse = true; return *this; }
    Reusability& setSorted() { m_sorted = true; return *this; }
    void clear() { m_reuse = false; m_sorted = false; }
    friend class WorkList;
  };
};

/*******************************/
/* D�finitions des expressions */
/*******************************/

class VirtualExpression {
public:
  enum Type
  {  TUndefined, TConstant, TChar, TString, TLocalVariable, TParameter, TGlobalVariable,
    TComparison, TUnaryOperator, TBinaryOperator, TDereference, TCast, TFunctionCall,
    TAssign
  };
  typedef WorkList::Reusability Reusability;

private:
  Type m_type;

protected:
  void setType(Type typeSource) { assert(m_type == TUndefined); m_type = typeSource; }

public:
  VirtualExpression() : m_type(TUndefined) {}
  virtual ~VirtualExpression() {}
  Type type() const { return m_type; }
  virtual void print(std::ostream& out) const = 0;
  virtual std::auto_ptr<VirtualType> newType(Function* function) const = 0;

  virtual void handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {}
};

class ConstantExpression : public VirtualExpression {
private:
  int m_value;

public:
  ConstantExpression(int value) : m_value(value) { setType(TConstant); }

  const int& value() const { return m_value; }
  virtual void print(std::ostream& out) const { out << m_value; }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  { return std::auto_ptr<VirtualType>(new BaseType(BaseType::VInt)); }
};

class CharExpression : public VirtualExpression {
private:
  char m_value;

public:
  CharExpression(int valueSource) : m_value(valueSource) { setType(TChar); }

  const char& value() const { return m_value; }
  virtual void print(std::ostream& out) const { out << m_value; }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  { return std::auto_ptr<VirtualType>(new BaseType(BaseType::VChar)); }
};

class StringExpression : public VirtualExpression {
private:
  std::string m_value;

public:
  StringExpression(const std::string& valueSource) : m_value(valueSource) { setType(TString); }

  virtual void print(std::ostream& out) const { out << '"' << m_value << '"'; }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  {  return std::auto_ptr<VirtualType>(new PointerType(new BaseType(BaseType::VChar))); }
};

class LocalVariableExpression : public VirtualExpression {
private:
  Scope m_scope;
  std::string m_name;
  int m_localIndex;

public:
  LocalVariableExpression(const std::string& name, int localIndex, Scope scope)
  :  m_scope(scope), m_name(name), m_localIndex(localIndex)
  {  setType(TLocalVariable); }

  virtual void print(std::ostream& out) const { out << "[local " << m_localIndex << ": " << m_name << ']'; }
  int getLocalScope() const { return m_localIndex; }
  int getFunctionIndex(Function& function) const;
  int getGlobalIndex() const { return m_scope.getFunctionIndex(m_localIndex); }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  {  return std::auto_ptr<VirtualType>(m_scope.getType(m_localIndex).clone()); }
};

class ParameterExpression : public VirtualExpression {
private:
  std::string m_name;
  int m_localIndex;

public:
  ParameterExpression(const std::string& name, int localIndex)
  :  m_name(name), m_localIndex(localIndex) { setType(TParameter); }

  int getIndex() const { return m_localIndex; }
  virtual void print(std::ostream& out) const { out << "[parameter " << m_localIndex << ": " << m_name << ']'; }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const;
};

class GlobalVariableExpression : public VirtualExpression {
private:
  std::string m_name;
  int m_localIndex;

public:
  GlobalVariableExpression(const std::string& name, int localIndex)
  :  m_name(name), m_localIndex(localIndex) { setType(TGlobalVariable); }

  const int getIndex() const { return m_localIndex; }
  virtual void print(std::ostream& out) const { out << "[global " << m_localIndex << ": " << m_name << ']'; }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const;
};

class ComparisonExpression : public VirtualExpression {
public:
  enum Operator { OUndefined, OCompareLess, OCompareLessOrEqual, OCompareEqual, OCompareDifferent,
    OCompareGreaterOrEqual, OCompareGreater };

  private:
    Operator m_operator;
    std::auto_ptr<VirtualExpression> m_fst, m_snd;

  public:
    ComparisonExpression() : m_operator(OUndefined) { setType(TComparison); }
    ComparisonExpression& setOperator(Operator operatorSource)
    {  assert(m_operator == OUndefined); m_operator = operatorSource; return *this; }
    ComparisonExpression& setFst(VirtualExpression* fst) { m_fst.reset(fst); return *this; }
    ComparisonExpression& setSnd(VirtualExpression* snd) { m_snd.reset(snd); return *this; }

    virtual void print(std::ostream& out) const
    {  out << '(';
    if (m_fst.get())
    m_fst->print(out);
    else
    out << "no-expr";
    out << ' ';
    if (m_operator != OUndefined) {
      if (m_operator < OCompareEqual) out << '<';
      else if (m_operator > OCompareDifferent) out << '>';
      else if (m_operator == OCompareEqual) out << '=';
      else if (m_operator == OCompareDifferent) out << '!';
      if ((m_operator > OCompareLess) && (m_operator < OCompareGreater)) out << '=';
    }
    else out << "no-op";
    out << ' ';
    if (m_snd.get())
    m_snd->print(out);
    else
    out << "no-expr";
    out << ')';
  }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  { return std::auto_ptr<VirtualType>(new BaseType(BaseType::VInt)); }
};

class UnaryOperatorExpression : public VirtualExpression {
public:
  enum Operator { OUndefined, OMinus, ONeg };

private:
  Operator m_operator;
  std::auto_ptr<VirtualExpression> m_subExpression;

public:
  UnaryOperatorExpression() : m_operator(OUndefined) { setType(TUnaryOperator); }
  UnaryOperatorExpression& setOperator(Operator operatorSource)
  {  assert(m_operator == OUndefined); m_operator = operatorSource; return *this; }
  UnaryOperatorExpression& setSubExpression(VirtualExpression* subExpression)
  {  m_subExpression.reset(subExpression); return *this; }

  virtual void print(std::ostream& out) const
  {  if (m_operator == OUndefined)
    out << "no-op";
    else
    out << ((m_operator == ONeg) ? '!' : '-');
    if (m_subExpression.get())
    m_subExpression->print(out);
    else
    out << "no-expr";
  };
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  {  return m_subExpression->newType(function); }
};

class BinaryOperatorExpression : public VirtualExpression {
public:
  enum Operator { OUndefined, OPlus, OMinus, OTimes, ODivide };

private:
  Operator m_operator;
  std::auto_ptr<VirtualExpression> m_fst, m_snd;

public:
  BinaryOperatorExpression() : m_operator(OUndefined) { setType(TBinaryOperator); }
  BinaryOperatorExpression& setOperator(Operator operatorSource)
  {  assert(m_operator == OUndefined); m_operator = operatorSource; return *this; }
  BinaryOperatorExpression& setFst(VirtualExpression* fst) { m_fst.reset(fst); return *this; }
  BinaryOperatorExpression& setSnd(VirtualExpression* snd) { m_snd.reset(snd); return *this; }

  virtual void print(std::ostream& out) const
  {  out << '(';
  if (m_fst.get())
  m_fst->print(out);
  else
  out << "no-expr";
  out << ' ';
  if (m_operator != OUndefined)
  out << ((m_operator == OPlus) ? '+' : ((m_operator == OMinus)
  ? '-' : ((m_operator == OTimes) ? '*' : '/')));
  else
  out << "no-op";
  out << ' ';
  if (m_snd.get())
  m_snd->print(out);
  else
  out << "no-expr";
  out << ')';
}
virtual std::auto_ptr<VirtualType> newType(Function* function) const
{  return m_fst->newType(function); }
};

class DereferenceExpression : public VirtualExpression {
private:
  std::auto_ptr<VirtualExpression> m_subExpression;

public:
  DereferenceExpression() { setType(TDereference); }

  DereferenceExpression& setSubExpression(VirtualExpression* subExpression)
  {  m_subExpression.reset(subExpression); return *this; }
  virtual void print(std::ostream& out) const
  {  out << '*';
  if (m_subExpression.get())
  m_subExpression->print(out);
  else
  out << "no-expr";
};
virtual std::auto_ptr<VirtualType> newType(Function* function) const
{  std::auto_ptr<VirtualType> result = m_subExpression->newType(function);
  assert(dynamic_cast<const PointerType*>(result.get()));
  return std::auto_ptr<VirtualType>(((PointerType&) *result).getSubType());
}
};

class ReferenceExpression : public VirtualExpression {
private:
  std::auto_ptr<VirtualExpression> m_subExpression;

public:
  ReferenceExpression() { setType(TDereference); }

  ReferenceExpression& setSubExpression(VirtualExpression* subExpression)
  {  m_subExpression.reset(subExpression); return *this; }
  virtual void print(std::ostream& out) const
  {  out << '&';
  if (m_subExpression.get())
  m_subExpression->print(out);
  else
  out << "no-expr";
};
virtual std::auto_ptr<VirtualType> newType(Function* function) const
{  std::auto_ptr<VirtualType> result = m_subExpression->newType(function);
  return std::auto_ptr<VirtualType>(new PointerType(result.release()));
}
};

class CastExpression : public VirtualExpression {
private:
  std::auto_ptr<VirtualType> m_type;
  std::auto_ptr<VirtualExpression> m_subExpression;

public:
  CastExpression() { VirtualExpression::setType(TCast); }

  CastExpression& setSubExpression(VirtualExpression* subExpression)
  {  m_subExpression.reset(subExpression); return *this; }
  CastExpression& setType(VirtualType* type) { m_type.reset(type); return *this; }
  virtual void print(std::ostream& out) const
  {  out << '(';
  if (m_type.get())
  m_type->print(out);
  else
  out << "no-type";
  out << ')' << ' ';
  if (m_subExpression.get())
  m_subExpression->print(out);
  else
  out << "no-expr";
}
virtual std::auto_ptr<VirtualType> newType(Function* function) const
{  return std::auto_ptr<VirtualType>(m_type->clone()); }
};

class FunctionCallExpression : public VirtualExpression {
private:
  Function* m_function;
  std::vector<VirtualExpression*> m_arguments;

public:
  FunctionCallExpression(Function& function) : m_function(&function) { setType(TFunctionCall); }
  virtual ~FunctionCallExpression()
  {  for (std::vector<VirtualExpression*>::iterator iIter = m_arguments.begin(); iIter != m_arguments.end(); ++iIter)
    if (*iIter) delete (*iIter);
    m_arguments.clear();
  }
  FunctionCallExpression& addArgument(VirtualExpression* argument)
  {  m_arguments.push_back(argument); return *this; }
  virtual void print(std::ostream& out) const;
  virtual std::auto_ptr<VirtualType> newType(Function* callerFunction) const;
};

class AssignExpression : public VirtualExpression {
private:
  std::auto_ptr<VirtualExpression> m_lvalue;
  std::auto_ptr<VirtualExpression> m_rvalue;

public:
  AssignExpression() { setType(TAssign); }

  AssignExpression& setLValue(VirtualExpression* lvalue) { m_lvalue.reset(lvalue); return *this; }
  AssignExpression& setRValue(VirtualExpression* rvalue) { m_rvalue.reset(rvalue); return *this; }

  virtual void print(std::ostream& out) const
  {  if (m_lvalue.get())
    m_lvalue->print(out);
    else
    out << "no-expr";
    out << " = ";
    if (m_rvalue.get())
    m_rvalue->print(out);
    else
    out << "no-expr";
  }
  virtual std::auto_ptr<VirtualType> newType(Function* function) const
  {  return m_lvalue->newType(function); }
};

/*******************************/
/* Définition des instructions */
/*******************************/

class IfInstruction;
class VirtualInstruction {
public:
  enum Type { TUndefined, TExpression, TIf, TGoto, TLabel, TReturn, TEnterBlock, TExitBlock };
  typedef WorkList::Reusability Reusability;

private:
  Type m_type;
  VirtualInstruction* m_next;
  VirtualInstruction* m_previous;
  int m_registrationIndex;
  int m_dominationHeight;

public:
  mutable void* mark;

protected:
  void setType(Type type) { assert(m_type == TUndefined); m_type = type; }
  void setNextTo(VirtualInstruction& next) { m_next = &next; }
  void setPreviousTo(VirtualInstruction& previous) { m_previous = &previous; }

  friend class IfInstruction;
  void setRegistrationIndex(int index)
  {  assert((m_registrationIndex < 0) && (index >= 0));
    m_registrationIndex = index;
  }

public:
  VirtualInstruction()
  : m_type(TUndefined), m_next(NULL), m_previous(NULL), m_registrationIndex(-1), m_dominationHeight(0), mark(NULL) {}
  virtual ~VirtualInstruction() {}

  Type type() const { return m_type; }
  virtual void handle(VirtualTask& task, WorkList& continuations, Reusability& reuse);
  virtual int countNexts() const { return m_next ? 1 : 0; }
  virtual int countPreviouses() const { return m_previous ? 1 : 0; }
  virtual bool propagateOnUnmarked(VirtualTask& task, WorkList& continuations, Reusability& reuse) const
  {  int nexts = countNexts();
    bool result = false;
    if (nexts == 0)
    result = true;
    if (m_next) {
      if (m_next->mark == 0) {
        task.clearInstruction();
        task.setInstruction(*m_next);
        reuse.setReuse();
      };
      result = (nexts == 1);
    };
    return result;
  }
  void setDominationHeight(int height)
  {
    m_dominationHeight = height;
  }
  const int& getDominationHeight() const { return m_dominationHeight; }

  VirtualInstruction* getSNextInstruction() const { return m_next; }
  VirtualInstruction* getSPreviousInstruction() const { return m_previous; }
  void connectTo(VirtualInstruction& next)
  {  m_next = &next; next.m_previous = this; }
  virtual void print(std::ostream& out) const = 0;
  void clearMark() { mark = NULL; }
  bool isValid() const { return m_registrationIndex >= 0; }
  const int& getRegistrationIndex() const { return m_registrationIndex; }
  friend class Function;
};

inline Comparison
VirtualTask::compare(const VirtualTask& source) const
{  return fcompare(getInstruction().getRegistrationIndex(),
  source.getInstruction().getRegistrationIndex());
}

inline void
WorkList::markInstructionWith(VirtualInstruction& instruction, VirtualTask& task)
{  instruction.mark = (void*) 1; }

class ExpressionInstruction : public VirtualInstruction {
private:
  std::auto_ptr<VirtualExpression> m_expression;

public:
  ExpressionInstruction() { setType(TExpression); }

  ExpressionInstruction& setExpression(VirtualExpression* expression) { m_expression.reset(expression); return *this; }
  virtual void print(std::ostream& out) const
  {  m_expression->print(out);
    out << ';' << '\n';
  }
};

class GotoInstruction;
class IfInstruction : public VirtualInstruction {
private:
  std::auto_ptr<VirtualExpression> m_expression;
  GotoInstruction* m_then;

public:
  IfInstruction() { setType(TIf); }

  virtual int countNexts() const { return VirtualInstruction::countNexts() + (m_then ? 1 : 0); }
  IfInstruction& setExpression(VirtualExpression* expression) { m_expression.reset(expression); return *this; }
  void connectToThen(GotoInstruction& gotoPoint);
  void connectToElse(GotoInstruction& gotoPoint);

  virtual bool propagateOnUnmarked(VirtualTask& task, WorkList& continuations, Reusability& reuse) const;
  virtual void print(std::ostream& out) const
  {  out << "if (";
  if (m_expression.get())
  m_expression->print(out);
  else
  out << "no-expr";
  out << ")\n";
}
};

class LabelInstruction;
class GotoInstruction : public VirtualInstruction {
public:
  enum Context { CUndefined, CAfterIfThen, CAfterIfElse, CLoop, CBeforeLabel };

private:
  Context m_context;

public:
  GotoInstruction() : m_context(CUndefined) { setType(TGoto); }
  void setAfterIfThen() { assert(m_context == CUndefined); m_context = CAfterIfThen; }
  void setAfterIfElse() { assert(m_context == CUndefined); m_context = CAfterIfElse; }
  void setLoop() { assert(m_context == CUndefined); m_context = CLoop; }
  void setBeforeLabel() { assert(m_context == CUndefined); m_context = CBeforeLabel; }
  void connectToLabel(LabelInstruction& liInstruction);
  virtual bool propagateOnUnmarked(VirtualTask& task, WorkList& continuations, Reusability& reuse) const
  {  bool hasResult = VirtualInstruction::propagateOnUnmarked(task, continuations, reuse);
    if ((m_context >= CLoop) || !m_context)
    reuse.setSorted();
    return hasResult;
  }
  virtual void print(std::ostream& out) const
  {  if (m_context == CLoop)
    out << "goto loop " << getSNextInstruction();
    else if (m_context == CAfterIfThen)
    out << "then";
    else if (m_context == CAfterIfElse)
    out << "else";
    else if (m_context == CBeforeLabel)
    out << "goto label " << getSNextInstruction();
    else
    out << "goto " << getSNextInstruction();
    out << '\n';
  }
};

inline void
IfInstruction::connectToThen(GotoInstruction& thenPoint)
{ m_then = &thenPoint; thenPoint.setPreviousTo(*this); thenPoint.setAfterIfThen(); }

inline void
IfInstruction::connectToElse(GotoInstruction& elsePoint)
{ connectTo(elsePoint); elsePoint.setAfterIfElse(); }


inline bool
IfInstruction::propagateOnUnmarked(VirtualTask& task, WorkList& continuations, Reusability& reuse) const {
  if (!VirtualInstruction::propagateOnUnmarked(task, continuations, reuse)) {
    if (m_then) {
      if (m_then->mark == 0) {
        std::auto_ptr<VirtualTask> newTask(task.clone());
        newTask->clearInstruction();
        newTask->setInstruction(*m_then);
        continuations.addNewSorted(newTask.release());
        reuse.setSorted();
      };
    };
  };
  return true;
}

class LabelInstruction : public VirtualInstruction {
private:
  GotoInstruction* m_goto;
  VirtualInstruction* m_dominator;

public:
  LabelInstruction() : m_goto(NULL), m_dominator(NULL) { setType(TLabel); }

  virtual int countPreviouses() const { return VirtualInstruction::countPreviouses() + (m_goto ? 1 : 0); }
  VirtualInstruction* getSDominator() const { return m_dominator; }
  void setGotoFrom(GotoInstruction& gotoPoint) { assert(!m_goto); m_goto = &gotoPoint; }
  virtual void print(std::ostream& out) const
  {
    out << "label " << this;
    if (m_dominator)
    {
      out << '\t' << "dominated by " << m_dominator->getRegistrationIndex() << ' ';
      m_dominator->print(out);
    }
    else
    {
      out << '\n';
    }
  }
};

inline void
GotoInstruction::connectToLabel(LabelInstruction& instruction)
{  setNextTo(instruction); instruction.setGotoFrom(*this); }


class ReturnInstruction : public VirtualInstruction {
private:
  std::auto_ptr<VirtualExpression> m_result;

public:
  ReturnInstruction() { setType(TReturn); }

  ReturnInstruction& setResult(VirtualExpression* expression) { m_result.reset(expression); return *this; }
  virtual void print(std::ostream& out) const
  {  out << "return ";
  m_result->print(out);
  out << ";\n";
}
};

class EnterBlockInstruction : public VirtualInstruction {
private:
  Scope m_scope;

public:
  EnterBlockInstruction() { setType(TEnterBlock); }

  EnterBlockInstruction& setScope(const Scope& scope) { m_scope = scope; return *this; }
  const Scope& scope() const { return m_scope; }
  virtual void handle(VirtualTask& task, WorkList& continuations, Reusability& reuse);

  virtual void print(std::ostream& out) const
  {  out << '{';
  m_scope.printAsDeclarations(out);
  out << '\n';
}
};

class ExitBlockInstruction : public VirtualInstruction {
private:
  int m_nbDeclarations;

public:
  ExitBlockInstruction() : m_nbDeclarations(0) { setType(TExitBlock); }

  virtual void handle(VirtualTask& task, WorkList& continuations, Reusability& reuse);
  ExitBlockInstruction& setScope(const Scope& sScope) { m_nbDeclarations = sScope.count(); return *this; }
  virtual void print(std::ostream& out) const
  {  out << "}\n"; }
};

/****************************/
/* D�finition des fonctions */
/****************************/

class Function;
class FunctionSignature {
private:
  std::vector<VirtualType*> m_parameterType;
  std::vector<std::string> m_parameterNames;
  std::auto_ptr<VirtualType> m_resultType;

public:
  FunctionSignature() {}
  ~FunctionSignature()
  {  for (std::vector<VirtualType*>::iterator iIter = m_parameterType.begin();
    iIter != m_parameterType.end(); ++iIter)
    if (*iIter) delete (*iIter);
    m_parameterType.clear();
  }

  void addNewParameter(const std::string& parameterName, VirtualType* type)
  {  m_parameterType.push_back(type);
    m_parameterNames.push_back(parameterName);
  }
  void setResult(VirtualType* type) { m_resultType.reset(type); }
  int size() const { return m_parameterType.size(); }
  const VirtualType& getTypeParameter(int uIndex) const { return *m_parameterType[uIndex]; }
  const VirtualType& getResultType() const { return *m_resultType; }
  bool findParameters(const std::string& name, int& local) const
  {  bool fResult = false;
    int uSize = m_parameterNames.size();
    for (int uIndex = 0; uIndex < uSize; ++uIndex)
    if (m_parameterNames[uIndex] == name) {
      local = uIndex;
      return true;
    };
    return fResult;
  }

  friend class Function;
};

class Function {
private:
  Scope* m_globalScope;
  FunctionSignature m_signature;
  std::string m_name;
  std::vector<VirtualInstruction*> m_instructions;

public:
  Function() : m_globalScope(NULL)  {}
  Function(const std::string& name) : m_globalScope(NULL), m_name(name)  {}
  Function(const std::string& name, Scope& globalScope)
  :  m_globalScope(&globalScope), m_name(name)  {}
  Function(const Function& source)
  :  m_globalScope(source.m_globalScope), m_name(source.m_name) {}
  ~Function()
  {  for (std::vector<VirtualInstruction*>::iterator iIter = m_instructions.begin();
    iIter != m_instructions.end(); ++iIter)
    if (*iIter) delete (*iIter);
    m_instructions.clear();
  }
  Function& operator=(const Function& source)
  {  m_globalScope = source.m_globalScope;
    m_name = source.m_name;
    return *this;
  }

  void print(std::ostream& out) const
  {  out << "function " << m_name << '\n';
  for (std::vector<VirtualInstruction*>::const_iterator iIter = m_instructions.begin();
  iIter != m_instructions.end(); ++iIter)
  (*iIter)->print(out);
}
bool operator<(const Function& source) const { return m_name < source.m_name; }
bool operator<=(const Function& source) const { return m_name <= source.m_name; }
bool operator>=(const Function& source) const { return m_name >= source.m_name; }
bool operator>(const Function& source) const { return m_name > source.m_name; }
bool operator==(const Function& source) const { return m_name == source.m_name; }
bool operator!=(const Function& source) const { return m_name != source.m_name; }
FunctionSignature& signature() { return m_signature; }
const FunctionSignature& signature() const { return m_signature; }
const std::string& getName() const { return m_name; }
bool findParameters(const std::string& name, int& local) const
{  return signature().findParameters(name, local); }
VirtualInstruction& getFirstInstruction() const
{  return *m_instructions[0]; }

void addNewInstructionAfter(VirtualInstruction* newInstruction, VirtualInstruction& previous)
{  newInstruction->setRegistrationIndex(m_instructions.size());
  m_instructions.push_back(newInstruction);
  previous.connectTo(*newInstruction);
}
void addFirstInstruction(VirtualInstruction* newInstruction)
{  assert(m_instructions.empty());
  newInstruction->setRegistrationIndex(0);
  m_instructions.push_back(newInstruction);
}
void addNewInstruction(VirtualInstruction* newInstruction)
{  newInstruction->setRegistrationIndex(m_instructions.size());
  m_instructions.push_back(newInstruction);
}
GotoInstruction& setFirst()
{  assert(m_instructions.empty());
  GotoInstruction* result = new GotoInstruction();
  result->setRegistrationIndex(m_instructions.size());
  m_instructions.push_back(result);
  return *result;
}
GotoInstruction& setThen(IfInstruction& ifInstruction)
{  GotoInstruction* thenPoint = new GotoInstruction();
  thenPoint->setRegistrationIndex(m_instructions.size());
  m_instructions.push_back(thenPoint);
  ifInstruction.connectToThen(*thenPoint);
  return *thenPoint;
}
GotoInstruction& setElse(IfInstruction& ifInstruction)
{  GotoInstruction* elsePoint = new GotoInstruction();
  elsePoint->setRegistrationIndex(m_instructions.size());
  m_instructions.push_back(elsePoint);
  ifInstruction.connectToElse(*elsePoint);
  return *elsePoint;
}
const VirtualType& getTypeGlobal(int uIndex) { return m_globalScope->getType(uIndex); }
const Scope& globalScope() const { assert(m_globalScope); return *m_globalScope; }
};

inline int
LocalVariableExpression::getFunctionIndex(Function& function) const
{  return m_scope.getFunctionIndex(m_localIndex) + function.signature().size(); }

inline std::auto_ptr<VirtualType>
ParameterExpression::newType(Function* function) const
{  return std::auto_ptr<VirtualType>(function->signature().getTypeParameter(m_localIndex).clone()); }

inline std::auto_ptr<VirtualType>
GlobalVariableExpression::newType(Function* function) const
{  return std::auto_ptr<VirtualType>(function->getTypeGlobal(m_localIndex).clone()); }

inline std::auto_ptr<VirtualType>
FunctionCallExpression::newType(Function* callerFunction) const
{  return std::auto_ptr<VirtualType>(m_function->signature().getResultType().clone()); }

/*****************************/
/* D�finition des programmes */
/*****************************/

int getTokenIdentifier(const char* szText);
class Program {
private:
  std::set<Function> m_functions;
  Scope m_globalScope;

public:
  Program() {}

  void print(std::ostream& out) const
  {  for (std::set<Function>::const_iterator functionIter = m_functions.begin();
    functionIter != m_functions.end(); ++functionIter)
    functionIter->print(out);
  }
  void printWithWorkList(std::ostream& out) const;
  class ParseContext {
  private:
    friend int getTokenIdentifier(const char* szText);
    class ParseState {
    private:
      enum Type { TUndefined, TIfThen, TIfElse, TDo, TWhile, TFor, TBlock };
      Type m_type;
      VirtualInstruction* m_reference;
      VirtualInstruction* m_endThen;

    public:
      ParseState() : m_type(TUndefined), m_reference(NULL), m_endThen(NULL) {}
      ParseState(const ParseState& source)
      :  m_type(source.m_type), m_reference(source.m_reference), m_endThen(source.m_endThen) {}

      GotoInstruction& setIf(IfInstruction& ifInstruction, Function& function)
      {  assert(m_type == TUndefined);
        m_type = TIfThen;
        m_reference = &ifInstruction;
        return function.setThen(ifInstruction);
      }
      GotoInstruction& setElse(Function& function, VirtualInstruction& viEndThen)
      {  assert(m_type == TIfThen && dynamic_cast<const IfInstruction*>(m_reference));
        m_type = TIfElse;
        m_endThen = &viEndThen;
        return function.setElse((IfInstruction&) *m_reference);
      }
      LabelInstruction& setDoLoop(LabelInstruction& label)
      {  assert(m_type == TUndefined);
        m_type = TDo;
        m_reference = &label;
        return label;
      }
      LabelInstruction& setWhileLoop(LabelInstruction& label)
      {  assert(m_type == TUndefined);
        m_type = TWhile;
        m_reference = &label;
        return label;
      }
      LabelInstruction& getLoopReference() const
      {  assert((m_type >= TDo || m_type <= TFor) && dynamic_cast<const LabelInstruction*>(m_reference));
        return (LabelInstruction&) *m_reference;
      }
      VirtualInstruction& getEndThen() const { assert(m_endThen); return *m_endThen; }
      IfInstruction& getCondition() const
      {  assert(dynamic_cast<const IfInstruction*>(m_reference));
        return (IfInstruction&) *m_reference;
      }
      EnterBlockInstruction& setBlock(EnterBlockInstruction& ebiEnterBlock)
      {  assert(m_type == TUndefined);
        m_type = TBlock;
        m_reference = &ebiEnterBlock;
        return ebiEnterBlock;
      }
      EnterBlockInstruction& getBlock()
      {  assert(m_type == TBlock && dynamic_cast<const EnterBlockInstruction*>(m_reference));
        return (EnterBlockInstruction&) *m_reference;
      }
    };

    Program* m_currentProgram;
    Function* m_function;
    VirtualInstruction* m_current;
    std::list<ParseState> m_states;
    Scope m_scope;

  public:
    ParseContext()
    :  m_currentProgram(NULL), m_function(NULL), m_current(NULL) {}
    ParseContext(Program& pProgram)
    :  m_currentProgram(&pProgram), m_function(NULL), m_current(NULL), m_scope(pProgram.m_globalScope) {}

    void setProgram(Program& program)
    {  assert(!m_currentProgram);
      m_currentProgram = &program;
      m_scope = program.m_globalScope;
    }
    void clear()
    {  m_currentProgram = NULL; m_function = NULL; m_current = NULL;
      m_states.clear(); m_scope = Scope();
    }
    Scope& scope() { return m_scope; }
    void addNewReturnInstruction(ReturnInstruction* returnInstruction)
    {  assert(m_function && m_current);
      m_function->addNewInstructionAfter(returnInstruction, *m_current);
      m_current = NULL;
    }
    void addNewInstruction(VirtualInstruction* instruction)
    {  assert(m_function && m_current);
      m_function->addNewInstructionAfter(instruction, *m_current);
      m_current = instruction;
    }
    void addDeclarationStatement(VirtualType* type, const std::string& name)
    {  m_scope.addDeclaration(name, type); }
    VirtualInstruction& getLastInstruction() const { assert(m_current); return *m_current; }
    void pushDoLoop()
    {  GotoInstruction* gotoPoint = new GotoInstruction();
      m_function->addNewInstructionAfter(gotoPoint, *m_current);
      gotoPoint->setBeforeLabel();
      LabelInstruction* label = new LabelInstruction();
      m_function->addNewInstructionAfter(label, *gotoPoint);
      m_states.push_back(ParseState());
      m_current = &m_states.back().setDoLoop(*label);
    }
    VirtualInstruction& closeDoLoop(VirtualInstruction& last, VirtualExpression* expression)
    {  assert(m_current == &last);
      LabelInstruction& label = m_states.back().getLoopReference();
      m_states.pop_back();
      IfInstruction* condition = new IfInstruction();
      m_function->addNewInstructionAfter(condition, *m_current);
      condition->setExpression(expression);
      GotoInstruction& thenPoint = m_function->setThen(*condition);
      GotoInstruction* loopPoint = new GotoInstruction();
      m_function->addNewInstructionAfter(loopPoint, thenPoint);
      loopPoint->setLoop();
      loopPoint->connectToLabel(label);
      return *(m_current = &m_function->setElse(*condition));
    }
    void pushWhileLoop(VirtualExpression* expression)
    {  LabelInstruction* label = new LabelInstruction();
      m_function->addNewInstructionAfter(label, *m_current);
      m_states.push_back(ParseState());
      m_current = &m_states.back().setWhileLoop(*label);
      IfInstruction* condition = new IfInstruction();
      m_function->addNewInstructionAfter(condition, *m_current);
      condition->setExpression(expression);
      m_current = &m_function->setThen(*condition);
    }
    VirtualInstruction& closeWhileLoop(VirtualInstruction& last)
    {  assert(m_current == &last);
      LabelInstruction& label = m_states.back().getLoopReference();
      GotoInstruction* loopPoint = new GotoInstruction();
      m_function->addNewInstructionAfter(loopPoint, last);
      loopPoint->setLoop();
      loopPoint->connectToLabel(label);
      m_states.pop_back();
      assert(dynamic_cast<const IfInstruction*>(label.getSNextInstruction()));
      IfInstruction& condition = *((IfInstruction*) label.getSNextInstruction());
      return *(m_current = &m_function->setElse(condition));
    }
    void pushIfThen(VirtualExpression* expression)
    {  IfInstruction* condition = new IfInstruction();
      m_function->addNewInstructionAfter(condition, *m_current);
      condition->setExpression(expression);
      m_states.push_back(ParseState());
      m_current = &m_states.back().setIf(*condition, *m_function);
    }
    void setToIfElse(VirtualInstruction& last)
    {  assert(m_current == &last);
      m_current = &m_states.back().setElse(*m_function, last);
    }
    VirtualInstruction& closeIfThen(VirtualInstruction& last)
    {  assert(m_current == &last);
      GotoInstruction* thenLabel = new GotoInstruction();
      m_function->addNewInstructionAfter(thenLabel, *m_current);
      thenLabel->setBeforeLabel();
      LabelInstruction* label = new LabelInstruction();
      m_function->addNewInstructionAfter(label, *thenLabel);
      GotoInstruction& giElse = m_function->setElse(m_states.back().getCondition());
      GotoInstruction* connectToLabel = new GotoInstruction();
      m_function->addNewInstructionAfter(connectToLabel, giElse);
      connectToLabel->setBeforeLabel();
      connectToLabel->connectToLabel(*label);
      m_states.pop_back();
      m_current = label;
      return *label;
    }
    VirtualInstruction& closeIfElse(VirtualInstruction& lastThen, VirtualInstruction& lastElse)
    {  assert(m_current == &lastElse && &m_states.back().getEndThen() == &lastThen);
      GotoInstruction* elseLabel = new GotoInstruction();
      m_function->addNewInstructionAfter(elseLabel, lastElse);
      elseLabel->setBeforeLabel();
      LabelInstruction* label = new LabelInstruction();
      m_function->addNewInstructionAfter(label, *elseLabel);
      GotoInstruction* connectToLabel = new GotoInstruction();
      m_function->addNewInstructionAfter(connectToLabel, lastThen);
      connectToLabel->setBeforeLabel();
      connectToLabel->connectToLabel(*label);
      m_states.pop_back();
      m_current = label;
      return *label;
    }
    void pushBlock()
    {  EnterBlockInstruction* block = new EnterBlockInstruction();
      m_function->addNewInstructionAfter(block, *m_current);
      m_scope.push();
      block->setScope(m_scope);
      m_states.push_back(ParseState());
      m_current = &m_states.back().setBlock(*block);
    }
    VirtualInstruction& closeBlock(VirtualInstruction& last)
    {  assert(m_current == &last);
      EnterBlockInstruction& enter = m_states.back().getBlock();
      m_states.pop_back();
      ExitBlockInstruction* pebiExit = new ExitBlockInstruction();
      pebiExit->setScope(enter.scope());
      m_function->addNewInstructionAfter(pebiExit, last);
      m_current = pebiExit;
      return *pebiExit;
    }
    Function& addFunction(const std::string& functionName)
    {  assert(m_currentProgram);
      return const_cast<Function&>(*(m_currentProgram->m_functions.insert(
        Function(functionName, m_currentProgram->m_globalScope)).first));
      }
      void pushFunction(const std::string& functionName)
      {  assert(m_currentProgram && !m_current);
        m_function = &const_cast<Function&>(*(m_currentProgram->m_functions.find(Function(functionName, m_currentProgram->m_globalScope))));
        m_scope = m_currentProgram->m_globalScope;
        EnterBlockInstruction* block = new EnterBlockInstruction();
        m_function->addFirstInstruction(block);
        m_scope.push();
        block->setScope(m_scope);
        m_states.push_back(ParseState());
        m_current = &m_states.back().setBlock(*block);
      }
      void popFunction()
      {  assert(m_currentProgram && m_function);
        EnterBlockInstruction& enter = m_states.back().getBlock();
        m_states.pop_back();
        ExitBlockInstruction* pebiExit = new ExitBlockInstruction();
        pebiExit->setScope(enter.scope());
        m_function->addNewInstruction(pebiExit);
        m_function = NULL;
        m_scope = m_currentProgram->m_globalScope;
        m_current = NULL;
      }
      void setEmptyFunction(const std::string& functionName)
      {  assert(m_currentProgram && !m_current);
        m_function = &const_cast<Function&>(*(m_currentProgram->m_functions.find(Function(functionName, m_currentProgram->m_globalScope))));
        EnterBlockInstruction* block = new EnterBlockInstruction();
        m_function->addFirstInstruction(block);
        ExitBlockInstruction* pebiExit = new ExitBlockInstruction();
        m_function->addNewInstruction(pebiExit);
        m_function = NULL;
      }
      void addGlobalDeclarationStatement(VirtualType* type, const std::string& name)
      {  assert(m_currentProgram);
        m_currentProgram->m_globalScope.addDeclaration(name, type);
      }
      const std::set<Function>& functions() const { return m_currentProgram->m_functions; }
    };
    friend class ParseContext;
  };

  #endif // SyntaxTreeH
