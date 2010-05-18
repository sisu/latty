#include "Bot.hpp"
#include "Vec.h"
#include <set>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <cmath>

#define M_PI 3.14159265358979323846

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

const double D = 100;
const double RAD = 400;

struct Scout {
	Unit* u;
	Position initial_target, target;

	Scout() {}

	Scout(Unit* u, Position p) {
		this->u = u;
		initial_target = target = p;
		u->rightClick(target);
	}

	void find_target() {
		Broodwar->printf("Dist to target: %f\n",u->getDistance(target));
		if(u->getDistance(target) < D) {
			// create new target
			int mult = rand() % 8;
			Position p;
			p.x() = initial_target.x() + int(cos(M_PI*mult/4) * RAD);
			p.y() = initial_target.y() + int(sin(M_PI*mult/4) * RAD);

			target = p;
			u->rightClick(target);
		}
	}
};

vector<Scout> scouts;

UVec probes;
UVec units;
UVec minerals;

vector<Region*> areas;
vector<BaseLocation*> bases;
vector<UVec> aunits;
vector<UVec> abuildings;
vector<UVec> aminerals;
vector<Unit*> nexuses;
int NA; // areas
int NB; // bases
//bool done = false;
int myStart, enemyStart;

int myMnr; // non-allocated minerals
map<Unit*,pair<int,UnitType> > startingBuild;
int comingCnt[512];
int comingSupply;
int curCnt[512];

bool forbidden[512][512];

bool okPlace(int a, TilePosition t)
{
	return !forbidden[t.y()][t.x()];
}

const int PYLON=156, GATEWAY=160, NEXUS=154, CYBER=164, FORGE=166, ASSIMILATOR=157;
const int PROBE=64, ZEALOT=65, DRAGOON=66;

template<int t>
int evalBP(int a, TilePosition t){return 0;}

int evalBB(int a, TilePosition t, int fd, int fn, int fc)
{
	Unit* nex = nexuses[a];
	if (nex) {
		int d = (int)nex->getDistance(t);
		double r = fn*abs(d-fd);
		// FIXME
		Chokepoint* c = *areas[a]->getChokepoints().begin();
		r += fc*c->getCenter().getDistance(t);
		return (int)r;
	}
	return 0;
}

template<>
int evalBP<PYLON>(int a, TilePosition t)
{
	return evalBB(a,t,1,-50,-1);
}
template<>
int evalBP<GATEWAY>(int a, TilePosition t)
{
	return evalBB(a,t,0,-4,-2);
}
template<>
int evalBP<FORGE>(int a, TilePosition t)
{
	return evalBB(a,t,0,-50,-1);
}
template<>
int evalBP<CYBER>(int a, TilePosition t)
{
	return evalBB(a,t,0,-5,-1);
}
template<> int evalBP<ASSIMILATOR>(int a, TilePosition t) {
	return evalBB(a,t,0,-1,0);
}

template<int type>
bool makeBuilding(int z)
{
	UnitType ut(type);
	if (ut.mineralPrice() > myMnr) return 0;
//	Broodwar->printf("Building to %d: %s",z,ut.getName().c_str());
	Region* r = areas[z];
	const Poly& p = r->getPolygon();

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
		if (startingBuild.count(u)) continue;
		if (u->isConstructing()) continue;
		if (!bd || u->getDistance(pos) < bd->getDistance(pos)) bd=u;
	}
	if (bd) {
//		Broodwar->printf("ordering builder %d %d ; %d", bd->getPosition().x(), bd->getPosition().y(), bd->getType());
//		bd->build(best, ut);
		bool ok = bd->build(best, ut);
		if (!ok) Broodwar->printf("BUILDING FAILED %s", ut.getName().c_str());
		else myMnr -= ut.mineralPrice(), startingBuild[bd]=make_pair(0,ut);
		return ok;
	}
	Broodwar->printf("failed building");
	return 0;
}
#if 0
template<int T>
bool makeBuilding()
{
	//FIXME
	return makeBuilding<T>(myStart);
}
#endif
double bestPrior;
bool checkPrior(double prior)
{
	if (bestPrior<0) bestPrior=prior;
	return bestPrior-prior<.4;
}
template<int T>
bool makeBuilding(double prior)
{
	if (!checkPrior(prior)) return 0;
	return makeBuilding<T>(myStart);
}
bool makeProbe(double value)
{
	if (!checkPrior(value)) return 0;
	if (myMnr<50) return 0;
	Unit* best=0;
	int bv=-99999;
	for(int i=0; i<sz(nexuses); ++i) {
		Unit* u=nexuses[i];
		if (!u || !u->exists() || !u->isCompleted()) continue;
		if (u->isTraining()) continue;
		// FIXME
		int v=0;
		if (v>bv) bv=v, best=u;
	}
	if (best) {
		bool ok = best->train(Protoss_Probe);
		if (ok) myMnr -= 50;
		return ok;
	}
	return 0;
}
bool makeUnit(UnitType t, double val)
{
	if (!checkPrior(val)) return 0;
	if (t.mineralPrice()>myMnr) return 0;
	Unit* best=0;
	int bv=-99999;
	for(int i=0; i<sz(units); ++i) {
		Unit* u=units[i];
		if (u->getType() != Protoss_Gateway) continue;
		if (!u || !u->exists() || !u->isCompleted()) continue;
		if (u->isTraining()) continue;
		// FIXME
		int v=0;
		if (v>bv) bv=v, best=u;
	}
	if (best) {
		bool ok = best->train(t);
		if (ok) myMnr -= t.mineralPrice();
		Broodwar->printf("MAKING UNIT %s: %d", t.getName().c_str(), ok);
		return ok;
	}
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
		const int n=10;
		double dx=(ex-x)/n, dy=(ey-y)/n;
		for(int i=0; i<n; ++i) {
			int ix=int(x), iy=int(y);
			const int d=3;
//			Broodwar->printf("banning around %d %d", ix,iy);
			for(int i=-d; i<=d; ++i) for(int j=-d; j<=d; ++j)
				forbidden[iy+i][ix+j]=1;
			x+=dx, y+=dy;
		}
	}
}

void updateMineralList();
void updateUnitList() {
	units.clear();
	const USet& uns = Broodwar->self()->getUnits();
	for(USCI it = uns.begin(); it != uns.end(); ++it) {
		Unit* u = *it;
		units.push_back(u);
	}

	for(int i=0; i<NA; ++i) aunits[i].clear(), nexuses[i]=0;
	memset(comingCnt,0,sizeof(comingCnt));
	memset(curCnt,0,sizeof(curCnt));
	for(int i=0; i<sz(units); ++i) {
		Unit* u = units[i];
		int j;
		for(j=0; j<NA; ++j) {
			const BWTA::Polygon& p = areas[j]->getPolygon();
			if (p.isInside(u->getPosition())) break;
		}
		if (j==NA) {
			Broodwar->printf("unit outside all areas...");
			continue;
		}
		aunits[j].push_back(u);
		if (u->getType()==UnitTypes::Protoss_Nexus)
			nexuses[j]=u;

		++comingCnt[u->getType().getID()];
		++curCnt[u->getType().getID()];
	}

	myMnr=Broodwar->self()->minerals();
	for(int i=0; i<sz(units); ++i) {
		Unit* u = units[i];
		if (u->getType()!=Protoss_Probe) continue;
		if (u->isConstructing() && u->getBuildType()!=None) {
			myMnr -= u->getBuildType().mineralPrice();
			++comingCnt[u->getBuildType().getID()];
		} else if (startingBuild.count(u)) {
			myMnr -= startingBuild[u].second.mineralPrice();
			++comingCnt[startingBuild[u].second.getID()];
		}
	}
	for(map<Unit*,pair<int,UnitType> >::iterator i=startingBuild.begin(); i!=startingBuild.end(); ) {
		int& x = i->second.first;
		Unit* u = i->first;
		if (x>10 || (u->isConstructing() && u->getBuildType()!=None)) {
			i = startingBuild.erase(i);
		} else ++x, ++i;
	}
	comingSupply = 9 + 8*comingCnt[Protoss_Pylon.getID()];
}

void updateProbeList() {
	probes.clear();
	for(UVI it = units.begin(); it != units.end(); ++it) {
		Unit* u = *it;

		bool ok = true;

		for(int i = 0; i < sz(scouts); ++i) {
			if(scouts[i].u == u) {
				ok = false;
				break;
			}
		}

		if (ok && u->getType()==Protoss_Probe) probes.push_back(u);
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
				const Poly& p = areas[a]->getPolygon();
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
Unit* nearestGasSource(Unit* u)
{
	Unit* b=0;
	double bd=1e50;
	for(int i=0; i<sz(units); ++i) {
		Unit* z=units[i];
		if (z->getType()!=Protoss_Assimilator) continue;
		if (!z->exists() || z->isBeingConstructed()) continue;
		double d=u->getDistance(z);
		if (d<bd) bd=d, b=z;
	}
	return b;
}

void taskifyProbes() {
	int gas = min((int)log(1.+sz(probes)), 3*curCnt[ASSIMILATOR]);
	for(int i = 0; i < sz(probes); ++i) {
		if(!probes[i]->isIdle()) continue;
		if (gas) {
			Unit* u = nearestGasSource(probes[i]);
			if(u) {
				probes[i]->rightClick(u);
				--gas;
			} else {
				Unit* u = nearestMineralSource(probes[i]);
				if (u) 
					probes[i]->rightClick(u);
			}
		} else {
			Unit* u = nearestMineralSource(probes[i]);
			if(u) {
				probes[i]->rightClick(u);
			}
		}
	}
}

struct Action {
	double value;
	virtual void exec() = 0;
};
bool cmpA(const Action* a, const Action* b)
{
	return a->value > b->value;
}
struct MkProbeA: Action {
	MkProbeA() {
		value=60./(5+2*comingCnt[PROBE]);
	}
	void exec() {
		makeProbe(value);
	}
};
struct MkPylonA: Action {
	MkPylonA() {
		int useds = Broodwar->self()->supplyUsed()/2;
		value=15./(4+5*(comingSupply - useds));
		if (useds>=comingSupply-2) value += 40;
		else if (comingSupply>=useds+10) value = -1;
		if (comingSupply>=200) value=-1;
	}
	void exec() {
		makeBuilding<PYLON>(value);
	}
};
struct MkGatewayA: Action {
	MkGatewayA() {
		int c = comingCnt[GATEWAY];
		value = 30./(4+10*c);
		if (!curCnt[PYLON]) value=-1;
	}
	void exec() {
		makeBuilding<GATEWAY>(value);
	}
};
struct MkFighterA: Action {
	MkFighterA() {
		int a=comingCnt[ZEALOT], b=comingCnt[DRAGOON];
		int t0=(int)1e9;
		for(int i=0; i<sz(units); ++i) {
			Unit* u = units[i];
			if (u->getType() != Protoss_Gateway) continue;
			if (!u->exists() || u->isBeingConstructed()) continue;
			if (!u->isTraining()) t0=0;
			else t0 = min(t0, u->getRemainingBuildTime());
		}
		double tt = (double)t0/UnitType(ZEALOT).buildTime();
		value=30./(5+10*log(1.0+a+b)) * (1-tt);
		if (!curCnt[GATEWAY]) value=-1;
	}
	void exec() {
		int a=comingCnt[ZEALOT], b=comingCnt[DRAGOON];
		if (3*a <= 2*b || Broodwar->self()->gas()<50 || !canMakeDragoon()) makeUnit(Protoss_Zealot,value);
		else makeUnit(Protoss_Dragoon,value);
	}
	static bool canMakeDragoon() {
		for(int i=0; i<sz(units); ++i) {
			Unit* u=units[i];
			if (u->getType()!=Protoss_Cybernetics_Core) continue;
			if (u->exists() && !u->isBeingConstructed()) return 1;
		}
		return 0;
	}
};
struct AttackA: Action {
	AttackA() {
		int a=curCnt[DRAGOON], b=curCnt[ZEALOT];
		if (a+b>5) value = 10*(a+b);
		else value=-1;
//		Broodwar->printf("attack %f",value);
	}
	void exec() {
//		Broodwar->printf("STARTING ATTACK");
		int target=enemyStart;
//		while(target<NB && bases[target] != getStartLocation(Broodwar->enemy())) ++target;
//		if (target==NB) Broodwar->printf("FAILED CHOOSING ATTACK TARGET");
		Position to=bases[target]->getPosition();
//		Broodwar->printf("Attack location: %d %d", to.x(), to.y());
		for(int i=0; i<sz(units); ++i) {
			Unit* u = units[i];
			if (!u->isIdle()) continue;
			UnitType t = u->getType();
			if (t!=Protoss_Dragoon && t!=Protoss_Zealot) continue;
			u->attackMove(to);
		}
	}
};
struct MkForgeA: Action {
	MkForgeA() {
		value=1;
		if (comingCnt[FORGE]) value=-1;
		if (!curCnt[PYLON]) value=-1;
	}
	void exec() {
		makeBuilding<FORGE>(value);
	}
};
struct MkCyberA: Action {
	MkCyberA() {
		value=1;
		if (comingCnt[CYBER] || !curCnt[FORGE] || !comingCnt[ASSIMILATOR]) value=-1;
		if (!curCnt[PYLON]) value=-1;
//		Broodwar->printf("cyber %f ; %d %d", value, comingCnt[CYBER], curCnt[FORGE]);
	}
	void exec() {
		makeBuilding<CYBER>(value);
	}
};
struct MkAssimA: Action {
	MkAssimA() {
		value=1;
		if (!curCnt[GATEWAY] || !curCnt[FORGE] || comingCnt[ASSIMILATOR]) value=-1;
	}
	void exec() {
		makeBuilding<ASSIMILATOR>(value);
	}
};
struct ScoutA: Action {
	ScoutA() {
		value=-1;
	}
	void exec() {
	}
};

void Bot::onFrame()
{
	updateUnitList();
	updateProbeList();
	updateMineralList();
//	startingBuild.clear();

	if(sz(scouts) + sz(probes) == 7 && sz(scouts) == 0) {
		Scout s(probes.back(),bases[enemyStart]->getPosition());
		probes.pop_back();
		scouts.push_back(s);
	}
	
	vector<Scout> newScouts;
	for(int i = 0; i < sz(scouts); ++i) {
		if(scouts[i].u->exists()) newScouts.push_back(scouts[i]); 
	}
	scouts = newScouts;
	for(int i = 0; i < sz(scouts); ++i) scouts[i].find_target();

	taskifyProbes();

	TilePosition btp = getStartLocation(Broodwar->self())->getTilePosition();
	btp.x() -= 5, btp.y() -= 5;
	//TilePosition ntp = TilePosition(btp.x()-5,btp.y()-5);

#if 0
	int useds = Broodwar->self()->supplyUsed()/2;
	if (useds < comingSupply-1) {
		if (myMnr >= 50) {
			makeProbe();
		}
	}
	if (useds>=comingSupply-2 && myMnr >= 100) {
		Broodwar->printf("need build; %d %d %d", comingSupply, comingCnt[PYLON], startingBuild.size());
		makeBuilding<PYLON>(myStart);
	}
#else
	bestPrior=-1;
	vector<Action*> as;
	as.push_back(new MkPylonA());
	as.push_back(new MkProbeA());
	as.push_back(new MkGatewayA());
	as.push_back(new MkFighterA());
	as.push_back(new ScoutA());
	as.push_back(new AttackA());
	as.push_back(new MkForgeA());
	as.push_back(new MkCyberA());
	as.push_back(new MkAssimA());

	sort(as.begin(),as.end(),cmpA);
//	as[0]->exec();
	for(int i=0; i<sz(as); ++i)
		if (as[i]->value>=0) as[i]->exec();

	for(int i=0; i<sz(as); ++i) delete as[i];


#endif
}

void Bot::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);
	Broodwar->setLocalSpeed(30);
	readMap();
	analyze();

	const set<BaseLocation*>& bs = getBaseLocations();
	const set<Region*>& rs = getRegions();
	NA=rs.size();
	NB=bs.size();
	Broodwar->printf("area count: %d %d", NA, NB);
//	areas.insert(areas.end(), rs.begin(), rs.end());
	bases.insert(bases.end(), bs.begin(), bs.end());
	areas.resize(NB);
	for(set<Region*>::const_iterator i=rs.begin(); i!=rs.end(); ++i) {
		Region* r = *i;
		int j;
		for(j=0; j<NB && bases[j]->getRegion()!=r; ++j);
		if (j<NB) areas[j]=r;
		else areas.push_back(r);
	}
	aunits.resize(NA);
	nexuses.resize(NA);
	aminerals.resize(NA);

	BaseLocation* start = getStartLocation(Broodwar->self());
	while(bases[myStart]!=start) ++myStart;
	BaseLocation* estart = getStartLocation(Broodwar->enemy());
	if (!estart) {
		enemyStart=0;
		int w=Broodwar->mapWidth()*TILE_SIZE, h=Broodwar->mapHeight()*TILE_SIZE;
		Position appr(w/2,100);
		if (bases[myStart]->getPosition().y() < h/2) appr.y()=h-100;
		for(int i=0; i<NB; ++i) {
			Position p = bases[i]->getPosition();
			if (appr.getDistance(p) < appr.getDistance(bases[enemyStart]->getPosition()))
				enemyStart=i;
		}
		Position p = bases[enemyStart]->getPosition();
		Broodwar->printf("guessing enemy start base: %d %d", p.x(),p.y());
	} else {
		while(bases[enemyStart]!=estart) ++enemyStart;
		Broodwar->printf("enemy start: %d %d", estart->getPosition().x(), estart->getPosition().y());
	}
	updateMineralList();
	const USet& uns = Broodwar->self()->getUnits();
	for(USCI it=uns.begin(); it!=uns.end(); ++it) {
		Unit* u=*it;
		if (u->getType()==UnitTypes::Protoss_Nexus) addNexus(myStart,u);
	}
}
