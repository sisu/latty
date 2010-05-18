
#include "battle.h"
#include <cmath>

using namespace BWAPI;
using namespace std;

void trashTalk()
{
	if(rand() % 2 == 0)
		Broodwar->sendText("Trying to hide, noob?");
	else if(rand() % 2 == 0)
		Broodwar->sendText("I'm gonna kill u!1");
	else
		Broodwar->sendText("Come back here and I'll correct your rotten personality!");
}


Unit* BW_Battle::getTarget(int i)
{
	Unit* target = 0;
	int min_life = 10000000;
	double distance = 1000000;

	// check that an attack is not interrupted if selecting a new target
	if(myUnits[i]->getGroundWeaponCooldown() == 25)
	{
		for(int k=0; k<(int)enemyUnits.size(); k++)
		{
			if(enemyUnits[k]->exists() && enemyUnits[k]->isVisible())
			{
				double new_dist = myUnits[i]->getDistance(enemyUnits[k]);
				if(new_dist < myUnits[i]->getType().groundWeapon()->maxRange() + 100)
				{
					int target_hp = getLife(enemyUnits[k]);
					
					// Targeting priority for running opponents is lesser.
					if(enemyUnits[k]->getOrderTarget() == 0 && enemyUnits[k]->getTarget() == 0)
						target_hp += 200;

					// intentional use of adjusted and unadjusted health counts.
					if( (target_hp - myTargets[k] < min_life) && (getLife(enemyUnits[k]) - myTargets[k] > 0) )
					{

						target = enemyUnits[k];
						distance = new_dist;
						min_life = target_hp - myTargets[k];
					}
					else
					{
						// Broodwar->printf("Too many shooting the same, won't hit that one");
					}
				}
			}
		}
	}

	return target;
}


BW_Battle::BW_Battle()
{
	opponents_killed = 0;
}

bool BW_Battle::shouldRetreat(int index)
{
	int current_hp = getLife(myUnits[index]);
	int damageTakenCumulative = prev_hp[index] - current_hp;

	// if no troops left, stop microing
	if(myUnits.size() < 4 && myUnits.size() <= enemyUnits.size())
		return false;

	// dont try to run if you are already dead
	if(this->instantDamage[index] > current_hp)
		return false;

	// if taking a lot of damage, retreat
	if(this->instantDamage[index] > 27)
		return true;

	// if targeted by many units - run
	if(this->soonDamage[index] > 45)
		return true;

	/// should check whether other my units have more hp?
	/// if has suffered "massive trauma", retreat a bit
	if(damageTakenCumulative > 40 && this->soonDamage[index] > 0)
		return true;

	if(current_hp - this->soonDamage[index] < 40)
		return true;

/*
	if(
		(prev_hp[i] > current_hp + 18 && attacked > 1) || 
		(attacked > 0 && prev_hp[i] - current_hp > 24) || 
		(attacked > 2) ||
		(prev_hp[i] > current_hp + 40)
	  )
	{
	  return true;
	}
*/

	return false;
}

void BW_Battle::sanityCheck()
{
	zealots = 0;
	dragoons = 0;

	if(myUnits.size() != prev_hp.size())
		Broodwar->printf("argh");
	if(enemyUnits.size() != enemy_prev_hp.size())
		Broodwar->printf("blargh");

	for(int i=0; i<(int)myUnits.size(); i++)
	{
		if(!myUnits[i]->exists())
		{
			myUnits[i] = myUnits.back();
			myUnits.pop_back();
			prev_hp[i] = prev_hp.back();
			prev_hp.pop_back();
			i--;
		}
		else
		{
			if((int)prev_hp.size() < i+1)
			{
				Broodwar->printf("Idiot added a unit without pushing hp variable");
				prev_hp.push_back(getLife(myUnits[i]));
			}

			if(myUnits[i]->getType() == UnitTypes::Protoss_Zealot)
				zealots++;
			else
				dragoons++;
		}
	}

	for(int k=0; k<(int)enemyUnits.size(); k++)
	{
		if(!enemyUnits[k]->exists())
		{
			enemyUnits[k] = enemyUnits.back();
			enemyUnits.pop_back();
			enemy_prev_hp[k] = enemy_prev_hp.back();
			enemy_prev_hp.pop_back();
			k--;
		}
		else
		{
			if((int)enemy_prev_hp.size() < k+1)
			{
				Broodwar->printf("Enemy HP size mismatch!!");
				enemy_prev_hp.push_back(getLife(enemyUnits[k]));
			}
		}
	}
}

// calculates expected damage to opponents during the next wave of attacks
// ignores the damage of my unit i (must ignore current target if finding optimal target for him)
void BW_Battle::updateTargets(int i)
{
	// My targets
	myTargets.clear();
	myTargets.resize(enemyUnits.size(), 0);

	for(int m=0; m<(int)myUnits.size(); m++)
	{
		if(m == i)
			continue;
		else
		{
			for(int n=0; n<(int)enemyUnits.size(); n++)
			{
				if(myUnits[m]->getOrderTarget() == enemyUnits[n] || myUnits[m]->getTarget() == enemyUnits[n])
				{
					myTargets[n] += damage(myUnits[m], enemyUnits[n]);
				}
			}
		}
	}

	// Opponent targets
	instantDamage.clear();
	soonDamage.clear();
	instantDamage.resize(myUnits.size(), 0);
	soonDamage.resize(myUnits.size(), 0);

	for(int m=0; m<(int)myUnits.size(); m++)
	{
		for(int k=0; k<(int)enemyUnits.size(); k++)
		{
			if(enemyUnits[k]->getOrderTarget() == myUnits[m] || enemyUnits[k]->getTarget() == myUnits[m])
			{
				if(enemyUnits[k]->getDistance(myUnits[m]) < enemyUnits[k]->getType().groundWeapon()->maxRange())
					instantDamage[m] += damage(enemyUnits[k], myUnits[m]);
				if(enemyUnits[k]->getDistance(myUnits[m]) < enemyUnits[k]->getType().groundWeapon()->maxRange() + 150)
					soonDamage[m] += damage(enemyUnits[k], myUnits[m]);
			}
		}
	}

}


int BW_Battle::damage(Unit* attacker, Unit* defender)
{
	if(attacker->getType() == UnitTypes::Protoss_Zealot)
		return 14;
	if(defender->getType() == UnitTypes::Protoss_Zealot)
		return 9;
	return 19;
}

int BW_Battle::getLife(Unit* unit)
{
	return unit->getHitPoints() + unit->getShields();
}


double BW_Battle::updateDistances()
{
	my_in_range = 0;
	op_in_range = 0;

	distance.clear();
	distance.resize(myUnits.size(), 1000);
	opdistance.clear();
	opdistance.resize(enemyUnits.size(), 1000);

	double sum = 0;
	double opsum = 0;

	for(int i=0; i<(int)myUnits.size(); i++)
	{
		for(int k=0; k<(int)this->enemyUnits.size(); k++)
		{
			double dist = myUnits[i]->getDistance(enemyUnits[k]) - myUnits[i]->getType().groundWeapon()->maxRange();
			if(dist < 0)
				dist = 0;

			if(dist < distance[i])
				distance[i] = dist;
		}

		if(distance[i] < 50)
			my_in_range++;
		sum += distance[i];
	}

	for(int i=0; i<(int)enemyUnits.size(); i++)
	{
		for(int k=0; k<(int)this->myUnits.size(); k++)
		{
			double dist = enemyUnits[i]->getDistance(myUnits[k]) - enemyUnits[i]->getType().groundWeapon()->maxRange();
			if(dist < 0)
				dist = 0;

			if(dist < opdistance[i])
				opdistance[i] = dist;
		}

		if(opdistance[i] < 50)
			op_in_range++;
		opsum += opdistance[i];
	}


	return sum / distance.size();
}


bool BW_Battle::engageOk(int i, double avg)
{
	if(my_in_range > op_in_range)
		return true;

	noRetreat = false;
	if(myUnits.size() * 1.5 < enemyUnits.size())
	{
		noRetreat = true;
		return true;
	}

	if(distance[i] < avg - 150)
		return false;
	return true;
}

Position BW_Battle::getEscape(int i)
{
	double hostile_x = 0.f;
	double hostile_y = 0.f;

//	Broodwar->printf("Unit %d escaping", myUnits[i]->getID());
	cmd[myUnits[i]] = "escape";

	Position f_pos = myUnits[i]->getPosition();

	double maxDepth = 0;
	for(int k=0; k<(int)enemyUnits.size(); k++)
	{
		Position h_pos = enemyUnits[k]->getPosition();
		double dist = f_pos.getApproxDistance(h_pos);

		double rangeDepth = enemyUnits[k]->getType().groundWeapon()->maxRange() - dist;
		if(rangeDepth > maxDepth)
			maxDepth = rangeDepth;

		double diff_x = h_pos.x() - f_pos.x();
		double diff_y = h_pos.y() - f_pos.y();

		hostile_x += 100. * diff_x / dist;
		hostile_y += 100. * diff_y / dist;
	}

	double sum = fabs(hostile_x) + fabs(hostile_y);
	hostile_x /= sum;
	hostile_y /= sum;

	double runDistance;
	// Zealot does not run from battle!
	if(myUnits[i]->getType().getName() == "Protoss Zealot")
		runDistance = 30;
	else
		runDistance = 150 + maxDepth;
	Position result(int(-hostile_x * runDistance), int(-hostile_y * runDistance));
	return result;
}

int BW_Battle::unitHide(Unit* unit)
{
	for(int i=0; i<(int)enemyUnits.size(); i++)
	{
		if(enemyUnits[i] == unit)
		{
			enemyUnits[i] = enemyUnits.back();
			enemyUnits.pop_back();

			enemy_prev_hp[i] = enemy_prev_hp.back();
			enemy_prev_hp.pop_back();
		}
	}

	return 1;
}


int BW_Battle::addUnit(Unit* unit)
{
	if(unit->getPlayer() == Broodwar->self())
	{
		myUnits.push_back(unit);
		prev_hp.push_back(getLife(unit));
	}
	else
	{
		bool unit_is_new = true;
		for(int i=0; i<(int)enemyUnits.size(); i++)
		{
			if(enemyUnits[i]->exists())
				if(unit == enemyUnits[i])
					unit_is_new = false;
		}

		if(unit_is_new)
		{
			enemyUnits.push_back(unit);
			this->enemy_prev_hp.push_back(getLife(unit));
		}
	}

	return 1;
}



int BW_Battle::unitDestroy(Unit* unit)
{
	if(unit->getPlayer() != Broodwar->self())
	{
		opponents_killed++;
	}
	else
	{
		// Lost a unit
	}

	for(int i=0; i<(int)myUnits.size(); i++)
	{
		if(myUnits[i]->getID() == unit->getID())
		{
			myUnits[i] = myUnits.back();
			myUnits.pop_back();

			prev_hp[i] = prev_hp.back();
			prev_hp.pop_back();
		}
	}


	for(int i=0; i<(int)enemyUnits.size(); i++)
	{
		if(enemyUnits[i]->getID() == unit->getID())
		{
			enemyUnits[i] = enemyUnits.back();
			enemyUnits.pop_back();

			enemy_prev_hp[i] = enemy_prev_hp.back();
			enemy_prev_hp.pop_back();
		}
	}

	return 1;
}


int BW_Battle::tick()
{
	bool scouting = false;
	int zealot_counter = 0;
	int dragoon_counter = 0;

	this->sanityCheck();

	/// For backing off when don't have units present
	double avg = this->updateDistances();
	this->updateTargets();

	for(int i=0; i<(int)myUnits.size(); i++)
	{
		updateTargets(i);

		// First see if current command requires attention
		//
		if(myUnits[i]->isIdle())
		{
			if(cmd[myUnits[i]] == "escape")
			{
				if(this->soonDamage[i] > 10)
				{
					// I'm being pursued. Run further?
					if(!noRetreat)
					{
						Broodwar->printf("I'm being pursued! I need to run :G");
						Position direction = this->getEscape(i);
						myUnits[i]->rightClick(myUnits[i]->getPosition() + direction);
						continue;
					}
				}
			}

			cmd[myUnits[i]] = "idle";
		}


		if(cmd[myUnits[i]] == "escape")
		{
			// if no longer targeted, turn to fight
			if(this->soonDamage[i] == 0)
				cmd[myUnits[i]] = "idle";
			else
				continue;
		}


		if(cmd[myUnits[i]] == "scout")
		{
			if(enemyUnits.size() > 0)
			{
				// Release unit from current action, for target selection.
				Broodwar->printf("Scouts have found opponents. Ending scout duties.");

				// Do trashtalk, but only once per "Opponents found" event
				if(i == 0)
					trashTalk();

				cmd[myUnits[i]] = "idle";
			}
		}

		//
		// Checks on current command end here..


		if(myUnits[i]->getType() == UnitTypes::Protoss_Zealot)
			zealot_counter++;
		else
			dragoon_counter++;


		// Make sure we fight together (keeps "formation")
		if(!engageOk(i, avg))
		{
			if(!noRetreat)
			{
				Position direction = this->getEscape(i);
				myUnits[i]->rightClick(myUnits[i]->getPosition() + direction);
				continue;
			}
		}


		// Check whether should make a tactical retreat on a single unit level
		if(myUnits[i]->getGroundWeaponCooldown() < 22 && myUnits[i]->getGroundWeaponCooldown() > 15)
		{
			if(shouldRetreat(i))
			{
				if(!noRetreat)
				{
					prev_hp[i] = getLife(myUnits[i]);
					Position direction = this->getEscape(i);
					myUnits[i]->rightClick(myUnits[i]->getPosition() + direction);
					continue;
				}
			}
		}


		// Target selection
		Unit* target = getTarget(i);

		if(target != 0)
		{
			Broodwar->printf("Target selection picked target for me");
			myUnits[i]->attackUnit(target);
			cmd[myUnits[i]] = "attack";
			continue;
		}


		if(this->cmd[myUnits[i]] == "idle")
		{
			// No targets in sight. What should we do now?
			/// Normally should have a target and release battle members to global control
			/// after no opponents are left.

			Position avg_op(0, 0);
			int divisor = 0;
			for(int m=0; m<(int)enemyUnits.size(); m++)
			{
				if(enemyUnits[m]->exists() && enemyUnits[m]->isVisible())
				{
					avg_op += enemyUnits[m]->getPosition();
					divisor++;
				}
			}

			if(divisor > 0)
			{
				Broodwar->printf("Unit is idle. Attacking enemies.");
				Position avg(avg_op.x() / divisor, avg_op.y() / divisor);
				myUnits[i]->attackMove(avg);
				cmd[myUnits[i]] = "attack";
				continue;
			}

			// MOVE TOWARDS ENEMY (WHAT COMES AFTER THIS IS WEEK 1 SPECIFIC CODE)
			// This is also the ONLY part of the code that is WEEK 1 specific.
			// The following part handles unit movement when no opponents are visible.

			/*

			if(opponents_killed > 0)
			{
				// Should search for opponents

				vector<BWAPI::Position> places;
				places.push_back(Position(150, 1200));
				places.push_back(Position(1000, 1200));
				places.push_back(Position(1850, 1200));
				places.push_back(Position(1000, 750));
				places.push_back(Position(1000, 1600));

				if(myUnits[i]->getDistance(places[scout_target % places.size()]) < 50)
				{
					trashTalk();
					Broodwar->printf("Selecting new scouting target");
					scout_target++;
				}

				myUnits[i]->attackMove(places[scout_target % places.size()]);
				cmd[myUnits[i]] = "scout";
				continue;
			}

			Position avg_blim(0, 0);
			for(int m=0; m<myUnits.size(); m++)
				avg_blim += myUnits[m]->getPosition();

			Position avg(avg_blim.x() / myUnits.size(), avg_blim.y() / myUnits.size());

			TilePosition myStart = Broodwar->self()->getStartLocation();
			int direction = -1;
			if(myStart.x() * 32 < 1200)
				direction = 1;

			bool wait = false;
			if(myUnits[i]->getType().getName() != "Protoss Zealot")
			{
				if(direction == 1)
				{
					if(myUnits[i]->getPosition().x() > avg.x() + 20)
						wait = true;
				}
				else if(direction == -1)
				{
					if(myUnits[i]->getPosition().x() < avg.x() - 20)
						wait = true;
				}
			}
			else
			{
				if(direction == 1)
				{
					if(myUnits[i]->getPosition().x() > avg.x() - 10)
						wait = true;
				}
				else if(direction == -1)
				{
					if(myUnits[i]->getPosition().x() < avg.x() + 10)
						wait = true;
				}
			}

			if(!wait)
			{

				if(myUnits[i]->getType().getName() == "Protoss Zealot")
				{
					myUnits[i]->attackMove(avg + Position(direction * 350, (zealot_counter - zealots / 2) * 40));
				}
				else
				{
					myUnits[i]->attackMove(avg + Position(direction * 250, (dragoon_counter - dragoons / 2 ) * 50));
				}
			}

			*/
		}
	}

	return 1;
}
