#ifndef AlgorithmsH
#define AlgorithmsH

#include "SyntaxTree.h"

enum TypeTask { TTUndefined, TTPrint, TTDomination };

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
