#include "SyntaxTree.h"
#include "Algorithms.h"
#include <stdio.h>

extern Program::ParseContext parseContext;
extern FILE *yyin;

#include <sstream>

int yyparse ();

int SymbolTable::addSSADeclaration(std::string& name, int localIndex, int& ident)
{
  std::stringstream out;
  out << name << '_' << ident;
  ++ident;
  m_localIndexes.insert(std::pair<std::string, int>(name = out.str(), m_types.size()));
  m_types.push_back(m_types[localIndex]);
  m_uniqueDefinitions.push_back(NULL);
  return m_types.size()-1;
}

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

void LocalVariableExpression::handle(VirtualTask &vtTask, WorkList& continuations, Reusability& reuse)
{
  int type = vtTask.getType();
  if (type == TTPhiInsertion)
  {
    assert(dynamic_cast<const PhiInsertionTask*>(&vtTask));
    PhiInsertionTask& task = (PhiInsertionTask&) vtTask;
    if (task.m_isLValue)
    {
      if (task.m_modified.find(this) == task.m_modified.end())
      {
        task.m_modified.insert(this);
      }
      if (task.m_dominationFrontier)
      {
        for (std::vector<GotoInstruction*>::const_iterator labelIter = task.m_dominationFrontier->begin(); labelIter != task.m_dominationFrontier->end(); ++labelIter)
        {
          LabelInstruction& label = (LabelInstruction&) *((*labelIter)->getSNextInstruction());
          if (!label.mark)
          {
            label.mark = new PhiInsertionTask::LabelResult();
          }
          PhiInsertionTask::LabelResult& result = *((PhiInsertionTask::LabelResult*) label.mark);
          if (result.map().find(this) == result.map().end())
          {
            std::pair<VirtualExpression*, std::pair<GotoInstruction*, GotoInstruction*> > insert;
            insert.first = this;
            insert.second.first = NULL;
            insert.second.second = NULL;
            result.map().insert(insert);
          }
        }
      }
    }
  }
  else if (type == TTRenaming)
  {
    assert(dynamic_cast<const RenamingTask*>(&vtTask) && dynamic_cast<const RenamingAgenda*>(&continuations));
    RenamingTask& task = (RenamingTask&) vtTask;
    if (task.m_isLValue)
    {
      if (!task.m_isOnlyPhi && m_scope.hasSSADefinition(m_localIndex))
      {
        RenamingAgenda& remaingContinuations = (RenamingAgenda&) continuations;
        task.m_localReplace = new LocalVariableExpression(*this);
        m_localIndex = m_scope.addSSADeclaration(m_name, m_localIndex, remaingContinuations.m_ident);
      }
      task.m_localRename = this;
    }
    else // !rtTask.m_isLValue
    {
      RenamingTask::VariableRenaming locate(*this, task.m_function);
      std::set<RenamingTask::VariableRenaming>::iterator found = task.m_renamings.find(locate);
      if (found != task.m_renamings.end())
      {
        m_localIndex = found->getNewValue().m_localIndex;
        m_name = found->getNewValue().m_name;
      }
    }
  }
}

void
ComparisonExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    if (m_fst.get())
    m_fst->handle(task, continuations, reuse);
    if (m_snd.get())
    m_snd->handle(task, continuations, reuse);
  };
}
void
UnaryOperatorExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    if (m_subExpression.get())
    m_subExpression->handle(task, continuations, reuse);
  };
}
void
BinaryOperatorExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    if (m_fst.get())
    m_fst->handle(task, continuations, reuse);
    if (m_snd.get())
    m_snd->handle(task, continuations, reuse);
  };
}
void
DereferenceExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    if (m_subExpression.get())
    m_subExpression->handle(task, continuations, reuse);
  };
}
void
ReferenceExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    if (m_subExpression.get())
    m_subExpression->handle(task, continuations, reuse);
  };
}
void
CastExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    if (m_subExpression.get())
    m_subExpression->handle(task, continuations, reuse);
  };
}
void
FunctionCallExpression::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if (type == TTRenaming) {
    for (std::vector<VirtualExpression*>::const_iterator iter = m_arguments.begin();
    iter != m_arguments.end(); ++iter) {
      if (*iter)
      (*iter)->handle(task, continuations, reuse);
    };
  };
}

void AssignExpression::handle(VirtualTask& vtTask, WorkList& continuations, Reusability& reuse)
{
  int type = vtTask.getType();
  if (type == TTPhiInsertion && m_lvalue.get())
  {
    assert(dynamic_cast<const PhiInsertionTask*>(&vtTask));
    PhiInsertionTask& task = (PhiInsertionTask&) vtTask;
    task.m_isLValue = true;
    m_lvalue->handle(task, continuations, reuse);
    task.m_isLValue = false;
  }
  else if (type == TTRenaming)
  {
    assert(dynamic_cast<const RenamingTask*>(&vtTask));
    RenamingTask& task = (RenamingTask&) vtTask;
    task.m_isLValue = true;
    m_lvalue->handle(task, continuations, reuse);
    task.m_isLValue = false;
    m_rvalue->handle(task, continuations, reuse);
  }
}

void PhiExpression::print(std::ostream& out) const
{
  out << "phi(";
  if (m_fst.get())
  {
    m_fst->print(out);
    if (m_fstFrom)
    {
      out << ' ' << m_fstFrom->getRegistrationIndex();
    }
  }
  else
  {
    out << "no-expr";
  }
  out << ", ";
  if (m_snd.get())
  {
    m_snd->print(out);
    if (m_sndFrom)
    {
      out << ' ' << m_sndFrom->getRegistrationIndex();
    }
  }
  else
  {
    out << "no-expr";
  }
  out << ')';
}

void PhiExpression::handle(VirtualTask& vtTask, WorkList& continuations, Reusability& reuse)
{
  int type = vtTask.getType();
  if (type == TTRenaming)
  {
    assert(dynamic_cast<const RenamingTask*>(&vtTask));
    RenamingTask& task = (RenamingTask&) vtTask;
    assert(m_fst.get());
    if (!m_snd.get())
    {
      m_snd.reset(m_fst->clone());
    }
    if (task.m_previousLabel == m_fstFrom)
    {
      m_fst->handle(task, continuations, reuse);
    }
    else if (!m_sndFrom)
    {
      m_sndFrom = task.m_previousLabel;
      std::set<RenamingTask::VariableRenaming>::const_iterator found = task.m_renamings.find(RenamingTask::VariableRenaming(*m_fst, task.m_function));
      LocalVariableExpression* newValue = NULL;
      if (found != task.m_renamings.end())
      {
        newValue = found->getSDominatorNewValue();
      }
      if (newValue)
      {
        m_snd.reset(newValue->clone());
      }
    }
    else
    {
      assert(task.m_previousLabel == m_sndFrom);
      m_snd->handle(vtTask, continuations, reuse);
    }
  }
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

void ExpressionInstruction::handle(VirtualTask& vtTask, WorkList& continuations, Reusability& reuse)
{
  int type = vtTask.getType();
  if (type == TTPhiInsertion)
  {
    if (m_expression.get())
    {
      m_expression->handle(vtTask, continuations, reuse);
    }
  }
  else if (type == TTRenaming)
  {
    if (m_expression.get())
    {
      m_expression->handle(vtTask, continuations, reuse);
    }
    assert(dynamic_cast<RenamingTask*>(&vtTask));
    RenamingTask& task = (RenamingTask&) vtTask;
    if (task.m_localRename)
    {
      if (task.m_localReplace)
      {
        std::set<RenamingTask::VariableRenaming>::iterator found = task.m_renamings.find(RenamingTask::VariableRenaming(*task.m_localReplace, task.m_function));
        if (found != task.m_renamings.end())
        {
          const_cast<RenamingTask::VariableRenaming&>(*found).setNewValue(new LocalVariableExpression(*task.m_localRename), task.m_previousLabel ? task.m_previousLabel : (task.m_lastBranch ? task.m_lastBranch->getSPreviousInstruction() : NULL));
          delete task.m_localReplace;
          task.m_localReplace = NULL;
        }
        else
        {
          assert(!task.m_lastBranch || dynamic_cast<IfInstruction*>(task.m_lastBranch->getSPreviousInstruction()));
          task.m_renamings.insert(RenamingTask::VariableRenaming(task.m_localReplace, new LocalVariableExpression(*task.m_localRename), task.m_function, task.m_previousLabel ? task.m_previousLabel : (task.m_lastBranch ? task.m_lastBranch->getSPreviousInstruction() : NULL)));
          task.m_localReplace = NULL;
        }
      }
      task.m_localRename->scope().setSSADefinition(task.m_localRename->getLocalScope(), *this);
      task.m_localRename = NULL;
    }
    if (task.m_previousLabel)
    {
      bool isLast = true;
      if (getSNextInstruction() && getSNextInstruction()->type() == TExpression)
      {
        assert(dynamic_cast<const ExpressionInstruction*>(getSNextInstruction()));
        if (((const ExpressionInstruction&) *getSNextInstruction()).isPhi())
        {
          isLast = false;
          if (task.m_isOnlyPhi)
          {
            VirtualInstruction::handle(task, continuations, reuse);
          }
        }
      }
      if (isLast)
      {
        assert(dynamic_cast<const LabelInstruction*>(task.m_previousLabel->getSNextInstruction()));
        VirtualInstruction* dominator = ((LabelInstruction*) task.m_previousLabel->getSNextInstruction())->getSDominator();
        assert(dominator);
        std::set<RenamingTask::VariableRenaming>::iterator iter = task.m_renamings.begin();
        while (iter != task.m_renamings.end())
        {
          if (!const_cast<RenamingTask::VariableRenaming&>(*iter).setDominator(*dominator, task.m_previousLabel))
          {
            std::set<RenamingTask::VariableRenaming>::iterator nextIter(iter);
            ++nextIter;
            const RenamingTask::VariableRenaming* next = (nextIter != task.m_renamings.end()) ? &(*nextIter) : NULL;
            task.m_renamings.erase(iter);
            if (next)
            {
              iter = task.m_renamings.find(*next);
            }
            else
            {
              iter = task.m_renamings.end();
            }
          }
          else
          {
            ++iter;
          }
        }
        task.m_previousLabel = NULL;
      }
      if (task.m_isOnlyPhi)
      {
        return;
      }
    }
  }
  VirtualInstruction::handle(vtTask, continuations, reuse);
}

void IfInstruction::handle(VirtualTask& vtTask, WorkList& continuations, Reusability& reuse)
{
  int type = vtTask.getType();
  if (type == TTRenaming)
  {
    if (m_expression.get())
    {
      m_expression->handle(vtTask, continuations, reuse);
    }
    assert(dynamic_cast<const RenamingTask*>(&vtTask));
    RenamingTask& task = (RenamingTask&) vtTask;
    std::set<RenamingTask::VariableRenaming>::iterator iter = task.m_renamings.begin();
    while (iter != task.m_renamings.end())
    {
      if (!const_cast<RenamingTask::VariableRenaming&>(*iter).setDominator(*this))
      {
        std::set<RenamingTask::VariableRenaming>::iterator nextIter(iter);
        ++nextIter;
        const RenamingTask::VariableRenaming* next = (nextIter != task.m_renamings.end()) ? &(*nextIter) : NULL;
        task.m_renamings.erase(iter);
        if (next)
        {
          iter = task.m_renamings.find(*next);
        }
        else
        {
          iter = task.m_renamings.end();
        }
      }
      else
      {
        ++iter;
      }
    }
  }
  VirtualInstruction::handle(vtTask, continuations, reuse);
}

void GotoInstruction::handle(VirtualTask& vtTask, WorkList& continutations, Reusability& reuse)
{
  int type = vtTask.getType();
  if ((type == TTPhiInsertion) && ((m_context == CAfterIfThen) || (m_context == CAfterIfElse)))
  {
    assert(dynamic_cast<const PhiInsertionTask*>(&vtTask));
    PhiInsertionTask& task = (PhiInsertionTask&) vtTask;
    task.m_dominationFrontier = &(this->getDominationFrontier());
    if (getSPreviousInstruction() && getSPreviousInstruction()->type() == TIf)
    {
      for (std::vector<GotoInstruction*>::const_iterator labelIter = task.m_dominationFrontier->begin(); labelIter != task.m_dominationFrontier->end(); ++labelIter)
      {
        LabelInstruction& label = (LabelInstruction&) *(*labelIter)->getSNextInstruction();
        if (!label.mark)
        {
          label.mark = new PhiInsertionTask::LabelResult();
        }
        ((PhiInsertionTask::LabelResult*) label.mark)->variablesToAdd().insert(task.m_modified.begin(), task.m_modified.end());
      }
    }
  }
  else if (type == TTRenaming)
  {
    assert(dynamic_cast<const RenamingTask*>(&vtTask));
    RenamingTask& task = (RenamingTask&) vtTask;
    if ((m_context == CAfterIfThen) || (m_context == CAfterIfElse))
    {
      task.m_lastBranch = this;
    }
    else if (m_context >= CLoop)
    {
      task.m_previousLabel = this;
      if (getSNextInstruction()->mark)
      {
        task.m_isOnlyPhi = true;
        if (getSNextInstruction())
        {
          task.clearInstruction();
          task.setInstruction(*getSNextInstruction());
          reuse.setReuse();
        }
        return;
      }
    }
  }
  VirtualInstruction::handle(vtTask, continutations, reuse);
}

bool GotoInstruction::propagateOnUnmarked(VirtualTask& vtTask, WorkList& continuations, Reusability& reuse) const
{
  if (m_context >= CLoop && (vtTask.getType() == TTPhiInsertion))
  {
    if (getSNextInstruction() && getSNextInstruction()->type() == TLabel)
    {
      assert(dynamic_cast<const LabelInstruction*>(getSNextInstruction()));
      LabelInstruction& label = (LabelInstruction&) *getSNextInstruction();
      assert(dynamic_cast<const PhiInsertionTask*>(&vtTask));
      PhiInsertionTask &task = (PhiInsertionTask&) vtTask;
      if (!label.mark)
      {
        label.mark = new PhiInsertionTask::LabelResult();
      }
      bool hasFoundFrontier = false;
      if (task.m_dominationFrontier)
      {
        for (std::vector<GotoInstruction*>::const_iterator labelIter = task.m_dominationFrontier->begin(); labelIter != task.m_dominationFrontier->end(); ++labelIter)
        {
          if ((*labelIter)->getSNextInstruction() == &label)
          {
            PhiInsertionTask::LabelResult& result = *((PhiInsertionTask::LabelResult*) label.mark);
            LocalVariableExpression afterLast(std::string(), task.m_scope.count(), task.m_scope);
            for (PhiInsertionTask::ModifiedVariables::iterator iter = task.m_modified.begin(); iter != task.m_modified.end(); ++iter)
            {
              if (PhiInsertionTask::IsBefore().operator()(*iter, &afterLast))
              {
                PhiInsertionTask::LabelResult::iterator found = result.map().find(*iter);
                if (found != result.map().end())
                {
                  if (found->second.first == NULL)
                  {
                    found->second.first = *labelIter;
                  }
                  else if (found->second.second == NULL)
                  {
                    found->second.second = *labelIter;
                  }
                  else
                  {
                    assert(false);
                  }
                }
              }
            }
            hasFoundFrontier = true;
          }
        }
      }
      if (!((PhiInsertionTask::LabelResult*) label.mark)->hasMark())
      {
        task.clearInstruction();
        task.setInstruction(*getSNextInstruction());
        reuse.setReuse();
      }
      if (hasFoundFrontier)
      {
        task.m_modified = ((PhiInsertionTask::LabelResult*) label.mark)->variablesToAdd();
      }
      else if (label.mark)
      {
        PhiInsertionTask::LabelResult& result = *(PhiInsertionTask::LabelResult*) label.mark;
        task.m_modified.insert(result.variablesToAdd().begin(), result.variablesToAdd().end());
      }
      return true;
    }
  }
  if ((m_context >= CLoop) || !m_context)
  {
    reuse.setSorted();
  }
  return VirtualInstruction::propagateOnUnmarked(vtTask, continuations, reuse);
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
  else if (type == TTPhiInsertion)
  {
    assert(dynamic_cast<const PhiInsertionTask*>(&vtTask));
    ((PhiInsertionTask&) vtTask).m_dominationFrontier = &m_dominationFrontier;
  }
  else if (type == TTLabelPhiFrontier)
  {
    assert(dynamic_cast<const LabelPhiFrontierTask*>(&vtTask) && dynamic_cast<const LabelPhiFrontierAgenda*>(&continuations));
    ((LabelPhiFrontierAgenda&) continuations).propagateOn(*this, ((LabelPhiFrontierTask&) vtTask).m_modified);
    return;
  }
  else if (type == TTRenaming)
  {
    assert(dynamic_cast<const RenamingTask*>(&vtTask));
    RenamingTask& task = (RenamingTask&) vtTask;
    if (task.m_previousLabel)
    {
      bool isLast = true;
      if (getSNextInstruction() && getSNextInstruction()->type() == TExpression)
      {
        assert(dynamic_cast<const ExpressionInstruction*>(getSNextInstruction()));
        if (((const ExpressionInstruction&) *getSNextInstruction()).isPhi())
        {
          isLast = false;
          if (task.m_isOnlyPhi)
          {
            VirtualInstruction::handle(vtTask, continuations, reuse);
          }
        }
      }
      if (isLast)
      {
        assert(m_dominator);
        std::set<RenamingTask::VariableRenaming>::iterator iter = task.m_renamings.begin();
        while (iter != task.m_renamings.end())
        {
          if (!const_cast<RenamingTask::VariableRenaming&>(*iter).setDominator(*m_dominator))
          {
            std::set<RenamingTask::VariableRenaming>::iterator nextIter(iter);
            ++nextIter;
            const RenamingTask::VariableRenaming* next = (nextIter != task.m_renamings.end()) ? &(*nextIter) : NULL;
            task.m_renamings.erase(iter);
            if (next)
            {
              iter = task.m_renamings.find(*next);
            }
            else
            {
              iter = task.m_renamings.end();
            }
          }
          else
          {
            ++iter;
          }
        }
        task.m_previousLabel = NULL;
      }
      if (task.m_isOnlyPhi)
      {
        return;
      }
    }
  }
  VirtualInstruction::handle(vtTask, continuations, reuse);
}

void
ReturnInstruction::handle(VirtualTask& task, WorkList& continuations, Reusability& reuse) {
  int type = task.getType();
  if ((type == TTRenaming) && m_result.get())
  m_result->handle(task, continuations, reuse);
  VirtualInstruction::handle(task, continuations, reuse);
}

void
EnterBlockInstruction::handle(VirtualTask& virtualTask, WorkList& continuations, Reusability& reuse) {
  VirtualInstruction::handle(virtualTask, continuations, reuse);
  int type = virtualTask.getType();
  if (type == TTPrint)
  {
    assert(dynamic_cast<const PrintTask*>(&virtualTask));
    PrintTask& task = (PrintTask&) virtualTask;
    task.m_ident++;
  }
  else if (type == TTPhiInsertion)
  {
    assert(dynamic_cast<const PhiInsertionTask*>(&virtualTask));
    ((PhiInsertionTask&) virtualTask).m_scope = m_scope;
  }
  else if (type == TTRenaming)
  {
    m_scope.setSizeDefinitions();
  }
}

void
ExitBlockInstruction::handle(VirtualTask& virtualTask, WorkList& continuations, Reusability& reuse) {
  int type = virtualTask.getType();
  if (type == TTPrint) {
    assert(dynamic_cast<const PrintTask*>(&virtualTask));
    PrintTask& task = (PrintTask&) virtualTask;
    assert(task.m_ident > 0);
    task.m_ident--;
  }
  else if (type == TTPhiInsertion)
  {
    assert(dynamic_cast<const PhiInsertionTask*>(&virtualTask));
    PhiInsertionTask& task = (PhiInsertionTask&) virtualTask;
    LocalVariableExpression lastExpr(std::string(), 0, task.m_scope);
    task.m_scope.pop();
    PhiInsertionTask::ModifiedVariables::iterator last = task.m_modified.upper_bound(&lastExpr);
    task.m_modified.erase(last, task.m_modified.end());
  }
  VirtualInstruction::handle(virtualTask, continuations, reuse);
}

void Function::setDominationFrontier()
{
  for (std::vector<VirtualInstruction*>::const_iterator iter = m_instructions.begin(); iter != m_instructions.end(); ++iter)
  {
    if ((*iter)->type() == VirtualInstruction::TLabel)
    {
      assert(dynamic_cast<const LabelInstruction*>(*iter));
      LabelInstruction& label = *((LabelInstruction*) *iter);
      if (label.m_goto != NULL)
      {
        VirtualInstruction* notDominator = label.m_goto;
        while (label.m_dominator != notDominator)
        {
          while (notDominator->type() != VirtualInstruction::TLabel && notDominator->getSPreviousInstruction() && notDominator->getSPreviousInstruction()->type() != VirtualInstruction::TIf)
          {
            notDominator = notDominator->getSPreviousInstruction();
          }
          if (notDominator->getSPreviousInstruction() && (notDominator->getSPreviousInstruction()->type() == VirtualInstruction::TIf))
          {
            assert(dynamic_cast<const GotoInstruction*>(notDominator));
            ((GotoInstruction&) *notDominator).addDominationFrontier(*label.m_goto);
            notDominator = notDominator->getSPreviousInstruction();
          }
          if (notDominator->type() == VirtualInstruction::TLabel)
          {
            assert(dynamic_cast<const LabelInstruction*>(notDominator));
            LabelInstruction& labelInstruction = (LabelInstruction&) *notDominator;
            labelInstruction.addDominationFrontier(*label.m_goto);
            notDominator = labelInstruction.m_dominator;
          }
        }
      }
      if (label.getSPreviousInstruction() != NULL)
      {
        VirtualInstruction* notDominator = label.getSPreviousInstruction();
        assert(dynamic_cast<const GotoInstruction*>(notDominator));
        GotoInstruction* origin = (GotoInstruction*) notDominator;
        while (label.m_dominator != notDominator)
        {
          while (notDominator->type() != VirtualInstruction::TLabel && notDominator->getSPreviousInstruction() && notDominator->getSPreviousInstruction()->type() != VirtualInstruction::TIf)
          {
            notDominator = notDominator->getSPreviousInstruction();
          }
          if (notDominator->getSPreviousInstruction() && notDominator->getSPreviousInstruction()->type() == VirtualInstruction::TIf)
          {
            assert(dynamic_cast<const GotoInstruction*>(notDominator));
            ((GotoInstruction&) *notDominator).addDominationFrontier( *origin);
            notDominator = notDominator->getSPreviousInstruction();
          }
          if (notDominator->type() == VirtualInstruction::TLabel)
          {
            assert(dynamic_cast<const LabelInstruction*>(notDominator));
            LabelInstruction& labelInstruction = (LabelInstruction&) *notDominator;
            labelInstruction.addDominationFrontier(*origin);
            notDominator = labelInstruction.m_dominator ;
          }
        }
      }
    }
  }
}

void Function::insertPhiFunctions(const std::vector<LabelInstruction*>& labels)
{
  for (std::vector<LabelInstruction*>::const_iterator iter = labels.begin(); iter != labels.end(); ++iter)
  {
    LabelInstruction& label = **iter;
    if (label.mark != NULL)
    {
      PhiInsertionTask::LabelResult* result = (PhiInsertionTask::LabelResult*) label.mark;
      VirtualInstruction* previous = &label;
      for (PhiInsertionTask::LabelResult::iterator labelIter = result->map().begin(); labelIter != result->map().end(); ++labelIter)
      {
        if (labelIter->second.first || labelIter->second.second)
        {
          ExpressionInstruction* assign = new ExpressionInstruction();
          // Requires casting for the next instruction (first argument)
          insertNewInstructionAfter((VirtualInstruction*) assign, *previous);
          previous = assign;
          AssignExpression* assignExpression = new AssignExpression();
          assign->setExpression(assignExpression);
          VirtualExpression* lvalue = labelIter->first->clone();
          assignExpression->setLValue(lvalue);
          PhiExpression* phi = new PhiExpression();
          assignExpression->setRValue(phi);
          if (labelIter->second.first)
          {
            phi->addReference(labelIter->first->clone(), *labelIter->second.first);
          }
          if (labelIter->second.second)
          {
            phi->addReference(labelIter->first->clone(), *labelIter->second.second);
          }
        }
      }
      delete result;
      label.mark = NULL;
    }
  }
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

void Program::computeDominationFrontiers()
{
  for (std::set<Function>::iterator functionIter = m_functions.begin(); functionIter != m_functions.end(); ++functionIter)
  {
    const_cast<Function&>(*functionIter).setDominationFrontier();
  }
}

void Program::insertPhiFunctions()
{
  for (std::set<Function>::const_iterator functionIter = m_functions.begin();  functionIter != m_functions.end(); ++functionIter)
  {
    PhiInsertionAgenda phiInsertionAgenda(*functionIter);
    phiInsertionAgenda.execute();
    for (std::vector<LabelInstruction*>::const_iterator labelIter = phiInsertionAgenda.labels().begin(); labelIter != phiInsertionAgenda.labels().end(); ++labelIter)
    {
      LabelPhiFrontierAgenda labelPhiFrontierAgenda;
      labelPhiFrontierAgenda.propagate(**labelIter);
      labelPhiFrontierAgenda.execute();
    }
    const_cast<Function&>(*functionIter).insertPhiFunctions(phiInsertionAgenda.labels());
  }
}

void Program::renameSSA()
{
  for (std::set<Function>::const_iterator functionIter = m_functions.begin(); functionIter != m_functions.end(); ++functionIter)
  {
    RenamingAgenda renamingAgenda(*functionIter);
    renamingAgenda.execute();
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
  program.computeDominationFrontiers();
  program.printWithWorkList(std::cout);
  std::cout << std::endl;
  program.insertPhiFunctions();
  program.printWithWorkList(std::cout);
  std::cout << std::endl;
  program.renameSSA();
  program.printWithWorkList(std::cout);
  std::cout << std::endl;
  return 0;
}
