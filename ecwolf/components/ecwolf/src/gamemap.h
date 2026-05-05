/*
** gamemap.h
**
*/

#ifndef __GAMEMAP_H__
#define __GAMEMAP_H__

#include "tarray.h"
#include "zstring.h"
#include "textures/textures.h"
#include "dobject.h"

class Thinker;
class UWMFParser;
class AActor;
class FileReader;
class FResourceFile;

enum
{
	SLIDE_Normal,
	SLIDE_Split,
	SLIDE_Invert
};

enum
{
	AM_Visible = 0x1,
	AM_DontOverlay = 0x2
};

class GameMap
{
	public:
		struct Header
		{
			FString name;
			FString music;
			unsigned int width;
			unsigned int height;
			unsigned int tileSize;
		};
		struct Thing
		{
			Thing() : x(0), y(0), z(0), type(NAME_None), angle(0),
				ambush(false), patrol(false), holo(false)
			{
				skill[0] = skill[1] = skill[2] = skill[3] = false;
			}

			fixed			x, y, z;
			FName			type;
			unsigned short	angle;
			bool			ambush;
			bool			patrol;
			bool			holo;
			bool			skill[4];
		};
		struct Trigger
		{
			Trigger() : x(0), y(0), z(0), active(true), action(0),
				playerUse(false), playerCross(false), monsterUse(false),
				isSecret(false), repeatable(false)
			{
				activate[0] = activate[1] = activate[2] = activate[3] = true;
				arg[0] = arg[1] = arg[2] = arg[3] = arg[4] = 0;
			}

			unsigned int	x, y, z;
			bool			active;

			enum Side { East, North, West, South };
			unsigned int	action;
			bool			activate[4];
			int				arg[5];

			bool			playerUse;
			bool			playerCross;
			bool			monsterUse;
			bool			isSecret;
			bool			repeatable;
		};
		struct Tile
		{
			Tile() : offsetVertical(false), offsetHorizontal(false),
				mapped(0), dontOverlay(false)
			{
				overhead.SetInvalid();
				sideSolid[0] = sideSolid[1] = sideSolid[2] = sideSolid[3] = true;
			}

			enum Side { East, North, West, South };
			FTextureID		texture[4];
			FTextureID		overhead;
			bool			sideSolid[4];
			bool			offsetVertical;
			bool			offsetHorizontal;
			FName			soundSequence;

			unsigned int	mapped; // filter level for always visible
			bool			dontOverlay;
		};
		struct Sector
		{
			enum Flat { Floor, Ceiling };
			FTextureID	texture[2];
		};
		struct Zone
		{
			unsigned short	index;
		};
		struct Plane
		{
			const GameMap	*gm;

			unsigned int	depth;
			struct Map
			{
				Map() : tile(NULL), sector(NULL), zone(NULL), thinker(NULL),
					triggers(NULL), pushReceptor(NULL), nexttag(NULL),
					tag(0), amFlags(0), slideStyle(0), pushAmount(0), pushDirection(0), visible(false)
				{
					slideAmount[0] = slideAmount[1] = slideAmount[2] = slideAmount[3] = 0;
					sideSolid[0] = sideSolid[1] = sideSolid[2] = sideSolid[3] = true;
				}

				~Map() { delete triggers; }

				unsigned int	GetX() const;
				unsigned int	GetY() const;
				Map				*GetAdjacent(int dir, bool opposite=false) const;
				void			SetTile(const Tile *tile);

				const Plane		*plane;

				const Tile		*tile;
				const Sector	*sector;
				const Zone		*zone;

				FTextureID		texture[4];

				TObjPtr<Thinker> thinker;
				uint16_t		slideAmount[4];
				
				TArray<Trigger>	*triggers;
				Map				*pushReceptor;
				Plane::Map		*nexttag;

				uint16_t		tag;
				uint16_t		amFlags;
				uint8_t			slideStyle;
				uint8_t			pushAmount;
				uint8_t			pushDirection;
				bool			visible;
				bool			sideSolid[4];
			}*	map;
		};

		GameMap(const FString &map);
		~GameMap();

		bool			ActivateTrigger(Trigger &trig, int direction, AActor *activator);
		void			ClearVisibility();
		const Header	&GetHeader() const { return header; }
		Plane::Map		*GetSpot(unsigned int x, unsigned int y, unsigned int z) const { return &planes[z].map[y*header.width+x]; }
		Plane::Map		*GetSpotByTag(unsigned int tag, Plane::Map *start) const;
		bool			IsValid() const { return valid; }
		bool			IsValidTileCoordinate(unsigned int x, unsigned int y, unsigned int z) const { return x < header.width && y < header.height && z < (unsigned int)planes.Size(); }
		void	LoadMap(bool loadingSave);
		unsigned int	NumPlanes() const { return planes.Size(); }
		const Plane		&GetPlane(unsigned int index) const { return planes[index]; }

		bool			CheckLink(const Zone *zone1, const Zone *zone2, bool recurse);
		void			GetHitlist(BYTE* hitlist) const;
		void			LinkZones(const Zone *zone1, const Zone *zone2, bool open);

		static bool		CheckMapExists(const FString &map);

		void PropagateMark();
        int  GetMarketLumpNum() const { return markerLump; }
        void SpawnThings() const;

		TMap<unsigned int, Plane::Map *> elevatorPosition;
	private:
		friend class UWMFParser;
		friend class FWadCollection;
		friend struct Plane::Map;

		Trigger	&NewTrigger(unsigned int x, unsigned int y, unsigned int z);
		void	UnloadLinks();
		Plane	&NewPlane();
		void	SetSpotTag(Plane::Map *spot, unsigned int tag);
		void	ReadMacData();
		void	ReadPlanesData();
		void	ReadUWMFData();
		void	ScanTiles();
		void	SetupLinks();

		FString	mapname;
		bool	valid;
		bool	isWad;
		bool	isUWMF;
		int		markerLump;
		int		numLumps;

		FResourceFile *file;
		TArray<FileReader *> lumps;

		Header			header;
		TArray<Tile>	tilePalette;
		TArray<Sector>	sectorPalette;
		TArray<Zone>	zonePalette;
		TArray<Thing>	things;
		TArray<Plane>	planes;
		TMap<unsigned int, Plane::Map *> tagMap;

		bool*				zoneTraversed;
		unsigned short**	zoneLinks;
};

enum ESpecialThings
{
	SMT_Player1Start,
	SMT_NumThings
};
extern const FName SpecialThingNames[SMT_NumThings];

typedef GameMap::Plane::Map *	MapSpot;
typedef GameMap::Plane			MapPlane;
typedef GameMap::Sector			MapSector;
typedef GameMap::Thing			MapThing;
typedef GameMap::Tile			MapTile;
typedef GameMap::Trigger		MapTrigger;
typedef GameMap::Zone			MapZone;

#include "farchive.h"
FArchive &operator<< (FArchive &arc, GameMap *&gm);
FArchive &operator<< (FArchive &arc, MapSpot &spot);
FArchive &operator<< (FArchive &arc, const MapSector *&sector);
FArchive &operator<< (FArchive &arc, const MapTile *&tile);
FArchive &operator<< (FArchive &arc, const MapZone *&zone);
FArchive &operator<< (FArchive &arc, MapTrigger &trigger);

extern GameMap *map;

#endif
