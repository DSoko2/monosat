/****************************************************************************************[Solver.h]
 The MIT License (MIT)

 Copyright (c) 2016, Sam Bayless

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/


#ifndef OPTIMIZE_CPP_
#define OPTIMIZE_CPP_
#include "monosat/core/Optimize.h"
#include <csignal>
#include <sys/resource.h>
#include <stdexcept>
#include <cstdarg>
#include <string>
#include <cstdint>
#include <limits>
namespace Monosat{

namespace Optimization{
static long long time_limit=-1;
static long long memory_limit=-1;

static bool has_system_time_limit=false;
static rlim_t system_time_limit;
static bool has_system_mem_limit=false;
static rlim_t system_mem_limit;

static Solver * solver;

static sighandler_t system_sigxcpu_handler = nullptr;

//static initializer, following http://stackoverflow.com/a/1681655
namespace {
struct initializer {
	initializer() {
		system_sigxcpu_handler=nullptr;
		time_limit=-1;
		memory_limit=-1;
		has_system_time_limit=false;
		has_system_mem_limit=false;
	}

	~initializer() {
		solver =nullptr;
	}
};
static initializer i;
}
void disableResourceLimits(Solver * S);
static void SIGNAL_HANDLER_api(int signum) {
	if(solver){
		fprintf(stderr,"Monosat resource limit reached\n");
		Solver * s = solver;
		disableResourceLimits(s);
		s->interrupt();
	}
}


void enableResourceLimits(Solver * S){
	if(!solver){
		assert(solver==nullptr);
		solver=S;

		struct rusage ru;
		getrusage(RUSAGE_SELF, &ru);
		__time_t cur_time = ru.ru_utime.tv_sec;

		rlimit rl;
		getrlimit(RLIMIT_CPU, &rl);
		time_limit = opt_limit_optimization_time;
		if(!has_system_time_limit){
			has_system_time_limit=true;
			system_time_limit=rl.rlim_cur;
		}

		if (time_limit < INT32_MAX && time_limit>=0) {
			assert(cur_time>=0);
			long long local_time_limit = 	 time_limit+cur_time;//make this a relative time limit
			if (rl.rlim_max == RLIM_INFINITY || (rlim_t) local_time_limit < rl.rlim_max) {
				rl.rlim_cur = local_time_limit;
				if (setrlimit(RLIMIT_CPU, &rl) == -1)
					fprintf(stderr,"WARNING! Could not set resource limit: CPU-time.\n");
			}
		}else{
			rl.rlim_cur = rl.rlim_max;
			if (setrlimit(RLIMIT_CPU, &rl) == -1)
				fprintf(stderr,"WARNING! Could not set resource limit: CPU-time.\n");
		}

		getrlimit(RLIMIT_AS, &rl);
		if(!has_system_mem_limit){
			has_system_mem_limit=true;
			system_mem_limit=rl.rlim_cur;
		}
		// Set limit on virtual memory:
		if (memory_limit < INT32_MAX && memory_limit>=0) {
			rlim_t new_mem_lim = (rlim_t) memory_limit * 1024 * 1024; //Is this safe?

			if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max) {
				rl.rlim_cur = new_mem_lim;
				if (setrlimit(RLIMIT_AS, &rl) == -1)
					fprintf(stderr, "WARNING! Could not set resource limit: Virtual memory.\n");
			}else{
				rl.rlim_cur = rl.rlim_max;
				if (setrlimit(RLIMIT_AS, &rl) == -1)
					fprintf(stderr, "WARNING! Could not set resource limit: Virtual memory.\n");
			}
		}
		sighandler_t old_sigxcpu = signal(SIGXCPU, SIGNAL_HANDLER_api);
		if(old_sigxcpu != SIGNAL_HANDLER_api){
			system_sigxcpu_handler = old_sigxcpu;//store this value for later
		}
	}
}

void disableResourceLimits(Solver * S){
	if(solver){

		solver=nullptr;
		rlimit rl;
		getrlimit(RLIMIT_CPU, &rl);
		if(has_system_time_limit){
			has_system_time_limit=false;
			if (rl.rlim_max == RLIM_INFINITY || (rlim_t)system_time_limit < rl.rlim_max) {
				rl.rlim_cur = system_time_limit;
				if (setrlimit(RLIMIT_CPU, &rl) == -1)
					fprintf(stderr,"WARNING! Could not set resource limit: CPU-time.\n");
			}else{
				rl.rlim_cur = rl.rlim_max;
				if (setrlimit(RLIMIT_CPU, &rl) == -1)
					fprintf(stderr,"WARNING! Could not set resource limit: CPU-time.\n");
			}
		}
		getrlimit(RLIMIT_AS, &rl);
		if(has_system_mem_limit){
			has_system_mem_limit=false;
			if (rl.rlim_max == RLIM_INFINITY || system_mem_limit < rl.rlim_max) {
				rl.rlim_cur = system_mem_limit;
				if (setrlimit(RLIMIT_AS, &rl) == -1)
					fprintf(stderr, "WARNING! Could not set resource limit: Virtual memory.\n");
			}else{
				rl.rlim_cur = rl.rlim_max;
				if (setrlimit(RLIMIT_AS, &rl) == -1)
					fprintf(stderr, "WARNING! Could not set resource limit: Virtual memory.\n");
			}
		}
		if (system_sigxcpu_handler){
			signal(SIGXCPU, system_sigxcpu_handler);
			system_sigxcpu_handler=nullptr;
		}
	}
}
}

int64_t optimize_linear(Monosat::SimpSolver * S, Monosat::BVTheorySolver<int64_t> * bvTheory,const vec<Lit> & assumes,int bvID, bool & hit_cutoff, int64_t & n_solves){
	hit_cutoff=false;
	vec<Lit> assume;
	for(Lit l:assumes)
		assume.push(l);
	vec<Lit> last_satisfying_assign;
	for(Var v = 0;v<S->nVars();v++){
		if(!S->isEliminated(v)) {
			if (S->value(v) == l_True) {
				last_satisfying_assign.push(mkLit(v));
			} else if (S->value(v) == l_False) {
				last_satisfying_assign.push(mkLit(v, true));
			} else {
				//this variable was unassigned.
			}
		}
	}

	int64_t value = bvTheory->getOverApprox(bvID);
	int64_t last_decision_value=value;
	if(opt_verb>=1){
		printf("Min bv%d = %ld",bvID,value);
	}
	// int bvID,const Weight & to, Var outerVar = var_Undef, bool decidable=true
	Lit last_decision_lit =  bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,value,var_Undef,opt_decide_optimization_lits));
	while(value>bvTheory->getUnderApprox(bvID,true) && !hit_cutoff){
		Lit decision_lit = bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,value-1,var_Undef,opt_decide_optimization_lits));
		assume.push(decision_lit);
		n_solves++;



		int conflict_limit = S->getConflictBudget();
		if(conflict_limit<0)
			conflict_limit=INT32_MAX;
		int opt_lim = opt_limit_optimization_conflicts;
		if(opt_lim<=0)
			opt_lim=INT32_MAX;
		int limit = std::min(opt_lim,conflict_limit);
		if(limit>= INT32_MAX){
			limit=-1;//disable limit.
		}
		S->setConfBudget(limit);
		bool r;
		lbool res = S->solveLimited(assume,false,false);
		if (res==l_Undef){
			hit_cutoff=true;
			if(opt_verb>0){
				printf("\nBudget exceeded during optimization, quiting early (model might not be optimal!)\n");
			}
			r=false;
		}else{
			r = res==l_True;
		}


		if (r){
			last_satisfying_assign.clear();
			for(Var v = 0;v<S->nVars();v++){
				if(!S->isEliminated(v)) {
					if (S->value(v) == l_True) {
						last_satisfying_assign.push(mkLit(v));
					} else if (S->value(v) == l_False) {
						last_satisfying_assign.push(mkLit(v, true));
					} else {
						//this variable was unassigned.
					}
				}
			}
			last_decision_lit=decision_lit;
			last_decision_value=value-1;
			if(S->value(decision_lit)!=l_True){
				throw std::runtime_error("Error in optimization (comparison not enforced)");
			}
			for(Lit l:assume){
				if(S->value(l)!=l_True){
					throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
				}
			}
			int64_t value2 = bvTheory->getOverApprox(bvID);
			if(value2>=value){
				throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
			}
			value=value2;

			assume.pop();
			if(opt_verb>=1){
				printf("\rMin bv%d = %ld",bvID,value);
			}
		}else{
			assume.pop();

			if(value<last_decision_value){
				//if that last decrease in value was by more than 1
				last_decision_lit =  bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,value,var_Undef,opt_decide_optimization_lits));
				last_decision_value=value;
			}
			assume.push(last_decision_lit);
			r= S->solve(last_satisfying_assign,false,false);

			if(!r){
				throw std::runtime_error("Error in optimization (instance has become unsat)");
			}
			for(Lit l:assume){
				if(S->value(l)!=l_True){
					throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
				}
			}
			if(value< bvTheory->getOverApprox(bvID)){
				throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
			}
			if(bvTheory->getOverApprox(bvID) < value){
				value = bvTheory->getOverApprox(bvID);
			}
			break;
		}
	}
	return value;
}
int evalPB(SimpSolver & S,const Objective & o, bool over_approx, bool eval_at_level_0=false){
	int sum_val = 0;
	assert(o.isPB());
	for(int i = 0;i<o.pb_lits.size();i++){
		Lit l = o.pb_lits[i];
		if(l==lit_Undef)
			continue;
		int weight = 1;
		if (i<o.pb_weights.size()){
			weight = o.pb_weights[i];
		}
		lbool val = S.value(l);
		if(eval_at_level_0 && S.level(var(l))>0){
			val = l_Undef;
		}
		if(val==l_True){
			sum_val+=weight;
		}else if (val==l_Undef){
			if (over_approx){
				sum_val+=weight;
			}
		}
	}
	return sum_val;

}
int64_t optimize_linear_pb(Monosat::SimpSolver * S, PB::PBConstraintSolver * pbSolver, const vec<Lit> & assumes, const Objective & o , bool & hit_cutoff, int64_t & n_solves){
	hit_cutoff=false;
	vec<Lit> assume;
	for(Lit l:assumes)
		assume.push(l);
	vec<Lit> last_satisfying_assign;
	for(Var v = 0;v<S->nVars();v++){
		if(!S->isEliminated(v)) {
			if (S->value(v) == l_True) {
				last_satisfying_assign.push(mkLit(v));
			} else if (S->value(v) == l_False) {
				last_satisfying_assign.push(mkLit(v, true));
			} else {
				//this variable was unassigned.
			}
		}
	}

	int value = evalPB(*S, o,true);
	int last_decision_value=value;

	// int bvID,const Weight & to, Var outerVar = var_Undef, bool decidable=true
	Lit last_decision_lit =   pbSolver->addConditionalConstr(o.pb_lits, o.pb_weights,value, PB::Ineq::LEQ);
	while(value>evalPB(*S,o,false) && !hit_cutoff){
		Lit decision_lit =  pbSolver->addConditionalConstr(o.pb_lits, o.pb_weights,value-1, PB::Ineq::LEQ);

		assume.push(decision_lit);
		n_solves++;



		int conflict_limit = S->getConflictBudget();
		if(conflict_limit<0)
			conflict_limit=INT32_MAX;
		int opt_lim = opt_limit_optimization_conflicts;
		if(opt_lim<=0)
			opt_lim=INT32_MAX;
		int limit = std::min(opt_lim,conflict_limit);
		if(limit>= INT32_MAX){
			limit=-1;//disable limit.
		}
		S->setConfBudget(limit);
		bool r;
		lbool res = S->solveLimited(assume,false,false);
		if (res==l_Undef){
			hit_cutoff=true;
			if(opt_verb>0){
				printf("\nBudget exceeded during optimization, quiting early (model might not be optimal!)\n");
			}
			r=false;
		}else{
			r = res==l_True;
		}


		if (r){
			last_satisfying_assign.clear();
			for(Var v = 0;v<S->nVars();v++){
				if(!S->isEliminated(v)) {
					if (S->value(v) == l_True) {
						last_satisfying_assign.push(mkLit(v));
					} else if (S->value(v) == l_False) {
						last_satisfying_assign.push(mkLit(v, true));
					} else {
						//this variable was unassigned.
					}
				}
			}
			last_decision_lit=decision_lit;
			last_decision_value=value-1;
			if(S->value(decision_lit)!=l_True){
				throw std::runtime_error("Error in optimization (comparison not enforced)");
			}
			for(Lit l:assume){
				if(S->value(l)!=l_True){
					throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
				}
			}
			int64_t value2 = evalPB(*S, o,true);//bvTheory->getOverApprox(bvID);
			if(value2>=value){
				throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
			}
			value=value2;

			assume.pop();

		}else{
			assume.pop();

			if(value<last_decision_value){
				//if that last decrease in value was by more than 1
				last_decision_lit = pbSolver->addConditionalConstr(o.pb_lits, o.pb_weights,value, PB::Ineq::LEQ);
						//bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,value,var_Undef,opt_decide_optimization_lits));
				last_decision_value=value;
			}
			assume.push(last_decision_lit);
			r= S->solve(last_satisfying_assign,false,false);

			if(!r){
				throw std::runtime_error("Error in optimization (instance has become unsat)");
			}
			for(Lit l:assume){
				if(S->value(l)!=l_True){
					throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
				}
			}
			int over = evalPB(*S, o, true);
			if(value< over){
				throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
			}
			if(over < value){
				value = over;
			}
			break;
		}
	}
	return value;
}


int64_t optimize_binary_pb(Monosat::SimpSolver * S,  PB::PBConstraintSolver * pbSolver, const vec<Lit> & assumes,const Objective & o,  bool & hit_cutoff, int64_t & n_solves){
	hit_cutoff=false;
	vec<Lit> assume;
	for(Lit l:assumes)
		assume.push(l);
	vec<Lit> last_satisfying_assign;
	for(Var v = 0;v<S->nVars();v++){
		if(!S->isEliminated(v)) {
			if (S->value(v) == l_True) {
				last_satisfying_assign.push(mkLit(v));
			} else if (S->value(v) == l_False) {
				last_satisfying_assign.push(mkLit(v, true));
			} else {
				//this variable was unassigned.
			}
		}
	}

	int min_val = evalPB(*S, o, false, true);// bvTheory->getUnderApprox(bvID,true);
	int max_val = evalPB(*S, o, true); //bvTheory->getOverApprox(bvID);

	int suggested_next_midpoint = -1;

	//try the minimum possible value first, just in case we get lucky.
	//it might also be a good idea to try min_val+1, and max_val-1.

	int underapprox_sat_val =  evalPB(*S, o, false);
	if(underapprox_sat_val<max_val)
		suggested_next_midpoint =	underapprox_sat_val;

	bool first_round=true;
	int64_t last_decision_value=max_val;
	Lit last_decision_lit =  lit_Undef;//  pbSolver->addConditionalConstr(o.pb_lits, o.pb_weights,max_val, PB::Ineq::LEQ); //bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,max_val,var_Undef,opt_decide_optimization_lits));
	while(min_val < max_val && !hit_cutoff){
		int64_t mid_point = min_val + (max_val - min_val) / 2;
		if(mid_point>=max_val)
			mid_point=max_val-1;//can this ever happen?
		if(suggested_next_midpoint>=min_val && suggested_next_midpoint<mid_point)
			mid_point =	suggested_next_midpoint;
		assert(mid_point>=0);assert(mid_point>=min_val);assert(mid_point<max_val);
		Lit decision_lit =  pbSolver->addConditionalConstr(o.pb_lits, o.pb_weights,mid_point, PB::Ineq::LEQ);
				//bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,mid_point,var_Undef,opt_decide_optimization_lits));
		assume.push(decision_lit);
		n_solves++;

		{
			int conflict_limit = S->getConflictBudget();
			if(conflict_limit<0)
				conflict_limit=INT32_MAX;
			int opt_lim = opt_limit_optimization_conflicts;
			if(opt_lim<=0)
				opt_lim=INT32_MAX;
			int limit = std::min(opt_lim,conflict_limit);
			if(limit>= INT32_MAX){
				limit=-1;//disable limit.
			}
			S->setConfBudget(limit);
		}

		Optimization::enableResourceLimits(S);
		bool r;
		lbool res = S->solveLimited(assume,false,false);
		Optimization::disableResourceLimits(S);
		if (res==l_Undef){
			hit_cutoff=true;
			if(opt_verb>0){
				printf("\nBudget exceeded during optimization, quiting early (model might not be optimal!)\n");
			}
			r=false;
		}else{
			r = res==l_True;
		}

		assume.pop();
		if (r){
			last_satisfying_assign.clear();
			for(Var v = 0;v<S->nVars();v++){
				if(!S->isEliminated(v) && v != var(last_decision_lit)) {
					if (S->value(v) == l_True) {
						last_satisfying_assign.push(mkLit(v));
					} else if (S->value(v) == l_False) {
						last_satisfying_assign.push(mkLit(v, true));
					} else {
						//this variable was unassigned.
					}
				}
			}
            S->addClause(~last_decision_lit);
			last_decision_lit=decision_lit;
            last_satisfying_assign.push(decision_lit);
			last_decision_value=mid_point;
			int new_value = evalPB(*S, o, true);//bvTheory->getOverApprox(bvID);
			if(new_value>=max_val){
				throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
			}
			int64_t underapprox_sat_val =  evalPB(*S, o, false); //bvTheory->getUnderApprox(bvID);
			if(underapprox_sat_val<max_val)
				suggested_next_midpoint =	underapprox_sat_val;
			assert(new_value<=mid_point);
			assert(new_value<max_val);
			max_val=new_value;
			if(new_value<min_val){
				//this can only happen if a budget was used and the solver quit early.
				min_val=new_value;
				assert(min_val>= evalPB(*S, o, false, true));
			}

		}else{
			min_val = mid_point+1;

			//set the solver back to its last satisfying assignment
			//this is technically not required, but it should be cheap, and will also reset the solvers decision phase heuristic
			r= S->solve(last_satisfying_assign,false,false);
			if(!r){
				throw std::runtime_error("Error in optimization (instance has become unsat)");
			}

		}
	}


	if(max_val<last_decision_value || last_decision_lit==lit_Undef){
		//if that last decrease in value was by more than 1
		last_decision_lit = pbSolver->addConditionalConstr(o.pb_lits, o.pb_weights,max_val, PB::Ineq::LEQ);
				//bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,max_val,var_Undef,opt_decide_optimization_lits));
		last_decision_value=max_val;
        last_satisfying_assign.push(last_decision_lit);
	}
	bool r;
	assume.push(last_decision_lit);
	r= S->solve(last_satisfying_assign,false,false);

	if(!r){
		throw std::runtime_error("Error in optimization (instance has become unsat)");
	}
	for(Lit l:assume){
		if(S->value(l)!=l_True){
			throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
		}
	}
	int over = evalPB(*S, o, true);
	if(max_val <over){
		throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
	}
	if(over < max_val){
		max_val = over;
	}
	return max_val;
}




int64_t optimize_binary(Monosat::SimpSolver * S, Monosat::BVTheorySolver<int64_t> * bvTheory,const vec<Lit> & assumes,int bvID,  bool & hit_cutoff, int64_t & n_solves){
	hit_cutoff=false;
	vec<Lit> assume;
	for(Lit l:assumes)
		assume.push(l);
	vec<Lit> last_satisfying_assign;
	for(Var v = 0;v<S->nVars();v++){
		if(!S->isEliminated(v)) {
			if (S->value(v) == l_True) {
				last_satisfying_assign.push(mkLit(v));
			} else if (S->value(v) == l_False) {
				last_satisfying_assign.push(mkLit(v, true));
			} else {
				//this variable was unassigned.
			}
		}
	}

	int64_t min_val = bvTheory->getUnderApprox(bvID,true);
	int64_t max_val = bvTheory->getOverApprox(bvID);
	if(opt_verb>=1){
		printf("Min bv%d = %ld",bvID,max_val);
	}
	int64_t suggested_next_midpoint = -1;

	//try the minimum possible value first, just in case we get lucky.
	//it might also be a good idea to try min_val+1, and max_val-1.

	int64_t underapprox_sat_val =  bvTheory->getUnderApprox(bvID);
	if(underapprox_sat_val<max_val)
		suggested_next_midpoint =	underapprox_sat_val;

	bool first_round=true;
	int64_t last_decision_value=max_val;
	Lit last_decision_lit =  bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,max_val,var_Undef,opt_decide_optimization_lits));
	while(min_val < max_val && !hit_cutoff){
		int64_t mid_point = min_val + (max_val - min_val) / 2;

		if(suggested_next_midpoint>=min_val && suggested_next_midpoint<mid_point)
			mid_point =	suggested_next_midpoint;
		assert(mid_point>=0);assert(mid_point>=min_val);assert(mid_point<max_val);

		Lit decision_lit = bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,mid_point,var_Undef,opt_decide_optimization_lits));
		assume.push(decision_lit);
		n_solves++;

		{
			int conflict_limit = S->getConflictBudget();
			if(conflict_limit<0)
				conflict_limit=INT32_MAX;
			int opt_lim = opt_limit_optimization_conflicts;
			if(opt_lim<=0)
				opt_lim=INT32_MAX;
			int limit = std::min(opt_lim,conflict_limit);
			if(limit>= INT32_MAX){
				limit=-1;//disable limit.
			}
			S->setConfBudget(limit);
		}

		Optimization::enableResourceLimits(S);
		bool r;
		lbool res = S->solveLimited(assume,false,false);
		Optimization::disableResourceLimits(S);
		if (res==l_Undef){
			hit_cutoff=true;
			if(opt_verb>0){
				printf("\nBudget exceeded during optimization, quiting early (model might not be optimal!)\n");
			}
			r=false;
		}else{
			r = res==l_True;
		}

		assume.pop();
		if (r){
			last_satisfying_assign.clear();
			for(Var v = 0;v<S->nVars();v++){
				if(!S->isEliminated(v)) {
				if(S->value(v)==l_True){
					last_satisfying_assign.push(mkLit(v));
				}else if(S->value(v)==l_False){
					last_satisfying_assign.push(mkLit(v,true));
				}else {
					//this variable was unassigned.
				}
				}
			}
			last_decision_lit=decision_lit;
			last_decision_value=mid_point;
			int64_t new_value = bvTheory->getOverApprox(bvID);
			if(new_value>=max_val){
				throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
			}
			int64_t underapprox_sat_val =  bvTheory->getUnderApprox(bvID);
			if(underapprox_sat_val<max_val)
				suggested_next_midpoint =	underapprox_sat_val;
			assert(new_value<=mid_point);
			assert(new_value<max_val);
			max_val=new_value;
			if(new_value<min_val){
				//this can only happen if a budget was used and the solver quit early.
				min_val=new_value;
				assert(min_val>=bvTheory->getUnderApprox(bvID,true));
			}
			if(opt_verb>=1){
				printf("\rMin bv%d = %ld",bvID,max_val);
			}
		}else{
			min_val = mid_point+1;

			//set the solver back to its last satisfying assignment
			//this is technically not required, but it should be cheap, and will also reset the solvers decision phase heuristic
			r= S->solve(last_satisfying_assign,false,false);
			if(!r){
				throw std::runtime_error("Error in optimization (instance has become unsat)");
			}

		}
	}


	if(max_val<last_decision_value){
		//if that last decrease in value was by more than 1
		last_decision_lit =  bvTheory->toSolver(bvTheory->newComparison(Comparison::leq,bvID,max_val,var_Undef,opt_decide_optimization_lits));
		last_decision_value=max_val;
	}
	bool r;
	assume.push(last_decision_lit);
	r= S->solve(last_satisfying_assign,false,false);

	if(!r){
		throw std::runtime_error("Error in optimization (instance has become unsat)");
	}
	for(Lit l:assume){
		if(S->value(l)!=l_True){
			throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
		}
	}
	if(max_val < bvTheory->getOverApprox(bvID)){
		throw std::runtime_error("Error in optimization (minimum values are inconsistent with model)");
	}
	if(bvTheory->getOverApprox(bvID) < max_val){
		max_val = bvTheory->getOverApprox(bvID);
	}
	return max_val;
}



lbool optimize_and_solve(SimpSolver & S,const vec<Lit> & assumes,const vec<Objective> & objectives,bool do_simp,  bool & found_optimal){
	vec<Lit> assume;
	for(Lit l:assumes)
		assume.push(l);
	static int solve_runs=0;
	found_optimal=true;
	solve_runs++;
	if(opt_verb>=1){
		if(solve_runs>1){
			printf("Solving(%d)...\n",solve_runs);
		}else{
			printf("Solving...\n");
		}
		fflush(stdout);
	}

	if (opt_verb > 1 && assume.size()) {
		printf("Assumptions: ");
		for (int i = 0; i < assume.size(); i++) {
			Lit l = assume[i];
			printf("%d, ", dimacs(l));
		}
		printf("\n");
	}
	if(!objectives.size()){

		return S.solveLimited(assume,opt_pre && do_simp, !opt_pre && do_simp);

	}else{
		bool any_pb=false;
		bool any_bv = false;
		for(Objective & o:objectives){
			any_pb|= (o.isPB() && o.pb_lits.size()>0 ); //don't need to create the pb theory solver if there is only 1 lit
			any_bv|= o.isBV();
		}

		if(any_bv && !S.getBVTheory()){
			throw std::runtime_error("No bitvector theory created (call initBVTheory())!");
		}
		if(any_pb && !S.getPB()){
			throw std::runtime_error("No pb solver created!");
		}

		bool r;
		lbool res= S.solveLimited(assume,opt_pre && do_simp,!opt_pre && do_simp);
		if (res==l_True){
			r=true;
		}else if (res==l_False){
			r=false;
		}else{
			found_optimal=false;
			return l_Undef;
		}

		if(r && objectives.size()){
			for(Lit l:assume){
				if(S.value(l)!=l_True){
					throw std::runtime_error("Error in optimization (model is inconsistent with assumptions)");
				}
			}
			Monosat::BVTheorySolver<int64_t> * bvTheory = (Monosat::BVTheorySolver<int64_t> *) S.getBVTheory();
			Monosat::PB::PBConstraintSolver * pbSolver = S.getPB();
			vec<int64_t> min_values;
			vec<int64_t> max_values;
			min_values.growTo(objectives.size());
			max_values.growTo(objectives.size());

			for(int i = 0;i<objectives.size();i++){
				if(objectives[i].isBV()){
					min_values[i]=bvTheory->getOverApprox(objectives[i].bvID,true);
					max_values[i]=bvTheory->getUnderApprox(objectives[i].bvID,true);
				}else{
					min_values[i]=evalPB(S,objectives[i],true);
					max_values[i]=evalPB(S, objectives[i],false);
				}
			}


			int64_t n_solves = 1;
			bool hit_cutoff=false;
			for (int i = 0;i<objectives.size() && !hit_cutoff;i++){
				if(objectives[i].isBV()) {
					int bvID = objectives[i].bvID;

					if (opt_verb >= 1) {
						printf("Minimizing bv%d (%d of %d)\n", bvID, i + 1, objectives.size());
					}

					if (!opt_binary_search_optimization) {
						min_values[i] = optimize_linear(&S, bvTheory, assume, bvID, hit_cutoff, n_solves);
					} else {
						min_values[i] = optimize_binary(&S, bvTheory, assume, bvID, hit_cutoff, n_solves);
					}
					if (hit_cutoff) {
						found_optimal = false;
					}
					if (opt_limit_optimization_time_per_arg)
						hit_cutoff = false;//keep trying to minimize subsequent arguments
					assume.push(bvTheory->toSolver(
							bvTheory->newComparison(Comparison::leq, bvID, min_values[i], var_Undef,
													opt_decide_optimization_lits)));

					assert(min_values[i] >= 0);
					if (opt_verb >= 1) {
						printf("\rMin bv%d = %ld\n", bvID, min_values[i]);
					}

				}else{

					if (opt_verb >= 1) {
						printf("Minimizing pb (%d of %d)\n", i + 1, objectives.size());
					}

					if (!opt_binary_search_optimization) {
						min_values[i] = optimize_linear_pb(&S, pbSolver, assume, objectives[i], hit_cutoff, n_solves);
					} else {
						min_values[i] = optimize_binary_pb(&S, pbSolver, assume,  objectives[i], hit_cutoff, n_solves);
					}
					if (hit_cutoff) {
						found_optimal = false;
					}
					if (opt_limit_optimization_time_per_arg)
						hit_cutoff = false;//keep trying to minimize subsequent arguments
					assume.push(pbSolver->addConditionalConstr(objectives[i].pb_lits, objectives[i].pb_weights,min_values[i], PB::Ineq::LEQ));
							//bvTheory->newComparison(Comparison::leq, bvID, min_values[i], var_Undef,
						//							opt_decide_optimization_lits)));

					assert(min_values[i] >= 0);

				}
				//enforce that this bitvector stays at the best value that was found for it
				for(int j = 0;j<i;j++){
					if(objectives[j].isBV()) {
						int bvID = objectives[j].bvID;
						int64_t min_value = min_values[j];
						int64_t model_val = bvTheory->getOverApprox(bvID);
						if (min_value < model_val) {
							throw std::runtime_error(
									"Error in optimization (minimum values are inconsistent with model for bv " +
									std::to_string(j) + " (bvid " + std::to_string(bvID) + " ): expected value <= " +
									std::to_string(min_value) + ", found " + std::to_string(model_val) + ")");
						} else if (model_val < min_value) {
							//if the best known value for any earlier bitvector, (which can happen if optimization is aborted early),
							//is found, enforce that this improved value must be kept in the future.
							Lit decision_lit = bvTheory->toSolver(
									bvTheory->newComparison(Comparison::leq, bvID, model_val, var_Undef,
															opt_decide_optimization_lits));
							assume.push(decision_lit);
							min_values[j] = model_val;
						}
					}else{
						int64_t min_value = min_values[j];
						int64_t model_val = evalPB(S,objectives[j],true);
						if (min_value < model_val) {
							throw std::runtime_error(
									"Error in optimization (minimum values are inconsistent with model for pb " +
									std::to_string(j) + " ): expected value <= " +
									std::to_string(min_value) + ", found " + std::to_string(model_val) + ")");
						} else if (model_val < min_value) {
							//if the best known value for any earlier bitvector, (which can happen if optimization is aborted early),
							//is found, enforce that this improved value must be kept in the future.
							Lit decision_lit = pbSolver->addConditionalConstr(objectives[j].pb_lits, objectives[j].pb_weights,model_val, PB::Ineq::LEQ);
							assume.push(decision_lit);
							min_values[j] = model_val;
						}
					}
				}
			}
			assert(r);

			if(opt_verb>0){
				printf("Minimum values found (after %ld calls) : ",n_solves);
				for(int i = 0;i<min_values.size();i++){
					int bvID = objectives[i].bvID;
					printf("bv%d=%ld,",bvID,min_values[i]);
				}
				printf("\n");
			}
			if(opt_check_solution){
				for(int i = 0;i<objectives.size();i++){
                    if(objectives[i].isBV()) {
                        int bvID = objectives[i].bvID;
                        int64_t min_value = min_values[i];
                        int64_t model_val = bvTheory->getOverApprox(bvID);
                        if (min_value < model_val) {
                            throw std::runtime_error(
                                    "Error in optimization (minimum values are inconsistent with model)");
                        }
                    }else{
                        int64_t min_value = min_values[i];
                        int64_t model_val =evalPB(S, objectives[i],true);
                        if (min_value < model_val) {
                            throw std::runtime_error(
                                    "Error in optimization (minimum values are inconsistent with model)");
                        }
                    }
				}
			}
		}

		return r? l_True:l_False;
	}


}
};
#endif /* OPTIMIZE_CPP_ */
