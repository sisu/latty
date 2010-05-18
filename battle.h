
#ifndef H_BATTLE_
#define H_BATTLE_

#include <BWAPI.h>
#include <vector>


struct BW_Battle
{
  BW_Battle();

  BWAPI::Unit* getTarget(int);     // Asks the AI to make a target selection for my unit. Returns zero if no good targets found.
  void sanityCheck();              // just in case something stupid happens.

  void updateTargets(int i = -1);  // Updates damage estimates for each unit, friend and foe
  double updateDistances();         // Updates unit distances to opposing forces
  bool engageOk(int i, double avg); // Checks whether unit positionings are acceptable for engaging
  int getLife(BWAPI::Unit*);       // Gets health of a unit
  int damage(BWAPI::Unit* attacker, BWAPI::Unit* defender); // Approximate how much damage a given unit does against another

  int tick();                    // Call on Frame
  int addUnit(BWAPI::Unit*);     // Inserts a new unit in the battle
  int unitHide(BWAPI::Unit*);    // Call on unitHide
  int unitDestroy(BWAPI::Unit*); // Call on unitDestroy

  bool shouldRetreat(int index);    // Checks whether a unit should make a tactical escape
  BWAPI::Position getEscape(int i); // Checks in which direction and how much my unit must run to escape

  int my_in_range;
  int op_in_range;

  bool noRetreat;

  int zealots;
  int dragoons;

  int opponents_killed;

  // Unit containers
  std::vector<BWAPI::Unit*> myUnits;
  std::vector<BWAPI::Unit*> enemyUnits;

  // Tracking of long term damage gains
  std::vector<int> prev_hp;
  std::vector<int> enemy_prev_hp;

  // For movement strategies
  std::vector<double> distance;
  std::vector<double> opdistance;

  int scout_target; // This applies only for week 1 tournament

  // Unit high level commands
  std::map<BWAPI::Unit*, std::string> cmd;

  // For target selection
  std::vector<int> myTargets;
  std::vector<int> instantDamage; // damage that i expect to take without delay (cannot evade)
  std::vector<int> soonDamage;    // damage that i expect to take without delay and soon (can evade some of it).
};

#endif

