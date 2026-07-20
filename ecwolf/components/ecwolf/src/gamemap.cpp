/*
** gamemap.cpp
**
*/

#include <climits>
#include <cassert>
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
	// Only clear spots touched by the previous render. Walking every large
	// PSRAM-backed Map entry each frame is especially costly on ESP32.
	for(unsigned int i = 0; i < visibleSpots.Size(); ++i)
		visibleSpots[i]->visible = false;
	visibleSpots.Clear();

	if(players[ConsolePlayer].camera)
		MarkVisible(GetSpot(players[ConsolePlayer].camera->tilex,
			players[ConsolePlayer].camera->tiley, 0));
}

void GameMap::MarkVisible(Plane::Map *spot)
{
	if(spot && !spot->visible)
	{
		spot->visible = true;
		visibleSpots.Push(spot);
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

const GameMap::Tile *GameMap::GetTile(unsigned int index) const
{
	if(index >= tilePalette.Size())
		return NULL;
	return &tilePalette[index];
}

unsigned int GameMap::GetTileIndex(const GameMap::Tile *tile) const
{
	if(!tile)
		return UINT_MAX;
	return static_cast<unsigned int>(tile - &tilePalette[0]);
}

const GameMap::Sector *GameMap::GetSector(unsigned int index) const
{
	if(index >= sectorPalette.Size())
		return NULL;
	return &sectorPalette[index];
}

unsigned int GameMap::GetSectorIndex(const GameMap::Sector *sector) const
{
	if(!sector)
		return UINT_MAX;
	return static_cast<unsigned int>(sector - &sectorPalette[0]);
}

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

FArchive &operator<< (FArchive &arc, GameMap *&gm)
{
	arc << gm->header.name
		<< gm->header.width
		<< gm->header.height
		<< gm->header.tileSize;

	if(GameSave::SaveVersion >= 1383348286)
	{
		unsigned int zone = gm->zonePalette.Size();
		while(--zone > 0)
		{
			unsigned int i = gm->zonePalette.Size() - zone;
			while(--i > 0)
				arc << gm->zoneLinks[zone][i];
		}
	}
	else
	{
		// Consume the obsolete packed zone-link format for old saves.
		uint32_t packing = 0;
		unsigned short shift = 0;
		unsigned int x = 0;
		unsigned int y = 1;
		unsigned int max = 1;

		arc << packing;
		do
		{
			++shift;
			if(++x >= max)
			{
				x = 0;
				++y;
				++max;
			}
			if(shift == sizeof(packing) * 8)
			{
				arc << packing;
				shift = 0;
			}
		}
		while(y < gm->zonePalette.Size());
	}

	if(arc.IsLoading())
		gm->visibleSpots.Clear();

	for(unsigned int p = 0; p < gm->NumPlanes(); ++p)
	{
		MapPlane &plane = gm->planes[p];
		arc << plane.depth;
		assert(plane.depth == 64);
		if(arc.IsLoading())
			plane.gm = gm;

		for(unsigned int i = 0; i < gm->header.width * gm->header.height; ++i)
		{
			MapSpot spot = &plane.map[i];
			BYTE pushdir = spot->pushDirection;
			arc << pushdir;
			spot->pushDirection = pushdir;

			arc << spot->texture[0] << spot->texture[1]
				<< spot->texture[2] << spot->texture[3]
				<< spot->visible;
			if(GameSave::SaveVersion >= 1393719642)
				arc << spot->amFlags;
			arc << spot->thinker
				<< spot->slideAmount[0] << spot->slideAmount[1]
				<< spot->slideAmount[2] << spot->slideAmount[3]
				<< spot->sideSolid[0] << spot->sideSolid[1]
				<< spot->sideSolid[2] << spot->sideSolid[3];

			// This port keeps trigger arrays optional to save memory, while the
			// upstream save format stores the TArray directly.
			if(arc.IsStoring())
			{
				if(spot->triggers)
					arc << *spot->triggers;
				else
				{
					TArray<MapTrigger> empty;
					arc << empty;
				}
			}
			else
			{
				if(!spot->triggers)
					spot->triggers = new TArray<MapTrigger>;
				arc << *spot->triggers;
				if(spot->triggers->Size() == 0)
				{
					delete spot->triggers;
					spot->triggers = NULL;
				}
			}

			arc << spot->pushAmount
				<< spot->tile
				<< spot->sector
				<< spot->zone
				<< spot->pushReceptor;

			if(GameSave::SaveProdVersion >= 0x001002FF &&
				GameSave::SaveVersion >= 1375246092)
				arc << spot->slideStyle;

			if(arc.IsLoading())
			{
				spot->plane = &plane;
				if(spot->visible)
					gm->visibleSpots.Push(spot);
			}
		}
	}

	if(GameSave::SaveVersion > 1438232816)
	{
		if(arc.IsStoring())
		{
			unsigned int count = gm->elevatorPosition.CountUsed();
			arc << count;
			TMap<unsigned int, MapSpot>::Iterator iter(gm->elevatorPosition);
			TMap<unsigned int, MapSpot>::Pair *pair;
			while(iter.NextPair(pair))
			{
				DWORD key = pair->Key;
				arc << key << pair->Value;
			}
		}
		else
		{
			unsigned int count;
			arc << count;
			gm->elevatorPosition.Clear();
			while(count-- > 0)
			{
				DWORD key;
				MapSpot value;
				arc << key << value;
				gm->elevatorPosition[key] = value;
			}
		}
	}

	return arc;
}

FArchive &operator<< (FArchive &arc, MapSpot &spot)
{
	if(arc.IsStoring())
	{
		unsigned int x = UINT_MAX;
		unsigned int y = UINT_MAX;
		if(spot)
		{
			x = spot->GetX();
			y = spot->GetY();
		}
		arc << x << y;
	}
	else
	{
		unsigned int x, y;
		arc << x << y;
		spot = (x == UINT_MAX || y == UINT_MAX) ? NULL : map->GetSpot(x, y, 0);
	}
	return arc;
}

FArchive &operator<< (FArchive &arc, const MapSector *&sector)
{
	unsigned int index;
	if(arc.IsStoring())
		index = map->GetSectorIndex(sector);
	arc << index;
	if(arc.IsLoading())
		sector = map->GetSector(index);
	return arc;
}

FArchive &operator<< (FArchive &arc, const MapTile *&tile)
{
	unsigned int index;
	if(arc.IsStoring())
		index = map->GetTileIndex(tile);
	arc << index;
	if(arc.IsLoading())
		tile = map->GetTile(index);
	return arc;
}

FArchive &operator<< (FArchive &arc, const MapZone *&zone)
{
	unsigned int index;
	if(arc.IsStoring())
		index = zone ? zone->index : UINT_MAX;
	arc << index;
	if(arc.IsLoading())
		zone = index == UINT_MAX ? NULL : &map->GetZone(index);
	return arc;
}

FArchive &operator<< (FArchive &arc, MapTrigger &trigger)
{
	arc << trigger.x << trigger.y << trigger.z
		<< trigger.active << trigger.action
		<< trigger.activate[0] << trigger.activate[1]
		<< trigger.activate[2] << trigger.activate[3]
		<< trigger.arg[0] << trigger.arg[1] << trigger.arg[2]
		<< trigger.arg[3] << trigger.arg[4]
		<< trigger.playerUse << trigger.playerCross << trigger.monsterUse
		<< trigger.isSecret << trigger.repeatable;
	return arc;
}
