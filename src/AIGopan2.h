#ifndef _AIGOPAN2_H
#define _AIGOPAN2_H

#include <queue>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Analysis/LoopInfo.h"

#include "ap_global1.h"
#include "pk.h"

#include "apron.h"
#include "Node.h"
#include "Live.h"
#include "SMT.h"
#include "PathTree.h"

using namespace llvm;

class AIGopan2 : public ModulePass, public InstVisitor<AIGopan2> {

	private:
		/// LV - result of the Live pass
		Live * LV;
		/// LI - result of the LoopInfo pass
		LoopInfo * LI;
		/// LSMT - result of the SMT pass
		SMT * LSMT;
		/// A - list of active Nodes, that still have to be computed
		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A;
		/// is_computed - remember the Nodes that don't need to be recomputed.
		/// This is used to remove duplicates in the A list.
		std::map<Node*,bool> is_computed;
	
		/// man - apron manager we use along the pass
		ap_manager_t* man;

		/// paths - remembers all the paths that have already been
		/// visited
		PathTree* pathtree;

		std::map<BasicBlock*,std::list<BasicBlock*> > lastpath;

	public:
		static char ID;	
	
	public:

		AIGopan2 () : ModulePass(ID), LV(NULL), LI(NULL), LSMT(NULL) {
				man = pk_manager_alloc(true);
				pathtree = new PathTree();
				init_apron();
				//linconstraints = ap_lincons1_array_make(ap_environment_alloc_empty(),0);
			}

		~AIGopan2 () {
				ap_manager_free(man);
				delete pathtree;
			}

		const char *getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		/// initFunction - initialize the function by creating the Node
		/// objects, and computing the strongly connected components.
		void initFunction(Function * F);
		
		/// printBasicBlock - print a basicBlock on standard output
		void printBasicBlock(BasicBlock * b);

		/// printPath - print a path on standard output
		void printPath(std::list<BasicBlock*> path);
	
		/// computeEnv - compute the new environment of Node n, based on 
		/// its intVar and RealVar maps
		void computeEnv(Node * n);
		
		/// focuspath - path we focus on
		std::vector<BasicBlock*> focuspath;
		/// index in focuspath of the focuspath's basicblock we are working on
		unsigned focusblock;
		

		std::list<std::vector<ap_tcons1_array_t*>*> constraints;
		ap_lincons1_array_t linconstraints;
		phivar PHIvars;
		
		/// computeTransform - computes in Xtemp the polyhedra resulting from
		/// the transformation  of n->X through the path
		void computeTransform (	Node * n, 
								std::list<BasicBlock*> path, 
								AbstractGopan &Xtemp);

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);

		/// computeCondition - creates the constraint arrays resulting from a
		/// comparison instruction.
		bool computeCondition(CmpInst * inst, 
				bool result,
				std::vector<ap_tcons1_array_t *> * cons);

		bool computeConstantCondition(ConstantInt * inst, 
				bool result,
				std::vector<ap_tcons1_array_t*> * cons);

		bool computePHINodeCondition(PHINode * inst, 
				bool result,
				std::vector<ap_tcons1_array_t*> * cons);

		// create_constraints - this function is called by computeCondition
		// it creates the constraint from its arguments and insert it into t_cons
		void create_constraints(
			ap_constyp_t constyp,
			ap_texpr1_t * expr,
			ap_texpr1_t * nexpr,
			std::vector<ap_tcons1_array_t*> * t_cons);

		void insert_env_vars_into_node_vars(ap_environment_t * env, Node * n, Value * V);

		void visitInstAndAddVarIfNecessary(Instruction &I);
		// Visit methods
		void visitReturnInst (ReturnInst &I);
		void visitBranchInst (BranchInst &I);
		void visitSwitchInst (SwitchInst &I);
		void visitIndirectBrInst (IndirectBrInst &I);
		void visitInvokeInst (InvokeInst &I);
		void visitUnwindInst (UnwindInst &I);
		void visitUnreachableInst (UnreachableInst &I);
		void visitICmpInst (ICmpInst &I);
		void visitFCmpInst (FCmpInst &I);
		void visitAllocaInst (AllocaInst &I);
		void visitLoadInst (LoadInst &I);
		void visitStoreInst (StoreInst &I);
		void visitGetElementPtrInst (GetElementPtrInst &I);
		void visitPHINode (PHINode &I);
		void visitTruncInst (TruncInst &I);
		void visitZExtInst (ZExtInst &I);
		void visitSExtInst (SExtInst &I);
		void visitFPTruncInst (FPTruncInst &I);
		void visitFPExtInst (FPExtInst &I);
		void visitFPToUIInst (FPToUIInst &I);
		void visitFPToSIInst (FPToSIInst &I);
		void visitUIToFPInst (UIToFPInst &I);
		void visitSIToFPInst (SIToFPInst &I);
		void visitPtrToIntInst (PtrToIntInst &I);
		void visitIntToPtrInst (IntToPtrInst &I);
		void visitBitCastInst (BitCastInst &I);
		void visitSelectInst (SelectInst &I);
		void visitCallInst(CallInst &I);
		void visitVAArgInst (VAArgInst &I);
		void visitExtractElementInst (ExtractElementInst &I);
		void visitInsertElementInst (InsertElementInst &I);
		void visitShuffleVectorInst (ShuffleVectorInst &I);
		void visitExtractValueInst (ExtractValueInst &I);
		void visitInsertValueInst (InsertValueInst &I);
		void visitTerminatorInst (TerminatorInst &I);
		void visitBinaryOperator (BinaryOperator &I);
		void visitCmpInst (CmpInst &I);
		void visitCastInst (CastInst &I);


		void visitInstruction(Instruction &I) {
			ferrs() << I.getOpcodeName();
			assert(0 && "Instruction not interpretable yet!");
		}

};

#endif