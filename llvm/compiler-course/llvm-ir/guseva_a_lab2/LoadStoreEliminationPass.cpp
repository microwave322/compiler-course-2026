#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

namespace {

struct LoadStoreEliminationPass
    : public llvm::PassInfoMixin<LoadStoreEliminationPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &) {
    bool Changed = false;

    for (auto &BB : F) {
      llvm::DenseMap<llvm::Value *, llvm::Value *> LastStoredValue;
      llvm::DenseMap<llvm::Value *, llvm::StoreInst *> LastStoreInst;
      llvm::SmallVector<llvm::Instruction *, 16> ToErase;

      for (auto &I : llvm::make_early_inc_range(BB)) {
        if (auto *LI = llvm::dyn_cast<llvm::LoadInst>(&I)) {
          if (LI->isVolatile() || LI->isAtomic()) {
            LastStoredValue.clear();
            LastStoreInst.clear();
            continue;
          }

          llvm::Value *Ptr = LI->getPointerOperand();

          auto It = LastStoredValue.find(Ptr);
          if (It != LastStoredValue.end() &&
              It->second->getType() == LI->getType()) {
            LI->replaceAllUsesWith(It->second);
            ToErase.push_back(LI);
            Changed = true;
          }

          if (LastStoreInst.count(Ptr))
            LastStoreInst[Ptr] = nullptr;

          continue;
        }

        if (auto *SI = llvm::dyn_cast<llvm::StoreInst>(&I)) {
          if (SI->isVolatile() || SI->isAtomic()) {
            LastStoredValue.clear();
            LastStoreInst.clear();
            continue;
          }

          llvm::Value *Ptr = SI->getPointerOperand();

          auto It = LastStoreInst.find(Ptr);
          if (It != LastStoreInst.end() && It->second) {
            ToErase.push_back(It->second);
            Changed = true;
          }

          LastStoredValue[Ptr] = SI->getValueOperand();
          LastStoreInst[Ptr] = SI;
          continue;
        }

        if (I.mayReadOrWriteMemory() || I.mayHaveSideEffects()) {
          LastStoredValue.clear();
          LastStoreInst.clear();
        }
      }

      for (llvm::Instruction *Inst : ToErase) {
        if (Inst && Inst->getParent())
          Inst->eraseFromParent();
      }
    }

    return Changed ? llvm::PreservedAnalyses::none()
                   : llvm::PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "LoadStoreEliminationPass", "0.1",
          [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) -> bool {
                  if (Name == "load-store-elim") {
                    FPM.addPass(LoadStoreEliminationPass{});
                    return true;
                  }
                  return false;
                });
          }};
}
