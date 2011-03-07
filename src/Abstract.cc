#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "Node.h"
#include "Expr.h"

Abstract::Abstract(ap_manager_t* _man, ap_environment_t * env) {
	//ap_environment_t * env = ap_environment_alloc_empty();
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	pilot = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	man = _man;
}


Abstract::Abstract(Abstract* A) {
	man = A->man;
	main = new ap_abstract1_t(ap_abstract1_copy(man,A->main));
	pilot = new ap_abstract1_t(ap_abstract1_copy(man,A->pilot));
}

Abstract::~Abstract() {
	ap_abstract1_clear(man,main);
	ap_abstract1_clear(man,pilot);
	delete main;
	delete pilot;
}

/// set_top - sets the abstract to top on the environment env
void Abstract::set_top(ap_environment_t * env) {
		ap_abstract1_clear(man,main);
		ap_abstract1_clear(man,pilot);
		delete main;
		delete pilot;
		main = new ap_abstract1_t(ap_abstract1_top(man,env));
		pilot = new ap_abstract1_t(ap_abstract1_top(man,env));
}

/// set_bottom - sets the abstract to bottom on the environment env
void Abstract::set_bottom(ap_environment_t * env) {
		ap_abstract1_clear(man,main);
		ap_abstract1_clear(man,pilot);
		delete main;
		delete pilot;
		main = new ap_abstract1_t(ap_abstract1_bottom(man,env));
		pilot = new ap_abstract1_t(ap_abstract1_bottom(man,env));
}

void Abstract::change_environment(ap_environment_t * env) {
	if (!ap_environment_is_eq(env,main->env))
		*main = ap_abstract1_change_environment(man,true,main,env,false);
	if (!ap_environment_is_eq(env,pilot->env))
	*pilot = ap_abstract1_change_environment(man,true,pilot,env,false);
}

bool Abstract::is_leq (Abstract *d) {
	if (ap_abstract1_is_eq(man,main,d->main)) {
		if (ap_abstract1_is_leq(man,pilot,d->pilot)) 
			return true; 
		else 
			return false;
	}
	if (ap_abstract1_is_leq(man,main,d->main))
		return true;
	return false;
}

/// widening - Compute the widening operation according to the Gopan & Reps
/// approach
void Abstract::widening(Node * n) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xpilot_widening;
	ap_abstract1_t Xpilot;

	if (is_leq(n->X)) {
		Xmain_widening = ap_abstract1_copy(man,n->X->main);
		Xpilot_widening = ap_abstract1_copy(man,n->X->pilot);
	} else if (ap_abstract1_is_leq(man,pilot,n->X->pilot)) {
		Xmain_widening = ap_abstract1_copy(man,pilot);
		Xpilot_widening = ap_abstract1_copy(man,pilot);
	} else {
		// we apply the widening operator only on the pilot value,
		// and we only join the main values.
		Xmain_widening = ap_abstract1_join(man,false,n->X->main,main);
		// before widening, n->X->pilot has to be included in pilot
		Xpilot = ap_abstract1_join(man,false,n->X->pilot,pilot);
		Xpilot_widening = ap_abstract1_widening(man,n->X->pilot,&Xpilot);
		ap_abstract1_clear(man,&Xpilot);
	}
	ap_abstract1_clear(man,main);
	ap_abstract1_clear(man,pilot);
	*main = Xmain_widening;
	*pilot = Xpilot_widening;
}

void Abstract::meet_tcons_array(ap_tcons1_array_t* tcons) {

	ap_environment_t * lcenv = common_environment(
			main->env,
			ap_tcons1_array_envref(tcons));
	*main = ap_abstract1_change_environment(man,true,main,lcenv,false);
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons);

	*pilot = ap_abstract1_change_environment(man,true,pilot,lcenv,false);
	*pilot = ap_abstract1_meet_tcons_array(man,true,pilot,tcons);
}

void Abstract::canonicalize() {
	ap_abstract1_canonicalize(man,main);
	ap_abstract1_canonicalize(man,pilot);
}

void Abstract::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	*main = ap_abstract1_assign_texpr_array(man,true,main,
			tvar,
			texpr,
			size,
			dest);
	*pilot = ap_abstract1_assign_texpr_array(man,true,pilot,
			tvar,
			texpr,
			size,
			dest);
}

void Abstract::join_array(ap_environment_t * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();

	ap_abstract1_t  Xmain[size];
	ap_abstract1_t  Xpilot[size];
	
	for (int i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env,false);
		Xpilot[i] = ap_abstract1_change_environment(man,false,X_pred[i]->pilot,env,false);
		delete X_pred[i];
	}
	
	ap_abstract1_clear(man,main);
	ap_abstract1_clear(man,pilot);
	if (size > 1) {
		*main = ap_abstract1_join_array(man,Xmain,size);	
		*pilot = ap_abstract1_join_array(man,Xpilot,size);	
	} else {
		*main = Xmain[0];
		*pilot = Xpilot[0];
		return;
	}

	for (int i=0; i < size; i++) {
		ap_abstract1_clear(man,&Xmain[i]);
		ap_abstract1_clear(man,&Xpilot[i]);
	}
}

void Abstract::print() {
	printf("MAIN VALUE:\n");
	ap_environment_fdump(stdout,main->env);
	ap_abstract1_fprint(stdout,man,main);
	printf("PILOT VALUE:\n");
	ap_environment_fdump(stdout,pilot->env);
	ap_abstract1_fprint(stdout,man,pilot);
}
