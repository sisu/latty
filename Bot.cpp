#include "Bot.hpp"
#include <set>
#include <vector>
using namespace std;
using namespace BWAPI;
using namespace BWTA;



struct MoveState {
	int minerals;
	int gas;

	vector<int> producing;
	typedef pair<Position,int> Build;
	vector<Build> building;
};

void Bot::onStart()
{
	readMap();
	analyze();
}
void Bot::onFrame()
{
}