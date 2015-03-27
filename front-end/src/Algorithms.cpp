#include "Algorithms.h"

VirtualInstruction* DominationTask::findDominatorWith(VirtualInstruction& source) const
{
  int thisHeight = m_height-1;
  VirtualInstruction *thisInstruction = m_previous, *sourceInstruction = &source;
  int sourceHeight = sourceInstruction->getDominationHeight();
  while (thisHeight > sourceHeight)
  {
    assert(thisInstruction);
    if (thisInstruction->countPreviouses() == 1)
    {
      // We simply get the previous instruction is there is only one
      thisInstruction = thisInstruction->getSPreviousInstruction();
    }
    else
    {
      // We pick the dominator if there is not one
      assert(dynamic_cast<const LabelInstruction*>(thisInstruction));
      // We can cast the instruction thanks to the assert
      thisInstruction = ((LabelInstruction*) thisInstruction)->getSDominator();
    }
    thisHeight = thisInstruction->getDominationHeight();
  }
  while (sourceHeight > thisHeight)
  {
    assert(sourceInstruction);
    if (sourceInstruction->countPreviouses() == 1)
    {
      // Same as above
      sourceInstruction = sourceInstruction->getSPreviousInstruction();
    }
    else
    {
      // Same as above
      assert(dynamic_cast<const LabelInstruction*>(sourceInstruction));
      sourceInstruction = ((LabelInstruction*) sourceInstruction)->getSDominator();
    }
    sourceHeight = sourceInstruction->getDominationHeight();
  }
  while (thisInstruction != sourceInstruction)
  {
    assert(thisInstruction && sourceInstruction);
    if (thisInstruction->countPreviouses() == 1)
    {
      // Same as above
      thisInstruction = thisInstruction->getSPreviousInstruction();
    }
    else
    {
      // Same as above
      assert(dynamic_cast<const LabelInstruction*>(thisInstruction));
      thisInstruction = ((LabelInstruction*) thisInstruction)->getSDominator();
    }
    if (sourceInstruction->countPreviouses() == 1)
    {
      // Same as above
      sourceInstruction = sourceInstruction->getSPreviousInstruction();
    }
    else
    {
      // Same as above
      assert(dynamic_cast<const LabelInstruction*>(sourceInstruction));
      sourceInstruction = ((LabelInstruction*) sourceInstruction)->getSDominator();
    }
  }
  return thisInstruction;
}

bool PhiInsertionTask::IsBefore::operator()(VirtualExpression* fst, VirtualExpression *snd) const
{
  if (fst->type() == snd->type())
  {
    if (fst->type() == VirtualExpression::TLocalVariable)
    {
      assert(dynamic_cast<const LocalVariableExpression*>(fst) && dynamic_cast<const LocalVariableExpression*>(snd));
      return ((const LocalVariableExpression&) *fst).getGlobalIndex() < ((const LocalVariableExpression&) *snd).getGlobalIndex();
    }
    if (fst->type() == VirtualExpression::TParameter)
    {
      assert(dynamic_cast<const ParameterExpression*>(fst) && dynamic_cast<const ParameterExpression*>(snd));
      return ((const ParameterExpression&) *fst).getIndex() < ((const ParameterExpression&) *snd).getIndex();
    }
    assert(dynamic_cast<const GlobalVariableExpression*>(fst) && dynamic_cast<const GlobalVariableExpression*>(snd));
    return ((const GlobalVariableExpression&) *fst).getIndex() < ((const GlobalVariableExpression&) *snd).getIndex();
  }
  return fst->type() > snd->type();
}
