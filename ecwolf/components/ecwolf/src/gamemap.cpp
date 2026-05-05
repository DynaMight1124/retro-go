/*
** gamemap.cpp
**
*/

#include <climits>
#include "id_ca.h"
#include "farchive.h"
#include "gamemap.h"
#include "tarray.h"
#include "w_wad.h"
#include "wl_def.h"
#include "lnspec.h"
#include "actor.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "r_sprites.h"
#include "resourcefiles/resourcefile.h"
#include "wl_loadsave.h"
#include "doomerrors.h"
#include "m_random.h"
#include "g_mapinfo.h"
#include "m_classes.h"

#ifdef ESP_PLATFORM
#include <rg_system.h>
#include <rg_utils.h>
#endif

const FName SpecialThingNames[SMT_NumThings] = {
	"$Player1Start"
};

GameMap::GameMap(const FString &map) : mapname(map), valid(false), isUWMF(false),
	file(NULL), zoneTraversed(NULL), zoneLinks(NULL)
{
    isWad = false;
	markerLump = Wads.CheckNumForName(map);

	FString mapWad;
	mapWad.Format("maps/%s.wad", map.GetChars());

	int wadLump = Wads.CheckNumForFullName(mapWad);
	if(wadLump > markerLump)
	{
		isWad = true;
		markerLump = wadLump;
	}

	if(markerLump == -1)
	{
		I_FatalError("Could not find map %s!", map.GetChars());
	}
    
	if(isWad)
	{
		file = FResourceFile::OpenResourceFile(mapWad.GetChars(), Wads.ReopenLumpNum(markerLump), true);
        if (file && file->LumpCount() > 1)
        {
		    for(unsigned int i = 1;i < file->LumpCount();++i)
			    lumps.Push(file->GetLump(i)->GetReader());
        }
	}
	else
	{
        const char* nextLumpName = Wads.GetLumpFullName(markerLump+1);
        if(nextLumpName != NULL && strcmp(nextLumpName, "PLANES") == 0)
        {
            lumps.Push(Wads.ReopenLumpNum(markerLump+1));
        }
        else if(nextLumpName != NULL && strcmp(nextLumpName, "TEXTMAP") == 0)
        {
            isUWMF = true;
            lumps.Push(Wads.ReopenLumpNum(markerLump+1));
        }
        else
        {
            // Standard map where the marker itself has the data or it's a raw WL6 lump
            lumps.Push(Wads.ReopenLumpNum(markerLump));
        }
	}

    // Default size to avoid crash if loading fails early
    header.width = 64;
    header.height = 64;
    header.tileSize = 64;

    valid = true;
}

GameMap::~GameMap()
{
	if(file)
		delete file;
    else
    {
        for(unsigned int i = 0;i < lumps.Size();++i)
            delete lumps[i];
    }

	for(unsigned int i = 0;i < planes.Size();++i)
		delete[] planes[i].map;
	UnloadLinks();
}

bool GameMap::ActivateTrigger(Trigger &trig, int direction, AActor *activator)
{
	if(!trig.repeatable && !trig.active)
		return false;

	MapSpot spot = GetSpot(trig.x, trig.y, trig.z);

	Specials::LineSpecialFunction func = Specials::LookupFunction(Specials::LineSpecials(trig.action));
	bool ret = func(spot, trig.arg, (MapTrigger::Side)direction, activator) != 0;
	if(ret)
	{
		if(trig.active && trig.isSecret)
			++gamestate.secretcount;
		trig.active = false;
	}
	return ret;
}

void GameMap::ClearVisibility()
{
	for(unsigned int i = 0; i < header.width * header.height; ++i)
	{
		for(unsigned int p = 0; p < planes.Size(); ++p)
			planes[p].map[i].visible = false;
	}
	if(players[ConsolePlayer].camera)
    {
        MapSpot spot = GetSpot(players[ConsolePlayer].camera->tilex, players[ConsolePlayer].camera->tiley, 0);
        if (spot) spot->visible = true;
    }
}

void GameMap::UnloadLinks()
{
	// zoneTraversed holds the base address for our single allocation in SetupLinks.
	if(!zoneTraversed)
		return;
	delete[] zoneTraversed;
	zoneTraversed = NULL;
	zoneLinks = NULL;
}

GameMap::Trigger &GameMap::NewTrigger(unsigned int x, unsigned int y, unsigned int z)
{
	MapSpot spot = GetSpot(x, y, z);
	if (!spot->triggers) spot->triggers = new TArray<Trigger>;
	Trigger newTrig;
	newTrig.x = x; newTrig.y = y; newTrig.z = z;
	spot->triggers->Push(newTrig);
	return (*spot->triggers)[spot->triggers->Size()-1];
}

void GameMap::PropagateMark()
{
	for(unsigned int p = 0;p < planes.Size();++p)
	{
		Plane &plane = planes[p];
		for(unsigned int i = 0;i < header.width*header.height;++i)
			GC::Mark(plane.map[i].thinker);
	}
}

unsigned int GameMap::Plane::Map::GetX() const
{
	return static_cast<unsigned int>(this - plane->map)%plane->gm->GetHeader().width;
}

unsigned int GameMap::Plane::Map::GetY() const
{
	return static_cast<unsigned int>(this - plane->map)/plane->gm->GetHeader().width;
}

MapSpot GameMap::Plane::Map::GetAdjacent(int dir, bool opposite) const
{
	if(opposite)
		dir = (dir+2)%4;

	unsigned int x = GetX();
	unsigned int y = GetY();
	switch(dir)
	{
		case 3: ++y; break;
		case 1: --y; break;
		case 2: --x; break;
		case 0: ++x; break;
	}
	if(y >= plane->gm->GetHeader().height || x >= plane->gm->GetHeader().width)
		return NULL;
	return &plane->map[y*plane->gm->GetHeader().width+x];
}

void GameMap::Plane::Map::SetTile(const class GameMap::Tile *tile)
{
	this->tile = tile;
	for(unsigned int i = 0;i < 4;++i)
	{
		if(tile)
		{
			sideSolid[i] = tile->sideSolid[i];
			texture[i] = tile->texture[i];
		}
		else
		{
			sideSolid[i] = false;
			texture[i].SetInvalid();
		}
	}
}

void GameMap::GetHitlist(BYTE* hitlist) const
{
	R_GetSpriteHitlist(hitlist);
	for(unsigned int i = planes.Size();i-- > 0;)
	{
		const Plane &plane = planes[i];
		for(unsigned int j = header.width*header.height;j-- > 0;)
		{
			const Plane::Map &spot = plane.map[j];
			if(spot.tile)
			{
				hitlist[spot.tile->texture[Tile::East].GetIndex()] =
					hitlist[spot.tile->texture[Tile::North].GetIndex()] =
					hitlist[spot.tile->texture[Tile::West].GetIndex()] =
					hitlist[spot.tile->texture[Tile::South].GetIndex()] |= 1;
			}
			if(spot.sector)
			{
				hitlist[spot.sector->texture[Sector::Floor].GetIndex()] =
					hitlist[spot.sector->texture[Sector::Ceiling].GetIndex()] |= 2;
			}
		}
	}
}

void GameMap::LinkZones(const Zone *zone1, const Zone *zone2, bool open)
{
	if(zone1 == zone2 || zone1 == NULL || zone2 == NULL)
		return;
	unsigned short &value = zone1->index < zone2->index ?
		zoneLinks[zone1->index][zone2->index - zone1->index] :
		zoneLinks[zone2->index][zone1->index - zone2->index];
	if(!open) { if(value > 0) --value; }
	else ++value;
}

bool GameMap::CheckLink(const Zone *zone1, const Zone *zone2, bool recurse)
{
	if(zone1 == zone2 || zone1 == NULL || zone2 == NULL)
		return true;
    if (!zoneLinks) return true;
	unsigned short value = zone1->index < zone2->index ?
		zoneLinks[zone1->index][zone2->index - zone1->index] :
		zoneLinks[zone2->index][zone1->index - zone2->index];
	return value > 0;
}

GameMap::Plane::Map * GameMap::GetSpotByTag(unsigned int tag, Plane::Map *spot) const
{
    if (tagMap.CountUsed() > 0)
    {
        Plane::Map **pSpot = const_cast<TMap<unsigned int, Plane::Map *> &>(tagMap).CheckKey(tag);
        if (pSpot) return *pSpot;
    }
    return NULL;
}

bool GameMap::CheckMapExists(const FString &map) { return Wads.CheckNumForName(map) != -1; }

void GameMap::LoadMap(bool loadingSave)
{
	if(!valid)
		I_FatalError("Tried to load invalid map %s!", mapname.GetChars());

	if(isUWMF)
		ReadUWMFData();
	else
		ReadPlanesData();

	if(!loadingSave)
		ScanTiles();
}

GameMap::Plane &GameMap::NewPlane()
{
	planes.Push(Plane());
	Plane &newPlane = planes[planes.Size()-1];
	newPlane.gm = this;
	newPlane.map = new Plane::Map[header.width*header.height];
	for(unsigned int i = 0;i < header.width*header.height;++i)
		newPlane.map[i].plane = &newPlane;
	return newPlane;
}

void GameMap::SetSpotTag(Plane::Map *spot, unsigned int tag)
{
	spot->tag = tag;
	Plane::Map **chainPtr = tagMap.CheckKey(tag);
	if(chainPtr)
	{
		Plane::Map *chain = *chainPtr;
		while(chain->nexttag)
			chain = chain->nexttag;
		chain->nexttag = spot;
	}
	else
		tagMap.Insert(tag, spot);
}

void GameMap::ScanTiles()
{
	for(unsigned int p = 0;p < planes.Size();++p)
	{
		MapSpot spot = planes[p].map;
		MapSpot endSpot = spot + header.width*header.height; 
		while(spot < endSpot)
		{
			if(spot->tile)
			{
				if(spot->tile->mapped > gamestate.difficulty->MapFilter)
					spot->amFlags |= AM_Visible;
				if(spot->tile->dontOverlay)
					spot->amFlags |= AM_DontOverlay;
			}
			++spot;
		}
	}
}

void GameMap::SetupLinks()
{
	if(zonePalette.Size() == 0) return;
	const unsigned int zdSize = sizeof(bool)*zonePalette.Size()
		+ sizeof(unsigned short)*((zonePalette.Size()*(zonePalette.Size()+1))>>1);
	byte* zoneData = new byte[zdSize + sizeof(unsigned short*)*zonePalette.Size()];
	memset(zoneData, 0, zdSize);
	zoneTraversed = reinterpret_cast<bool*>(zoneData);
	unsigned short* ptr = reinterpret_cast<unsigned short*>(zoneData + sizeof(bool)*zonePalette.Size());
	zoneLinks = reinterpret_cast<unsigned short**>(zoneData+zdSize);
	for(unsigned int i = 0; i < zonePalette.Size(); ++i)
	{
		zoneLinks[i] = ptr;
		ptr += zonePalette.Size()-i;
		zoneLinks[i][0] = 1;
	}
}

extern void SpawnPlayer (int tilex, int tiley, int dir);
void GameMap::SpawnThings() const
{
	for(unsigned int i = 0;i < things.Size();++i)
	{
		const Thing &thing = things[i];
		if(!thing.skill[gamestate.difficulty->SpawnFilter])
			continue;

		if(thing.type == SpecialThingNames[SMT_Player1Start])
		{
			SpawnPlayer(thing.x>>FRACBITS, thing.y>>FRACBITS, thing.angle);
		}
		else
		{
			const ClassDef *cls = ClassDef::FindClass(thing.type);
			if(cls) AActor::Spawn(cls, thing.x, thing.y, thing.z, SPAWN_AllowReplacement|(thing.patrol ? SPAWN_Patrol : 0));
		}
	}
    if (players[0].mo == NULL)
    {
        SpawnPlayer(10, 10, 0);
    }
}

FArchive &operator<< (FArchive &arc, GameMap *&gm) { return arc; }
FArchive &operator<< (FArchive &arc, MapSpot &spot) { return arc; }
FArchive &operator<< (FArchive &arc, const MapSector *&sector) { return arc; }
FArchive &operator<< (FArchive &arc, const MapTile *&tile) { return arc; }
FArchive &operator<< (FArchive &arc, const MapZone *&zone) { return arc; }
FArchive &operator<< (FArchive &arc, MapTrigger &trigger) { return arc; }
