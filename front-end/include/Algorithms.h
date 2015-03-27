#ifndef AlgorithmsH
#define AlgorithmsH

#include "SyntaxTree.h"

enum TypeTask { TTUndefined, TTPrint, TTDomination, TTPhiInsertion, TTLabelPhiFrontier, TTRenaming };

class PrintTask : public VirtualTask {
public:
  int m_ident;

public:
  PrintTask(const VirtualInstruction& instruction) : m_ident(0) { setInstruction(instruction); }
  PrintTask(const PrintTask& source) : VirtualTask(source), m_ident(source.m_ident) {}

  virtual void applyOn(VirtualInstruction& instruction, WorkList& continuations);
  virtual VirtualTask* clone() const { return new PrintTask(*this); }
  virtual int getType() const { return TTPrint; }
};

class PrintAgenda : public WorkList {
public:
  std::ostream& m_out;
  std::list<VirtualInstruction*> m_markedInstruction;

public:
  PrintAgenda(std::ostream& out, const Function& function)
  :  m_out(out)
  {  addNewAsFirst(new PrintTask(function.getFirstInstruction())); }
  virtual ~PrintAgenda()
  {  for (std::list<VirtualInstruction*>::iterator iter = m_markedInstruction.begin();
    iter != m_markedInstruction.end(); ++iter)
    (*iter)->mark = NULL;
    m_markedInstruction.clear();
  }
  virtual void markInstructionWith(VirtualInstruction& instruction, VirtualTask& task)
  {  m_markedInstruction.push_back(&instruction);
    instruction.mark = (void*) 1;
  }
};

class DominationTask : public VirtualTask {
public:
  int m_height;
  VirtualInstruction* m_previous;

public:
  DominationTask(const VirtualInstruction& instruction) : m_height(1), m_previous(NULL)
  {
    setInstruction(instruction);
  }
  DominationTask(const DominationTask& source) : VirtualTask(source), m_height(source.m_height), m_previous(NULL)
  {

  }

  void setHeight(int newHeight)
  {
    assert(newHeight < m_height);
    m_height = newHeight;
  }

  VirtualInstruction* findDominatorWith(VirtualInstruction& instruction) const;

  virtual void applyOn(VirtualInstruction& instruction, WorkList& continuations)
  {
    instruction.setDominationHeight(m_height);
    m_previous = &instruction;
    m_height++;
  }

  virtual VirtualTask* clone() const
  {
    return new DominationTask(*this);
  }

  virtual int getType() const
  {
    return TTDomination;
  }

  virtual bool mergeWith(VirtualTask& vtSource)
  {
    assert(dynamic_cast<const DominationTask*>(&vtSource));
    DominationTask& source = (DominationTask&) vtSource;
    m_previous = findDominatorWith(*(source.m_previous));
    m_height = m_previous->getDominationHeight()+1;
    return true;
  }
};

class DominationAgenda : public WorkList {
public:
  DominationAgenda(const Function& function)
  {
    addNewAsFirst(new DominationTask(function.getFirstInstruction()));
  }
  virtual void markInstructionWith(VirtualInstruction& instruction, VirtualTask& task)
  {

  }
};

class PhiInsertionTask : public VirtualTask
{
public:
  class IsBefore
  {
  public:
    bool operator()(VirtualExpression* fst, VirtualExpression *snd) const;
  };

public:
  std::vector<GotoInstruction*>* m_dominationFrontier;
  Scope m_scope;
  std::set<VirtualExpression*, IsBefore> m_modified;
  bool m_isLValue;
  typedef std::set<VirtualExpression*, IsBefore> ModifiedVariables;
  class LabelResult
  {
  private:
    typedef std::map<VirtualExpression*, std::pair<GotoInstruction*, GotoInstruction*>, PhiInsertionTask::IsBefore> Map;
    Map m_map;
    bool m_hasMark;
    Scope m_scope;
    ModifiedVariables m_variablesToAdd;

  public:
    LabelResult() : m_hasMark(false)
    {

    }
    void setMark()
    {
      m_hasMark = true;
    }
    void setScope(const Scope& scope)
    {
      m_scope = scope;
    }
    LocalVariableExpression getAfterScopeLocalVariable() const
    {
      return LocalVariableExpression(std::string(), m_scope.count(), m_scope);
    }
    Map& map()
    {
      return m_map;
    }
    bool hasMark() const
    {
      return m_hasMark;
    }
    const ModifiedVariables& variablesToAdd() const
    {
      return m_variablesToAdd;
    }
    ModifiedVariables& variablesToAdd()
    {
      return m_variablesToAdd;
    }
    typedef Map::iterator iterator;
  };

public:
  PhiInsertionTask(const Function& function) : m_dominationFrontier(NULL), m_scope(function.globalScope()), m_isLValue(false)
  {
    setInstruction(function.getFirstInstruction());
  }
  PhiInsertionTask(GotoInstruction& gotoInstruction, const PhiInsertionTask& source) : m_dominationFrontier(&gotoInstruction.getDominationFrontier()), m_scope(source.m_scope), m_isLValue(false)
  {
    setInstruction(gotoInstruction);
  }
  PhiInsertionTask(const PhiInsertionTask& source): VirtualTask(source), m_scope(source.m_scope), m_modified(source.m_modified), m_isLValue(false)
  {

  }
  virtual VirtualTask* clone() const
  {
    return new PhiInsertionTask(*this);
  }
  virtual int getType() const
  {
    return TTPhiInsertion;
  }
};

class PhiInsertionAgenda : public WorkList
{
private:
  std::vector<LabelInstruction*> m_labels;
public:
  PhiInsertionAgenda(const Function& function)
  {
    addNewAsFirst(new PhiInsertionTask(function));
  }
  virtual ~PhiInsertionAgenda()
  {
    for (std::vector<LabelInstruction*>::iterator labelIter = m_labels.begin(); labelIter != m_labels.end(); ++labelIter)
    {
      if ((*labelIter)->mark)
      {
        delete (PhiInsertionTask::LabelResult*) (*labelIter)->mark;
        (*labelIter)->mark = NULL;
      }
    }
    m_labels.clear();
  }
  const std::vector<LabelInstruction*>& labels() const
  {
    return m_labels;
  }
  virtual void markInstructionWith(VirtualInstruction& instruction, VirtualTask&  vtTask)
  {
    if (instruction.type() == VirtualInstruction::TLabel)
    {
      assert(dynamic_cast<const PhiInsertionTask*>(&vtTask));
      const PhiInsertionTask& task = (const PhiInsertionTask&) vtTask;
      if (instruction.mark == NULL)
      {
        assert(dynamic_cast<const LabelInstruction*>(&instruction));
        instruction.mark = new PhiInsertionTask::LabelResult();
      }
      PhiInsertionTask::LabelResult& result = *((PhiInsertionTask::LabelResult*) instruction.mark);
      if (!result.hasMark())
      {
        m_labels.push_back((LabelInstruction*) &instruction);
        result.setMark();
        result.setScope(task.m_scope);
      }
    }
  }
};

class LabelPhiFrontierTask : public VirtualTask
{
public:
  typedef PhiInsertionTask::IsBefore IsBefore;
  std::set<VirtualExpression*, IsBefore> m_modified;

public:
  LabelPhiFrontierTask(const LabelInstruction& label, std::set<VirtualExpression*, IsBefore>& modified)
  {
    setInstruction(label);
    m_modified.swap(modified);
  }
  LabelPhiFrontierTask(const LabelPhiFrontierTask& source) : VirtualTask(source)
  {

  }

  virtual VirtualTask* clone() const
  {
    return new LabelPhiFrontierTask(*this);
  }
  virtual int getType() const
  {
    return TTLabelPhiFrontier;
  }
};

class LabelPhiFrontierAgenda : public WorkList
{
public:
  LabelPhiFrontierAgenda()
  {

  }

  typedef PhiInsertionTask::IsBefore IsBefore;
  typedef PhiInsertionTask::LabelResult LabelResult;
  void propagate(const LabelInstruction& label);
  void propagateOn(const LabelInstruction& label, const std::set<VirtualExpression*, IsBefore>& originModified);
};

/* Tâche de renommage */

class RenamingAgenda;
class RenamingTask: public VirtualTask
{
public:
  class VariableRenaming
  {
  private:
    VirtualExpression* m_toReplace;
    LocalVariableExpression* m_newValue;
    Function* m_function;
    VirtualInstruction* m_lastDominator;
    std::shared_ptr<VariableRenaming> m_parent;
  public:
    VariableRenaming(VirtualExpression& locate, const Function& function) : m_toReplace(&locate), m_newValue(NULL), m_function(&const_cast<Function&>(function))
    {

    }
    VariableRenaming(VirtualExpression* toReplace, LocalVariableExpression* newValue, const Function& function, VirtualInstruction* lastDominator) : m_toReplace(toReplace), m_newValue(newValue), m_function(&const_cast<Function&>(function)), m_lastDominator(lastDominator)
    {

    }
    class Transfert
    {

    };
    VariableRenaming(const VariableRenaming& source, Transfert) : m_toReplace(NULL), m_newValue(source.m_newValue), m_function(source.m_function), m_lastDominator(source.m_lastDominator)
    {
      const_cast<VariableRenaming&>(source).m_newValue = NULL;
    }
    VariableRenaming(const VariableRenaming& source) : m_toReplace(source.m_toReplace), m_newValue(source.m_newValue), m_function(source.m_function), m_lastDominator(source.m_lastDominator), m_parent(source.m_parent)
    {

    }
    VariableRenaming& operator=(const VariableRenaming& source)
    {
      m_toReplace = source.m_toReplace;
      m_newValue = source.m_newValue;
      m_function = source.m_function;
      m_lastDominator = source.m_lastDominator;
      m_parent = source.m_parent;
      return *this;
    }
    void cloneExpressions()
    {
      m_toReplace = m_toReplace->clone();
      if (m_newValue)
      {
        m_newValue = (LocalVariableExpression*) m_newValue->clone();
      }
    }
    Comparison compare(const VariableRenaming& source) const;
    bool operator==(const VariableRenaming& source) const
    {
      return compare(source) == CEqual;
    }
    bool operator<(const VariableRenaming& source) const
    {
      return compare(source) == CLess;
    }
    bool operator>(const VariableRenaming& source) const
    {
      return compare(source) == CGreater;
    }
    bool operator<=(const VariableRenaming& source) const
    {
      Comparison result = compare(source);
      return result == CLess || result == CEqual;
    }
    bool operator>=(const VariableRenaming& source) const
    {
      Comparison result = compare(source);
      return result == CGreater || result == CEqual;
    }
    bool operator!=(const VariableRenaming& source) const
    {
      Comparison result = compare(source);
      return result == CGreater || result == CLess;
    }
    void free()
    {
      if (m_toReplace)
      {
        delete m_toReplace;
        m_toReplace = NULL;
      }
      if (m_newValue)
      {
        delete m_newValue;
        m_newValue = NULL;
      }
    }
    LocalVariableExpression& getNewValue() const
    {
      return m_newValue ? *m_newValue : *m_parent->m_newValue;
    }
    LocalVariableExpression* getSDominatorNewValue() const
    {
      return m_parent.get() ? m_parent->m_newValue : NULL;
    }
    void setNewValue(LocalVariableExpression* newValue, VirtualInstruction* lastDominator)
    {
      m_newValue = newValue;
      m_lastDominator = lastDominator;
    }
    bool setDominator(VirtualInstruction& dominator, GotoInstruction* previousLabel=NULL);
    friend class RenamingTask;
  };

public:
  std::set<VariableRenaming> m_renamings;
  bool m_isLValue; // attribut hérité
  LocalVariableExpression* m_localRename; // attribut synthétisé
  LocalVariableExpression* m_localReplace; // attribut synthétisé
  const Function& m_function;
  GotoInstruction* m_previousLabel;
  GotoInstruction* m_lastBranch;
  bool m_isOnlyPhi;

private:
  static std::set<VariableRenaming>* cloneRenamings(const std::set<VariableRenaming>& source)
  {
    std::set<VariableRenaming>* result = new std::set<VariableRenaming>(source);
    for (std::set<VariableRenaming>::const_iterator iter = result->begin(); iter != result->end(); ++iter)
    {
      const_cast<VariableRenaming&>(*iter).cloneExpressions();
    }
    return result;
  }
  static void freeRenamings(std::set<VariableRenaming>& source)
  {
    for (std::set<VariableRenaming>::const_iterator iter = source.begin(); iter != source.end(); ++iter)
    {
      const_cast<VariableRenaming&>(*iter).free();
    }
    source.clear();
  }

public:
  RenamingTask(const Function& function) : m_isLValue(false), m_localRename(NULL), m_localReplace(NULL), m_function(function), m_previousLabel(NULL), m_lastBranch(NULL), m_isOnlyPhi(false)
  {
    setInstruction(function.getFirstInstruction());
  }
  RenamingTask(const RenamingTask& source) : VirtualTask(source), m_renamings(source.m_renamings), m_isLValue(false), m_localRename(NULL), m_localReplace(NULL), m_function(source.m_function), m_previousLabel(source.m_previousLabel), m_lastBranch(source.m_lastBranch), m_isOnlyPhi(source.m_isOnlyPhi)
  {
    for (std::set<VariableRenaming>::const_iterator iter = m_renamings.begin(); iter != m_renamings.end(); ++iter)
    {
      const_cast<VariableRenaming&>(*iter).cloneExpressions();
    }
  }
  virtual VirtualTask* clone() const
  {
    return new RenamingTask(*this);
  }
  virtual int getType() const
  {
    return TTRenaming;
  }
  virtual bool mergeWith(VirtualTask& source)
  {
    ((RenamingTask&) source).m_isOnlyPhi = true;
    return false;
  }
  friend class RenamingAgenda;
};

class RenamingAgenda : public WorkList
{
public:
  const Function* m_function;
  std::list<LabelInstruction*> m_markedInstruction;
  int m_ident;

public:
  RenamingAgenda(const Function& function) : m_function(&function), m_ident(1)
  {
    addNewAsFirst(new RenamingTask(function));
  }
  ~RenamingAgenda()
  {
    for (std::list<LabelInstruction*>::iterator iter = m_markedInstruction.begin(); iter != m_markedInstruction.end(); ++iter)
    {
      (*iter)->mark = NULL;
    }
    m_markedInstruction.clear();
  }

  virtual void markInstructionWith(VirtualInstruction& instruction, VirtualTask& task)
  {
    if ((instruction.type() == VirtualInstruction::TLabel) && !instruction.mark)
    {
      m_markedInstruction.push_back((LabelInstruction*) &instruction);
      instruction.mark = (void*) 1;
    }
  }
};

inline void
PrintTask::applyOn(VirtualInstruction& instruction, WorkList& continuations) {
  assert(dynamic_cast<const PrintAgenda*>(&continuations));
  PrintAgenda& agenda = (PrintAgenda&) continuations;
  agenda.m_out << instruction.getRegistrationIndex() << "  ";
  if (instruction.getRegistrationIndex() < 10)
  agenda.m_out << ' ';
  for (int shift = m_ident; --shift >= 0;)
  agenda.m_out << "  ";
  instruction.print(agenda.m_out);
}

#endif // AlgorithmsH
