#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "memtraceTools.h"

using namespace llvm;

#if LLVM_VERSION_MAJOR >= 8

namespace {

struct MemoizeRegion {
  CallInst *StartCall;
  SmallPtrSet<CallInst *, 1> Calls;
  SmallPtrSet<Function *, 1> FuncsToAnnotate;
};

struct MemoizationPass : public ModulePass {
  static char ID;

  MemoizationPass() : ModulePass(ID) {}

  llvm::SmallVector<MemoizeRegion, 2> Regions;

  void memoizationFuncs(Function &Func) {
    auto capture = false;
    for (auto &I : instructions(Func)) {
      if (auto CI = dyn_cast<CallInst>(&I)) {
        auto CF = CI->getCalledFunction();
        if (CF->getName().contains("memoize_start")) {
          capture = true;
          MemoizeRegion R;
          R.StartCall = CI;
          Regions.push_back(std::move(R));
        } else if (CF->getName().contains("memoize_end")) {
          capture = false;
        } else if (capture) {
          Regions.back().Calls.insert(CI);
        }
      }
    }
  }

  void writeFunctionData(Module &M, MemoizeRegion &R) {
    for (auto C : R.Calls) {
      auto Func = C->getCalledFunction();
      if (Func->getName() == "__kmpc_fork_call") {
        auto omp_func_name =
            C->getArgOperand(2)->stripPointerCasts()->getName();
        auto omp_func = M.getFunction(omp_func_name);
        R.FuncsToAnnotate.insert(omp_func);
      } else {
        if(Func->isMaterializable()){
        }
      }
    }

    auto loads = 0;
    auto stores = 0;
    for (auto F : R.FuncsToAnnotate) {
      for (auto const &I : instructions(F)) {
        if (auto SI = dyn_cast<StoreInst>(&I)) {
          ++stores;
        } else if(auto LI = dyn_cast<LoadInst>(&I)){
          ++loads;
        }
      }
    }

    auto ir_loads = ConstantInt::get(M.getContext(), APInt(32, loads, false));
    auto ir_stores = ConstantInt::get(M.getContext(), APInt(32, stores, false));

    R.StartCall->setArgOperand(0, ir_loads);
    R.StartCall->setArgOperand(1, ir_stores);
  }

  bool runOnModule(Module &M) override {
    for (auto &F : M.functions()) {
      memoizationFuncs(F);
    }

    for (auto R : Regions) {
      writeFunctionData(M, R);
    }

    return true;
  }
};

} // namespace

char MemoizationPass::ID = 0;
static RegisterPass<MemoizationPass>
    X("sst-fingerprint", "SSTMAC memoize fingerprinting pass", false, false);

#endif
