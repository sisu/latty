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
typedef vector<Unit*> UVec;
typedef UVec::iterator UVI;

UVec probes;
UVec units;

vector<BaseLocation*> areas;
vector<vector<Unit*> > aunits;

void addPyon(int x)
{
	BaseLocation* b = areas[x];
	vector<Unit*>& v = aunits[x];
}

void Bot::onStart()
{
	readMap();
	analyze();

	const set<BaseLocation*>& bs = getBaseLocations();
	areas.insert(areas.end(), bs.begin(), bs.end());
}

void updateUnitList() {
	units.clear();
	USet uns = Broodwar->self()->getUnits();
	for(UVI it = units.begin(); it != units.end(); ++it) {
		Unit* u = *it;
		units.push_back(u);
	}
}

void updateProbeList() {
	probes.clear();
	for(UVI it = units.begin(); it != units.end(); ++it) {
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