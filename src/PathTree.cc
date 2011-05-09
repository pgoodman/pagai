#include "PathTree.h"

PathTree::PathTree() {}

PathTree::~PathTree() {}

int rank_vector(std::vector<BasicBlock*> v, BasicBlock* b) {
	int i = 0;
	while (i < v.size() && v[i] != b) {
		i++;
	}
	if (i == v.size()) 
		return -1;
	else
		return i;
}

void PathTree::insert(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	pathnode * v;
	BasicBlock * current;
	int i;

	workingpath.assign(path.begin(), path.end());
	v = &start;
	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
	
		i = rank_vector(v->name,current);
		if ( i == -1) {
			v->name.push_back(current);
			v->next.push_back(new pathnode());
			v = v->next[v->name.size()-1];
		} else {
			v = v->next[i];
		}
	}
}

bool PathTree::exist(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	pathnode * v;
	BasicBlock * current;
	int i;

	workingpath.assign(path.begin(), path.end());
	v = &start;
	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
	
		i = rank_vector(v->name,current);
		if ( i == -1) {
			return false;
		} else {
			v = v->next[i];
		}
	}
	return true;
}
