#ifndef _ENTITYMANAGER_H
#define _ENTITYMANAGER_H

#define MAX_BUILDING_TYPES 8

#include "j1Module.h"
#include <unordered_map>
#include <map>
#include "Entity.h"
#include"j1Input.h"
#include"j1Map.h"
#include"j1Pathfinding.h"
#include"j1Audio.h"

#include<vector>
#include <algorithm>

//Can delete
#include "j1Scene.h"
#include"j1Textures.h"
#include "Player.h"
#include"Animation.h"
#include"MaykMath.h"



enum class UnitType;
enum BuildingType;
enum CivilizationType {
	VIKING,
	GREEK,
	NONE
};

struct CreationPreview
{
	bool active = false;
	int width;
	int height;
	bool canBuild = false;

};

struct BuildingInfo
{
	CivilizationType civilization;
	SDL_Rect spriteRect;
	iPoint blitSize;

	BuildingType buildingType;
	int tileLenght;
};

enum class SpriteSheetType
{
	BUILDINGS,
	ASSASSIN,
	PRIEST,
	SPEAR_SOLDIER
};

class Entity;
class Player;

//Temporal sorting function
struct entity_Sort
{
	inline bool operator() (Entity* struct1, Entity* struct2)
	{
		return (struct1->position.y < struct2->position.y);
	}
};

class EntityManager : public j1Module
{
public:

	EntityManager();

	// Destructor
	~EntityManager();

	// Called before render is available
	bool Awake(pugi::xml_node&);

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	void EnterBuildMode();

	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	////Called when loading the game
	bool Load(pugi::xml_node&);

	////Called when saving the game
	bool Save(pugi::xml_node&) const;

	bool DeleteEntity(Entity*);

	Entity* CreatePlayerEntity(std::string civilization_string = "");
	Entity* CreateUnitEntity(UnitType, iPoint, CivilizationType);
	Entity* CreateBuildingEntity(iPoint, BuildingType, BuildingInfo, CivilizationType);
	void UpdateBuildPreview(int);
	void SetBuildIndex(int);

	//Load data packets
	void LoadBuildingsData(pugi::xml_node&);
	iPoint CalculateBuildingSize(int, int, int);
	void DrawEverything();

	void LoadBuildingsBlitRect();

	Player* getPlayer() const;
	void FxUnits(int channel, int fx, int posx, int posy);


	static bool IsPointInsideQuad(SDL_Rect rect, int x, int y);
	int volume;
	iPoint MapPos();

public:

	std::unordered_map<EntityType, std::list<Entity*>> entities;
	CreationPreview crPreview;
	SDL_Texture* debugTex;

	//The way to store the spritesheets
	std::unordered_map<SpriteSheetType, SDL_Texture*> entitySpriteSheets;
	std::vector<BuildingInfo> buildingsData;

	QuadTree quadTree;
	AABBTree aabbTree;

	//int volume;

	//Textures
	SDL_Texture* level_tex;
	SDL_Rect level_rect;

	SDL_Texture* circle_unit_tex;
	SDL_Rect circle_unit_rect;

	SDL_Texture* enemyTextureAssassin;
	SDL_Texture* enemyTextureMonk;
	SDL_Texture* enemyTextureCleric;

private:
	int buildingTestIndex = 0;
	Animation animationManager;

public:
	SDL_Rect constructorSpriteRect;
	SDL_Rect destructedSpriteRect;
	SDL_Rect construction_bar_back;
	SDL_Rect construction_bar_empty;
	SDL_Rect construction_bar_front;
	SDL_Rect life_bar_front;
	SDL_Rect life_bar_front_enemy;
	SDL_Rect research_bar_front;
	SDL_Rect unit_life_bar_back;
	SDL_Rect unit_life_bar_empty;
	SDL_Rect unit_life_bar_front;
	SDL_Rect unit_life_bar_front_enemy;

	std::unordered_map<UnitType, std::unordered_map<AnimationType, std::unordered_map<Direction, Animation_char>>> animations;
	std::multimap<int, Entity*> orderedSprites;

	int CreateMonk_sound;
	int CreateAssasin_sound;
	int increase_sacrifice;
	int DestroyBuilding;
	int Select_sfx;


	bool initCivilizations;

	void BuildCivilizations(CivilizationType);
	void InitVikings();
	void InitGreek();
	bool loading;

	bool playerCreated;
};
#endif // !_ENTITYMANAGER_H
