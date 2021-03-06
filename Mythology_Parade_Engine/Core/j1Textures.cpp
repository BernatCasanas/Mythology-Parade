#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "AssetsManager.h"

#include "SDL_image/include/SDL_image.h"
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )

j1Textures::j1Textures() : j1Module()
{
	name.append("textures");
}

// Destructor
j1Textures::~j1Textures()
{}

// Called before render is available
bool j1Textures::Awake(pugi::xml_node& config)
{
	LOG("Init Image library");
	bool ret = true;
	// load support for the PNG image format
	int flags = IMG_INIT_PNG;
	int init = IMG_Init(flags);

	if((init & flags) != flags)
	{
		LOG("Could not initialize Image lib. IMG_Init: %s", IMG_GetError());
		ret = false;
	}
	active = true;

	return ret;
}

// Called before the first frame
bool j1Textures::Start()
{
	LOG("start textures");
	bool ret = true;
	return ret;
}

// Called before quitting
bool j1Textures::CleanUp()
{
	LOG("Freeing textures and Image library");

	for (std::list<SDL_Texture*>::iterator it = textures.begin(); it != textures.end(); it++)
	{
		SDL_DestroyTexture(it._Ptr->_Myval);
	}

	textures.clear();
	IMG_Quit();
	return true;
}

// Load new texture from file path
SDL_Texture* const j1Textures::Load(const char* path)
{
	SDL_Surface* surface = nullptr;
	SDL_Texture* texture = nullptr;
	surface = IMG_Load_RW(App->assets_manager->Load(path), 1);;

	if(surface == nullptr)
	{
		LOG("Could not load surface with path: %s. IMG_Load: %s", path, IMG_GetError());
	}
	else
	{
		texture = LoadSurface(surface);
		SDL_FreeSurface(surface);
		surface = nullptr;
		LOG("Load success");
	}

	return texture;
}

// Unload texture
bool j1Textures::UnLoad(SDL_Texture* texture)
{
	for (std::list<SDL_Texture*>::iterator it = textures.begin(); it != textures.end(); it++)
	{
		if(texture == it._Ptr->_Myval)
		{
			SDL_DestroyTexture(it._Ptr->_Myval);
			textures.remove(it._Ptr->_Myval);
			return true;
		}
	}

	return false;
}

// Translate a surface into a texture
SDL_Texture* const j1Textures::LoadSurface(SDL_Surface* surface)
{
	SDL_Texture* texture = SDL_CreateTextureFromSurface(App->render->renderer, surface);

	if(texture == NULL)
	{
		LOG("Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		textures.push_back(texture);
	}

	return texture;
}

// Retrieve size of a texture
void j1Textures::GetSize(const SDL_Texture* texture, uint& width, uint& height) const
{
	SDL_QueryTexture((SDL_Texture*)texture, NULL, NULL, (int*) &width, (int*) &height);
}

SDL_Surface * const j1Textures::GetSurface(const char * path) const
{
	SDL_Surface* surf = IMG_Load_RW(App->assets_manager->Load(path), 1);

	return surf != nullptr ? surf : nullptr;
}
