#include <map>
#include <sstream>

#include "cuddObj.hh"

#include "PathTree.h"
#include "Analyzer.h"

PathTree::PathTree() {
	mgr = new Cudd(0,0);
	//mgr->makeVerbose();
	Bdd = new BDD(mgr->bddZero());
	Bdd_prime = new BDD(mgr->bddZero());
	BddIndex=0;
	background = mgr->bddZero().getNode();
	zero = mgr->bddZero().getNode();
}

PathTree::~PathTree() {
	delete Bdd;
	delete Bdd_prime;
	delete mgr;	
}

BDD PathTree::getBDDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map) {
	int n;
	if (!map.count(b)) {
		n = BddIndex;
		levels[n] = b;
		BddIndex++;
		map[b] = n;
	} else {
		n = map[b];	
	}
	return mgr->bddVar(n);
}

const std::string PathTree::getNodeName(BasicBlock* b, bool src, SMT * smt) const {
	if (smt != NULL) return smt->getNodeName(b,src);
	std::ostringstream name;
	if (src)
		name << "bs_";
	else
		name << "b_";
	name << b;
	return name.str();
}

const std::string PathTree::getStringFromLevel(int const i, SMT * smt) {
	BasicBlock * bb = levels[i]; 
	if (BddVarStart.count(bb) && BddVarStart[bb]==i)
		return getNodeName(bb,true,smt);
	else
		return getNodeName(bb,false,smt);
}

void PathTree::DumpDotBDD(BDD graph, std::string filename) {
	std::ostringstream name;
	name << filename << ".dot";

	int n = BddVar.size() + BddVarStart.size();

	char * inames[n];
	for (std::map<BasicBlock*,int>::iterator it = BddVar.begin(), et = BddVar.end(); it != et; it++) {
		inames[it->second] = strdup(getNodeName(it->first,false).c_str());
	}
	for (std::map<BasicBlock*,int>::iterator it = BddVarStart.begin(), et = BddVarStart.end(); it != et; it++) {
		inames[it->second] = strdup(getNodeName(it->first,true).c_str());
	}

    char const* onames[] = {"B"};
    DdNode *Dds[] = {graph.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
	Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            (char**) inames, (char**) onames, fp);
	fclose(fp);
}

SMT_expr PathTree::generateSMTformula(SMT * smt) {
	std::map<DdNode*,SMT_expr> Bdd_expr;
	DdNode *node;
	DdNode *N, *Nv, *Nnv;
	DdGen *gen;
	DdNode * true_node;
	std::vector<SMT_expr> formula;
	for(gen = Cudd_FirstNode (mgr->getManager(), Bdd->getNode(), &node);
	!Cudd_IsGenEmpty(gen);
	(void) Cudd_NextNode(gen, &node)) {
	
		// remove the extra 1 from the adress if exists
	    N = Cudd_Regular(node);
	
	    if (Cudd_IsConstant(N)) {
			// Terminal node
			if (node != background && node != zero) {
				// this is the 1 node
				Bdd_expr[node] = smt->man->SMT_mk_true();	
				// we remember the adress of this node for future simplification of
				// the formula
				true_node = N;
			} else {
				Bdd_expr[node] = smt->man->SMT_mk_false();	
			}
		} else {
			std::string bb = getStringFromLevel(N->index,smt);
			SMT_var bbvar = smt->man->SMT_mk_bool_var(bb);
			SMT_expr bbexpr = smt->man->SMT_mk_expr_from_bool_var(bbvar);
	
			Nv  = Cudd_T(N);
			Nnv = Cudd_E(N);
			bool Nnv_comp = false;
			if (Cudd_IsComplement(Nnv)) {
				Nnv = Cudd_Not(Nnv);
				Nnv_comp = true;
			}
			SMT_expr Nv_expr = Bdd_expr[Nv];
			SMT_expr Nnv_expr = Bdd_expr[Nnv];
			if (Nnv_comp)
				Nnv_expr = smt->man->SMT_mk_not(Nnv_expr);
		
			if (Nnv == true_node && Nnv_comp) {
				// we don't need to create an ite formula, because the else condition
				// is always false
				std::vector<SMT_expr> cunj;
				cunj.push_back(bbexpr);
				cunj.push_back(Nv_expr);
				Bdd_expr[node] = smt->man->SMT_mk_and(cunj);
			} else {
				Bdd_expr[node] = smt->man->SMT_mk_ite(bbexpr,Nv_expr,Nnv_expr);
			}

			// if the node is used multiple times, we create a variable for this
			// node
			if (node->ref > 1) {
				std::ostringstream name;
				name << "Bdd_" << node;
				SMT_var var = smt->man->SMT_mk_bool_var(name.str());
				SMT_expr vexpr = smt->man->SMT_mk_expr_from_bool_var(var);
				formula.push_back(smt->man->SMT_mk_eq(vexpr,Bdd_expr[node]));
				Bdd_expr[node] = vexpr;	
			}
		}
	}
	Cudd_GenFree(gen);
	
	if (Cudd_IsComplement(Bdd->getNode())) 
		// sometimes, Bdd is complemented, especially when Bdd = false
		formula.push_back(smt->man->SMT_mk_not(Bdd_expr[Cudd_Regular(Bdd->getNode())]));
	else
		formula.push_back(Bdd_expr[Cudd_Regular(Bdd->getNode())]);
	return smt->man->SMT_mk_and(formula);
} 

void PathTree::insert(std::list<BasicBlock*> path, bool primed) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());
	
	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar));
		f = f * block;
	}
	if (primed) {
		*Bdd_prime = *Bdd_prime + f;
	} else {
		*Bdd = *Bdd + f;
	}
}

void PathTree::remove(std::list<BasicBlock*> path, bool primed) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar));
		f = f * block;
	}

	if (primed) {
		*Bdd_prime = *Bdd_prime * !f;
	} else {
		*Bdd = *Bdd * !f;
	}
}

void PathTree::clear(bool primed) {
	if (primed) {
		*Bdd_prime = mgr->bddZero();
	} else {
		*Bdd = mgr->bddZero();
	}
}

bool PathTree::exist(std::list<BasicBlock*> path, bool primed) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	
	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar));
		f = f * block;
	}
	bool res;
	if (primed) {
		res = f <= *Bdd_prime;
	} else {
		res = f <= *Bdd;
	}
	return res;
}

void PathTree::mergeBDD() {
	*Bdd = *Bdd + *Bdd_prime;
	*Bdd_prime = mgr->bddZero();
}

bool PathTree::isZero(bool primed) {
	if (primed) {
		return !(*Bdd_prime > mgr->bddZero());
	} else {
		return !(*Bdd > mgr->bddZero());
	}
}
