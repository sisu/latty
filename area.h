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
void calcDangerFrom(bool* start, bool* end, bool add, double ff)
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
		if (add) danger[d.a] += 1/(1+d.d/ff);
		else danger[d.a] /= 1/(1+d.d/ff);
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
		borderArea[i] = myArea[i] && danger[i]>0;
#if 0
		if (!myArea[i]) continue;
		for(int j=0; j<sz(conn[i]); ++j) if (danger[conn[i][j]]>0) {
			borderArea[i]=1;
			Broodwar->printf("borderArea: %d", i);
		}
#endif
	}
	calcDangerFrom(myArea, enemyArea,0,2000);
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
#if 0
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
#endif
	}
	myArea[centerArea]=myArea[myStart]=myArea[startNextArea]=1;
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

struct MSTS {
	TilePosition t;
	int d;
	int f;
	bool operator<(const MSTS& s) const {
		return d>s.d;
	}
};
void startMST()
{
	for(int i=0; i<NA; ++i) {
		TilePosition a = areas[i]->getCenter();
		for(int j=0; j<sz(conn[i]); ++j) {
			int t = conn[i][j];
			TilePosition b = areas[t]->getCenter();
			int z = 2*(int)a.getDistance(b);
			for(int k=0; k<z; ++k) {
				TilePosition c((k*a.x()+(z-k)*b.x())/z, (k*a.y()+(z-k)*b.y())/z);
				for(int i=-1; i<2; ++i)
					for(int j=-1; j<2; ++j)
						forbidden[c.y()+i][c.x()+j]=1;
			}
		}
	}
	return;
#if 0


	const int dx[]={0,1,0,-1};
	const int dy[]={1,0,-1,0};
	int w = Broodwar->mapWidth();
	int h = Broodwar->mapHeight();

	vector<char> done(w*h);
	vector<int> from(w*h);
	vector<char> end(w*h);
	for(int i=0; i<NB; ++i) {
		TilePosition t = bases[i]->getPosition();
//		Broodwar->printf("asd %d %d", t.x(),t.y());
		int a = w*t.y()+t.x();
		if (a<0 || a>=sz(end)) continue;
		end[w*t.y()+t.x()] = 1;
	}
//	return;

	priority_queue<MSTS> q;
	TilePosition s0 = bases[0]->getPosition();
//	s0.x()/=2, s0.y()/=2;
	MSTS strt={s0,0,-1};
	q.push(strt);
	while(!q.empty()) {
		MSTS m=q.top();q.pop();
		TilePosition t=m.t;
		int a0 = w*t.y()+t.x();
		if (done[a0] || end[a0]) continue;
		done[a0]=1;
		from[a0]=m.f;

		for(int i=0; i<4; ++i) {
			TilePosition tt(t.x()+dx[i], t.y()+dy[i]);
			int a=w*tt.y()+tt.x();
			if (Broodwar->isWalkable(4*tt.x(),4*tt.y()) || done[a]) continue;
			MSTS mm = {tt,m.d+1,a0};
			q.push(mm);
		}
	}

	for(int i=0; i<NA; ++i) {
		TilePosition t = bases[i]->getPosition();
		t.x()/=2, t.y()/=2;
		int a = w*t.x()+t.y();
		do {
			int x=a%w, y=a/w;
			for(int i=-1; i<2; ++i) {
				for(int j=-1; j<2; ++j) {
					int x2=2*x+i;
					int y2=2*y+j;
					if (x2>=0 && y2>=0 && x2<w && y2<h)
						forbidden[y2][x2]=1;
				}
			}
			a = from[a];
		} while(a>=0);
	}
#endif
}