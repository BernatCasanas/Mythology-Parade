#ifndef __COMBATUNIT_H__
#define __COMBATUNIT_H__

#include "Unit.h"
#include "LevelSystem.h"
#include"Animation.h"

class CombatUnit :	public Unit, public LevelSystem
{
private:
	int damage;
	int range;
	int speed;

public:
	CombatUnit(UnitType, iPoint);
	~CombatUnit();

	bool Update(float);
	void Action(Entity*) override;
	void LevelUp();

private:
	void Init(int maxHealth, int damage, int range, int speed);

	SDL_Texture* level_tex = App->tex->Load("gui/StarLevel.png");
	SDL_Rect level_rect = { 0,0,10,10 };
public:
	int GetDamageValue();
	int GetRangeValue();
	int GetSpeedValue();
	void IncreaseHealth(int);
	void IncreaseSpeed(int);
	void IncreaseDamage(int);

};

#endif // !__COMBATUNIT_H__
