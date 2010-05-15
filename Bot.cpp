#include "Bot.hpp"
#include <set>
#include <vector>
using namespace std;
using namespace BWAPI;
using namespace BWTA;

#define sz(x) ((int)x.size())

typedef set<Unit*> USet;
typedef USet::iterator USI;
typedef USet::const_iterator USCI;

vector<Unit*> probes;
vector<Unit*> units;

void Bot::onStart()
{
	readMap();
	analyze();
}

void updateUnitList() {
	units.clear();
	USet uns = Broodwar->self()->getUnits();
	for(USCI it = units.begin(); it != units.end(); ++it) {
		Unit* u = *it;
		units.push_back(u);
	}
}

void updateProbeList() {
	probes.clear();
	for(USCI it = units.begin(); it != units.end(); ++i) {
		Unit* u = *it;
	}
}

void taskifyProbes() {
	for(int i = 0; i < sz(probes); ++i) {
		
	}
}

void Bot::onFrame()
{
	updateUnitList();
	updateProbeList();
	taskifyProbes();
}