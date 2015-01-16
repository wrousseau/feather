#ifndef AlgorithmsH
#define AlgorithmsH

#include "SyntaxTree.h"

enum TypeTask { TTUndefined, TTPrint };

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

