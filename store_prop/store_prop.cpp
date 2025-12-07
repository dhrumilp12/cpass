#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/CFG.h"

#include "llvm/IR/Instructions.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <vector>
#include <string>
#include <set>
#include <queue>

#define SRC_IDX 0
#define DST_IDX 1

using namespace llvm;
using namespace std;

typedef map<Value*, Value*> ACPTable;

class BasicBlockInfo {
  public:
    BitVector COPY;
    BitVector KILL;
    BitVector CPIn;
    BitVector CPOut;
    ACPTable  ACP;
    
    BasicBlockInfo(unsigned int max_copies)
    {
        COPY.resize(max_copies);
        KILL.resize(max_copies);
        CPIn.resize(max_copies);
        CPOut.resize(max_copies);

        COPY.reset();
        KILL.reset();
        CPIn.reset();
        CPOut.set();
    }
};

namespace {
struct StorePropagation : public PassInfoMixin<StorePropagation> {
private:
	void localStorePropagation(Function &F);
	void globalStorePropagation(Function &F);
	void propagateStores(BasicBlock &bb, ACPTable &acp);

public:
	static cl::opt<bool> verbose;
	PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

cl::opt<bool> StorePropagation::verbose(
    "store-prop-verbose",
    cl::desc("Enable verbose output for StorePropagation"),
    cl::init(false));

PreservedAnalyses StorePropagation::run(Function &F, FunctionAnalysisManager &AM) {
	if (verbose)
		errs() << "Running StorePropagation on function: " << F.getName() << "\n";

	localStorePropagation(F);
	globalStorePropagation(F);

	return PreservedAnalyses::none();
}


/* LLVM attaches an optnone attribute to functions compiled with -O0. This
 * pass removes the optnone attribute so that we can apply the
 * StorePropagation pass to code compiled with -O0.
 */
struct RemoveOptNone : public PassInfoMixin<RemoveOptNone> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
        for (Function &F : M) {
            if (F.hasFnAttribute(Attribute::OptimizeNone)) {
                F.removeFnAttr(Attribute::OptimizeNone);
                if (F.hasFnAttribute(Attribute::NoInline))
                    F.removeFnAttr(Attribute::NoInline);
            }
        }
        return PreservedAnalyses::none();
    }
};

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "StorePropagation", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "remove-optnone") {
                        MPM.addPass(RemoveOptNone());
                        return true;
                    }
                    return false;
                });

            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "store-prop") {
                        FPM.addPass(StorePropagation());
                        return true;
                    }
                    return false;
                });
        }};
}
}

class DataFlowAnalysis
{
    private:
        /* LLVM does not store the position of instructions in the Instruction
         * class, so we create maps of the store instructions to make them
         * easier to use and reference in the BitVector objects
         */
        std::vector<Value*> copies;
        std::map<Value*, int> copy_idx;
        std::map<int, Value*> idx_copy;
        std::map<BasicBlock*, BasicBlockInfo*> bb_info;
        unsigned int nr_copies;

        void addCopy(Value *v);
        void initCopyIdxs(Function &F);
        void initCOPYAndKILLSets(Function &F);
        void initCPInAndCPOutSets(Function &F);
        void initACPs();

    public:
        DataFlowAnalysis(Function &F);
        ACPTable &getACP(BasicBlock &bb);
        void printCopyIdxs();
        void printDFA();
};


/* NOTE: You should not modify any of the code or headers above this line. To
 * complete the pass, fill in the code for the stub functions defined below.
 */


/*
 * propagateStores performs store propagation over the block bb using the
 * associated values in the ACP table. It also removes load instructions if
 * they are no longer useful.
 *
 * Useful tips:
 *
 * Use C++ features to iterate over the instructions in a block, e.g.:
 *   Instruction *iptr;
 *   for (Instruction &ins : bb) {
 *     iptr = &ins;  
 *     ...
 *   }
 *
 * You can use isa to determine the type of an instruction, e.g.:
 *   if (isa<StoreInst>(iptr)) {
 *     // iptr points to an Instruction that is a StoreInst
 *   }
 *
 * Other useful LLVM routines:
 *   int  Instruction::getOperand(int)
 *   void Instruction::setOperand(int,int)
 *   int  Instruction::getNumOperands()
 *   void Instruction::eraseFromParent()
 */
void StorePropagation::propagateStores(BasicBlock &bb, ACPTable &acp)
{
    // Walk instructions in order and maintain the ACP table.
    for (auto it = bb.begin(); it != bb.end(); )
    {
        Instruction *I = &*it++;
        
        // Copy-propagate operands using the current ACP.
        for (unsigned opIdx = 0; opIdx < I->getNumOperands(); ++opIdx) {
            Value *Op = I->getOperand(opIdx);
            auto acpIt = acp.find(Op);
            if (acpIt != acp.end()) {
                Value *Src = acpIt->second;
                // Only substitute if the types match and we actually change something.
                if (Src != Op && Src->getType() == Op->getType()) {
                    I->setOperand(opIdx, Src);
                }
            }
        }

        //Handle memory-affecting instructions.

        // STORE: update mapping for the destination location.
        if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
            Value *Src = SI->getOperand(SRC_IDX); // value being stored
            Value *Dst = SI->getOperand(DST_IDX); // location (pointer)

            // Memory at Dst is overwritten: previous info about *Dst is invalid.
            acp.erase(Dst);

            // New copy <Dst, Src>
            acp[Dst] = Src;
            continue;
        }

        // LOAD: if we know the value at *Ptr, replace the load with that value.
        if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
            Value *Ptr = LI->getPointerOperand();
            auto itLoc = acp.find(Ptr);
            if (itLoc != acp.end()) {
                Value *Known = itLoc->second;
                if (Known->getType() == LI->getType()) {
                    // Replace uses of the load with the known value and delete the load.
                    LI->replaceAllUsesWith(Known);
                    LI->eraseFromParent();
                }
            }
            continue;
        }

        
    }
}

/*
 * localStorePropagation performs local store propagation (LSP) over the basic
 * blocks in the function F. The algorithm for LSP described on pp. 357-358 in
 * the provided text (Muchnick).
 *
 * Useful tips:
 *
 * Use C++ features to iterate over the blocks in F, e.g.:
 *   for (BasicBlock &bb : F) {
 *     ...
 *   }
 *
 * This routine should call propagateStores
 */
void StorePropagation::localStorePropagation(Function &F)
{
    // Run local store propagation on
    // each basic block with a fresh, empty ACP table.
    for (BasicBlock &bb : F) {
        ACPTable acp;
        propagateStores(bb, acp);
    }

    if (verbose)
    {
        errs() << "post local\n" << F << "\n";
    }
}


/*
 * globalStorePropagation performs global store propagation (GSP) over the basic
 * blocks in the function F. The algorithm for GSP is described on pp. 358-360
 * in the provided text (Muchnick).
 *
 * Useful tips:
 *
 * This routine will use the DataFlowAnalysis to construct COPY, KILL, CPIn,
 * and CPOut sets and an ACP table for each block.
 *
 * Use C++ features to iterate over the blocks in F, e.g.:
 *   for (BasicBlock &bb : F) {
 *     ...
 *   }
 *
 * This routine should also call propagateStores
 */
void StorePropagation::globalStorePropagation(Function &F)
{
    // Build data-flow info.
    DataFlowAnalysis *dfa = new DataFlowAnalysis(F);

    // Run global store propagation on each basic block using its ACP table.
    for (BasicBlock &bb : F) {
        ACPTable &acp = dfa->getACP(bb);
        propagateStores(bb, acp);
    }

    if (verbose)
    {
        errs() << "post global\n" << F << "\n";
    }

    delete dfa;
}



/*
 * addCopy is a helper routine for initCopyIdxs. It updates state information
 * to record the index of a single copy instruction
 */
void DataFlowAnalysis::addCopy(Value* v)
{
    // Assign a unique index to each copy instruction/value (if not already present).
    if (copy_idx.count(v) == 0) {
        int idx = nr_copies;
        copy_idx[v] = idx;
        idx_copy[idx] = v;
        copies.push_back(v);
        nr_copies++;
    }
}


/* 
 * initCopyIdxs creates a table that records unique identifiers for each copy
 * (i.e., argument and store) instructions in LLVM.
 *
 * LLVM does not store the position of instructions in the Instruction class,
 * so this routine is used to record unique identifiers for each copy
 * instruction in the Function F. This step makes it easier to identify copy
 * instructions in the COPY, KILL, CPIn, and CPOut sets.
 * 
 * Useful tips:
 *
 * You should record function arguments and store instructions as copy
 * instructions.
 *
 * Some useful LLVM routines in this routine are:
 *   Function::arg_iterator Function::arg_begin()
 *   Function::arg_iterator Function::arg_end()
 *   bool llvm::isa<T>(Instruction *)
 */
void DataFlowAnalysis::initCopyIdxs(Function &F)
{
    // Clear any previous state.
    copies.clear();
    copy_idx.clear();
    idx_copy.clear();
    nr_copies = 0;

    /* Treat function arguments as copy sources (Muchnick-style “definitions”
       available at the entry). They behave like degenerate copies a <- a. */
    for (Function::arg_iterator AI = F.arg_begin(); AI != F.arg_end(); ++AI) {
        addCopy(&*AI);
    }

    // Treat each store instruction as a copy instruction:  *dst <- src.
    for (BasicBlock &bb : F) {
        for (Instruction &ins : bb) {
            if (isa<StoreInst>(&ins)) {
                addCopy(&ins);
            }
        }
    }

    nr_copies = copies.size();
}



/*
 * initCOPYAndKILLSets initializes the COPY and KILL sets for each basic block
 * in the function F.
 *
 * Useful tips:
 *
 * This routine should visit the blocks in reverse post order. You can use an
 * LLVM iterator to complete this traversal, e.g.:
 *
 *   BasicBlock *bb;
 *   ReversePostOrderTraversal<Function*> RPOT(&F);
 *   for ( auto BB = RPOT.begin(); BB != RPOT.end(); BB++ ) {
 *       bb = *BB;
 *       ...
 *   }
 *
 * This routine should create BasicBlockInfo objects for each basic block and
 * record the BasicBlockInfo for each block in the bb_info map.
 *
 * Some useful LLVM routines in this routine are:
 *   bool llvm::isa<T>(Instruction *)
 *   int  Instruction::getOperand(int)
 */
void DataFlowAnalysis::initCOPYAndKILLSets(Function &F)
{
    // Create per-basic-block info objects.
    for (BasicBlock &bb : F) {
        bb_info[&bb] = new BasicBlockInfo(nr_copies);
    }

    // Precompute the "destination location" for each copy instruction.
    // For arguments, treat the argument itself as its own "location".
    std::vector<Value*> copyDst(nr_copies, nullptr);
    for (unsigned i = 0; i < nr_copies; ++i) {
        Value *V = copies[i];
        if (auto *A = dyn_cast<Argument>(V)) {
            copyDst[i] = A;
        } else if (auto *SI = dyn_cast<StoreInst>(V)) {
            copyDst[i] = SI->getOperand(DST_IDX); // pointer being stored into
        }
    }

    // Mark arguments as COPY in the entry block (they reach the end of the entry).
    BasicBlock *entry = &F.getEntryBlock();
    BasicBlockInfo *entryInfo = bb_info[entry];
    for (Function::arg_iterator AI = F.arg_begin(); AI != F.arg_end(); ++AI) {
        auto it = copy_idx.find(&*AI);
        if (it != copy_idx.end()) {
            entryInfo->COPY.set(it->second);
        }
    }

    // Now compute COPY and KILL sets for each basic block.
    ReversePostOrderTraversal<Function*> RPOT(&F);
    for (auto BB = RPOT.begin(); BB != RPOT.end(); ++BB) {
        BasicBlock *bb = *BB;
        BasicBlockInfo *bbi = bb_info[bb];

        // Track the last store to each location within this block
        std::map<Value*, int> lastCopyForLoc;

        for (Instruction &ins : *bb) {
            if (auto *SI = dyn_cast<StoreInst>(&ins)) {
                Value *loc = SI->getOperand(DST_IDX);
                int thisIdx = copy_idx[&ins];

                // This store kills all *other* copies to the same location.
                for (unsigned ci = 0; ci < nr_copies; ++ci) {
                    if (copyDst[ci] == loc && (int)ci != thisIdx) {
                        bbi->KILL.set(ci);
                    }
                }

                // Remember this as the most recent store to 'loc' in this block.
                lastCopyForLoc[loc] = thisIdx;
            }
            else if (auto *CB = dyn_cast<CallBase>(&ins)) {
                // Be conservative: a call may clobber memory.
                // Kill all copies.
                for (unsigned ci = 0; ci < nr_copies; ++ci) {
                    bbi->KILL.set(ci);
                }
                lastCopyForLoc.clear();
            }
        }

        // Any "last store" per location is a COPY that reaches the end of bb.
        for (auto &kv : lastCopyForLoc) {
            int idx = kv.second;
            bbi->COPY.set(idx);
        }
    }
}


/*
 * initCPInAndCPOutSets initializes the CPIn and CPOut sets for each basic
 * block in the function F.
 *
 * Useful tips:
 * 
 * Similar to initCOPYAndKillSets, you will need to traverse the blocks in
 * reverse post order.
 *
 * You can iterate the predecessors and successors of a block bb using
 * LLVM-defined iterators "predecessors" and "successors", e.g.:
 *
 *   for ( BasicBlock* pred : predecessors( bb ) ) {
 *       // pred points to a predecessor of bb
 *       ...
 *   }
 *
 * You will need to define a special case for the entry block (and some way to
 * identify the entry block).
 *
 * Use set operations on the appropriate BitVector to create CPIn and CPOut.
 */
void DataFlowAnalysis::initCPInAndCPOutSets(Function &F)
{
    BasicBlock *entry = &F.getEntryBlock();

    bool changed = true;

    // Classic forward data-flow iteration in reverse postorder.
    while (changed) {
        changed = false;

        ReversePostOrderTraversal<Function*> RPOT(&F);
        for (auto BB = RPOT.begin(); BB != RPOT.end(); ++BB) {
            BasicBlock *bb = *BB;
            BasicBlockInfo *bbi = bb_info[bb];

            BitVector oldIn  = bbi->CPIn;
            BitVector oldOut = bbi->CPOut;

            //  Compute CPIn(bb) 
            if (bb == entry) {
                // Entry has no predecessors: CPIn(entry) = empty set.
                bbi->CPIn.reset();
            } else {
                bool firstPred = true;
                BitVector inBV(nr_copies);

                // CPIn(bb) = intersection of CPOut(pred) over all preds.
                for (BasicBlock *pred : predecessors(bb)) {
                    BasicBlockInfo *pinfo = bb_info[pred];
                    if (firstPred) {
                        inBV = pinfo->CPOut;
                        firstPred = false;
                    } else {
                        inBV &= pinfo->CPOut;
                    }
                }

                if (firstPred) {
                    // No predecessors (unreachable): treat as empty.
                    bbi->CPIn.reset();
                } else {
                    bbi->CPIn = inBV;
                }
            }

            // Compute CPOut(bb) = COPY(bb) ∪ (CPIn(bb) − KILL(bb)) 
            BitVector notKill = bbi->KILL;
            notKill.flip();                    // ~KILL

            BitVector outBV = bbi->CPIn;       // CPIn
            outBV &= notKill;                  // CPIn - KILL
            outBV |= bbi->COPY;                // ∪ COPY

            bbi->CPOut = outBV;

            if (oldIn != bbi->CPIn || oldOut != bbi->CPOut) {
                changed = true;
            }
        }
    }
}

/*
 * initACPs creates an ACP table for each basic block, which will be used to
 * conduct global copy propagation.
 *
 * Useful tips:
 *
 * You will need to use CPIn to determine if a copy should be in the ACP for
 * this block.
 */
void DataFlowAnalysis::initACPs()
{
    // Use CPIn for each block to seed its ACP table, as in Muchnick’s
    // global copy propagation (Figure 12.24 + p.360).
    for (auto &pair : bb_info) {
        BasicBlockInfo *bbi = pair.second;
        ACPTable &acp = bbi->ACP;

        for (unsigned i = 0; i < nr_copies; ++i) {
            if (!bbi->CPIn[i]) continue;

            Value *V = idx_copy[i];

            if (auto *A = dyn_cast<Argument>(V)) {
                // Degenerate copy: a <- a
                acp[A] = A;
            } else if (auto *SI = dyn_cast<StoreInst>(V)) {
                Value *Src = SI->getOperand(SRC_IDX);
                Value *Dst = SI->getOperand(DST_IDX);
                acp[Dst] = Src;
            }
        }
    }
}

ACPTable &DataFlowAnalysis::getACP(BasicBlock &bb)
{
    // bb_info was filled in initCOPYAndKILLSets(F) and initACPs().
    BasicBlockInfo *bbi = bb_info[&bb];
    return bbi->ACP;
}

void DataFlowAnalysis::printCopyIdxs()
{
    errs() << "copy_idx:" << "\n";
    for ( auto it = copy_idx.begin(); it != copy_idx.end(); ++it )
    {
        errs() << "  " << format("%-3d", it->second)
               << " --> " << *( it->first ) << "\n";
    }
    errs() << "\n";
}

void DataFlowAnalysis::printDFA()
{
    unsigned int i;

    // used for formatting
    std::string str;
    llvm::raw_string_ostream rso( str );

    for ( auto it = bb_info.begin(); it != bb_info.end(); ++it )
    {
        BasicBlockInfo *bbi = bb_info[it->first];

        errs() << "BB ";
        it->first->printAsOperand(errs(), false);
        errs() << "\n";

        errs() << "  CPIn  ";
        for ( i = 0; i < bbi->CPIn.size(); i++ )
        {
            errs() << bbi->CPIn[i] << ' ';
        }
        errs() << "\n";

        errs() << "  CPOut ";
        for ( i = 0; i < bbi->CPOut.size(); i++ )
        {
            errs() << bbi->CPOut[i] << ' ';
        }
        errs() << "\n";

        errs() << "  COPY  ";
        for ( i = 0; i < bbi->COPY.size(); i++ )
        {
            errs() << bbi->COPY[i] << ' ';
        }
        errs() << "\n";

        errs() << "  KILL  ";
        for ( i = 0; i < bbi->KILL.size(); i++ )
        {
            errs() << bbi->KILL[i] << ' ';
        }
        errs() << "\n";

        errs() << "  ACP:" << "\n";
        for ( auto it = bbi->ACP.begin(); it != bbi->ACP.end(); ++it )
        {
            rso << *( it->first );
            errs() << "  " << format("%-30s", rso.str().c_str()) << "==  "
                   << *( it->second ) << "\n";
            str.clear();
        }
        errs() << "\n" << "\n";
    }
}

/*
 * DataFlowAnalysis constructs the data flow analysis for the function F.
 *
 * You will not need to modify this routine.
 */
DataFlowAnalysis::DataFlowAnalysis( Function &F )
{
    initCopyIdxs(F);
    initCOPYAndKILLSets(F);
    initCPInAndCPOutSets(F);
    initACPs();

    if (StorePropagation::verbose) {
        errs() << "post DFA" << "\n";
        printCopyIdxs();
        printDFA();
    }
}