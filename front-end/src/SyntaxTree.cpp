#include "SyntaxTree.h"
#include "Algorithms.h"
#include <stdio.h>

extern Program::ParseContext parseContext;
extern FILE *yyin;

int yyparse ();

Scope::FindResult
Scope::find(const std::string& name, int& local, Scope& scope) const {
  FindResult result = FRNotFound;
  std::shared_ptr<SymbolTable> current = m_last;
  while (current.get() && result == FRNotFound) {
    if (current->contain(name)) {
      local = current->localIndex(name);
      if (current->m_parent.get()) {
        result = FRLocal;
        scope = Scope(current);
      }
      else
      result = FRGlobal;
    };
    current = current->m_parent;
  };
  return result;
}

int
Scope::getFunctionIndex(int local) const {
  std::shared_ptr<SymbolTable> current = m_last;
  while (current->m_parent.get()) {
    local += current->m_parent->m_localIndexes.size();
    current = current->m_parent;
  };
  return local;
}

void
WorkList::execute() {
  while (!m_tasks.empty()) {
    std::auto_ptr<VirtualTask> current(extractFirst());
    Reusability reuse;
    current->getInstruction().handle(*current, *this, reuse);
    if (reuse.m_reuse) {
      if (reuse.m_sorted)
      addNewSorted(current.release());
      else
      addNewAsFirst(current.release());
    };
  };
}

void
WorkList::addNewSorted(VirtualTask* virtualTask) {
  if (!m_tasks.empty()) {
    std::list<VirtualTask*>::iterator iter = m_tasks.begin();
    std::auto_ptr<VirtualTask> task(virtualTask);
    Comparison compare = (*iter)->compare(*virtualTask);
    while (compare < CEqual) {
      if (++iter != m_tasks.end())
      compare = (*iter)->compare(*virtualTask);
      else
      break;
    };
    if (compare == CEqual)
    if (!(*iter)->mergeWith(*task))
    m_tasks.insert(iter, task.release());
    if (compare == CGreater)
    m_tasks.insert(iter, task.release());
    else if (compare == CLess)
    m_tasks.push_back(task.release());
  }
  else
  m_tasks.push_back(virtualTask);
}

void
FunctionCallExpression::print(std::ostream& out) const {
  if (m_function)
  out << m_function->getName() << '(';
  else
  out << "no-function";
  std::vector<VirtualExpression*>::const_iterator iter = m_arguments.begin();
  bool goOn = (iter != m_arguments.end());
  while (goOn) {
    (*iter)->print(out);
    if ((goOn = (++iter != m_arguments.end())) != false)
    out << ", ";
  };
  out << ')';
}

void
VirtualInstruction::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  task.applyOn(*this, continuations);
  continuations.markInstructionWith(task.getInstruction(), task);
  propagateOnUnmarked(task, continuations, reuse);
}

void LabelInstruction::handle(VirtualTask& vtTask, WorkList& continuations, Reusability& reuse)
{
  int type = vtTask.getType();
  if (type == TTDomination)
  {
    assert(dynamic_cast<const DominationTask*>(&vtTask));
    DominationTask& task = (DominationTask&) vtTask;
    if (!m_dominator)
    {
      m_dominator = task.m_previous;
    }
    else
    {
      VirtualInstruction* newDominator = task.findDominatorWith(*getSPreviousInstruction());
      if (newDominator == m_dominator)
      {
        return;
      }
      task.setHeight(newDominator->getDominationHeight()+1);
      m_dominator = newDominator;
    }
  }
  VirtualInstruction::handle(vtTask, continuations, reuse);
}

void
EnterBlockInstruction::handle(VirtualTask& virtualTask, WorkList& continuations, Reusability& reuse) {
  VirtualInstruction::handle(virtualTask, continuations, reuse);
  if (virtualTask.getType() == TTPrint) {
    assert(dynamic_cast<const PrintTask*>(&virtualTask));
    PrintTask& task = (PrintTask&) virtualTask;
    task.m_ident++;
  };
}

void
ExitBlockInstruction::handle(VirtualTask& virtualTask, WorkList& continuations, Reusability& reuse) {
  if (virtualTask.getType() == TTPrint) {
    assert(dynamic_cast<const PrintTask*>(&virtualTask));
    PrintTask& task = (PrintTask&) virtualTask;
    assert(task.m_ident > 0);
    task.m_ident--;
  };
  VirtualInstruction::handle(virtualTask, continuations, reuse);
}

void
Program::printWithWorkList(std::ostream& out) const {
  for (std::set<Function>::const_iterator functionIter = m_functions.begin();
  functionIter != m_functions.end(); ++functionIter) {
    out << "function " << functionIter->getName() << '\n';
    PrintAgenda agenda(out, *functionIter);
    agenda.execute();
    out << '\n';
  };
}

void Program::computeDominators()
{
  for (std::set<Function>::const_iterator functionIter = m_functions.begin(); functionIter != m_functions.end(); ++functionIter)
  {
    DominationAgenda agenda(*functionIter);
    agenda.execute();
  }
}

extern int yydebug;

int main( int argc, char** argv ) {
  // yydebug = 1;
  Program program;
  parseContext.setProgram(program);
  if (argc > 1)
  yyin = fopen(argv[1], "r");
  while (yyparse() != 0);
  if (yyin)
  fclose(yyin);
  std::cout << std::endl;
  program.print(std::cout);
  std::cout << std::endl;
  program.printWithWorkList(std::cout);
  std::cout << std::endl;
  program.computeDominators();
  program.printWithWorkList(std::cout);
  std::cout << std::endl;
  return 0;
}
