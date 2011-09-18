#ifndef COMPARE_H
#define COMPARE_H

#include "SMT.h"
#include "Node.h"
#include "Abstract.h"
#include "Analyzer.h"

using namespace llvm;

class CmpResults {

	public:
		int gt;
		int lt;
		int eq;
		int un;

		CmpResults(): gt(0), lt(0), eq(0), un(0) {};
};

class Compare : public ModulePass {

	private:
		SMT * LSMT;

		std::map<
			Techniques,
			std::map<Techniques,CmpResults> 
			> results;

		int compareAbstract(Abstract * A, Abstract * B);

		void compareTechniques(
			Node * n, 
			Techniques t1, 
			Techniques t2);

		void printAllResults();
		void printResults(Techniques t1, Techniques t2);
	public:
		static char ID;

		Compare();

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);

};
#endif
