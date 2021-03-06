#include "EntityManager.h"
#include "CombatUnit.h"
#include "Building.h"
#include "Player.h"
#include "j1Gui.h"
#include "AssetsManager.h"
#include "j1FadeToBlack.h"
#include "j1App.h"
#include "ResearchMenu.h"
#include "j1TutorialScene.h"

#include "p2Log.h"
EntityManager::EntityManager() : CreateAssasin_sound(0), CreateMonk_sound(0), increase_sacrifice(0), DestroyBuilding(0), construction_bar_back({0, 0, 0, 0}),
construction_bar_empty({ 0, 0, 0, 0 }), construction_bar_front({ 0, 0, 0, 0 }), constructorSpriteRect({0, 0, 0, 0}), debugTex(nullptr),
destructedSpriteRect({ 0, 0, 0, 0 }), life_bar_front({ 0, 0, 0, 0 }), life_bar_front_enemy({0, 0, 0, 0}),
 research_bar_front({ 0, 0, 0, 0 }), unit_life_bar_back({ 0, 0, 0, 0 }), unit_life_bar_empty({ 0, 0, 0, 0 }),
unit_life_bar_front({ 0, 0, 0, 0 }),  unit_life_bar_front_enemy({0, 0, 0, 0}), volume(0)
{
	name.append("entity_manager");
	buildingsData.reserve(MAX_BUILDING_TYPES);
	buildingTestIndex = 0;
	playerCreated = false;
	level_tex = nullptr;
	circle_unit_tex = nullptr;
	level_rect = { 0,0,10,10 };
	circle_unit_rect = { 0,0,64,32 };
	enemyTextureAssassin = nullptr;
	enemyTextureCleric = nullptr;
	enemyTextureMonk = nullptr;
}

//Destructor
EntityManager::~EntityManager()
{}

//Called before render is available
bool EntityManager::Awake(pugi::xml_node& a)
{
	App->fowManager->RequestMaskGeneration(10);
	//Load buildings info
	pugi::xml_document buildings;
	loading = false;
	char* buffer;
	int bytesFile = App->assets_manager->Load(a.child("buildings").attribute("file").as_string(), &buffer);
	pugi::xml_parse_result result = buildings.load_buffer(buffer, bytesFile);
	RELEASE_ARRAY(buffer);
	LoadBuildingsData(buildings.child("map").child("objectgroup"));
	life_bar_front = { 1310,503,115,10 };
	life_bar_front_enemy = { 1310,483,115,10 };
	research_bar_front = { 1310,543,115,10 };
	construction_bar_back = { 1299,560,125,17 };
	construction_bar_front = { 1310, 523, 115, 10 };
	construction_bar_empty = { 1299,582,125,17 };
	unit_life_bar_back = { 1406,481,75,10 };
	unit_life_bar_empty = { 1406,494,75,10 };
	unit_life_bar_front = { 1413,470,63,6 };
	unit_life_bar_front_enemy = { 1327,470,63,6 };


	//Not working because renderer is not created yet ;-;
	//std::string path = "assets/buildings/";
	//path.append(buildings.child("map").child("imagelayer").child("image").attribute("source").as_string());
	//tempBuildingTexture = App->tex->Load(path.c_str());


	//INFO: This is a good way to itinerate all the map, to itinerate only items in one key, use only the second for loop
	for (unsigned i = 0; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			Entity* ent = it._Ptr->_Myval;
			ent->Awake(a.child(ent->name.c_str()));
		}
	}
	active = false;



	return true;
}

// Called before the first frame
bool EntityManager::Start()
{
	//TODO: NO HARDCODE BOY
	entitySpriteSheets[SpriteSheetType::BUILDINGS] = App->tex->Load("assets/buildings/Buildings.png");

	animations[UnitType::ASSASSIN] = animationManager.Load("assets/units/Assassin.tmx", UnitType::ASSASSIN);
	animations[UnitType::MONK] = animationManager.Load("assets/units/Monk.tmx", UnitType::MONK);
	animations[UnitType::PRIEST] = animationManager.Load("assets/units/Priest.tmx", UnitType::PRIEST);
	animations[UnitType::DRAUGAR] = animationManager.Load("assets/units/Draugar.tmx", UnitType::DRAUGAR);
	animations[UnitType::JOTNAR] = animationManager.Load("assets/units/Jotnar.tmx", UnitType::JOTNAR);
	animations[UnitType::CYCLOP] = animationManager.Load("assets/units/Cyclop.tmx", UnitType::CYCLOP);
	animations[UnitType::MINOTAUR] = animationManager.Load("assets/units/Minotaur.tmx", UnitType::MINOTAUR);
	animations[UnitType::CLERIC] = animationManager.Load("assets/units/Cleric.tmx", UnitType::CLERIC);

	CreateAssasin_sound = App->audio->LoadFx("audio/fx/Appear_assasin.wav");
	CreateMonk_sound = App->audio->LoadFx("audio/fx/Appear_monk.wav");
	increase_sacrifice = App->audio->LoadFx("audio/fx/VOLUME_Increase_sacrifice (1).wav");
	DestroyBuilding = App->audio->LoadFx("audio/fx/Building_destruction.wav");
	Select_sfx = App->audio->LoadFx("audio/ui/Menu Select 1.wav");
	
	

	level_tex =  App->tex->Load("gui/StarLevel.png");
	circle_unit_tex = App->tex->Load("assets/units/CercleUnitats.png");
	enemyTextureAssassin = App->tex->Load("assets/units/IAAssassinSpriteSheet.png");
	enemyTextureCleric = App->tex->Load("assets/units/IAClericSpriteSheet.png");
	enemyTextureMonk = App->tex->Load("assets/units/IAMonkSpriteSheet.png");
	for (unsigned i = 0; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			it._Ptr->_Myval->Start();
		}
	}

	initCivilizations = true;

	return true;
}

void EntityManager::LoadBuildingsBlitRect()
{
	for (unsigned int i = 0; i < buildingsData.size(); i++)
	{
		BuildingInfo* info = &buildingsData[i];
		int blitWidth = info->tileLenght * App->map->data.tile_width;
		info->blitSize = CalculateBuildingSize(blitWidth, info->spriteRect.w, info->spriteRect.h);
	}
}

// Called each loop iteration
bool EntityManager::PreUpdate()
{
	for (unsigned i = 0; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			it._Ptr->_Myval->PreUpdate();
		}
	}
	return true;
}

// Called each loop iteration
bool EntityManager::Update(float dt)
{
	for (unsigned i = 0; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			(*it)->Update(dt);
		}
	}


	//for (std::list<Entity*>::iterator it = entities[EntityType::UNIT].begin(); it != entities[EntityType::UNIT].end(); it++)
	//{
	//	if ((*it)->type == EntityType::UNIT)
	//	{
	//		Unit* tmp = (Unit*)it._Ptr->_Myval;
	//		if (tmp->toDelete)
	//		{
	//			entities[EntityType::UNIT].erase(it);
	//			DeleteEntity(tmp);
	//			//delete tmp;
	//		}

	//	}
	//}

	std::list<Entity*>::iterator i = entities[EntityType::UNIT].begin();
	while (i != entities[EntityType::UNIT].end())
	{
		if ((*i)->type == EntityType::UNIT)
		{
			Unit* tmp = (Unit*)i._Ptr->_Myval;
			bool isActive = (*tmp).toDelete;
			if (isActive)
			{
				entities[EntityType::UNIT].erase(i++);  // alternatively, i = items.erase(i);
				//entities[EntityType::UNIT].erase(it);
				DeleteEntity(tmp);
			}
			else
			{
				++i;
			}
		}
	}

	//TODO: Move this logic to the player
	if (App->input->GetKey(SDL_SCANCODE_1) == KEY_DOWN && App->scene->godMode==true)
	{
		EnterBuildMode();
	}

	if (crPreview.active == true && App->input->GetMouseButtonDown(3) == KEY_UP) {
		EnterBuildMode();
	}

	if (App->input->GetKey(SDL_SCANCODE_2) == KEY_DOWN && App->scene->godMode == true)
	{
		if (buildingTestIndex < MAX_BUILDING_TYPES - 1)
		{
			buildingTestIndex++;
		}
		else
		{
			buildingTestIndex = 0;
		}
		UpdateBuildPreview(buildingTestIndex);
	}

	if (crPreview.active)
	{
		iPoint mouse = App->map->GetMousePositionOnMap();
		BuildingInfo build = buildingsData[buildingTestIndex];

		crPreview.canBuild = true;
		debugTex = App->scene->debugBlue_tex;

		for (int i = 0; i <= 1; i++)
		{
			for (int y = mouse.y; y > mouse.y - crPreview.height; y--)
			{
				for (int x = mouse.x; x < mouse.x + crPreview.width; x++)
				{
					if (i == 0)
					{
						if (crPreview.canBuild && App->pathfinding->IsWalkable({ x, y }) == false)
						{
							debugTex = App->scene->debugRed_tex;
							crPreview.canBuild = false;
						}
					}
					else
					{
						if (IN_RANGE(x, 0, App->map->data.width - 1) == 1 && IN_RANGE(y, 0, App->map->data.height - 1) == 1)
						{
							iPoint p = App->map->MapToWorld(x, y);
							App->render->Blit(debugTex, p.x, p.y);
						}
					}
				}
			}
		}

		//Preview of build
		iPoint p = App->map->MapToWorld(mouse.x, mouse.y);
		App->render->Blit(entitySpriteSheets[SpriteSheetType::BUILDINGS], p.x, p.y + ((App->map->data.tile_height / 2) * build.tileLenght) - build.blitSize.y, { build.blitSize.x, build.blitSize.y }, &build.spriteRect);
	}

	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN && crPreview.active && crPreview.canBuild)
	{
		int faithToDescrease = 0;
		iPoint mouse = App->map->GetMousePositionOnMap();
		iPoint spawnPos = App->map->MapToWorld(mouse.x, mouse.y);
		spawnPos.y += App->map->data.tile_height / 2;
		switch (buildingTestIndex)
		{
		case 0:
			CreateBuildingEntity(spawnPos, BuildingType::FORTRESS, buildingsData[buildingTestIndex], CivilizationType::VIKING);
			break;
		case 4:
			CreateBuildingEntity(spawnPos, BuildingType::FORTRESS, buildingsData[buildingTestIndex],CivilizationType::GREEK);
			break;
		case 1:
			CreateBuildingEntity(spawnPos, BuildingType::MONASTERY, buildingsData[buildingTestIndex], CivilizationType::VIKING);
			faithToDescrease = 50;
			break;
		case 5:
			CreateBuildingEntity(spawnPos, BuildingType::MONASTERY , buildingsData[buildingTestIndex],CivilizationType::GREEK);
			faithToDescrease = 50;
			break;
		case 2:
			CreateBuildingEntity(spawnPos, BuildingType::TEMPLE, buildingsData[buildingTestIndex], CivilizationType::VIKING);
			faithToDescrease = 50;
			break;
		case 6:
			CreateBuildingEntity(spawnPos, BuildingType::TEMPLE, buildingsData[buildingTestIndex], CivilizationType::GREEK);
			faithToDescrease = 50;
			break;
		case 3:
			CreateBuildingEntity(spawnPos, BuildingType::ENCAMPMENT, buildingsData[buildingTestIndex], CivilizationType::VIKING);
			faithToDescrease = 50;
			break;

		case 7:
			CreateBuildingEntity(spawnPos, BuildingType::ENCAMPMENT, buildingsData[buildingTestIndex], CivilizationType::GREEK);
			faithToDescrease = 50;
			break;
		}

		//Onces you build disable building mode
		App->entityManager->getPlayer()->DecreaseFaith(faithToDescrease);
		crPreview.active = false;
	}

	//Update FOW
	App->fowManager->MapNeedsUpdate();

	//Update aabbTree
	aabbTree.UpdateAllNodes(aabbTree.baseNode);

	return true;
}



bool EntityManager::PostUpdate()
{

	for (unsigned i = 0; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			(*it)->PostUpdate();
		}
	}

	//TODO 7: Test collision detection in Debug and Release mode
	for (std::list<Entity*>::iterator unitA = entities[EntityType::UNIT].begin(); unitA != entities[EntityType::UNIT].end(); unitA++)
	{

		//Get the nodes to check inside this unit position
		std::vector<AABBNode*> nodesToCheck;
		aabbTree.LoadLeafNodesInsideRect(&aabbTree.baseNode, nodesToCheck, (*unitA)->getMovementRect());

		//Iterate all nodes
		for (int i = 0; i < nodesToCheck.size(); i++)
		{

			//Iterate all data in that node
			for (std::list<Entity*>::iterator unitB = nodesToCheck[i]->data.begin(); unitB != nodesToCheck[i]->data.end(); unitB++)
			{
				//Check for collisions
				if (unitA._Ptr->_Myval != unitB._Ptr->_Myval && MaykMath::CheckRectCollision((*unitA)->getMovementRect(), (*unitB)->getMovementRect()))
				{

					fPoint direction = (*unitA)->position - (*unitB)->position;

					if (direction.IsZero())
					{
						direction = { 1, 1 };
					}

					fPoint normalDirection = fPoint::Normalize(direction);
					iPoint tilePrediction = App->map->WorldToMap((*unitB)->position - normalDirection);

					//LOG("Unit to unit collision");
					if (App->pathfinding->CheckBoundaries((*unitB)->GetTilePosition())) 
					{
						bool correct = false;
						if (App->pathfinding->IsWalkable((*unitB)->GetTilePosition()) && App->pathfinding->IsWalkable(tilePrediction))
						{
							correct = true;
						}
						//if (!App->pathfinding->IsWalkable((*unitB)->GetTilePosition()) && !App->pathfinding->IsWalkable(tilePrediction))
						//{
						//	correct = true;
						//}
						//if (!App->pathfinding->IsWalkable((*unitB)->GetTilePosition()) && App->pathfinding->IsWalkable(tilePrediction))
						//{
						//	correct = true;
						//}

						if (correct) 
						{
							(*unitB)->position -= normalDirection;
						}
					}

				}
			}
		}
		//Clear the node vector
		nodesToCheck.clear();

		//Find the lowest node in this point
		quadTree.FindLowestNodeInPoint(&quadTree.baseNode, (*unitA)->position);
		if (quadTree.lowestNode)
		{

			//Check every data element in this node
			for (std::list<Entity*>::iterator it2 = quadTree.lowestNode->data.begin(); it2 != quadTree.lowestNode->data.end(); it2++)
			{

				fPoint htPos = (*it2)->position;
				SDL_Rect htColl = (*it2)->getCollisionRect();

				Point B = { htPos.x + (htColl.w / 2), htPos.y + (htColl.h / 3.5f) };
				Point A = { htPos.x + htColl.w, htPos.y };
				Point C = { htPos.x, htPos.y };
				Point D = { htPos.x + (htColl.w / 2), htPos.y - (htColl.h / 5) };

				//App->render->DrawLine(B.x, B.y, A.x, A.y, 255, 0, 0);
				//App->render->DrawLine(A.x, A.y, C.x, C.y, 255, 0, 0);
				//App->render->DrawLine(C.x, C.y, D.x, D.y, 255, 0, 0);
				//App->render->DrawLine(D.x, D.y, B.x, B.y, 255, 0, 0);

				if (MaykMath::IsPointInsideOffAxisRectangle(B, A, C, D, (*unitA)->position))
				{
					fPoint buildingCenter = (*it2)->position + fPoint((*it2)->getCollisionRect().w / 2, 0);

					fPoint direction = buildingCenter - (*unitA)->position;
					(*unitA)->position -= fPoint::Normalize(direction);
				}
			}

			quadTree.lowestNode = nullptr;
		}
	}

	return true;
}

// Called before quitting
bool EntityManager::CleanUp()
{
	App->tex->UnLoad(enemyTextureAssassin);
	enemyTextureAssassin = nullptr;

	//Double for?
	for (unsigned i = 0; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			Entity* ent = it._Ptr->_Myval;
			delete ent;
		}
		entities[(EntityType)i].clear();
	}
	for (unsigned int i = 0; i < entitySpriteSheets.size(); i++)
	{
		if(entitySpriteSheets[(SpriteSheetType)i])
			App->tex->UnLoad(entitySpriteSheets[(SpriteSheetType)i]);
	}
	entities.clear();

	//BUG: This is not working
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		for (unsigned int k = 0; k < animations[(UnitType)i].size(); k++)
		{
			for (unsigned int j = 0; j < animations[(UnitType)i][(AnimationType)k].size(); j++)
			{
				animations[(UnitType)i][(AnimationType)k][(Direction)j].Clean();
			}
			animations[(UnitType)i][(AnimationType)k].clear();
		}
		animations[(UnitType)i].clear();
	}
	animations.clear();

	for (auto& it : animationManager.charData)
	{
		// Do stuff
		it.second.Clean();
	}
	App->audio->CleanFxs(Select_sfx);
	App->audio->CleanFxs(DestroyBuilding);
	App->audio->CleanFxs(increase_sacrifice);
	App->audio->CleanFxs(CreateMonk_sound);
	App->audio->CleanFxs(CreateAssasin_sound);



	return true;
}

////Called when loading the game
bool EntityManager::Load(pugi::xml_node& n)
{
	CivilizationType civ;
	entities.clear();
	loading = true;

	//PLAYER LOADING
	pugi::xml_node p = n.child("players").first_child();
	if (p.name() == "viking") civ = CivilizationType::VIKING;
	else civ = CivilizationType::GREEK;
	Player* player = static_cast<Player*>(CreatePlayerEntity(p.name()));
	player->research_assassin = p.child("research").child("assassin").attribute("research").as_bool();
	player->research_chaotic_beast = p.child("research").child("chaotic_beast").attribute("research").as_bool();
	player->research_chaotic_miracle = p.child("research").child("chaotic_miracle").attribute("research").as_bool();
	player->research_chaotic_victory = p.child("research").child("chaotic_victory").attribute("research").as_bool();
	player->research_cleric = p.child("research").child("cleric").attribute("research").as_bool();
	player->research_encampment = p.child("research").child("encampment").attribute("research").as_bool();
	player->research_lawful_beast = p.child("research").child("lawful_beast").attribute("research").as_bool();
	player->research_lawful_miracle = p.child("research").child("lawful_miracle").attribute("research").as_bool();
	player->research_lawful_victory = p.child("research").child("lawful_victory").attribute("research").as_bool();
	player->research_temple = p.child("research").child("temple").attribute("research").as_bool();


	player->SetFaith(p.child("economy").attribute("faith").as_int());
	player->SetSacrifices(p.child("economy").attribute("sacrifices").as_int());
	player->SetPrayers(p.child("economy").attribute("prayers").as_int());


	//UNITS LOADING
	pugi::xml_node it = n.child("entities").child("unit");
	for (it; it; it = it.next_sibling("unit"))
	{
		iPoint pos;
		pos.x = it.attribute("position_x").as_int();
		pos.y = it.attribute("position_y").as_int();

		CivilizationType unit_civ;
		if (!strcmp(it.attribute("civilization").as_string(), "viking")) unit_civ = CivilizationType::VIKING;
		else unit_civ = CivilizationType::GREEK;

		if (!strcmp(it.attribute("type").as_string(), "monk")) {
			Unit* monk = static_cast<Unit*>(CreateUnitEntity(UnitType::MONK, pos, unit_civ));
			monk->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "assassin")) {
			CombatUnit* assassin = static_cast<CombatUnit*>(CreateUnitEntity(UnitType::ASSASSIN, pos, unit_civ));
			assassin->SetLevel(it.attribute("level").as_int());
			assassin->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "pikeman")) {
			CombatUnit* pikeman = static_cast<CombatUnit*>(CreateUnitEntity(UnitType::PIKEMAN, pos, unit_civ));
			pikeman->SetLevel(it.attribute("level").as_int());
			pikeman->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "explorer")) {
			CombatUnit* explorer = static_cast<CombatUnit*>(CreateUnitEntity(UnitType::EXPLORER, pos, unit_civ));
			explorer->SetLevel(it.attribute("level").as_int());
			explorer->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "priest")) {
			CombatUnit* priest = static_cast<CombatUnit*>(CreateUnitEntity(UnitType::PRIEST, pos, unit_civ));
			priest->SetLevel(it.attribute("level").as_int());
			priest->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "footman")) {
			CombatUnit* footman = static_cast<CombatUnit*>(CreateUnitEntity(UnitType::FOOTMAN, pos, unit_civ));
			footman->SetLevel(it.attribute("level").as_int());
			footman->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "cyclop")) {
			Unit* cyclop = static_cast<Unit*>(CreateUnitEntity(UnitType::CYCLOP, pos, unit_civ));
			cyclop->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "minotaur")) {
			Unit* minotaur = static_cast<Unit*>(CreateUnitEntity(UnitType::MINOTAUR, pos, unit_civ));
			minotaur->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "jotnar")) {
			Unit* jotnar = static_cast<Unit*>(CreateUnitEntity(UnitType::JOTNAR, pos, unit_civ));
			jotnar->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "draugar")) {
			Unit* draugar = static_cast<Unit*>(CreateUnitEntity(UnitType::DRAUGAR, pos, unit_civ));
			draugar->SetHealth(it.attribute("health").as_int());
		}
		else if (!strcmp(it.attribute("type").as_string(), "cleric")) {
			Unit* cleric = static_cast<Unit*>(CreateUnitEntity(UnitType::CLERIC, pos, unit_civ));
			cleric->SetHealth(it.attribute("health").as_int());
		}
	}

	//BUILDINGS LOADING
	it = n.child("buildings").child("build");
	for (it; it; it = it.next_sibling("build"))
	{
		BuildingStatus status;
		BuildingAction action;
		CivilizationType build_civ;
		iPoint pos;
		int databuild;

		pos.x = it.attribute("position_x").as_int();
		pos.y = it.attribute("position_y").as_int();

		if (!strcmp(it.attribute("status").as_string(), "constructing")) status = BuildingStatus::CONSTRUCTING;
		else if (!strcmp(it.attribute("status").as_string(), "finished")) status = BuildingStatus::FINISHED;
		else status = BuildingStatus::DESTROYED;

		if (!strcmp(it.attribute("action").as_string(), "nothing")) action = BuildingAction::NOTHING;
		else if (!strcmp(it.attribute("action").as_string(), "producing")) action = BuildingAction::PRODUCING;
		else action = BuildingAction::RESEARCHING;


		if (!strcmp(it.attribute("type").as_string(), "monastery")) {
			if (!strcmp(it.attribute("civilization").as_string(), "viking")) { build_civ = CivilizationType::VIKING; databuild = 1; }
			else { build_civ = CivilizationType::GREEK; databuild = 5; }
			Building* monastery = static_cast<Building*>(CreateBuildingEntity(pos, BuildingType::MONASTERY, buildingsData[databuild], build_civ));
			monastery->SetHealth(it.attribute("health").as_int());
			if (status == BuildingStatus::CONSTRUCTING) {
				monastery->timer_construction.StartAt(it.attribute("time").as_float());
				monastery->SetPercentage(it.attribute("percentage").as_float());
			} //CARGAR PERCENTAGE CONSTRUCTING
			monastery->buildingStatus = status;
			monastery->buildingAction = action;
			if (action == BuildingAction::PRODUCING) { monastery->StartProducing(it.attribute("element").as_string()); monastery->timer_construction.StartAt(it.attribute("time").as_int()); }
			else if (action == BuildingAction::RESEARCHING) { monastery->StartResearching(it.attribute("element").as_string()); monastery->timer_construction.StartAt(it.attribute("time").as_int()); }

		}

		else if (!strcmp(it.attribute("type").as_string(), "temple")) {
			if (!strcmp(it.attribute("civilization").as_string(), "viking")) { build_civ = CivilizationType::VIKING; databuild = 2; }
			else { build_civ = CivilizationType::GREEK; databuild = 6; }
			Building* temple = static_cast<Building*>(CreateBuildingEntity(pos, BuildingType::TEMPLE, buildingsData[databuild], build_civ));
			temple->SetHealth(it.attribute("health").as_int());
			if (status == BuildingStatus::CONSTRUCTING) {
				temple->timer_construction.StartAt(it.attribute("time").as_float());
				temple->SetPercentage(it.attribute("percentage").as_float());
			}
			temple->buildingStatus = status;
			temple->buildingAction = action;
			if (action == BuildingAction::PRODUCING) { temple->StartProducing(it.attribute("element").as_string()); temple->timer_construction.StartAt(it.attribute("time").as_int()); }
			else if (action == BuildingAction::RESEARCHING) { temple->StartResearching(it.attribute("element").as_string()); temple->timer_construction.StartAt(it.attribute("time").as_int()); }

		}

		else if (!strcmp(it.attribute("type").as_string(), "encampment")) {
			if (!strcmp(it.attribute("civilization").as_string(), "viking")) { build_civ = CivilizationType::VIKING; databuild = 3; }
			else { build_civ = CivilizationType::GREEK; databuild = 7; }
			Building* encampment = static_cast<Building*>(CreateBuildingEntity(pos, BuildingType::ENCAMPMENT, buildingsData[databuild], build_civ));
			encampment->SetHealth(it.attribute("health").as_int());
			if (status == BuildingStatus::CONSTRUCTING) {
				encampment->timer_construction.StartAt(it.attribute("time").as_float());
				encampment->SetPercentage(it.attribute("percentage").as_float());
			}
			encampment->buildingStatus = status;
			encampment->buildingAction = action;
			if (action == BuildingAction::PRODUCING) { encampment->StartProducing(it.attribute("element").as_string()); encampment->timer_construction.StartAt(it.attribute("time").as_int()); }
			else if (action == BuildingAction::RESEARCHING) { encampment->StartResearching(it.attribute("element").as_string()); encampment->timer_construction.StartAt(it.attribute("time").as_int()); }

		}

		else if (!strcmp(it.attribute("type").as_string(), "fortress")) {

			if (!strcmp(it.attribute("civilization").as_string(), "viking")) { build_civ = CivilizationType::VIKING; databuild = 0; }
			else { build_civ = CivilizationType::GREEK; databuild = 4; }
			Building* fortress = static_cast<Building*>(CreateBuildingEntity(pos, BuildingType::FORTRESS, buildingsData[databuild], build_civ));
			fortress->SetHealth(it.attribute("health").as_int());
			if (status == BuildingStatus::CONSTRUCTING) {
				fortress->timer_construction.StartAt(it.attribute("time").as_float());
				fortress->SetPercentage(it.attribute("percentage").as_float());
			}
			fortress->buildingStatus = status;
			fortress->buildingAction = action;
			if (action == BuildingAction::PRODUCING) { fortress->StartProducing(it.attribute("element").as_string()); fortress->timer_construction.StartAt(it.attribute("time").as_int()); }
			else if (action == BuildingAction::RESEARCHING) { fortress->StartResearching(it.attribute("element").as_string()); fortress->timer_construction.StartAt(it.attribute("time").as_int()); }
		}
	}

	App->scene->research_menu->UpdatePlayer(getPlayer());
	loading = false;

	return true;
}

////Called when saving the game

bool EntityManager::Save(pugi::xml_node& s) const
{
	pugi::xml_node node = s.append_child("entities");
	bool assassin = true, monk = true;
	std::list<Entity*> list = App->entityManager->entities[EntityType::UNIT];
	for each (Unit * var in list)
	{
		if (var->civilization != getPlayer()->civilization)
			continue;

		pugi::xml_node entity = node.append_child("unit");
		entity.append_attribute("type").set_value(var->name.c_str());
		entity.append_attribute("position_x").set_value(var->position.x);
		entity.append_attribute("position_y").set_value(var->position.y);


		if (var->civilization == CivilizationType::GREEK)
			entity.append_attribute("civilization").set_value("greek");
		else if (var->civilization == CivilizationType::VIKING)
			entity.append_attribute("civilization").set_value("viking");


		entity.append_attribute("health").set_value(var->GetHealth());

		if (var->canLevel)
		{
			CombatUnit* combatVar = (CombatUnit*)var;
			entity.append_attribute("level").set_value(combatVar->GetLevel());
		}
	}

	pugi::xml_node node2 = s.append_child("buildings");
	std::list<Entity*> list2 = App->entityManager->entities[EntityType::BUILDING];


	for each (Building * var2 in list2)
	{
		if (var2->civilization != getPlayer()->civilization)
			continue;

		pugi::xml_node building = node2.append_child("build");

		building.append_attribute("type").set_value(var2->name.c_str());
		building.append_attribute("position_x").set_value(var2->position.x);
		building.append_attribute("position_y").set_value(var2->position.y);


		if (var2->civilization == CivilizationType::GREEK)
			building.append_attribute("civilization").set_value("greek");
		else if (var2->civilization == CivilizationType::VIKING)
			building.append_attribute("civilization").set_value("viking");


		building.append_attribute("health").set_value(var2->GetHealth());


		if (var2->buildingStatus == BuildingStatus::CONSTRUCTING) {
			building.append_attribute("status").set_value("constructing");
			building.append_attribute("time").set_value(var2->timer_construction.ReadSec());
			building.append_attribute("progress").set_value(var2->GetPercentage());
		}
		else if(var2->buildingStatus == BuildingStatus::DESTROYED)
			building.append_attribute("status").set_value("destroyed");
		else
			building.append_attribute("status").set_value("finished");


		if(var2->buildingAction == BuildingAction::NOTHING)
			building.append_attribute("action").set_value("nothing");
		else if (var2->buildingAction == BuildingAction::PRODUCING) {
			building.append_attribute("action").set_value("producing");
			building.append_attribute("time").set_value(var2->GetTimeProducing());
			building.append_attribute("element").set_value(var2->GetElementProducing().c_str());
		}
		else {
			building.append_attribute("action").set_value("researching");
			building.append_attribute("time").set_value(var2->GetTimeProducing());
			building.append_attribute("element").set_value(var2->GetElementProducing().c_str());
		}
	}

	pugi::xml_node node3 = s.append_child("players");
	std::list<Entity*> list3 = App->entityManager->entities[EntityType::PLAYER];


	Player* p = getPlayer();

	pugi::xml_node player;
	if (p->civilization == CivilizationType::VIKING)
		player = node3.append_child("viking");
	else if (p->civilization == CivilizationType::GREEK)
		player = node3.append_child("greek");

	pugi::xml_node economy = player.append_child("economy");
	economy.append_attribute("faith").set_value(p->GetFaith());
	economy.append_attribute("prayers").set_value(p->GetPrayers());
	economy.append_attribute("sacrifices").set_value(p->GetSacrifices());


	pugi::xml_node research = player.append_child("research");
	research.append_child("temple").append_attribute("research").set_value(p->research_temple);
	research.append_child("encampment").append_attribute("research").set_value(p->research_encampment);
	research.append_child("cleric").append_attribute("research").set_value(p->research_cleric);
	research.append_child("assassin").append_attribute("research").set_value(p->research_assassin);
	research.append_child("lawful_beast").append_attribute("research").set_value(p->research_lawful_beast);
	research.append_child("chaotic_beast").append_attribute("research").set_value(p->research_chaotic_beast);
	research.append_child("lawful_miracle").append_attribute("research").set_value(p->research_lawful_miracle);
	research.append_child("chaotic_miracle").append_attribute("research").set_value(p->research_chaotic_miracle);
	research.append_child("lawful_victory").append_attribute("research").set_value(p->research_lawful_victory);
	research.append_child("chaotic_victory").append_attribute("research").set_value(p->research_chaotic_victory);

	return true;
}

Entity* EntityManager::CreatePlayerEntity(std::string civilization_string)
{
	Entity* ret = nullptr;

	ret = new Player();
	ret->type = EntityType::PLAYER;
	Player* p = (Player*)ret;

	entities[EntityType::PLAYER].push_back(ret);

	if (civilization_string == "viking") {
		ret->civilization = CivilizationType::VIKING;
		p->player_type = CivilizationType::VIKING;

	}
	else if (civilization_string == "greek") {
		ret->civilization = CivilizationType::GREEK;
		p->player_type = CivilizationType::GREEK;
	}
	else {
		ret->civilization = CivilizationType::VIKING;
		p->player_type = CivilizationType::VIKING;
	}
	ret->Start();

	return ret;
}

void EntityManager::InitVikings()
{
	if (App->scene->isInTutorial == false) {
		iPoint fortress = { 21,23 };
		fortress = App->map->MapToWorld(fortress.x, fortress.y);
		fortress.x -= App->map->GetTilesHalfSize().x;

		iPoint monkPos = { 26,24 };
		iPoint assassinPos = { 25,24 };
		monkPos = App->map->MapToWorld(monkPos.x, monkPos.y);
		assassinPos = App->map->MapToWorld(assassinPos.x, assassinPos.y);

		App->entityManager->CreateBuildingEntity(fortress, BuildingType::FORTRESS, App->entityManager->buildingsData[0], CivilizationType::VIKING);
		App->entityManager->CreateUnitEntity(UnitType::MONK, monkPos, CivilizationType::VIKING);
		App->entityManager->CreateUnitEntity(UnitType::ASSASSIN, assassinPos, CivilizationType::VIKING);
	}
	else if (App->scene->isInTutorial == true) {
		iPoint fortress = { 69,70 };
		fortress = App->map->MapToWorld(fortress.x, fortress.y);
		fortress.x -= App->map->GetTilesHalfSize().x;

		iPoint monkPos = { 69,76 };
		iPoint assassinPos = { 77,68 };
		monkPos = App->map->MapToWorld(monkPos.x, monkPos.y);
		assassinPos = App->map->MapToWorld(assassinPos.x, assassinPos.y);

		App->entityManager->CreateBuildingEntity(fortress, BuildingType::FORTRESS, App->entityManager->buildingsData[0], CivilizationType::VIKING);
		App->entityManager->CreateUnitEntity(UnitType::MONK, monkPos, CivilizationType::VIKING);
		if (getPlayer()->civilization == CivilizationType::VIKING) App->entityManager->CreateUnitEntity(UnitType::ASSASSIN, assassinPos, CivilizationType::VIKING);
	}
}

void EntityManager::InitGreek()
{
	if (App->scene->isInTutorial == false) {
		iPoint fortress = { 129,137 };
		fortress = App->map->MapToWorld(fortress.x, fortress.y);
		fortress.x -= App->map->GetTilesHalfSize().x;

		iPoint monkPos = { 130,139 };
		iPoint assassinPos = { 129,139 };
		monkPos = App->map->MapToWorld(monkPos.x, monkPos.y);
		assassinPos = App->map->MapToWorld(assassinPos.x, assassinPos.y);

		App->entityManager->CreateBuildingEntity(fortress, BuildingType::FORTRESS, App->entityManager->buildingsData[4], CivilizationType::GREEK);
		App->entityManager->CreateUnitEntity(UnitType::MONK, monkPos, CivilizationType::GREEK);
		App->entityManager->CreateUnitEntity(UnitType::ASSASSIN, assassinPos, CivilizationType::GREEK);
	}
	else if (App->scene->isInTutorial == true) {
		iPoint fortress = { 88,89 };
		fortress = App->map->MapToWorld(fortress.x, fortress.y);
		fortress.x -= App->map->GetTilesHalfSize().x;

		iPoint monkPos = { 78,85 };
		iPoint assassinPos = { 85,76 };
		monkPos = App->map->MapToWorld(monkPos.x, monkPos.y);
		assassinPos = App->map->MapToWorld(assassinPos.x, assassinPos.y);

		App->entityManager->CreateBuildingEntity(fortress, BuildingType::FORTRESS, App->entityManager->buildingsData[4], CivilizationType::GREEK);
		App->entityManager->CreateUnitEntity(UnitType::MONK, monkPos, CivilizationType::GREEK);
		if(getPlayer()->civilization == CivilizationType::GREEK) App->entityManager->CreateUnitEntity(UnitType::ASSASSIN, assassinPos, CivilizationType::GREEK);
	}
}

void EntityManager::BuildCivilizations(CivilizationType c)
{
	if (c == CivilizationType::GREEK)
	{
		InitGreek();
	}
	else if (c == CivilizationType::VIKING)
	{
		InitVikings();
	}
}

Entity* EntityManager::CreateUnitEntity(UnitType type, iPoint pos, CivilizationType civ)
{
	pos.x += 20;
	AABBNode* node = aabbTree.FindLowestNodeInPoint(&aabbTree.baseNode, (Point)pos);

	Entity* ret = nullptr;

	switch (type)
	{
	case UnitType::ASSASSIN:
		ret = new CombatUnit(UnitType::ASSASSIN, pos);
		switch (civ)
		{
		case VIKING:
			if (App->fade_to_black->actual_civilization == "greek") {
				ret->texture = enemyTextureAssassin;
			}
			break;
		case GREEK:
			if (App->fade_to_black->actual_civilization == "viking") {
				ret->texture = enemyTextureAssassin;
			}
			break;
		}
		FxUnits(4, CreateAssasin_sound, pos.x, pos.y);
		break;
	case UnitType::MONK:
		ret = new Unit(UnitType::MONK, pos);
		switch (civ)
		{
		case VIKING:
			if (App->fade_to_black->actual_civilization == "greek") {
				ret->texture = enemyTextureMonk;
			}
			break;
		case GREEK:
			if (App->fade_to_black->actual_civilization == "viking") {
				ret->texture = enemyTextureMonk;
			}
			break;
		}
		
		break;
	case UnitType::PIKEMAN:
		ret = new CombatUnit(UnitType::PIKEMAN, pos);
		FxUnits(4, CreateAssasin_sound, pos.x, pos.y);
		break;
	case UnitType::JOTNAR:
		ret = new Unit(UnitType::JOTNAR, pos);
		break;
	case UnitType::DRAUGAR:
		ret = new Unit(UnitType::DRAUGAR, pos);
		break;
	case UnitType::CYCLOP:
		ret = new Unit(UnitType::CYCLOP, pos);
		break;
	case UnitType::MINOTAUR:
		ret = new Unit(UnitType::MINOTAUR, pos);
		break;
	case UnitType::CLERIC:
		ret = new Unit(UnitType::CLERIC, pos);
		FxUnits(4, CreateMonk_sound, pos.x, pos.y);
		switch (civ)
		{
		case VIKING:
			if (App->fade_to_black->actual_civilization == "greek") {
				ret->texture = enemyTextureCleric;
			}
			break;
		case GREEK:
			if (App->fade_to_black->actual_civilization == "viking") {
				ret->texture = enemyTextureCleric;
			}
			break;
		}
		break;
	}
	if (ret != nullptr)
	{
		ret->civilization = civ;
		ret->type = EntityType::UNIT;
		if (ret->texture == nullptr) {
			ret->texture = animationManager.charData[type].texture;
		}
		entities[EntityType::UNIT].push_back(ret);

		aabbTree.AddUnitToTree(*ret);

	}
	return ret;
}

void EntityManager::DrawEverything()
{

	float dt = App->GetDT();

	for (unsigned i = (int)EntityType::UNIT; i < entities.size(); i++)
	{
		for (std::list<Entity*>::iterator it = entities[(EntityType)i].begin(); it != entities[(EntityType)i].end(); it++)
		{
			orderedSprites.insert({(*it)->position.y, (*it)});
		}
	}

	auto range = orderedSprites.equal_range(0);
	for (auto i = orderedSprites.begin(); i != orderedSprites.end(); i = range.second)
	{
		// Get the range of the current key
		range = orderedSprites.equal_range(i->first);

		// Now print out that whole range
		for (auto d = range.first; d != range.second; ++d)
			(*d).second->Draw(dt);
	}

	orderedSprites.clear();

}

Entity* EntityManager::CreateBuildingEntity(iPoint pos, BuildingType type, BuildingInfo info, CivilizationType civilization)
{

	//Figure lowest node out
	quadTree.FindLowestNodeInPoint(&quadTree.baseNode, { pos.x, pos.y });


	Entity* ret = nullptr;
	switch (type)
	{
	case FORTRESS:
		ret = new Building(BuildingType::FORTRESS, pos, info);
		FxUnits(4, App->audio->Building_placed, pos.x, pos.y);
		break;
	case MONASTERY:
		ret = new Building(BuildingType::MONASTERY, pos, info);
		FxUnits(4, App->audio->Building_placed, pos.x, pos.y);
		break;
	case TEMPLE:
		ret = new Building(BuildingType::TEMPLE, pos, info);
		FxUnits(4, App->audio->Building_placed, pos.x, pos.y);
		break;
	case ENCAMPMENT:
		ret = new Building(BuildingType::ENCAMPMENT, pos, info);
		FxUnits(4, App->audio->Building_placed, pos.x, pos.y);
		break;
	}


	iPoint iso = pos;
	iso += App->map->GetTilesHalfSize();
	iso = App->map->WorldToMap(iso.x, iso.y);

	for (int y = iso.y; y > iso.y - info.tileLenght; y--)
	{
		for (int x = iso.x; x < iso.x + info.tileLenght; x++)
		{

			if (IN_RANGE(x, 0, App->map->data.width - 1) == 1 && IN_RANGE(y, 0, App->map->data.height - 1) == 1)
			{
				App->pathfinding->ChangeMapValue({ x, y }, 0);
			}
		}
	}

	ret->civilization = civilization;
	ret->type = EntityType::BUILDING;
	ret->texture = entitySpriteSheets[SpriteSheetType::BUILDINGS];


	entities[EntityType::BUILDING].push_back(ret);
	//TODO: sort elements only inside the screen (QuadTree)
	entities[EntityType::BUILDING].sort(entity_Sort());

	//Add to quadTree
	quadTree.AddEntityToNode(*ret, { pos.x + App->map->data.tile_width / 2, pos.y });

	return ret;
}

void EntityManager::EnterBuildMode()
{
	crPreview.active = !crPreview.active;
	UpdateBuildPreview(buildingTestIndex);
}
void EntityManager::SetBuildIndex(int index)
{
	if (index < MAX_BUILDING_TYPES) {
		buildingTestIndex = index;
	}

	UpdateBuildPreview(buildingTestIndex);
}

//Called when deleting a new Entity
bool EntityManager::DeleteEntity(Entity* e)
{
	if (e != nullptr)
	{

		switch (e->type)
		{
			case EntityType::UNIT:
			{
				//Delete from AABBtree
				AABBNode* node = aabbTree.FindLowestNodeInPoint(&aabbTree.baseNode, static_cast<Point>(e->position));

				node->data.remove(e);

				if (node->data.size() <= 0)
				{
					AABBNode* parent = node->parent;

					//if (parent)
					//{
					//	for (int i = 0; i < parent->childNodes.size(); i++)
					//	{
					//		if (parent->childNodes[i].data.size() > 0)
					//		{
					//			//parent->data.merge(parent->childNodes[i].data);
					//			parent->data.insert(parent->data.end(), parent->childNodes[i].data.begin(), parent->childNodes[i].data.end());

					//			parent->childNodes[i].data.clear();
					//		}
					//	}
					//}

					//parent->isDivided = false;
					//parent->childNodes.clear();
					//parent->childNodes.shrink_to_fit();


				}
				break;
			}


			case EntityType::BUILDING:
			{

				quadTree.FindLowestNodeInPoint(&quadTree.baseNode, e->position);

				quadTree.lowestNode->data.remove(e);

						//QuadNode* parent = quadTree.lowestNode->parent;

						//bool isEmpty = true;

						////Same as aabbTree, you need to check if every possible child inside a child is empty before merging
						//if (parent)
						//{
						//	for (int i = 0; i < QUADNODE_CHILD_NUMBER; i++)
						//	{
						//		if (parent->childNodes[i].data.size() > 0)
						//		{
						//			isEmpty = false;
						//		}
						//	}

						//	if (isEmpty && parent != nullptr)
						//	{
						//		for (int i = 0; i < QUADNODE_CHILD_NUMBER; i++)
						//		{
						//			parent->data.insert(parent->data.end(), parent->childNodes[i].data.begin(), parent->childNodes[i].data.end());
						//			parent->childNodes[i].data.clear();
						//		}

						//		parent->isDivided = false;
						//		parent->childNodes.clear();
						//		parent->childNodes.shrink_to_fit();

						//	}
						//}

				quadTree.lowestNode = nullptr;

				break;
			}
		}
		entities[e->type].remove(e);
	}
	return true;
}

void EntityManager::UpdateBuildPreview(int index)
{
	BuildingInfo data = buildingsData[index];
	crPreview.height = data.tileLenght;
	crPreview.width = data.tileLenght;
}

void EntityManager::LoadBuildingsData(pugi::xml_node& node)
{
	if (node != NULL)
	{
		pugi::xml_node obj;

		for (obj = node.child("object"); obj; obj = obj.next_sibling("object"))
		{

			BuildingInfo info;
			bool push = true;
			info.spriteRect = { obj.attribute("x").as_int(), obj.attribute("y").as_int(), obj.attribute("width").as_int(), obj.attribute("height").as_int() };

			pugi::xml_node data = obj.child("properties");
			if (data != NULL)
			{
				pugi::xml_node prop;
				for (prop = data.child("property"); prop; prop = prop.next_sibling("property"))
				{
					//OPT: Not the best way but meh

					std::string name = prop.attribute("name").as_string();
					if (name == "civilization")
					{
						CivilizationType type = (CivilizationType)prop.attribute("value").as_int();
						info.civilization = type;
						if (IN_RANGE(type, VIKING, GREEK) == 0)
						{
							push = false;
						}
					}
					else if (push == false && name == "consType")
					{
						if (prop.attribute("value").as_int() == 0)
						{
							destructedSpriteRect = info.spriteRect;
						}
						else
						{
							constructorSpriteRect = info.spriteRect;
						}
						break;
					}
					else if (name == "tileSquareLenght")
					{
						info.tileLenght = prop.attribute("value").as_int();
					}
					else if (name == "type")
					{
						info.buildingType = (BuildingType)prop.attribute("value").as_int();
					}
				}
			}
			//TODO: Find a wat to mesure this with the tileLenght
			info.blitSize = { info.spriteRect.w, info.spriteRect.h };

			if (push)
				buildingsData.push_back(info);
		}
	}


}

iPoint EntityManager::CalculateBuildingSize(int bw, int w, int h)
{
	return {bw , (bw * h) / w};
}

Player* EntityManager::getPlayer() const
{
	return (Player*)App->entityManager->entities[EntityType::PLAYER].begin()._Ptr->_Myval;
}

bool EntityManager::IsPointInsideQuad(SDL_Rect rect, int x, int y)
{

	if (x >= rect.x && x <= rect.x + rect.w && y >= rect.y + rect.h && y <= rect.y)
		return true;

	return false;
}

void EntityManager::FxUnits(int channel, int fx, int posx, int posy)
{
	if (Mix_Playing(channel) == 0) {
		Mix_HaltChannel(channel);

		iPoint distance = { posx - (-App->render->camera.x + App->render->camera.w / 2), posy - (-App->render->camera.y + App->render->camera.h / 2) };

		int distance_normalized = (distance.x * distance.x + distance.y * distance.y);
		distance_normalized = distance_normalized / 750;
		volume = (distance_normalized * 255) / App->render->camera.w;

		float angle = 90;
		if (App->render->camera.y == 0) {
			angle = atan(-App->render->camera.x);
		}
		else {
			angle = atan((-App->render->camera.x) / (App->render->camera.y));
		}
		angle = angle * 57 + 360;


		if (volume < 0) { volume = 0; }
		if (volume > 255) { volume = 255; }

		Mix_SetPosition(channel, angle, volume);
		App->audio->PlayFx(channel, fx, 0);
	}

	/*if (Mix_Playing(channel) == 0) {

		Mix_HaltChannel(channel);


		iPoint distance = { ((posx - App->render->camera.x * App->render->camera.x) + (posy - App->render->camera.y * App->render->camera.y)) };
		distance = distance;
		int volume = (distance * 2000) / App->render->camera.w;
		if (volume < 0) {
			volume = 0;
		}
		if (volume > 200) {
			volume = 200;
		}

		float angle = 90;
		if (App->render->camera.y == 0) {
			angle = atan(-App->render->camera.x);
		}
		else {
			angle = atan((-App->render->camera.x) / (App->render->camera.y));
		}
		angle = angle * 57 + 360;

		Mix_SetPosition(channel, angle, volume);
		App->audio->PlayFx(channel, fx, 0);
	}*/
}
