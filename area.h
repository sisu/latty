#include <queue>

double danger[64];
bool myArea[64], enemyArea[64];

int owner[256];
bool borderArea[64];

vector<int> conn[64];

#if 0
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
#endif

struct DangerS {
	int a;
	double d;
	bool operator<(const DangerS& s) const {
		return d>s.d;
	}
};
void calcDangerFrom(bool* start, bool* end, double f)
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
		danger[d.a] += f/(1+d.d/1000.);
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
	calcDangerFrom(enemyArea, myArea,1);
	for(int i=0; i<NA; ++i) {
		if (!myArea[i]) continue;
		for(int j=0; j<sz(conn[i]); ++j) if (danger[conn[i][j]]>0) borderArea[i]=1;
	}
	calcDangerFrom(myArea, enemyArea,-1);
}
void calcAreas()
{
	memset(myArea,0,sizeof(myArea));
	memset(enemyArea,0,sizeof(enemyArea));
	for(int i=0; i<NA; ++i) {
		for(int j=0; j<sz(aunits[i]); ++j) {
			Unit* u = aunits[i][j];
			if (u->getType().isBuilding()) myArea[i]=1;
		}
		enemyArea[enemyStart]=1;
	}
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