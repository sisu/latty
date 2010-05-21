#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <windows.h>

//static bool analyzed;
//static bool analysis_just_finished;
static BWTA::Region* home;
static BWTA::Region* enemy_base;
//DWORD WINAPI AnalyzeThread();

struct Bot : BWAPI::AIModule
{
	virtual void onStart();
	virtual void onFrame();
  /*
  virtual void onEnd(bool isWinner);
  virtual bool onSendText(std::string text);
  virtual void onPlayerLeft(BWAPI::Player* player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDestroy(BWAPI::Unit* unit);
  virtual void onUnitMorph(BWAPI::Unit* unit);
  virtual void onUnitShow(BWAPI::Unit* unit);
  virtual void onUnitHide(BWAPI::Unit* unit);
  virtual void onUnitRenegade(BWAPI::Unit* unit);
  */
  virtual void onUnitCreate(BWAPI::Unit* unit);
};