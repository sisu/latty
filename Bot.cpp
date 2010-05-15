#include "Bot.hpp"
#include <set>
#include <vector>
using namespace std;
using namespace BWAPI;
using namespace BWTA;

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
void Bot::onFrame()
{
}