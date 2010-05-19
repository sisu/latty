#include "Bot.hpp"
#include "Vec.h"
#include "battle.h"
#include <set>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <functional>

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

const double D = 50;
const double RAD = 300;

/*
int upperBaseScouting [][2] = {{72,18},{80,13},{82,5},{62,4},{61,13}};
int lowerBaseScouting [][2] = {{63,109},{56,113},{76,122},{71,113},{63,109}};
*/

int BSC = 9;
int BSSM = 3; // base scouting spawning magic

bool ownBaseDown = false;

int upperBaseScouting [][2] = {{56,10},{55,7},{62,4},{61,13},{72,18},{80,13},{82,5},{88,9},{86,12}};
int lowerBaseScouting [][2] = {{47,112},{45,117},{51,122},{56,113},{63,109},{71,113},{76,122},{81,119},{78,115}};

struct Scout {
	Unit* u;
	//Position initial_target, target;
	TilePosition target;
	bool usePreGivenAreas;
	bool upper;
	int curTarg;
	bool dir;

	Scout() {}

	Scout(Unit* u, bool upper, int targ) {
		this->u = u;
		usePreGivenAreas = true;
		curTarg = targ;
		this->upper = upper;
		dir = true;
		updateTarget();
	}
/*
	Scout(Unit* u, Position p) {
		this->u = u;
		initial_target = target = p;
		u->rightClick(target);
	}
*/
	void updateTarget() {
		if(upper) {
			target = TilePosition(upperBaseScouting[curTarg][0],upperBaseScouting[curTarg][1]);
		} else {
			target = TilePosition(lowerBaseScouting[curTarg][0],lowerBaseScouting[curTarg][1]);
		}
		u->rightClick(target);
	}

	void find_target() {
//		Broodwar->printf("Current target: %d",curTarg);
		if(u->getDistance(target) < D) {
			if(dir && curTarg == BSC - 1) {
				curTarg -= BSSM;
				dir = !dir;
			} else if(!dir && curTarg == 0) {
				curTarg += BSSM;
				dir = !dir;
			} else {
				if(dir) ++curTarg;
				else --curTarg;
			}
			updateTarget();
		}
		/*
		Broodwar->printf("Dist to target: %f\n",u->getDistance(target));
		if(u->getDistance(target) < D) {
			// create new target
			if(usePreGivenAreas) {
				
			} else {
				int mult = rand() % 8;
				Position p;
				p.x() = initial_target.x() + int(cos(M_PI*mult/4) * RAD);
				p.y() = initial_target.y() + int(sin(M_PI*mult/4) * RAD);

				target = p;
				u->rightClick(target);
			}
		}
		*/
	}
};

struct Builder {
	TilePosition pos;
	UnitType ut;
	Unit* u;
	bool aborted;

	Builder() {}
	Builder(Unit* u, TilePosition pos, UnitType ut) {
		this->u = u;
		this->pos = pos;
		this->ut = ut;
		aborted = false;
		u->rightClick(pos);
	}

	void tryToBuild() {
		double nearest = 1e9;
		for(USCI it = Broodwar->enemy()->getUnits().begin(); it != Broodwar->enemy()->getUnits().end(); ++it) {
			double dist = u->getDistance(*it);
			if(dist < nearest) nearest = dist;
		}

		if(nearest < 400) {
			u->stop();
			aborted = true;
		}
		if(u->getDistance(pos) < 100) {
			Broodwar->printf("Ma yritan rakentaa");
			u->build(pos,ut);
			aborted = true;
		}
	}
};

int frameCount = 0;
int lastScoutAlive = 0;

vector<Scout> scouts;
vector<Builder> builders;
set<Unit*> seenEnemyUnits;

bool scoutingOwnBase = false;
Scout ownBaseScout;
int obsLastFrame;

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

int owner[256];
map<Region*,int> regNum;
Chokepoint* borderLine;
int borderArea;

BW_Battle* battle;

bool okPlace(int a, TilePosition t)
{
	return !forbidden[t.y()][t.x()];
}

const int PYLON=156, GATEWAY=160, NEXUS=154, CYBER=164, FORGE=166, ASSIMILATOR=157, PHOTON=162;
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

template<> int evalBP<GATEWAY>(int a, TilePosition t) {
	return evalBB(a,t,0,-4,-2);
}
template<> int evalBP<FORGE>(int a, TilePosition t) {
	return evalBB(a,t,0,-50,-1);
}
template<> int evalBP<CYBER>(int a, TilePosition t) {
	return evalBB(a,t,0,-5,-1);
}
template<> int evalBP<ASSIMILATOR>(int a, TilePosition t) {
	return evalBB(a,t,0,-1,0);
}
template<> int evalBP<NEXUS>(int a, TilePosition t) {
	if (a>=NB) return (int)-1e9;
	return (int)-bases[a]->getPosition().getDistance(t);
}
template<> int evalBP<PHOTON>(int a, TilePosition t) {
	if (!borderLine) return 0;
	int d = (int)borderLine->getCenter().getDistance(t);
	return -abs(d-250);
}
template<> int evalBP<PYLON>(int a, TilePosition t) {
	if (a==borderArea) return evalBP<PHOTON>(a,t);
	return evalBB(a,t,1,-50,-1);
}

template<int type>
bool makeBuilding(int z)
{
	UnitType ut(type);
	myMnr -= ut.mineralPrice();
	if (myMnr<0) return 0;
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
			if (Broodwar->canBuildHere(0, t, ut) && okPlace(z,t)) {
				int v = evalBP<type>(z,t);
				if (v>bv) bv=v, best=t;
			}
		}
	}

	if(best == TilePosition()) {
		return 0;
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
		/*
		bool ok = bd->build(best, ut);
		if (!ok) Broodwar->printf("BUILDING FAILED %s", ut.getName().c_str());
		else startingBuild[bd]=make_pair(0,ut);
		return ok;
		*/
		remove(probes.begin(),probes.end(),bd);
		probes.pop_back();
		Builder bldr(bd,best,ut);
		builders.push_back(bldr);
		return true;
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
#if 0
	if (bestPrior<0) bestPrior=prior;
	return bestPrior-prior<.4;
#else
	return 1;
#endif
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
	myMnr -= 50;
	if (myMnr<0) return 0;
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
		return ok;
	}
	return 0;
}
bool makeUnit(UnitType t, double val)
{
	if (!checkPrior(val)) return 0;
	myMnr -= t.mineralPrice();
	if (myMnr<0) return 0;
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

typedef const set<Chokepoint*> CCPS;
bool aused[256];
template<class T, class Fst, class Comb>
T areaDFS(int n, Chokepoint* no, Fst fst, Comb comb)
{
//	Broodwar->printf("dfs %d", n);
	aused[n]=1;
	Region* r=areas[n];
	CCPS cps = r->getChokepoints();
	T res=fst(n);
	for(CCPS::const_iterator i=cps.begin(); i!=cps.end(); ++i) {
		Chokepoint* p=*i;
		if (p==no) continue;
		const pair<Region*,Region*> rs = p->getRegions();
		Region* other = rs.first==r ? rs.second : rs.first;
		int m = regNum[other];
//		Brood
		if (aused[m]) continue;
		res = comb(res, areaDFS<T>(m, no, fst, comb));
	}
	return res;
}

int const1(int ){return 1;}
int sizeDFS(int n, Chokepoint* no)
{
	memset(aused,0,sizeof(aused));
	return areaDFS<int>(n, no, const1, plus<int>());
}
typedef pair<int,int> IP;
IP getOwner(int n){return owner[n]>0 ? IP(1,0) : owner[n]<0 ? IP(0,1) : IP(0,0);}
IP combOwner(IP a, IP b){return IP(a.first+b.first, a.second+b.second);}
IP ownerDFS(int n, Chokepoint* no)
{
	memset(aused,0,sizeof(aused));
	return areaDFS<IP>(n, no, getOwner, combOwner);
}
void calcBorders()
{
	// TODO: multiple borders
	const CCPS cps = getChokepoints();
	Chokepoint* bc=0;
	int br=0;
	int bv=-999;
	for(CCPS::const_iterator i=cps.begin(); i!=cps.end(); ++i) {
		Chokepoint* p = *i;
		pair<Region*,Region*> rs = p->getRegions();
		int a=regNum[rs.first], b=regNum[rs.second];
		int sa = sizeDFS(a, p);
//		Broodwar->printf("sa %d (%d,%d)", sa, a, b);
		if (aused[b]) continue;
		int sb = sizeDFS(b, p);
		IP oa = ownerDFS(a, p);
		IP ob = ownerDFS(b, p);
//		Broodwar->printf("lol %d %d ; %d %d ; %d %d", sa, sb, oa.first,oa.second,ob.first,ob.second);
		if (oa.first && oa.second) continue;
		if (ob.first && ob.second) continue;
		if (oa.first) {
			if (sa>bv) bv=sa, br=a, bc=p;
		} else {
			if (sb>bv) bv=sb, br=b, bc=p;
		}
	}
	borderLine=bc;
	borderArea=br;
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
		if (!u->isBeingConstructed()) ++curCnt[u->getType().getID()];
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
void taskifyFighters()
{
	if (!borderLine) return;
	Position to = borderLine->getCenter();
	for(int i=0; i<sz(units); ++i) {
		Unit* u = units[i];
		UnitType t=u->getType();
		if (t!=Protoss_Zealot && t!=Protoss_Dragoon) continue;
		if (!u->isIdle()) continue;
		if (u->getDistance(to)>100) u->attackMove(to);
	}
}
void updateBattle()
{
	if (!battle) return;
	const USet& es = Broodwar->enemy()->getUnits();
	for(USCI i=es.begin(); i!=es.end(); ++i) {
		battle->addUnit(*i);
	}
	battle->tick();

	if (battle->myUnits.size() < 3) {
		delete battle;
		battle=0;
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
		int cc = curCnt[GATEWAY];
		int d=0;
		for(int i=0; i<sz(units); ++i) {
			Unit* u = units[i];
			if (u->getType()!=Protoss_Gateway || u->isBeingConstructed()) continue;
			d += u->isTraining();
		}
		double r= cc ? (double)d/cc : 0;
		value = (1-r)*40./(4+12*c);
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

double armyValue() {
	return curCnt[ZEALOT]+curCnt[DRAGOON]*1.5;
}

double enemyArmyValue(double dist) {
	// according to scouting
	
	// calculate number of gateways (TODO: approximate resources?)

	int gatewayCount = 0;
	int zealotCount = 0;
	int dragoonCount = 0;

	for(USCI it = seenEnemyUnits.begin(); it != seenEnemyUnits.end(); ++it) {
		if((*it)->getType() == Protoss_Gateway) {
			++gatewayCount;
		}
		if((*it)->getType() == Protoss_Zealot) {
			++zealotCount;
		}
		if((*it)->getType() == Protoss_Dragoon) {
			++dragoonCount;
		}
	}
	return gatewayCount * dist / 2000.0 + zealotCount + dragoonCount * 1.5;
}

int unitsOnArea(int a, UnitType t)
{
	int r=0;
	for(int i=0; i<sz(aunits[a]); ++i) {
		Unit* u=aunits[a][i];
		if (u->getType()==t) ++r;
	}
	return r;
}

struct AttackA: Action {
	AttackA() {
		pair<Region*,Region*> p = borderLine->getRegions();
		int x=regNum[p.first], y=regNum[p.second];
		int a=unitsOnArea(x, Protoss_Zealot) + unitsOnArea(y, Protoss_Zealot);
		int b=unitsOnArea(x, Protoss_Dragoon) + unitsOnArea(y, Protoss_Dragoon);
		if (a+b>5) value = 10*(a+b);
		else value = -1;

		if(lastScoutAlive + 400 > frameCount) {
			int target=enemyStart;
			Position to=bases[target]->getPosition();

			double dist = 0;
			int cnt = 0;

			for(int i = 0; i < sz(units); ++i) {
				Unit* u = units[i];
				UnitType t = u->getType();
				if(t != Protoss_Dragoon && t != Protoss_Zealot) continue;
				++cnt;
				dist += u->getDistance(to);
			}

			dist /= cnt;

			double eav = 0, av = 0;
			eav = enemyArmyValue(dist);
			av = armyValue();

			if (eav > av) value = -1;
		}

		if (battle) value=-1;
	}

	void exec() {
		int target=enemyStart;
		Position to=bases[target]->getPosition();
		battle = new BW_Battle;
		for(int i = 0; i < sz(units); ++i) {
			Unit* u = units[i];
			if(!u->isIdle()) continue;
			UnitType t = u->getType();
			if(t != Protoss_Dragoon && t != Protoss_Zealot) continue;
			u->attackMove(to);
			battle->addUnit(u);
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
struct MkNexusA: Action {
	MkNexusA() {
		const int P_PER_B = 20;
		double rat = curCnt[PROBE] / (double)comingCnt[NEXUS];
		value=rat - P_PER_B;
		// FIXME
		value=-1;
	}
	void exec() {
		int to=0;
		int bv=-999999;
		Position strt=bases[myStart]->getPosition();
		for(int i=0; i<NB; ++i) {
			Unit* u = nexuses[i];
			if (u && u->exists()) continue;
			int v=(int)-strt.getDistance(bases[i]->getPosition());
			if (v>bv) bv=v, to=i;
		}
		makeBuilding<NEXUS>(to);
	}
};
struct MkPhotonA: Action {
	MkPhotonA() {
		int t = borderArea;
		int c=0;
		for(int i=0; i<sz(aunits[t]); ++i) {
			Unit* u = aunits[t][i];
			if (u->getType()==Protoss_Photon_Cannon) ++c;
		}
		value = 30./(4+10*c);
//		Broodwar->printf("support: %d", hasSupport());
		if (!curCnt[FORGE] || !hasSupport()) value=-1;
	}
	void exec() {
//		Broodwar->printf("Photon??? %d", myMnr);
		makeBuilding<PHOTON>(borderArea);
	}
	static bool hasSupport() {
		int t = borderArea;
		for(int i=0; i<sz(aunits[t]); ++i) {
			Unit* u = aunits[t][i];
			if (u->getType()==Protoss_Pylon && !u->isBeingConstructed()) return 1;
		}
		return 0;
	}
};
struct SupportPylonA: Action {
	SupportPylonA() {
		int a=curCnt[DRAGOON], b=curCnt[ZEALOT];
		int c=0;
		for(int i=0; i<sz(aunits[borderArea]); ++i) {
			Unit* u = aunits[borderArea][i];
			if (u->getType()==Protoss_Pylon) ++c;
		}
		for(int i=0; i<sz(probes); ++i) {
			Unit* u = probes[i];
			if (u->isConstructing() && u->getBuildType()==Protoss_Pylon) {
				Position to = u->getTargetPosition();
				if (areas[borderArea]->getPolygon().isInside(to)) ++c;
			}
		}

		int pc=comingCnt[PHOTON];
		double r = (.1+pc)/(.01+c);
		value=(r-6)*20*log(1.+a+b)/(5+4*c);
		if (!comingCnt[FORGE]) value=-1;
	}
	void exec() {
		int to=borderArea;
		makeBuilding<PYLON>(to);
	}
};
struct UpgradeA: Action {
	UpgradeA() {
		value=.5*curCnt[ZEALOT];
		if (!curCnt[FORGE]) value=-1;
		Unit* f=0;
		for(int i=0; i<sz(units); ++i) if (units[i]->getType()==Protoss_Forge) f=units[i];
		if (!f || f->isUpgrading()) value=-1;
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Plasma_Shields)) value=-1;
	}
	void exec() {
		using namespace UpgradeTypes;
		static UpgradeType upgrades[]={Protoss_Armor,Protoss_Ground_Weapons,Protoss_Plasma_Shields};
		Player* s = Broodwar->self();
		Unit* f=0;
		for(int i=0; i<sz(units); ++i) if (units[i]->getType()==Protoss_Forge) f=units[i];
		UpgradeType us[]={Protoss_Armor,Protoss_Ground_Weapons,Protoss_Plasma_Shields};
		for(int i=0; i<3; ++i) {
			UpgradeType t=us[i];
			if (!s->getUpgradeLevel(t)) {
				f->upgrade(t), myMnr-=t.mineralPriceBase();
				break;
			}
		}
	}
};
struct RangeUpA: Action {
	RangeUpA() {
		value=2*curCnt[DRAGOON];
		if (!curCnt[CYBER]) value=-1;
		Unit* c=0;
		for(int i=0; i<sz(units); ++i) if (units[i]->getType()==Protoss_Cybernetics_Core) c=units[i];
		if (!c || c->isUpgrading()) value=-1;
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge)) value=-1;
	}
	void exec() {
		Unit* c=0;
		for(int i=0; i<sz(units); ++i) if (units[i]->getType()==Protoss_Cybernetics_Core) c=units[i];
		c->upgrade(UpgradeTypes::Singularity_Charge);
	}
};

void Bot::onFrame()
{
	updateUnitList();
	updateProbeList();
	updateMineralList();
//	startingBuild.clear();

	copy(Broodwar->enemy()->getUnits().begin(),Broodwar->enemy()->getUnits().end(),inserter(seenEnemyUnits,seenEnemyUnits.begin()));
//	Broodwar->printf("Total enemy units seen: %d",seenEnemyUnits.size());

	

	if(sz(scouts) + sz(probes) == 7 && sz(scouts) == 0) {
		//Scout s(probes.back(),bases[enemyStart]->getPosition());
		bool attackUp = true;
		if(bases[myStart]->getPosition().y() < Broodwar->mapHeight()/2*TILE_SIZE)
			attackUp = false;
		Scout s(probes.back(),attackUp,0);
		probes.pop_back();
		scouts.push_back(s);
	}
	
	vector<Scout> newScouts;
	for(int i = 0; i < sz(scouts); ++i) {
		if(scouts[i].u->exists()) newScouts.push_back(scouts[i]); 
	}
	if(scouts.size() && !newScouts.size()) {
		lastScoutAlive = frameCount;
	}
	scouts = newScouts;
	for(int i = 0; i < sz(scouts); ++i) scouts[i].find_target();

	vector<Builder> newBuilders;
	for(int i = 0; i < sz(builders); ++i) {
		if(builders[i].u->exists()) newBuilders.push_back(builders[i]);
	}

	builders = newBuilders;
	newBuilders.clear();

	for(int i = 0; i < sz(builders); ++i) {
		if(builders[i].aborted) probes.push_back(builders[i].u);
		else newBuilders.push_back(builders[i]);
	}

	builders = newBuilders;

	for(int i = 0; i < sz(builders); ++i) builders[i].tryToBuild();

	if(frameCount % (24*90) == 0 && frameCount > 24*60 && !scoutingOwnBase) {
		if(probes.size() != 0) {
			scoutingOwnBase = true;
			TilePosition tp;
			if(ownBaseDown) {
				tp = TilePosition(lowerBaseScouting[0][0],lowerBaseScouting[0][1]);
			} else {
				tp = TilePosition(upperBaseScouting[0][0],upperBaseScouting[0][1]);
			}
			
			double nearest = 1e9;
			Unit* nearUnit = NULL;

			for(int i = 0; i < sz(probes); ++i) {
				double dist = probes[i]->getDistance(tp);
				if(dist < nearest) {
					nearest = dist;
					nearUnit = probes[i];
				}
			}

			remove(probes.begin(),probes.end(),nearUnit);
			probes.pop_back();

			ownBaseScout = Scout(nearUnit,!ownBaseDown,0);
			obsLastFrame = frameCount + 24*30;
		}
	}

	if(scoutingOwnBase && obsLastFrame < frameCount) {
		scoutingOwnBase = false;
		ownBaseScout.u->stop();
		probes.push_back(ownBaseScout.u);
	} else if(scoutingOwnBase) {
		Broodwar->printf("Scouting own base");
		ownBaseScout.find_target();
	}

	taskifyProbes();
	taskifyFighters();
	updateBattle();

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
	as.push_back(new AttackA());
	as.push_back(new MkForgeA());
	as.push_back(new MkCyberA());
	as.push_back(new MkAssimA());
	as.push_back(new MkNexusA());
	as.push_back(new MkPhotonA());
	as.push_back(new SupportPylonA());
	as.push_back(new UpgradeA());
	as.push_back(new RangeUpA());

	sort(as.begin(),as.end(),cmpA);
//	as[0]->exec();
	for(int i=0; i<sz(as); ++i)
		if (as[i]->value>=0) as[i]->exec();

	for(int i=0; i<sz(as); ++i) delete as[i];


#endif

	void drawMap();
	drawMap();

	++frameCount;
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
		if (j<NB) areas[j]=r, regNum[r]=j;
		else regNum[r]=areas.size(), areas.push_back(r);
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

	owner[myStart]=1;
	owner[enemyStart]=-1;
	calcBorders();

	if (borderLine) {
		Position mid = borderLine->getCenter();
		Broodwar->printf("Borderline: %d %d", mid.x(), mid.y());
	} else {
		Broodwar->printf("NO BORDERLINE???");
	}


	if(bases[myStart]->getPosition().y() < Broodwar->mapHeight()/2*TILE_SIZE) {
		ownBaseDown = false;
	} else ownBaseDown = true;
}

void drawMap()
{
   for(std::set<BWTA::BaseLocation*>::const_iterator i=BWTA::getBaseLocations().begin();i!=BWTA::getBaseLocations().end();i++)
    {
      TilePosition p=(*i)->getTilePosition();
      Position c=(*i)->getPosition();

      //draw outline of center location
      Broodwar->drawBox(CoordinateType::Map,p.x()*32,p.y()*32,p.x()*32+4*32,p.y()*32+3*32,Colors::Blue,false);

      //draw a circle at each mineral patch
      for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getStaticMinerals().begin();j!=(*i)->getStaticMinerals().end();j++)
      {
        Position q=(*j)->getInitialPosition();
        Broodwar->drawCircle(CoordinateType::Map,q.x(),q.y(),30,Colors::Cyan,false);
      }

      //draw the outlines of vespene geysers
      for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getGeysers().begin();j!=(*i)->getGeysers().end();j++)
      {
        TilePosition q=(*j)->getInitialTilePosition();
        Broodwar->drawBox(CoordinateType::Map,q.x()*32,q.y()*32,q.x()*32+4*32,q.y()*32+2*32,Colors::Orange,false);
      }

      //if this is an island expansion, draw a yellow circle around the base location
      if ((*i)->isIsland())
      {
        Broodwar->drawCircle(CoordinateType::Map,c.x(),c.y(),80,Colors::Yellow,false);
      }
    }
    
    //we will iterate through all the regions and draw the polygon outline of it in green.
    for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
    {
      BWTA::Polygon p=(*r)->getPolygon();
      for(int j=0;j<(int)p.size();j++)
      {
        Position point1=p[j];
        Position point2=p[(j+1) % p.size()];
        Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Green);
      }
    }

    //we will visualize the chokepoints with red lines
    for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
    {
      for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
      {
        Position point1=(*c)->getSides().first;
        Position point2=(*c)->getSides().second;
        Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
      }
    }
}