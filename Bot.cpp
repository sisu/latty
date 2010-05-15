#include "Bot.hpp"
#include "Vec.h"
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
vector<UVec> aunits;
vector<UVec> abuildings;
vector<UVec> aminerals;
vector<Unit*> nexuses;
int NA;
bool done = false;
int startArea;

bool forbidden[512][512];

bool okPlace(int a, TilePosition t)
{
	return !forbidden[t.y()][t.x()];
}

const int PYLON = 156, GATEWAY=160, NEXUS=154;

template<int t>
int evalBP(int a, TilePosition t){return 0;}

int evalBB(int a, TilePosition t, int fd, int fn, int fc)
{
	Unit* nex = nexuses[a];
	if (nex) {
		int d = (int)nex->getDistance(t);
		double r = fn*abs(d-fd);
		Chokepoint* c = *areas[a]->getRegion()->getChokepoints().begin();
		r += fc*c->getCenter().getDistance(t);
		return (int)r;
	}
	return 0;
}

template<>
int evalBP<PYLON>(int a, TilePosition t)
{
	return evalBB(a,t,5,-5,-1);
}
template<>
int evalBP<GATEWAY>(int a, TilePosition t)
{
	return evalBB(a,t,0,-4,-2);
}

template<int type>
bool addBuilding(int z)
{
	UnitType ut(type);
	Broodwar->printf("Building to %d: %s",z,ut.getName().c_str());
	BaseLocation* b = areas[z];
	const Poly& p = b->getRegion()->getPolygon();

	int x0=(int)1e9,x1=(int)-1e9,y0=(int)1e9,y1=(int)-1e9;
	for(int i=0; i<sz(p); ++i) {
		Position a=p[i];
		TilePosition t(a);
		int x=t.x(), y=t.y();
		x0 = min(x0,x);
		x1 = max(x1,x);
		y0 = min(y0,y);
		y1 = max(y1,y);
	}
	TilePosition best;
	int bv=(int)-1e9;
	for(int y=y0; y<=y1; ++y) {
		for(int x=x0; x<=x1; ++x) {
			TilePosition t(x,y);
			if (Broodwar->isExplored(t) && Broodwar->canBuildHere(0, t, ut) && okPlace(z,t)) {
				int v = evalBP<type>(z,t);
				if (v>bv) bv=v, best=t;
			}
		}
	}
	Position pos(best);
	Broodwar->printf("build place %d %d\n", pos.x(), pos.y());
	Unit* bd=0;
	for(int i=0; i<sz(probes); ++i) {
		Unit* u=probes[i];
		if (!bd || u->getDistance(pos) < bd->getDistance(pos)) bd=u;
	}
	if (bd) {
//		Broodwar->printf("ordering builder %d %d ; %d", bd->getPosition().x(), bd->getPosition().y(), bd->getType());
//		bd->build(best, ut);
		bool ok = bd->build(best, Protoss_Pylon);
		if (!ok) Broodwar->printf("BUILDING FAILED");
		return 1;
	}
	Broodwar->printf("failed building");
	return 0;
}

void addNexus(int a, Unit* u)
{
	TilePosition t = u->getTilePosition();
	for(int i=0; i<sz(aminerals[a]); ++i) {
		Unit* m=aminerals[a][i];
		TilePosition e = m->getTilePosition();
		double x=t.x(), y=t.y();
		double ex=e.x(), ey=e.y();
		const int n=50;
		double dx=(ex-x)/n, dy=(ey-y)/n;
		for(int i=0; i<20; ++i) {
			int ix=int(x), iy=int(y);
			const int d=5;
			Broodwar->printf("banning around %d %d", ix,iy);
			for(int i=-d; i<=d; ++i) for(int j=-d; j<=d; ++j)
				forbidden[iy+i][ix+j]=1;
			x+=dx, y+=dy;
		}
	}
}

void updateMineralList();
void Bot::onStart()
{
	Broodwar->setLocalSpeed(30);
	readMap();
	analyze();

	const set<BaseLocation*>& bs = getBaseLocations();
	NA=bs.size();
	Broodwar->printf("area count: %d", NA);
	areas.insert(areas.end(), bs.begin(), bs.end());
	aunits.resize(NA);
	nexuses.resize(NA);
	aminerals.resize(NA);

	BaseLocation* start = getStartLocation(Broodwar->self());
	while(areas[startArea]!=start) ++startArea;
	updateMineralList();
	const USet& uns = Broodwar->self()->getUnits();
	for(USCI it=uns.begin(); it!=uns.end(); ++it) {
		Unit* u=*it;
		if (u->getType()==UnitTypes::Protoss_Nexus) addNexus(startArea,u);
	}
}

void updateUnitList() {
	units.clear();
	const USet& uns = Broodwar->self()->getUnits();
	for(USCI it = uns.begin(); it != uns.end(); ++it) {
		Unit* u = *it;
		units.push_back(u);
	}

	for(int i=0; i<NA; ++i) aunits[i].clear(), nexuses[i]=0;
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
		if (u->getType()==UnitTypes::Protoss_Nexus)
			nexuses[j]=u;
	}
}

void updateProbeList() {
	probes.clear();
	for(UVI it = units.begin(); it != units.end(); ++it) {
		Unit* u = *it;
		if (u->getType()==Protoss_Probe) probes.push_back(u);
	}
}

void updateMineralList() {
	const USet& gsm = Broodwar->getStaticMinerals();
	minerals.clear();
	for(USCI it = gsm.begin(); it != gsm.end(); ++it) {
		Unit* u = *it;
		if(u->getType() == Resource_Mineral_Field) {
			minerals.push_back(u);

			int a;
			for(a=0; a<sz(areas); ++a) {
				const Poly& p = areas[a]->getRegion()->getPolygon();
				if (p.isInside(u->getPosition())) break;
			}
			if (a<sz(areas)) aminerals[a].push_back(u);
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
	
	TilePosition btp = getStartLocation(Broodwar->self())->getTilePosition();
	btp.x() -= 5, btp.y() -= 5;
	//TilePosition ntp = TilePosition(btp.x()-5,btp.y()-5);

	if(Broodwar->self()->minerals() >= 100) {
		if (!done) {
//		units[rand() % sz(units)]->build(btp,Protoss_Pylon);
			done = addBuilding<PYLON>(startArea);
		}
	} else done=0;
}