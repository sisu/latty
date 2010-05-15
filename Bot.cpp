#include "Bot.hpp"
#include <set>
#include <vector>
using namespace std;
using namespace BWAPI;
using namespace BWTA;
using namespace UnitTypes;

#define sz(x) ((int)x.size())

typedef set<Unit*> USet;
typedef USet::iterator USI;
typedef USet::const_iterator USCI;
typedef vector<Unit*> UVec;
typedef UVec::iterator UVI;

UVec probes;
UVec units;
UVec minerals;

vector<BaseLocation*> areas;
vector<vector<Unit*> > aunits;

void addPylon(int x)
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
	const USet& uns = Broodwar->self()->getUnits();
	for(USCI it = uns.begin(); it != uns.end(); ++it) {
		Unit* u = *it;
		units.push_back(u);
	}
}

void updateProbeList() {
	probes.clear();
	for(UVI it = units.begin(); it != units.end(); ++it) {
		Unit* u = *it;
		probes.push_back(u);
	}
}

void updateMineralList() {
	const USet& gsm = Broodwar->getStaticMinerals();
	minerals.clear();
	for(USCI it = gsm.begin(); it != gsm.end(); ++it) {
		Unit* u = *it;
		if(u->getType() == Resource_Mineral_Field) {
			minerals.push_back(u);
		}
	}
}

Unit* nearestMineralSource(Unit* u) {
	double smallestDistance = 1e50;
	Unit* ret = NULL;
	for(int i = 0; i < sz(minerals); ++i) {
		double dist = u->getDistance(minerals[i]);
		if(dist < smallestDistance) {
			smallestDistance = dist;
			ret = minerals[i];
		}
	}
	return ret;
}

void taskifyProbes() {
	for(int i = 0; i < sz(probes); ++i) {
		if(!probes[i]->isIdle()) continue;
		Unit* u = nearestMineralSource(probes[i]);
		if(u) {
			probes[i]->rightClick(u);
		}
	}
}

void Bot::onFrame()
{
	updateUnitList();
	updateProbeList();
	updateMineralList();

	taskifyProbes();
}