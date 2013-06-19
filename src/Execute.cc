/**
 * \file Execute.cc
 * \brief Implementation of the Execute class
 * \author Julien Henry
 */
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/LinkAllVMCore.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_os_ostream.h"

//#include "llvm/Target/TargetData.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Transforms/Scalar.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/LoopInfo.h"

#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIopt_incr.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "AIGuided.h"
#include "ModulePassWrapper.h"
#include "Node.h"
#include "Execute.h"
#include "Live.h"
#include "SMTpass.h"
#include "SMTlib.h"
#include "Compare.h"
#include "CompareDomain.h"
#include "CompareNarrowing.h"
#include "Analyzer.h"
#include "GenerateSMT.h"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Module.h"

using namespace llvm;

static cl::opt<std::string>
DefaultDataLayout("default-data-layout", 
          cl::desc("data layout string to use if not specified by module"),
          cl::value_desc("layout-string"), cl::init(""));

bool is_Cfile(std::string InputFilename) {
	if (InputFilename.compare(InputFilename.size()-2,2,".c") == 0)
		return true;
	return false;
}

std::auto_ptr<Module> getModuleFromBCFile(std::string InputFilename) {
	Module * n = NULL;
	std::auto_ptr<Module> M_null(n);

	LLVMContext & Context = getGlobalContext();
	
	std::string ErrorMessage;
	std::auto_ptr<Module> M;
	{
	        OwningPtr<MemoryBuffer> BufferPtr;

		if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFilename, BufferPtr))
			ErrorMessage = ec.message();
		else
			M.reset(ParseBitcodeFile(BufferPtr.get(), Context, &ErrorMessage));
	}

	if (M.get() == 0) {
		errs() << "Error : ";
		if (ErrorMessage.size())
			errs() << ErrorMessage << "\n";
		else
			errs() << "failed to read the bitcode file.\n";
		return M_null;
	}

	return M;
}

void execute::exec(std::string InputFilename, std::string OutputFilename) {

	raw_fd_ostream *FDOut = NULL;

	if (OutputFilename != "") {

		std::string error;
		FDOut = new raw_fd_ostream(OutputFilename.c_str(), error);
		if (!error.empty()) {
			errs() << error << '\n';
			delete FDOut;
			return;
		}
		Out = new formatted_raw_ostream(*FDOut, formatted_raw_ostream::DELETE_STREAM);

		// Make sure that the Output file gets unlinked from the disk if we get a
		// SIGINT
		sys::RemoveFileOnSignal(sys::Path(OutputFilename));
	} else {
		Out = &llvm::outs();
		Out->SetUnbuffered();
	}
	
	llvm::Module * M;
	std::auto_ptr<Module> M_ptr;

	llvm::OwningPtr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());

	if (!is_Cfile(InputFilename)) {
		M_ptr = getModuleFromBCFile(InputFilename);
		M = M_ptr.get();
	} else {
		// Arguments to pass to the clang frontend
		std::vector<const char *> args;
		args.push_back(InputFilename.c_str());
		args.push_back("-g");
		 
		// The compiler invocation needs a DiagnosticsEngine so it can report problems
		clang::TextDiagnosticPrinter *DiagClient = new clang::TextDiagnosticPrinter(llvm::errs(), clang::DiagnosticOptions());
		//clang::DiagnosticOptions *DiagClient = new clang::DiagnosticOptions();
		llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());
		clang::DiagnosticsEngine Diags(DiagID, DiagClient);
		
		// Create the compiler invocation
		llvm::OwningPtr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
		clang::CompilerInvocation::CreateFromArgs(*CI, &args[0], &args[0] + args.size(), Diags);
		
		clang::CompilerInstance Clang;
		Clang.setInvocation(CI.take());
		
		Clang.createDiagnostics(args.size(), &args[0]);
		
		if (!Clang.ExecuteAction(*Act))
		    return;
		
		llvm::Module *module = Act->takeModule();
		M = module;
	}

	if (M == NULL) return;

	PassRegistry &Registry = *PassRegistry::getPassRegistry();
	initializeAnalysis(Registry);

	// Build up all of the passes that we want to do to the module.
	PassManager Passes;

	FunctionPass *LoopInfoPass = new LoopInfo();

	Passes.add(createVerifierPass());
	Passes.add(createGCLoweringPass());
	
	// this pass converts SwitchInst instructions into a sequence of
	// binary branch instructions, much easier to deal with
	Passes.add(createLowerSwitchPass());	
	Passes.add(createLowerInvokePass());
	Passes.add(createPromoteMemoryToRegisterPass());
	//Passes.add(createLoopSimplifyPass());	
	Passes.add(LoopInfoPass);

	// in case we want to run an Alias analysis pass : 
	//
	//Passes.add(createGlobalsModRefPass());
	//Passes.add(createBasicAliasAnalysisPass());
	//Passes.add(createScalarEvolutionAliasAnalysisPass());
	//Passes.add(createTypeBasedAliasAnalysisPass());


	if (onlyOutputsRho()) {
		Passes.add(new GenerateSMT());
	} else if (compareTechniques()) {
		Passes.add(new Compare(getComparedTechniques()));
	} else if (compareNarrowing()) {
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				Passes.add(new CompareNarrowing<LOOKAHEAD_WIDENING>());
				break;
			case GUIDED:
				Passes.add(new CompareNarrowing<GUIDED>());
				break;
			case PATH_FOCUSING:
				Passes.add(new CompareNarrowing<PATH_FOCUSING>());
				break;
			case PATH_FOCUSING_INCR:
				Passes.add(new CompareNarrowing<PATH_FOCUSING_INCR>());
				break;
			case LW_WITH_PF:
				Passes.add(new CompareNarrowing<LW_WITH_PF>());
				break;
			case COMBINED_INCR:
				Passes.add(new CompareNarrowing<COMBINED_INCR>());
				break;
			case SIMPLE:
				Passes.add(new CompareNarrowing<SIMPLE>());
				break;
			case LW_WITH_PF_DISJ:
				Passes.add(new CompareNarrowing<LW_WITH_PF_DISJ>());
				break;
		}
	} else if (compareDomain()) {
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				Passes.add(new CompareDomain<LOOKAHEAD_WIDENING>());
				break;
			case GUIDED:
				Passes.add(new CompareNarrowing<GUIDED>());
				break;
			case PATH_FOCUSING:
				Passes.add(new CompareDomain<PATH_FOCUSING>());
				break;
			case PATH_FOCUSING_INCR:
				Passes.add(new CompareDomain<PATH_FOCUSING_INCR>());
				break;
			case LW_WITH_PF:
				Passes.add(new CompareDomain<LW_WITH_PF>());
				break;
			case COMBINED_INCR:
				Passes.add(new CompareDomain<COMBINED_INCR>());
				break;
			case SIMPLE:
				Passes.add(new CompareDomain<SIMPLE>());
				break;
			case LW_WITH_PF_DISJ:
				Passes.add(new CompareDomain<LW_WITH_PF_DISJ>());
				break;
		}
	} else { 
		ModulePass *AIPass;
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AIPass = new ModulePassWrapper<AIGopan, 0>();
				break;
			case GUIDED:
				AIPass = new ModulePassWrapper<AIGuided, 0>();
				break;
			case PATH_FOCUSING:
				AIPass = new ModulePassWrapper<AIpf, 0>();
				break;
			case PATH_FOCUSING_INCR:
				AIPass = new ModulePassWrapper<AIpf_incr, 0>();
				break;
			case LW_WITH_PF:
				AIPass = new ModulePassWrapper<AIopt, 0>();
				break;
			case COMBINED_INCR:
				AIPass = new ModulePassWrapper<AIopt_incr, 0>();
				break;
			case SIMPLE:
				AIPass = new ModulePassWrapper<AIClassic, 0>();
				break;
			case LW_WITH_PF_DISJ:
				AIPass = new ModulePassWrapper<AIdis, 0>();
				break;
		}
		Passes.add(AIPass);
	}
	Passes.run(*M);

	if (onlyOutputsRho()) {
		return;
	}
	// we properly delete all the created Nodes
	std::map<BasicBlock*,Node*>::iterator it = Nodes.begin(), et = Nodes.end();
	for (;it != et; it++) {
		delete (*it).second;
	}

	Pr::releaseMemory();
	SMTpass::releaseMemory();
	ReleaseTimingData();
	Expr::clear_exprs();

	if (OutputFilename != "") {
		delete Out;
	}
}

