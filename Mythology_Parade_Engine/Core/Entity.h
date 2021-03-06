#ifndef _ENTITY_H
#define _ENTITY_H

#include "j1Module.h"
#include "SDL/include/SDL_rect.h"
#include "j1App.h"
#include "j1Render.h"
#include "HealthSystem.h"
#include "FoWBitDefs.h"
#include"MaykMath.h"
#include"Animation.h"

struct SDL_Texture;
enum CivilizationType;

enum class EntityType
{
	PLAYER,
	UNIT,
	BUILDING
};

class Entity : public HealthSystem
{
public:
	Entity();

	// Destructor
	virtual ~Entity() 
	{
		CleanUp();
	}

	// Called before render is available
	virtual bool Awake(pugi::xml_node&)
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	virtual bool Draw(float dt) 
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PreUpdate()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PostUpdate()
	{
		return true;
	}

	// Called before quitting
	virtual bool CleanUp()
	{
		return true;
	}

	//Called when loading the game
	virtual bool Load(pugi::xml_node&)
	{
		return true;
	}

	//Called when saving the game
	virtual bool Save(pugi::xml_node&) const
	{
		return true;
	}

	virtual void Kill(iPoint direction) {

	}	
	
	virtual AnimationType GetState();

public:

	EntityType type;

	SDL_Texture* texture; //Change it to Character_TMX
	fPoint position;

	//Rect in the spritesheet
	SDL_Rect spriteRect;

	//W and H for the blit
	iPoint blitRect;

	//Side
	CivilizationType civilization;

	//Copy of node
	pugi::xml_node entity_node;

	bool displayDebug;

	SDL_Rect getCollisionRect()
	{
		return collisionRect;
	}

	Rect getMovementRect()
	{
		return { collisionRect.x, collisionRect.y - 10, collisionRect.w, 10};
	}

	Rect getCollisionAsrect()
	{
		return { collisionRect.x, collisionRect.y + (collisionRect.h * 2) + 16, collisionRect.w, -collisionRect.h };
	}

	Rect getCollisionMathRect()
	{
		return { collisionRect.x, collisionRect.y + collisionRect.h, collisionRect.w, -collisionRect.h };
	}

	iPoint getMiddlePoint()
	{
		return { collisionRect.x + (collisionRect.w / 2), collisionRect.y + (collisionRect.h / 2) };
	}

	iPoint GetTilePosition();

	bool canLevel;

	bool shown_minimap;

	bool shown;

	bool isSelected();

	void SetSelected(bool value);

	
	std::string name;
protected:
	SDL_RendererFlip flipState;
	SDL_Rect collisionRect;


	int fowRadius;
	//Conditions
	bool _isSelected;

};
#endif // !ENTITY_H