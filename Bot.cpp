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
typedef BWTA::Polygon Poly;

UVec probes;
UVec units;
UVec minerals;

vector<BaseLocation*> areas;
vector<vector<Unit*> > aunits;
vector<vector<Unit*> > abuildings;
int NA;

template<int t>
int evalBP(int x, int y){return 0;}

const int PYLON = 0;
template<>
int evalBP<PYLON>(int x, int y)
{
	return rand();
}

template<int type>
bool addBuild(int x)
{
	BaseLocation* b = areas[x];
	vector<Unit*>& v = abuildings[x];
	const Poly& p = b->getRegion()->getPolygon();

	int x0=1e9,x1=-1e9,y0=1e9,y1=-1e9;
	for(int i=0; i<sz(p); ++i) {
		Position a=p[i];
		TilePosition t(a);
		int x=t.x, y=t.y;
		x0 = min(x0,x);
		x1 = max(x1,x);
		y0 = min(y0,y);
		y1 = max(y1,y);
	}
	TilePosition best;
	int bv=-1e9;
	UnitType ut(type);
	for(int y=y0; y<=y1; ++y) {
		for(int x=x0; x<=x1; ++x) {
			TilePosition t(x,y);
			if (Broodwar->canBuildHere(0, t, ut)) {
				int v = evalBP<type>(t.x(),t.y());
				if (v>bv) bv=v, best=t;
			}
		}
	}
	Position pos(best);
	Unit* bd=0;
	for(int i=0; i<sz(probes); ++i) {
		Unit* u=probes[i];
		if (!bd || u->getDistance(pos) < bd->getDistance(pos)) bd=u;
	}
	if (bd) {
		bd->build(best, ut);
		return 1;
	}
	return 0;
}

void Bot::onStart()
{
	readMap();
	analyze();

	const set<BaseLocation*>& bs = getBaseLocations();
	areas.insert(areas.end(), bs.begin(), bs.end());
	aunits.resize(bs.size());
	NA=bs.size();
}

void updateUnitList() {
	units.clear();
	const USet& uns = Broodwar->self()->getUnits();
	for(USCI it = uns.begin(); it != uns.end(); ++it) {
		Unit* u = *it;
		units.push_back(u);
	}

	for(int i=0; i<NA; ++i) aunits[i].clear();
	for(int i=0; i<sz(units); ++i) {
		Unit* u = units[i];
		int j;
		for(j=0; j<NA; ++j) {
			const BWTA::Polygon& p = areas[j]->getRegion()->getPolygon();
			if (p.isInside(u->getPosition())) break;
		}
		if (j==NA) {
			Broodwar->printf("unit outside all areas...");
			continue;
		}
		aunits[j].push_back(u);
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