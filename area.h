#include <queue>

double danger[64];
bool myArea[64], enemyArea[64];

int owner[256];
bool borderArea[64];

vector<int> conn[64];

struct DangerS {
	int a;
	double d;
	bool operator<(const DangerS& s) const {
		return d>s.d;
	}
};
void calcDangerFrom(bool* start, bool* end, double f, double ff)
{
	bool dused[64]={};
	priority_queue<DangerS> q;
	for(int i=0; i<NA; ++i) {
		if (start[i]) {
			DangerS d={i,0};
			q.push(d);
		}
	}
	while(!q.empty()) {
		DangerS d=q.top();
		q.pop();
		if (dused[d.a]) continue;
		dused[d.a]=1;
		danger[d.a] += f/(1+d.d/ff);
		if (end[d.a]) continue;

		for(int i=0; i<sz(conn[d.a]); ++i) {
			int t=conn[d.a][i];
			if (dused[t]) continue;
			DangerS dd={t,d.d+areas[d.a]->getCenter().getDistance(areas[t]->getCenter())};
			q.push(dd);
		}
	}
}

void calcBorderArea()
{
	memset(danger,0,sizeof(danger));
	calcDangerFrom(enemyArea, myArea,1,1000);
	for(int i=0; i<NA; ++i) {
		if (!myArea[i]) continue;
		for(int j=0; j<sz(conn[i]); ++j) if (danger[conn[i][j]]>0) {
			borderArea[i]=1;
			Broodwar->printf("borderArea: %d", i);
		}
	}
	calcDangerFrom(myArea, enemyArea,-.8,2000);
}
void calcAreas()
{
	memset(myArea,0,sizeof(myArea));
	memset(enemyArea,0,sizeof(enemyArea));
	for(int i=0; i<NA; ++i) {
		int c=0;
		for(int j=0; j<sz(aunits[i]); ++j) {
			Unit* u = aunits[i][j];
			if (u->getType().isBuilding()) myArea[i]=1;
			if (u->getType()==Protoss_Zealot || u->getType()==Protoss_Dragoon)
				++c;
		}
		// FIXME: slow?
		int ec=0;
		const set<Unit*>& es = Broodwar->enemy()->getUnits();
		for(set<Unit*>::const_iterator j=es.begin(); j!=es.end(); ++j) {
			Unit* u = *j;
			if (u->getType()==Protoss_Zealot || u->getType()==Protoss_Dragoon
				|| u->getType()==Protoss_Photon_Cannon) {
				if (areas[i]->getPolygon().isInside(u->getPosition()))
					++ec;
			}
		}

		if (c>2*ec) myArea[i]=1;
//		if (danger[i]<=0) myArea[i]=1;
	}
	enemyArea[enemyStart]=1;
}
void analyzeAreaState()
{
	calcAreas();
	calcBorderArea();
}

void makeGraph()
{
	for(int i=0; i<NA; ++i) {
		const set<Chokepoint*> cs=areas[i]->getChokepoints();
		for(set<Chokepoint*>::const_iterator j=cs.begin(); j!=cs.end(); ++j) {
			Chokepoint* c=*j;
			pair<Region*,Region*> p = c->getRegions();
			int t = regNum[p.first==areas[i] ? p.second : p.first];
			conn[i].push_back(t);
		}
	}
}

#if 0
void expand()
{
	if (!battles.empty()) return;
	if (fighters.size()<2) return;
	int to=0;
	double bv=-1e9;
	for(int i=0; i<NA; ++i) {
		if (myArea[i]) continue;
		if (danger[i]<=0) continue;
		double v = -danger[i];
		if (v>bv) bv=v, to=i;
	}
	if (-bv < .1) {
		Position p = areas[to]->getCenter();
		Broodwar->printf("EXPANDING TO %d %d", p.x(),p.y());
		Battle b(p);
		battles.push_back(b);
	}
}
#endif
