/****************************************************************************************[Solver.h]
 The MIT License (MIT)

 Copyright (c) 2014, Sam Bayless

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
#include <fsm/alg/NFAGraphDawgAccept.h>

#include "FSMDawgAcceptDetector.h"
#include "FSMTheory.h"

using namespace Monosat;


FSMDawgAcceptDetector::FSMDawgAcceptDetector(int detectorID, FSMTheorySolver * outer, DynamicFSM &g_under,
		DynamicFSM &g_over, int source,  double seed) :
		FSMDetector(detectorID), outer(outer), g_under(g_under), g_over(g_over), source(source), rnd_seed(seed),order_heap(AcceptOrderLt(*this,activity)){

	FSMDawg * root = new FSMDawg();

	underReachStatus = new FSMDawgAcceptDetector::AcceptStatus(*this, true);
	overReachStatus = new FSMDawgAcceptDetector::AcceptStatus(*this, false);


		underapprox_detector = new NFAGraphDawgAccept<FSMDawgAcceptDetector::AcceptStatus>(g_under, source,  *underReachStatus,
																			  opt_fsm_track_used_transitions);
	overapprox_detector = new NFAGraphDawgAccept<FSMDawgAcceptDetector::AcceptStatus>(g_over, source,  *overReachStatus,
																			  opt_fsm_track_used_transitions);

	underprop_marker = outer->newReasonMarker(getID());
	overprop_marker = outer->newReasonMarker(getID());
}
void FSMDawgAcceptDetector::releaseDawgLit(Var accept_var){

}
void FSMDawgAcceptDetector::addAcceptDawgLit(int state, FSMDawg * dawg, Var outer_reach_var){
	assert(dawg->id<0);
	dawg->id = trackedDawgs.size();
	trackedDawgs.push(dawg);
	int dawgID = dawg->id;

	while(accept_lits.size()<=dawgID){
		accept_lits.push();
		accept_lits.last().growTo(g_under.nodes(),lit_Undef);
	}

	if(first_destination==-1)
		first_destination= state;

	if (accept_lits[dawgID][state] != lit_Undef) {
		Lit r = accept_lits[dawgID][state];
		//force equality between the new lit and the old reach lit, in the SAT solver
		outer->makeEqualInSolver(outer->toSolver(r), mkLit(outer_reach_var));
		return;
	}

	g_under.invalidate();
	g_over.invalidate();

	Var accept_var = outer->newVar(outer_reach_var, getID(),g_over.getID());

	if (first_var == var_Undef) {
		first_var = accept_var;
	} else {
		assert(accept_var >= first_var);
	}
	int index = accept_var - first_var;

	//is_changed.growTo(index+1);
	Lit acceptLit = mkLit(accept_var, false);
	all_lits.push(acceptLit);
	assert(accept_lits[dawgID][state] == lit_Undef);
	//if(reach_lits[to]==lit_Undef){
	accept_lits[dawgID][state] = acceptLit;
	while (accept_lit_map.size() <= accept_var - first_var) {
		accept_lit_map.push({-1,-1});
	}
	accept_lit_map[accept_var - first_var] = {dawgID,state};
	accepting_state.growTo(g_over.states());
	accepting_state[state]=true;
	activity.growTo(index+1,0);
	underapprox_detector->setTrackDawgAcceptance(dawg,state,true,true);
	overapprox_detector->setTrackDawgAcceptance(dawg,state,true,true);
}


void FSMDawgAcceptDetector::AcceptStatus::accepts(FSMDawg * d,int state,int edgeID,int label, bool accepts){
	int dawgID = d->id;
	assert(dawgID>=0);
	Lit l = detector.accept_lits[dawgID][state];

	if (l != lit_Undef){// && !detector.is_changed[detector.indexOf(var(l))]) {
		if(!accepts){
			l=~l;
		}
		if (polarity == accepts){
			lbool assign = detector.outer->value(l);
			//detector.is_changed[detector.indexOf(var(l))] = true;
			detector.changed.push( { l, state,dawgID,polarity});
		}
	}
}

Lit FSMDawgAcceptDetector::decide(int level){
	if (to_decide.size() && last_decision_status == overapprox_detector->numUpdates()) {
		while (to_decide.size()) {
			Lit l = to_decide.last();
			to_decide.pop();
			if (outer->value(l) == l_Undef) {
				/*if(opt_verb>1){
					printf("fsm decide %d\n",dimacs(l));
				}*/
				return l;
			}
		}
	}
	int last_size = order_heap.size()+1;
	while(order_heap.size()){
		if(order_heap.size()>=last_size){
			throw std::runtime_error("Error in fsm accept detector");
		}
		last_size = order_heap.size();
		Var acceptVar = order_heap.peekMin();
		Lit l = mkLit(acceptVar);


		if (l == lit_Undef) {
			order_heap.removeMin();
			continue;
		}
		int index =  indexOf(var(l));
		int node = accept_lit_map[index].to;
		int dawgID = accept_lit_map[index].str;
		FSMDawg * dawg = trackedDawgs[dawgID];
		assert(dawg);
		assert(dawg->id ==dawgID);
		/*if(opt_verb>1){
			printf("fsm pick decide %d\n",index);
		}*/

		if (outer->value(l) == l_True && opt_decide_fsm_pos) {
				int j = node;

				if ( !underapprox_detector->acceptsDawg(dawg,node)) {
					assert(overapprox_detector->acceptsDawg(dawg,node));//else we'd have a conflict
					to_decide.clear();
					last_decision_status = overapprox_detector->numUpdates();
					decision_path.clear();

					int p = j;
					int last_edge = -1;
					int last = j;


					//ok, read back the path from the over to find a candidate edge we can decide
					//find the earliest unconnected node on this path
					overapprox_detector->getAbstractPath(dawg, node, decision_path,true);
					last_decision_status = overapprox_detector->numUpdates();
					for(NFATransition & t:decision_path){
						int edgeID = t.edgeID;
						int input = t.input;
						Lit l  = mkLit(outer->getTransitionVar(g_over.getID(),edgeID,input,0));
						if(outer->value(l)==l_Undef)
							to_decide.push(l);
					}


					if (to_decide.size() && last_decision_status == overapprox_detector->numUpdates()) {
						while (to_decide.size()) {
							Lit l = to_decide.last();
							to_decide.pop();
							if (outer->value(l) == l_Undef) {
								/*if(opt_verb>1){
									printf("fsm decide %d\n",dimacs(l));
								}*/
								return l;
							}
						}
					}
				}
			}else if (outer->value(l)==l_False && opt_decide_fsm_neg){
				//find any transitions that are not on any paths of the accepting strings, and assign them false.
				if (overapprox_detector->acceptsDawg(dawg,node) ) {
					assert(!underapprox_detector->acceptsDawg(dawg,node));//else we'd have a conflict
					//for each negated reachability constraint, we can find a cut through the unassigned edges in the over-approx and disable one of those edges.



					//try to disconnect this node from source by walking back along the path in the over approx, and disabling the first unassigned edge we see.
					//(there must be at least one such edge, else the variable would be connected in the under approximation as well - in which case it would already have been propagated.
					overapprox_detector->getAbstractPath(dawg, node, decision_path,true);
					last_decision_status = overapprox_detector->numUpdates();
					for(NFATransition & t:decision_path){
						int edgeID = t.edgeID;
						int input = t.input;
						Lit l  = mkLit(outer->getTransitionVar(g_over.getID(),edgeID,input,0));
						if(outer->value(l)==l_Undef) {
							//to_decide.push(l);
						/*	if(opt_verb>1){
								printf("fsm decide %d\n",dimacs(l));
							}*/
							return l;
						}
					}


				}

			}
		if(order_heap.peekMin()==acceptVar){
			order_heap.removeMin();
		}


		}

	return lit_Undef;
}

bool FSMDawgAcceptDetector::propagate(vec<Lit> & conflict) {
	static int iter = 0;
	if (++iter == 17) {
		int a = 1;
	}

	if(opt_fsm_symmetry_breaking){
		if(!checkSymmetryConstraints(conflict))
			return false;
	}

	if(outer->decisionLevel()==0){
		//update which accepting states need to have positive/negative properties checked

		for(Lit l:all_lits){
			if(l!=lit_Undef){
				int state =accept_lit_map[var(l) - first_var].to;
				int dawgID = accept_lit_map[var(l) - first_var].str;
				FSMDawg * dawg = trackedDawgs[dawgID];
				if(outer->value(l)==l_False){
					underapprox_detector->setTrackDawgAcceptance(dawg,state,true,false);
					overapprox_detector->setTrackDawgAcceptance(dawg,state,true,false);
				}else if (outer->value(l)==l_True){
					underapprox_detector->setTrackDawgAcceptance(dawg,state,false,true);
					overapprox_detector->setTrackDawgAcceptance(dawg,state,false,true);
				}else{
					underapprox_detector->setTrackDawgAcceptance(dawg,state,true,true);
					overapprox_detector->setTrackDawgAcceptance(dawg,state,true,true);
				}
			}
		}
	}

	changed.clear();
	bool skipped_positive = false;
	if (underapprox_detector && (!opt_detect_pure_theory_lits || unassigned_positives > 0)) {
		double startdreachtime = rtime(2);
		stats_under_updates++;
		underapprox_detector->update();
		double reachUpdateElapsed = rtime(2) - startdreachtime;
		//outer->reachupdatetime+=reachUpdateElapsed;
		stats_under_update_time += rtime(2) - startdreachtime;
	} else {
		skipped_positive = true;
		//outer->stats_pure_skipped++;
		stats_skipped_under_updates++;
	}
	bool skipped_negative = false;
	if (overapprox_detector && (!opt_detect_pure_theory_lits || unassigned_negatives > 0)) {
		double startunreachtime = rtime(2);
		stats_over_updates++;
		overapprox_detector->update();
		double unreachUpdateElapsed = rtime(2) - startunreachtime;
		stats_over_update_time += rtime(2) - startunreachtime;
	} else {
		skipped_negative = true;
		stats_skipped_over_updates++;
	}

	if (opt_rnd_shuffle) {
		randomShuffle(rnd_seed, changed);
	}


	while (changed.size()) {
			int sz = changed.size();
			Lit l = changed.last().l;
			bool polarity = changed.last().polarity;
			int u = changed.last().u;
			int dawgID = changed.last().str;
			FSMDawg * dawg = trackedDawgs[dawgID];
			//assert(is_changed[indexOf(var(l))]);


			if (underapprox_detector && polarity && !sign(l) && underapprox_detector->acceptsDawg(dawg,u)) {

			} else if (overapprox_detector && !polarity && sign(l) && !overapprox_detector->acceptsDawg(dawg,u)) {

			} else {
				if(sz==changed.size()) {
					//it is possible, in rare cases, for literals to have been added to the chagned list while processing the changed list.
					//in that case, don't pop anything off the changed list, and instead accept that this literal may be re-processed
					assert(sz == changed.size());
					assert(changed.last().u == u);
					//is_changed[indexOf(var(l))] = false;
					changed.pop();
					//this can happen if the changed node's reachability status was reported before a backtrack in the solver.
				}
				continue;
			}

			bool reach = !sign(l);
			if (outer->value(l) == l_True) {
				//do nothing
				if(opt_theory_internal_vsids_fsm && ! order_heap.inHeap(var(l))){
					order_heap.insert(var(l));
				}
			} else if (outer->value(l) == l_Undef) {

				if (reach)
					outer->enqueue(l, underprop_marker);
				else
					outer->enqueue(l, overprop_marker);
			} else if (outer->value(l) == l_False) {
				conflict.push(l);

				if (reach) {
					buildAcceptReason(u,dawg, conflict);
				} else {
					//The reason is a cut separating s from t
					buildNonAcceptReason(u,dawg, conflict);
				}

				return false;
			}
			if(sz==changed.size()) {
				//it is possible, in rare cases, for literals to have been added to the chagned list while processing the changed list.
				//in that case, don't pop anything off the changed list, and instead accept that this literal may be re-processed
				assert(sz ==
					   changed.size());//This can be really tricky - if you are not careful, an a reach detector's update phase was skipped at the beginning of propagate, then if the reach detector is called during propagate it can push a change onto the list, which can cause the wrong item to be removed here.
				assert(changed.last().u == u);
				//is_changed[indexOf(var(l))] = false;
				changed.pop();
			}
		}

	assert(changed.size()==0);
	return true;
}


void FSMDawgAcceptDetector::buildReason(Lit p, vec<Lit> & reason, CRef marker) {
	if (marker == underprop_marker) {
		reason.push(p);
		Var v = var(p);
		int u = getState(v);
		int dawgID = getString(v);
		FSMDawg * dawg = trackedDawgs[dawgID];
		buildAcceptReason(u,dawg, reason);
	} else if (marker == overprop_marker) {
		reason.push(p);
		Var v = var(p);
		int t = getState(v);

		int dawgID = getString(v);
		FSMDawg * dawg = trackedDawgs[dawgID];
		buildNonAcceptReason(t,dawg, reason);
	}  else {
		assert(false);
	}
}

	bool FSMDawgAcceptDetector::checkSymmetryConstraints(vec<Lit> & conflict) {
		if(opt_fsm_symmetry_breaking==1){
			return checkSymmetryConstraintsPopCount(conflict);
		}else if (opt_fsm_symmetry_breaking==2){
			return checkSymmetryConstraintsBitVec(conflict);
		}else{
			return true;
		}
	}

	bool FSMDawgAcceptDetector::checkSymmetryConstraintsBitVec(vec<Lit> & conflict) {
		for (int i = 0; i < g_over.states(); i++) {
			if (i == source || accepting_state[i])
				continue;

			Bitset & prevOver = _bitvec1;
			prevOver.zero();

			int pos = 0;
			for(int k = 0;k<g_over.nIncident(i);k++){
				int edgeID = g_over.incident(i,k).id;
				for(int l = 0;l< g_over.inAlphabet();l++ ){
					prevOver.growTo(pos+1);
					if( g_over.transitionEnabled(edgeID,l,0)){

						prevOver.set(pos);
					}
					pos++;
				}
			}

			for (int j = i+1; j < g_over.states(); j++) {
				if (j == source || accepting_state[j] )
					continue;
					Bitset & curUnder = _bitvec2;
					curUnder.zero();
					//if i is < j, then the number of enabled transitions in i must be <= the number in j
					int pos = 0;
					for(int k = 0;k<g_under.nIncident(j);k++){
						int edgeID = g_under.incident(j,k).id;
						for(int l = 0;l< g_under.inAlphabet();l++ ){
							curUnder.growTo(pos+1);
							if(g_under.transitionEnabled(edgeID,l,0)){

								curUnder.set(pos);
							}
							pos++;
						}
					}
					if (curUnder.GreaterThan(prevOver)){
						//this is a symmetry conflict
						//either node i must enable a disabled transition, or node j must disable an enabled transition

						vec<Var> overVars;
						vec<Var> underVars;
						for(int k = 0;k<g_over.nIncident(i);k++){
							int edgeID = g_over.incident(i,k).id;
							for(int l = 0;l< g_over.inAlphabet();l++ ){
								overVars.push(outer->getTransitionVar(g_over.getID(),edgeID,l,0));
							}
						}

						for(int k = 0;k<g_under.nIncident(j);k++){
							int edgeID = g_under.incident(j,k).id;
							for(int l = 0;l< g_under.inAlphabet();l++ ){
								Var v = outer->getTransitionVar(g_over.getID(), edgeID,l,0);
								underVars.push(v);
							}
						}
						assert(underVars.size()==overVars.size());
						assert(pos==underVars.size());
						pos--;
							for(;pos>=0;pos--) {

								if (curUnder[pos]) {

									if(underVars[pos]!=var_Undef){
										Lit l = ~mkLit(underVars[pos]);
										conflict.push(l);
										assert(outer->value(l)==l_False);
									}
								}
								if (!prevOver[pos]) {

									if(overVars[pos]!=var_Undef){
										Lit l = mkLit(overVars[pos]);
										conflict.push(l);
										assert(outer->value(l)==l_False);
									}
								}
								if(prevOver[pos] && !curUnder[pos]){
									break;
								}
							} ;
						   if(opt_verb>1){
								printf("FSM Symmetry breaking clause, with %d lits\n", conflict.size());
							}
							return false;
					}


					}

			}
		return true;
	}

	bool FSMDawgAcceptDetector::checkSymmetryConstraintsPopCount(vec<Lit> & conflict) {
		for (int i = 0; i < g_over.states(); i++) {
			if (i == source || accepting_state[i])
				continue;

			int n_enabled_outgoing_over = 0;
			for(int k = 0;k<g_over.nIncident(i);k++){
				int edgeID = g_over.incident(i,k).id;
				for(int l = 0;l< g_over.inAlphabet();l++ ){
					n_enabled_outgoing_over+=g_over.transitionEnabled(edgeID,l,0);
				}
			}
			if (n_enabled_outgoing_over==0)
				continue;

			for (int j = i+1; j < g_over.states(); j++) {
				if (j == source || accepting_state[j] )
					continue;

					//if i is < j, then the number of enabled transitions in i must be <= the number in j
					int n_enabled_outgoing_under = 0;
					for(int k = 0;k<g_under.nIncident(j);k++){
						int edgeID = g_under.incident(j,k).id;
						for(int l = 0;l< g_under.inAlphabet();l++ ){
							n_enabled_outgoing_under+=g_under.transitionEnabled(edgeID,l,0);
						}
					}
					if (n_enabled_outgoing_under>n_enabled_outgoing_over){
						//this is a symmetry conflict
						//either node i must enable a disabled transition, or node j must disable an enabled transition
						for(int k = 0;k<g_over.nIncident(i);k++){
							int edgeID = g_over.incident(i,k).id;
							for(int l = 0;l< g_over.inAlphabet();l++ ){
								if(!g_over.transitionEnabled(edgeID,l,0)){
									Var v = outer->getTransitionVar(g_over.getID(), edgeID,l,0);
									if(v!=var_Undef){
										conflict.push(mkLit(v));
									}
								}
							}
						}

						for(int k = 0;k<g_under.nIncident(j);k++){
							int edgeID = g_under.incident(j,k).id;
							for(int l = 0;l< g_under.inAlphabet();l++ ){
								if(g_under.transitionEnabled(edgeID,l,0)){
									Var v = outer->getTransitionVar(g_over.getID(), edgeID,l,0);
									if(v!=var_Undef){
										conflict.push(~mkLit(v));
									}
									//conflict.push(~mkLit(outer->getTransitionVar(g_under.getID(), edgeID,l,0)));
								}
							}
						}
						if(opt_verb>1){
							printf("FSM Symmetry breaking clause, with %d lits\n", conflict.size());
						}
						return false;
					}

			}
		}
		return true;
	}



void FSMDawgAcceptDetector::buildAcceptReason(int node,FSMDawg * dawg, vec<Lit> & conflict){
	static int iter = 0;
	++iter;
//find a path - ideally, the one that traverses the fewest unique transitions - from source to node, learn that one of the transitions on that path must be disabled.
/*	g_under.draw(source);
	vec<int> & string = strings[str];
	printf("Accepts: \"");
	for(int s:string){
		printf("%d ",s);
	}
	printf("\"\n");*/
	static vec<NFATransition> path;
	path.clear();
	bool hasPath =underapprox_detector->getPath(dawg,node,path);
	assert(hasPath);
	assert(underapprox_detector->acceptsDawg(dawg,node));
	for(auto & t:path){
		int edgeID = t.edgeID;
		int input = t.input;
		Var v = outer->getTransitionVar(g_over.getID(),edgeID,input,0);
		assert(outer->value(v)==l_True);
		conflict.push(mkLit(v,true));
	}
	//note: if there are repeated edges in this conflict, they will be cheaply removed by the sat solver anyhow, so that is not a major problem.
	bumpConflict(conflict);
}
void FSMDawgAcceptDetector::buildNonAcceptReason(int node,FSMDawg * dawg, vec<Lit> & conflict){

	static int iter = 0;
//optionally, remove all transitions from the graph that would not be traversed by this string operating on the level 0 overapprox graph.

	//This doesn't work:
	//then, find a cut in the remaining graph through the disabled transitions, separating source from node.

	//Instead:
	//The cut has to be made through disabled transitions in the unrolled graph, with only appropriate transitions for the string (+emoves) traversible in each time frame.
	//graph must be unrolled to length of string.

	//instead of actually unrolling the graph, I am going to traverse it backwards, 'unrolling it' implicitly.
	static vec<int> to_visit;
	static vec<int> next_visit;

	/*
	g_over.draw(source);

	printf("%d: Doesn't accept: \"",ter);
	for(int s:string){
		printf("%d ",s);
	}
	printf("\"\n");
*/



	if(++iter==10189){
		int a=1;
	}

	assert(!overapprox_detector->acceptsDawg(dawg,node));
	//int strpos = string.size()-1;
	to_visit.clear();
	next_visit.clear();

	static vec<bool> cur_seen;
	static vec<bool> next_seen;
	cur_seen.clear();
	cur_seen.growTo(g_under.states());

	next_seen.clear();
	next_seen.growTo(g_under.states());

	int n_transitions = 0;
	for(FSMDawg * d:dawg->transitions){
		if(d)
			n_transitions++;
	}

	if(n_transitions==0){
		//special handling for empty string. Only e-move edges are considered.
		assert(node!=source);//otherwise, this would have been accepted.

		if(!g_over.emovesEnabled()){
			//no way to get to the node without consuming strings.
			return;
		}
		cur_seen[node]=true;
		to_visit.push(node);
		for(int j = 0;j<to_visit.size();j++){
			int u = to_visit[j];



			for (int i = 0;i<g_under.nIncoming(u);i++){
				int edgeID = g_under.incoming(u,i).id;
				int from = g_under.incoming(u,i).node;

				if(g_over.emovesEnabled()){
					if (g_over.transitionEnabled(edgeID,0,0)){
						if (!cur_seen[from]){
							cur_seen[from]=true;
							to_visit.push(from);//emove transition, if enabled
						}
					}else{
						Var v = outer->getTransitionVar(g_over.getID(),edgeID,0,0);
						if (v!=var_Undef){
							assert(outer->value(v)==l_False);
							if (outer->level(v)>0){
								//learn v
								conflict.push(mkLit(v));
							}
						}
					}
				}


			}
		}

		for(int s:to_visit){
			assert(cur_seen[s]);
			cur_seen[s]=false;
		}
		to_visit.clear();
		bumpConflict(conflict);
		return;
	}


	for(int s:next_visit){
		assert(next_seen[s]);
		next_seen[s]=false;
	}
	next_visit.clear();

	next_visit.push(source);
	next_seen[source]=true;
	//dawg->seen=true;
	buildNonAcceptReasonRecursive(dawg,next_visit,conflict);
	bumpConflict(conflict);

/*	printf("conflict: ");
	for (int i = 1;i<conflict.size();i++){
		Lit l = conflict[i];
		int edgeID = outer->getEdgeID(var(l));
		int f = g_under.getEdge(edgeID).from;
		int t = g_under.getEdge(edgeID).to;
		printf("(%d->%d %d),",f,t,outer->getLabel(var(l)));
	}*/
}

void FSMDawgAcceptDetector::buildNonAcceptReasonRecursive(FSMDawg * d,vec<int> & to_visit_,vec<Lit> & conflict){

	vec<int> to_visit;

	for(int v:to_visit_){
		to_visit.push(v);

	}
	vec<bool> next_seen;

	vec<int> next_visit;
	for(int l =0;l<d->transitions.size();l++) {
		if(!d->transitions[l])
			continue;
		next_visit.clear();
		next_seen.clear();
		next_seen.growTo(g_under.nodes());
		FSMDawg * child = d->transitions[l];
		for (int j = 0; j < to_visit.size(); j++) {
			int u = to_visit[j];

			for (int i = 0; i < g_under.nIncoming(u); i++) {
				int edgeID = g_under.incoming(u, i).id;
				int from = g_under.incoming(u, i).node;
				//not supporting emoves yet

				if (g_over.transitionEnabled(edgeID, l, 0)) {
					if (!next_seen[from]) {
						next_seen[from] = true;
						next_visit.push(from);
					}
				} else {
					Var v = outer->getTransitionVar(g_over.getID(), edgeID, l, 0);
					if (v != var_Undef) {
						assert(outer->value(v) == l_False);
						if (outer->level(v) > 0) {
							//learn v
							conflict.push(mkLit(v));//rely on the sat solver to remove duplicates, here...
						}
					}
				}
			}
		}
		buildNonAcceptReasonRecursive(child, next_visit ,conflict);
	}

}

void FSMDawgAcceptDetector::printSolution(std::ostream& out){
	g_under.draw(source);
}
bool FSMDawgAcceptDetector::checkSatisfied() {

	return true;
}


