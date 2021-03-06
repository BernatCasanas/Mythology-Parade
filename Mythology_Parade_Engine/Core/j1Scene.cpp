#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Input.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Map.h"
#include "j1PathFinding.h"
#include "j1Gui.h"
#include "j1Fonts.h"
#include "Animation.h"
#include "EntityManager.h"
#include "j1Minimap.h"
#include "CombatUnit.h"
#include "j1Scene.h"
#include "j1FadeToBlack.h"
#include "HUD.h"
#include "ResearchMenu.h"
#include "IA.h"

#include "j1TitleScene.h"
#include "j1ParticleManager.h"


j1Scene::j1Scene() : j1Module()
{
	name.append("scene");

	clickToPath = false;
	nextUnit_selected = nextBuilding_selected = building_meteor = false;
	update_selection = false;
	dont_update_types_of_troops = true;
	update_production_list = false;
}

// Destructor
j1Scene::~j1Scene()
{}

// Called before render is available
bool j1Scene::Awake(pugi::xml_node& config)
{
	LOG("Loading Scene");
	bool ret = true;

	App->scene->active = false;
	SDL_ShowCursor(0);
	hud = nullptr;
	research_menu = nullptr;

	return ret;
}

// Called before the first frame
bool j1Scene::Start()
{

	if (App->map->Load("MainMap.tmx") == true)
	{
		int w, h;
		uchar* data = NULL;
		if (App->map->CreateWalkabilityMap(w, h, &data))
			App->pathfinding->SetMap(w, h, data);

		App->fowManager->CreateFoWMap(App->map->data.width, App->map->data.height);

		mapLimitsRect = App->map->GetMapRect();
		App->pathfinding->maxPathLenght = App->map->GetMapMaxLenght();
		App->entityManager->LoadBuildingsBlitRect();

		//Init quadTree
		iPoint position;
		iPoint size;
		position = App->map->WorldToMap(0, 0);
		size = iPoint(App->map->data.width * App->map->data.tile_width, App->map->data.height * App->map->data.tile_height);
		App->entityManager->quadTree.Init(TreeType::ISOMETRIC, position.x + (App->map->data.tile_width / 2), position.y, size.x, size.y);

		SDL_ShowCursor(0);

		RELEASE_ARRAY(data);
	}

  //Load building debug textures
	debugBlue_tex = App->tex->Load("maps/path2.png");
	debugRed_tex = App->tex->Load("maps/cantBuild.png");

	

	App->gui->sfx_UI[(int)UI_Audio::SAVE] = App->audio->LoadFx("audio/ui/Save.wav");
	App->gui->sfx_UI[(int)UI_Audio::LOAD] = App->audio->LoadFx("audio/ui/load.wav");
	App->gui->sfx_UI[(int)UI_Audio::CONFIRMATION] = App->audio->LoadFx("audio/ui/Click_Standard2.wav");
	App->gui->sfx_UI[(int)UI_Audio::OPTIONS] = App->audio->LoadFx("audio/ui/Settings_Click.wav");
	App->gui->sfx_UI[(int)UI_Audio::RESTART] = App->audio->LoadFx("audio/ui/Restart.wav");
	App->gui->sfx_UI[(int)UI_Audio::SURRENDER] = App->audio->LoadFx("audio/ui/Surrender.wav");
	App->gui->sfx_UI[(int)UI_Audio::EXIT] = App->audio->LoadFx("audio/ui/Exit.wav");
	App->gui->sfx_UI[(int)UI_Audio::CLOSE] = App->audio->LoadFx("audio/ui/Close_Menu.wav");
	App->gui->sfx_UI[(int)UI_Audio::HOVER] = App->audio->LoadFx("audio/ui/Hover.wav");
	App->gui->sfx_UI[(int)UI_Audio::MAIN_MENU] = App->audio->LoadFx("audio/ui/Click_Main_Menu.wav");

	WinViking_sound = App->audio->LoadFx("audio/fx/WinVikings.wav");
	WinGreek_sound = App->audio->LoadFx("audio/fx/win_greeks.wav");
	Lose_sound = App->audio->LoadFx("audio/fx/lose_sound.wav");
	OpenPauseMenu_sfx = App->audio->LoadFx("audio/fx/OpenPause.wav");
	Research_sound = App->audio->LoadFx("audio/fx/Research_Sound.wav");
	ResearchFinished = App->audio->LoadFx("audio/fx/ResearchFinished.wav");
	


	paused_game = false;
	godMode = false;

	//iPoint position;
	//iPoint size;
	//position = App->map->WorldToMap(0, 0);
	//size = iPoint(App->map->data.width * App->map->data.tile_width, App->map->data.height * App->map->data.tile_height);
	//quadTree = new QuadTree(TreeType::ISOMETRIC, position.x + (App->map->data.tile_width / 2), position.y, size.x, size.y);
	//quadTree->baseNode->SubDivide(quadTree->baseNode, 5);

	App->audio->PlayMusic("audio/music/Ambient1.ogg", 2.0F);

	//Creating players
	Player* player = static_cast<Player*>(App->entityManager->CreatePlayerEntity(App->fade_to_black->actual_civilization));

	if (player->player_type == CivilizationType::VIKING)
	{
		App->render->camera.x = 580;
		App->render->camera.y = -369;
	}
	else if (player->player_type == CivilizationType::GREEK)
	{
		App->render->camera.x = 779;
		App->render->camera.y = -3931;
	}

	research_menu = new ResearchMenu(player);
	hud = new HUD(research_menu);

	return true;
}

// Called each loop iteration
bool j1Scene::PreUpdate()
{
	// debug pathfing ------------------
	if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
	{
		ClickToPath();
	}
	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN && clickToPath)
	{
		ClickToPath();
		clickToPath = false;
		App->entityManager->getPlayer()->dontSelect = true;
		App->gui->cursor_move = false;
	}
	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN && App->gui->cursor_attack==true)
	{
		App->gui->cursor_attack = false;
	}
	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN && App->gui->cursor_heal == true)
	{
		App->gui->cursor_heal = false;
	}

	if (App->title_scene->wantToLoad)
	{
		App->LoadGame("info.xml");
		App->title_scene->wantToLoad = false;
	}

	// Move Camera if click on the minimap
	int mouse_x, mouse_y;
	if (((App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) || (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)) && paused_game == false)
	{
		App->input->GetMousePosition(mouse_x, mouse_y);
		SDL_Rect minimap = { App->minimap->position.x +15, App->minimap->position.y + 7, App->minimap->width - 28, App->minimap->height -15};

		if ((mouse_x > minimap.x) && (mouse_x < minimap.x + minimap.w) && (mouse_y > minimap.y) && (mouse_y < minimap.y + minimap.h))
		{
			iPoint minimap_mouse_position = App->minimap->ScreenToMinimapToWorld(mouse_x, mouse_y);
			App->render->camera.x = -(minimap_mouse_position.x - App->render->camera.w * 0.5f);
			App->render->camera.y = -(minimap_mouse_position.y - App->render->camera.h * 0.5f);
		}
	}
	return true;
}

void j1Scene::ClickToPath()
{
	//TMP: Temporal pathfinding debug
	std::list<Entity*> list = App->entityManager->getPlayer()->GetEntitiesSelected();

	if (list.size() > 0)
	{
		float n = App->entityManager->getPlayer()->GetEntitiesSelected().size();
		float x = 0, y = 0;

		for (std::list<Entity*>::iterator it = list.begin(); it != list.end(); it++)
		{
			x += it._Ptr->_Myval->position.x;
			y += it._Ptr->_Myval->position.y;
		}

		x /= n;
		y /= n;

		iPoint origin = App->map->WorldToMap((int)x, (int)y);
		iPoint ending = App->map->GetMousePositionOnMap();
		LOG("Origin: %i, %i", origin.x, origin.y);
		LOG("Ending: %i, %i", ending.x, ending.y);

		iPoint PointToArrow = App->map->MapToWorld(ending.x, ending.y);
		App->particleManager->CreateParticle({ PointToArrow.x,PointToArrow.y}, { 0,0 }, 10, ParticleAnimation::Arrows_Cursor);

		int posX, posY;
		App->input->GetMousePosition(posX, posY);
		iPoint p = App->render->ScreenToWorld(posX, posY);
		p = App->render->ScreenToWorld(posX, posY);

		CivilizationType playerCiv = App->entityManager->getPlayer()->civilization;
		bool attacking = false;


		//for (int i = 1; i < 3; i++)
		//{
		//	{
		//		for (std::list<Entity*>::iterator it = App->entityManager->entities[static_cast<EntityType>(i)].begin(); it != App->entityManager->entities[static_cast<EntityType>(i)].end(); it++)
		//		{
		//			if (!it._Ptr->_Myval->IsDeath()) {
		//				SDL_Rect collider = it._Ptr->_Myval->getCollisionRect();
		//				if (it._Ptr->_Myval->civilization != playerCiv && EntityManager::IsPointInsideQuad(collider, p.x, p.y))
		//				{
		//					Unit* unt = nullptr;
		//					for (std::list<Entity*>::iterator sel = list.begin(); sel != list.end(); sel++)
		//					{
		//						unit = (Unit*)sel._Ptr->_Myval;
		//						unt->enemyTarget = it._Ptr->_Myval;
		//						attacking = true;
		//					}
		//				}
		//			}
		//		}
		//	}
		//}


		//if (!attacking)
		//{
		//	Unit* unt = nullptr;
		//	for (std::list<Entity*>::iterator sel = list.begin(); sel != list.end(); sel++)
		//	{
		//		unt = (Unit*)sel._Ptr->_Myval;
		//		//unt->enemyTarget = nullptr;
		//	}
		//}

		iPoint mapPos = { 0, 0 };
		for (auto& it : list)
		{
			mapPos = App->map->WorldToMap(it->position.x, it->position.y);
			if (mapPos != ending)
				App->pathfinding->RequestPath(mapPos, ending, it);
		}

	}
}

// Called each loop iteration
bool j1Scene::Update(float dt)
{
	//Update selection if needed
	if (update_selection == true) {
		hud->HUDUpdateSelection(App->entityManager->getPlayer()->GetEntitiesSelected(), nullptr, dont_update_types_of_troops);
		update_selection = false;
		dont_update_types_of_troops = true;
	}

	//Update IA bar
	float ia_bar_percentage = App->ia->timer_ia.ReadSec() / App->ia->time_ia;
	if (ia_bar_percentage > 1)
		ia_bar_percentage = 1;
	hud->UpdateIABar(ia_bar_percentage);

	// Gui ---
	switch (hud->close_menus)
	{
	case CloseSceneMenus::Pause:
		App->audio->FadeAudio(which_audio_fade::change_volume, 2, 150);
		hud->DeactivatePauseMenu();
		hud->close_menus = CloseSceneMenus::None;
		break;
	case CloseSceneMenus::Options:
		hud->DeactivateOptionsMenu();
		hud->close_menus = CloseSceneMenus::None;
		break;
	case CloseSceneMenus::Confirmation:
		hud->DeactivateConfirmationMenu();
		hud->close_menus = CloseSceneMenus::None;
		break;
	case CloseSceneMenus::Confirmation_and_Pause:
		hud->DeactivateConfirmationMenu();
		hud->DeactivatePauseMenu();
		hud->close_menus = CloseSceneMenus::None;
		break;
	case CloseSceneMenus::Research:
		hud->DeactivateResearchMenu();
		hud->close_menus = CloseSceneMenus::None;
		break;
	}
	// -------
	//if (App->input->GetKey(SDL_SCANCODE_F3) == KEY_DOWN)
	//	App->LoadGame("save_game.xml");

	//if (App->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN)
	//	App->SaveGame("save_game.xml");

	if (App->input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN) {
		hud->ActivatePauseMenu();
		App->audio->FadeAudio(which_audio_fade::change_volume, 2, 50);
	}
    if (paused_game == true) {
      if (hud->ui_volume_sliders[0] != nullptr)
        hud->UpdateSlider(0);
      if (hud->ui_volume_sliders[3] != nullptr)
        hud->UpdateSlider(3);
    }


	SDL_Rect correctedCamera = App->render->camera;
	correctedCamera.x = -correctedCamera.x;
	correctedCamera.y = -correctedCamera.y;

	if (App->input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT || App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
	{
		if (correctedCamera.y - floor(1000.0f * dt) >= mapLimitsRect.y)
		{
			App->render->camera.y += floor(1000.0f * dt);
		}
		else
		{
			App->render->camera.y = 0;
		}
	}

	if (App->input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT || App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
	{
		if (correctedCamera.y + App->render->camera.h + floor(1000.0f * dt) <= mapLimitsRect.h)
		{
			App->render->camera.y -= floor(1000.0f * dt);
		}
		else
		{
			App->render->camera.y = -mapLimitsRect.h + App->render->camera.h;
		}
	}

	if (App->input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT || App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
	{
		if (correctedCamera.x - floor(1000.0f * dt) >= mapLimitsRect.x)
		{
			App->render->camera.x += floor(1000.0f * dt);
		}
		else
		{
			App->render->camera.x = -mapLimitsRect.x;
		}
	}

	if (App->input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT || App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
	{
		if (correctedCamera.x + App->render->camera.w + floor(1000.0f * dt) <= mapLimitsRect.x + mapLimitsRect.w)
		{
			App->render->camera.x -= floor(1000.0f * dt);
		}
		else
		{
			App->render->camera.x = -(mapLimitsRect.x + mapLimitsRect.w) + App->render->camera.w;
		}
	}


	App->map->Draw();


	//Quad draw

	//if (App->input->drawDebug)
	//	App->render->DrawQuadTree(quadTree->type, quadTree->baseNode);

	if (godMode) {
		iPoint p = App->map->GetMousePositionOnMap();
		if (!App->entityManager->crPreview.active && IN_RANGE(p.x, 0, App->map->data.width - 1) == 1 && IN_RANGE(p.y, 0, App->map->data.height - 1) == 1)
		{
			p = App->map->MapToWorld(p.x, p.y);
			App->render->Blit(debugBlue_tex, p.x, p.y);
		}
	}


	//App->render->DrawQuad(mapLimitsRect, 255, 255, 255, 40);

	/*CheckSpatial Audio
	if (App->input->GetKey(SDL_SCANCODE_M) == KEY_DOWN) {
		Mix_HaltChannel(-1);
		Mix_SetPosition(2, 270, 200);
		App->audio->PlayFx(2, WinGreek_sound, 1);
	}
	if (App->input->GetKey(SDL_SCANCODE_O) == KEY_DOWN) {
		Mix_HaltChannel(-1);
		Mix_SetPosition(3, 90, 1);
		App->audio->PlayFx(3, WinGreek_sound, 1);
	}*/

	if (App->input->GetMouseWheel() > 0) {
		if (hud->HUDScrollUp() == true) {
			App->render->camera.x = -(hud->thing_selected->position.x - App->render->camera.w * 0.5f);
			App->render->camera.y = -(hud->thing_selected->position.y - App->render->camera.h * 0.5f);
		}
	}
	else if (App->input->GetMouseWheel() < 0) {
		if (hud->HUDScrollDown() == true) {
			App->render->camera.x = -(hud->thing_selected->position.x - App->render->camera.w * 0.5f);
			App->render->camera.y = -(hud->thing_selected->position.y - App->render->camera.h * 0.5f);
		}
	}

	return true;
}

// Called each loop iteration
bool j1Scene::PostUpdate()
{

	if (App->entityManager->quadTree.displayTree)
		App->render->DrawQuadTree(App->entityManager->quadTree.type, App->entityManager->quadTree.baseNode);

	if (App->entityManager->aabbTree.displayTree)
		App->render->DrawAABBTree(App->entityManager->aabbTree.baseNode);

	if (hud->thing_selected != nullptr) {
		hud->UpdateSelectedThing();
		if (App->entityManager->getPlayer()->player_type == CivilizationType::VIKING)
			hud->ManageActionButtons();
		else if (App->entityManager->getPlayer()->player_type == CivilizationType::GREEK)
			hud->ManageActionButtons(false, false);
	}
	bool ret = true;

	return ret;
}

// Called before quitting
bool j1Scene::CleanUp()
{
	LOG("Freeing scene");

	App->tex->UnLoad(debugBlue_tex);
	App->tex->UnLoad(debugRed_tex);
	if (hud != nullptr) {
		hud->DeactivateResearchMenu();
		hud->DeactivateConfirmationMenu();
		hud->DeactivateOptionsMenu();
		hud->DeactivatePauseMenu();
		hud->HUDDeleteListTroops();
		hud->HUDDeleteSelectedTroop();
		hud->HUDDeleteActionButtons();
		App->gui->DeleteUIElement(hud->ui_ingame);
		hud->ui_ingame = nullptr;
		App->gui->DeleteUIElement(hud->ia_bar_back);
		hud->ia_bar_back = nullptr;
		App->gui->DeleteUIElement(hud->ia_bar_front);
		hud->ia_bar_front = nullptr;
		for (int i = 0; i < 3; i++)
		{
			App->gui->DeleteUIElement(hud->ui_text_ingame[i]);
			hud->ui_text_ingame[i] = nullptr;
		}
		//quadTree->Clear();
		delete hud;
		hud = nullptr;
		delete research_menu;
		research_menu = nullptr;
	}
	
	

	App->audio->CleanFxs(ResearchFinished);
	App->audio->CleanFxs(Research_sound);
	App->audio->CleanFxs(OpenPauseMenu_sfx);
	App->audio->CleanFxs(Lose_sound);
	App->audio->CleanFxs(WinGreek_sound);
	App->audio->CleanFxs(WinViking_sound);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::MAIN_MENU]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::HOVER]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::CLOSE]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::EXIT]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::SURRENDER]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::RESTART]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::OPTIONS]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::CONFIRMATION]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::LOAD]);
	App->audio->CleanFxs(App->gui->sfx_UI[(int)UI_Audio::SAVE]);

	return true;
}



// Called when returning to main menu (either winning/losing or by menu options like exit)
void j1Scene::BackToTitleMenu() {
	if(isInTutorial==true)
		App->fade_to_black->FadeToBlack(which_fade::tutorial_to_title, 2);
	else
		App->fade_to_black->FadeToBlack(which_fade::scene_to_title, 2);

	//App->change_scene = true;
}

// Called when restarting the game
void j1Scene::RestartGame() {
	CivilizationType civ = App->entityManager->getPlayer()->civilization;
	if (civ == CivilizationType::GREEK)
	{
		if (isInTutorial == true)
			App->fade_to_black->FadeToBlack(which_fade::tutorial_to_tutorial, 2, "greek");
		else
			App->fade_to_black->FadeToBlack(which_fade::scene_to_scene, 2, "greek");
	}
	else if (civ == CivilizationType::VIKING) 
	{
		if (isInTutorial == true)
			App->fade_to_black->FadeToBlack(which_fade::tutorial_to_tutorial, 2, "viking");
		else
			App->fade_to_black->FadeToBlack(which_fade::scene_to_scene, 2, "viking");
	}
	App->entityManager->initCivilizations = true;
}

// Called to return faith after canceling a production
void j1Scene::ReturnFaith(std::string thing_canceled)
{
	if (thing_canceled == "Assassin") {
		App->entityManager->getPlayer()->IncreaseFaith(25);
	}
	else if (thing_canceled == "Monk") {
		App->entityManager->getPlayer()->IncreaseFaith(20);
	}
	else if (thing_canceled == "Victory") {
		App->entityManager->getPlayer()->IncreaseFaith(100);
	}
	else if (thing_canceled == "Sacrifices") {
		App->entityManager->getPlayer()->IncreaseFaith(15);
	}
	else if (thing_canceled == "Prayers") {
		App->entityManager->getPlayer()->IncreaseFaith(15);
	}
	else if (thing_canceled == "Cleric") {
		App->entityManager->getPlayer()->IncreaseFaith(25);
	}
	else if (thing_canceled == "Lawful_Beast") {
		App->entityManager->getPlayer()->IncreaseFaith(50);
	}
	else if (thing_canceled == "Chaotic_Beast") {
		App->entityManager->getPlayer()->IncreaseFaith(50);
	}

}

void j1Scene::OnClick(UI* element, float argument)
{

	switch (element->type)
	{

	case Type::BUTTON:

		if (element->name == "SAVE")
		{
			hud->confirmation_option = "SAVE";
			hud->ActivateConfirmationMenu("SAVE THE GAME");
		}
		else if (element->name == "LOAD")
		{
			hud->confirmation_option = "LOAD";
			hud->ActivateConfirmationMenu("LOAD THE GAME");
		}
		else if (element->name == "OPTIONS")
		{
			hud->ActivateOptionsMenu();
		}
		else if (element->name == "CLOSE OPTIONS")
		{
			hud->close_menus = CloseSceneMenus::Options;
		}
		else if (element->name == "RESTART")
		{
			hud->confirmation_option = "RESTART";
			hud->ActivateConfirmationMenu("RESTART");
		}
		else if (element->name == "SURRENDER")
		{
			hud->confirmation_option = "SURRENDER";
			hud->ActivateConfirmationMenu("SURRENDER");
			//////TODO: HERE LOSE CONDITION WILL BE TRUE
		}
		else if (element->name == "EXIT")
		{
			hud->confirmation_option = "EXIT";
			hud->ActivateConfirmationMenu("EXIT");
		}
		else if (element->name == "NO")
		{
			hud->close_menus = CloseSceneMenus::Confirmation;
		}
		else if (element->name == "YES")
		{
			hud->close_menus = CloseSceneMenus::Confirmation;
			if (hud->confirmation_option.compare("SAVE") == 0)
			{
				App->SaveGame("info.xml");
			}
			else if (hud->confirmation_option.compare("LOAD") == 0)
			{
				App->LoadGame("info.xml");
			}
			else if (hud->confirmation_option.compare("RESTART") == 0)
			{
				//close_menus = CloseSceneMenus::Confirmation_and_Pause;
				RestartGame();
			}
			else if (hud->confirmation_option.compare("SURRENDER") == 0)
			{
				App->entityManager->getPlayer()->player_lose = true;
				hud->close_menus = CloseSceneMenus::Confirmation_and_Pause;
			}
			else if (hud->confirmation_option.compare("EXIT") == 0)
			{
				App->entityManager->initCivilizations = true;
				BackToTitleMenu();
			}
		}
		else if (element->name == "CLOSE RESEARCH")
		{
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "FULLSCREEN") {
			App->win->ToggleFullscreen();
			if (hud->ui_button_options[1]->sprite1.y == 21) {
				hud->ui_button_options[1]->sprite1.y = hud->ui_button_options[1]->sprite2.y = hud->ui_button_options[1]->sprite3.y = 61;
			}
			else if(hud->ui_button_options[1]->sprite1.y == 61) {
				hud->ui_button_options[1]->sprite1.y = hud->ui_button_options[1]->sprite2.y = hud->ui_button_options[1]->sprite3.y = 21;
			}
		}
		else if (element->name == "CLOSE")
		{
			hud->close_menus = CloseSceneMenus::Pause;
		}
		else if (element->name == "Research")
		{
			hud->ActivateResearchMenu();
		}
		else if (element->name == "RESEARCH TEMPLE") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Temple", true);
			//building->StartResearching("Temple");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH ENCAMPMENT") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Encampment", true);
			//building->StartResearching("Encampment");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH CLERIC") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Cleric", true);
			//building->StartResearching("Cleric");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH ASSASSIN") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Assassin", true);
			//building->StartResearching("Assassin");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH LAWFUL BEAST") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Lawful Beast", true);
			//building->StartResearching("Lawful Beast");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH CHAOTIC BEAST") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Chaotic Beast", true);
			//building->StartResearching("Chaotic Beast");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH LAWFUL MIRACLE") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Lawful Miracle", true);
			//building->StartResearching("Lawful Miracle");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH CHAOTIC MIRACLE") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Chaotic Miracle", true);
			//building->StartResearching("Chaotic Miracle");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH LAWFUL VICTORY") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Lawful Victory", true);
			//building->StartResearching("Lawful Victory");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "RESEARCH CHAOTIC VICTORY") {
			Building* building = (Building*)hud->thing_selected;
			building->ProduceQueue("Chaotic Victory", true);
			//building->StartResearching("Chaotic Victory");
			hud->close_menus = CloseSceneMenus::Research;
		}
		else if (element->name == "Produce_Temple")
		{
			//CONSTRUCT TEMPLE
			App->entityManager->EnterBuildMode();
			if (App->entityManager->getPlayer()->civilization == CivilizationType::VIKING)
				App->entityManager->SetBuildIndex(2);
			else if (App->entityManager->getPlayer()->civilization == CivilizationType::GREEK)
				App->entityManager->SetBuildIndex(6);
		}
		else if (element->name == "Produce_Encampment")
		{
			//CONSTRUCT ENCAMPMENT
			App->entityManager->EnterBuildMode();
			if(App->entityManager->getPlayer()->civilization==CivilizationType::VIKING)
				App->entityManager->SetBuildIndex(3);
			else if (App->entityManager->getPlayer()->civilization == CivilizationType::GREEK)
				App->entityManager->SetBuildIndex(7);

		}
		else if (element->name == "Produce_Monastery")
		{
			//CONSTRUCT MONASTERY
			App->entityManager->EnterBuildMode();
			if (App->entityManager->getPlayer()->civilization == CivilizationType::VIKING)
				App->entityManager->SetBuildIndex(1);
			else if (App->entityManager->getPlayer()->civilization == CivilizationType::GREEK)
				App->entityManager->SetBuildIndex(5);
		}
		else if (element->name == "Produce_Assassin")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(25);

			building->ProduceQueue("Assassin");
		}
		else if (element->name == "Produce_Monk")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(20);
			building->ProduceQueue("Monk");
		}
		else if (element->name == "Produce_Victory")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(100);
			building->ProduceQueue("Victory");
		}
		else if (element->name == "Produce_Sacrifices")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(15);
			building->ProduceQueue("Sacrifices");
		}
		else if (element->name == "Produce_Prayers")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(15);
			building->ProduceQueue("Prayers");
		}
		else if (element->name == "Upgrade") {
			//Upgrade level
			CombatUnit* unit =(CombatUnit*)App->entityManager->getPlayer()->GetEntitiesSelected().begin()._Ptr->_Myval;
			unit->LevelUp();
		}
		else if (element->name == "Move")
		{
			clickToPath = true;
			App->gui->cursor_move = true;
		}
		else if (element->name == "Attack")
		{
			App->gui->cursor_attack = true;
			clickToPath = true;
			//BERNAT & JORDI
		}
		else if (element->name == "Heal")
		{
			App->gui->cursor_heal = true;
			nextUnit_selected = true;

		}
		else if (element->name == "Produce_Cleric")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(25);
			building->ProduceQueue("Cleric");
		}
		else if (element->name == "Produce_Chaotic_Beast")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(50);
			building->ProduceQueue("Chaotic_Beast");
		}
		else if (element->name == "Produce_Lawful_Beast")
		{
			Building* building = (Building*)hud->thing_selected;
			App->entityManager->getPlayer()->DecreaseFaith(50);
			building->ProduceQueue("Lawful_Beast");
		}
		else if (element->name == "Produce_Lawful_Miracle")
		{
			if (App->entityManager->getPlayer()->prayers >= 15)
			{
				nextBuilding_selected = true;
			}
		}
		else if (element->name == "Produce_Chaotic_Miracle")
		{
			if (App->entityManager->getPlayer()->sacrifices >= 15)
			{
				building_meteor = true;
			}
		}
		else if (element->name == "Troop") {
			hud->ClickOnSelectionButton(element->sprite1);
		}
		else if (element->name == "Thing_Produced") {
			hud->CancelProduction(element->GetScreenPos());
		}
		break;

	default:
		break;
	}


}

void j1Scene::FinishResearching(std::string thing_researched) {
	if (thing_researched == "Temple") {
		App->entityManager->getPlayer()->research_temple = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Encampment") {
		App->entityManager->getPlayer()->research_encampment = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Cleric") {
		App->entityManager->getPlayer()->research_cleric = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Assassin") {
		App->entityManager->getPlayer()->research_assassin = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Lawful Beast") {
		App->entityManager->getPlayer()->research_lawful_beast = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Chaotic Beast") {
		App->entityManager->getPlayer()->research_chaotic_beast = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Lawful Miracle") {
		App->entityManager->getPlayer()->research_lawful_miracle = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Chaotic Miracle") {
		App->entityManager->getPlayer()->research_chaotic_miracle = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Lawful Victory") {
		App->entityManager->getPlayer()->research_lawful_victory = true;
		App->audio->PlayFx(4, ResearchFinished);
	}
	else if (thing_researched == "Chaotic Victory") {
		App->entityManager->getPlayer()->research_chaotic_victory = true;
		App->audio->PlayFx(4, ResearchFinished);
	}

}