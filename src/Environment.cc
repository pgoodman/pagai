/**
 * \file Environment.cc
 * \brief Implementation of the Environment class
 * \author Julien Henry
 */

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"
#include "Environment.h"
#include "Debug.h"

using namespace llvm;

Environment::Environment() {
	env = ap_environment_alloc_empty();
}
 
Environment::Environment(const Environment &e) {
	env = ap_environment_copy(e.env);
}

Environment::Environment(std::set<ap_var_t> * intvars, std::set<ap_var_t> * realvars) {
	ap_var_t * _intvars = (ap_var_t*)malloc(intvars->size()*sizeof(ap_var_t));
	ap_var_t * _realvars = (ap_var_t*)malloc(realvars->size()*sizeof(ap_var_t));

	int j = 0;
	for (std::set<ap_var_t>::iterator i = intvars->begin(),
			e = intvars->end(); i != e; ++i) {
		_intvars[j] = *i;
		j++;
	}

	j = 0;
	for (std::set<ap_var_t>::iterator i = realvars->begin(),
			e = realvars->end(); i != e; ++i) {
		_realvars[j] = *i;
		j++;
	}

	env = ap_environment_alloc(_intvars,intvars->size(),_realvars,realvars->size());
	free(_intvars);
	free(_realvars);
}

Environment::Environment(Abstract * A) {
	env = ap_environment_copy(A->main->env);
}

Environment::Environment(ap_tcons1_array_t * cons) {
	env = ap_environment_copy(cons->env);
}

Environment::Environment(Constraint * cons) {
	env = ap_environment_copy(cons->get_ap_tcons1()->env);
}

Environment::Environment(Constraint_array * cons) {
	env = ap_environment_copy(cons->getEnv());
}

Environment::Environment(ap_environment_t * e) {
	env = ap_environment_copy(e);
}

Environment::~Environment() {
	ap_environment_free(env);
}

Environment & Environment::operator= (const Environment &e) {
	ap_environment_free(env);
	env = ap_environment_copy(e.env);
	return *this;
}

ap_environment_t * Environment::getEnv() {
	return env;
}

void Environment::get_vars(std::set<ap_var_t> * intdims, std::set<ap_var_t> * realdims) {
	ap_var_t var;
	Value* val;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		intdims->insert(var);
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		realdims->insert(var);
	}
}

void Environment::get_vars_live_in(BasicBlock * b, Live * LV, std::set<ap_var_t> * intdims, std::set<ap_var_t> * realdims) {
	ap_var_t var;
	Value* val;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		val = (Value*)var;
		if (LV->isLiveByLinearityInBlock(val,b,true)) {
			intdims->insert(var);
		}
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		val = (Value*)var;
		if (LV->isLiveByLinearityInBlock(val,b,true)) {
			realdims->insert(var);
		}
	}
}

ap_environment_t * Environment::common_environment(
		ap_environment_t * env1, 
		ap_environment_t * env2) {

	ap_dimchange_t * dimchange1 = NULL;
	ap_dimchange_t * dimchange2 = NULL;
	ap_environment_t * lcenv = ap_environment_lce(
			env1,
			env2,
			&dimchange1,
			&dimchange2);

	if (dimchange1 != NULL)
		ap_dimchange_free(dimchange1);
	if (dimchange2 != NULL)
		ap_dimchange_free(dimchange2);

	return lcenv;
}

void Environment::common_environment(ap_texpr1_t * exp1, ap_texpr1_t * exp2) {
	ap_environment_t * env1 = exp1->env;
	ap_environment_t * env2 = exp2->env;
	ap_environment_t * common = common_environment(env1,env2);
	ap_texpr1_extend_environment_with(exp1,common);
	ap_texpr1_extend_environment_with(exp2,common);
	ap_environment_free(common);
}

Environment * Environment::common_environment(Expr* exp1, Expr* exp2) {
	ap_environment_t * env1 = exp1->getExpr()->env;
	ap_environment_t * env2 = exp2->getExpr()->env;
	ap_environment_t * common = common_environment(env1,env2);
	Environment * res = new Environment(common);
	ap_environment_free(common);
	return res;
}

Environment * Environment::common_environment(Environment* env1, Environment* env2) {
	ap_environment_t * common = common_environment(env1->env,env2->env);
	Environment * res = new Environment(common);
	ap_environment_free(common);
	return res;
}

Environment * Environment::intersection(Environment * env1, Environment * env2) {
	ap_environment_t * lcenv = common_environment(env1->env,env2->env);
	ap_environment_t * intersect = ap_environment_copy(lcenv);	
	ap_environment_t * tmp = NULL;	

	for (size_t i = 0; i < lcenv->intdim + lcenv->realdim; i++) {
		ap_var_t var = ap_environment_var_of_dim(lcenv,(ap_dim_t)i);
		if (!ap_environment_mem_var(env1->env,var) || !ap_environment_mem_var(env2->env,var)) {
			//size_t size = intersect->intdim + intersect->realdim;
			tmp = ap_environment_remove(intersect,&var,1);
			ap_environment_free(intersect);
			intersect = tmp;
		}	
	}	
	Environment * res = new Environment(intersect);
	ap_environment_free(intersect);
	ap_environment_free(lcenv);
	return res;
}

void Environment::print() {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR: tmpfile has not been created\n";
		return;
	}

	ap_environment_fdump(tmp,env);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}