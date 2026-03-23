#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

namespace {

struct TrackedStore {
  llvm::StoreInst *SI;
  llvm::Value *StoredValue;
  llvm::MemoryLocation Loc;
};

struct LoadStoreEliminationPass
    : public llvm::PassInfoMixin<LoadStoreEliminationPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM) {
    llvm::AAResults &AA = FAM.getResult<llvm::AAManager>(F);
    bool Changed = false;

    for (auto &BB : F) {
      llvm::SmallVector<TrackedStore, 16> AvailableStores;
      llvm::SmallVector<llvm::Instruction *, 16> ToErase;

      for (auto &I : llvm::make_early_inc_range(BB)) {
        if (auto *LI = llvm::dyn_cast<llvm::LoadInst>(&I)) {
          if (LI->isVolatile() || LI->isAtomic()) {
            AvailableStores.clear();
            continue;
          }

          llvm::MemoryLocation LoadLoc = llvm::MemoryLocation::get(LI);

          for (auto It = AvailableStores.rbegin(); It != AvailableStores.rend();
               ++It) {
            llvm::AliasResult AR = AA.alias(It->Loc, LoadLoc);

            if (AR == llvm::AliasResult::MustAlias) {
              if (It->StoredValue->getType() == LI->getType()) {
                LI->replaceAllUsesWith(It->StoredValue);
                ToErase.push_back(LI);
                Changed = true;
              }
              break;
            }

            if (AR != llvm::AliasResult::NoAlias) {
              break;
            }
          }

          for (auto &S : AvailableStores) {
            llvm::AliasResult AR = AA.alias(S.Loc, LoadLoc);
            if (AR != llvm::AliasResult::NoAlias)
              S.SI = nullptr;
          }

          continue;
        }

        if (auto *SI = llvm::dyn_cast<llvm::StoreInst>(&I)) {
          if (SI->isVolatile() || SI->isAtomic()) {
            AvailableStores.clear();
            continue;
          }

          llvm::MemoryLocation StoreLoc = llvm::MemoryLocation::get(SI);
          llvm::Value *StoredVal = SI->getValueOperand();

          for (auto It = AvailableStores.begin();
               It != AvailableStores.end();) {
            llvm::AliasResult AR = AA.alias(It->Loc, StoreLoc);

            if (AR == llvm::AliasResult::MustAlias) {
              if (It->SI) {
                ToErase.push_back(It->SI);
                Changed = true;
              }
              It = AvailableStores.erase(It);
              continue;
            }

            if (AR != llvm::AliasResult::NoAlias) {
              It->SI = nullptr;
            }

            ++It;
          }

          AvailableStores.push_back({SI, StoredVal, StoreLoc});
          continue;
        }

        if (I.mayReadOrWriteMemory() || I.mayHaveSideEffects()) {
          AvailableStores.clear();
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
