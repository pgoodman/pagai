#include "Abstract.h"

int Abstract::compare(Abstract * d) {
	bool f = false;
	bool g = false;
return 0;
//	LSMT->push_context();
//	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,A);
//	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,B);
//
//	LSMT->push_context();
//	// f = A and not B
//	std::vector<SMT_expr> cunj;
//	cunj.push_back(A_smt);
//	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
//	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
//	if (LSMT->SMTsolve_simple(test)) {
//		f = true;
//	}
//	LSMT->pop_context();
//
//	// g = B and not A
//	cunj.clear();
//	cunj.push_back(B_smt);
//	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
//	test = LSMT->man->SMT_mk_and(cunj);
//	if (LSMT->SMTsolve_simple(test)) {
//		g = true;
//	}
//
//	LSMT->pop_context();
//	
//	if (!f && !g) {
//		return 0;
//	} else if (!f && g) {
//		return 1;
//	} else if (f && !g) {
//		return -1;
//	} else {
//		return -2;
//	}
}

bool Abstract::is_leq(Abstract * d) {
	return (compare(d) == 0);
}

bool Abstract::is_eq(Abstract * d) {
	return (compare(d) == 1);
}
