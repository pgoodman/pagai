#ifndef _AI_H
#define _AI_H

#include <queue>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"

#include "AIpass.h"

using namespace llvm;

/// Abstract Interpretation with Path Focusing algorithm (using SMT-solving)
class AIpf : public ModulePass, public AIPass {

	public:
		static char ID;	

	public:

		AIpf(char &_ID, Apron_Manager_Type _man, bool _NewNarrow) : ModulePass(_ID), AIPass(_man,_NewNarrow) {
			init();
			passID.D = _man;
			passID.N = _NewNarrow;
		}

		AIpf (): ModulePass(ID) {
			init();
			passID.D = getApronManager();
			passID.N = useNewNarrowing();
		}
		
		void init()
			{
				//aman = new AbstractManGopan();
				aman = new AbstractManClassic();
				passID.T = PATH_FOCUSING;
				//Passes[PATH_FOCUSING] = passID;	
			}

		~AIpf () {}

		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;
		std::set<BasicBlock*> getSuccessors(BasicBlock * b) const;

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);
};

#endif
