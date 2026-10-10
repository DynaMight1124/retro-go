/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "server.h"
#include "../game/save_build.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f (void)
{
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!dedicated->value)
	{
		Com_Printf ("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_Set ("public", "1");

	for (i=1 ; i<MAX_MASTERS ; i++)
		memset (&master_adr[i], 0, sizeof(master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		if (slot == MAX_MASTERS)
			break;

		if (!NET_StringToAdr (Cmd_Argv(i), &master_adr[i]))
		{
			Com_Printf ("Bad address: %s\n", Cmd_Argv(i));
			continue;
		}
		if (master_adr[slot].port == 0)
			master_adr[slot].port = BigShort (PORT_MASTER);

		Com_Printf ("Master server at %s\n", NET_AdrToString (master_adr[slot]));

		Com_Printf ("Sending a ping.\n");

		Netchan_OutOfBandPrint (NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.last_heartbeat = -9999999;
}



/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	if (Cmd_Argc() < 2)
		return false;

	s = Cmd_Argv(1);

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = atoi(Cmd_Argv(1));
		if (idnum < 0 || idnum >= maxclients->value)
		{
			Com_Printf ("Bad client slot: %i\n", idnum);
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state)
		{
			Com_Printf ("Client %i is not active\n", idnum);
			return false;
		}
		return true;
	}

	// check for a name match
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (!strcmp(cl->name, s))
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	Com_Printf ("Userid %s is not on the server\n", s);
	return false;
}


/*
===============================================================================

SAVEGAME FILES

===============================================================================
*/

/*
=====================
SV_WipeSavegame

Delete a native slot directly beneath FS_Gamedir().
=====================
*/
void SV_WipeSavegame (char *savename)
{
	char	name[MAX_OSPATH];
	char	*s;

	Com_DPrintf("SV_WipeSaveGame(%s)\n", savename);

	Com_sprintf (name, sizeof(name), "%s/%s/server.ssv", FS_Gamedir (), savename);
	remove (name);
	Com_sprintf (name, sizeof(name), "%s/%s/game.ssv", FS_Gamedir (), savename);
	remove (name);

	Com_sprintf (name, sizeof(name), "%s/%s/*.sav", FS_Gamedir (), savename);
	s = Sys_FindFirst( name, 0, 0 );
	while (s)
	{
		remove (s);
		s = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose ();
	Com_sprintf (name, sizeof(name), "%s/%s/*.sv2", FS_Gamedir (), savename);
	s = Sys_FindFirst(name, 0, 0 );
	while (s)
	{
		remove (s);
		s = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose ();
}


/*
================
CopyFile
================
*/
// Reserved sibling directories are never accepted as user slot names.
static qboolean SV_ValidSaveDir(const char *dir)
{
    if (!dir || !dir[0]) return false;
    size_t length = 0;
    for (; dir[length]; ++length) {
        unsigned char c = (unsigned char)dir[length];
        if (length >= 32 || !((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }
    return true;
}

static qboolean SV_SavePath(char *path, const char *slot, const char *file)
{
    int length = file ? snprintf(path, MAX_OSPATH, "%s/%s/%s", FS_Gamedir(), slot, file) :
        snprintf(path, MAX_OSPATH, "%s/%s", FS_Gamedir(), slot);
    return length >= 0 && length < MAX_OSPATH;
}

static qboolean CopyFile (char *src, char *dst)
{
    FILE *input = fopen(src, "rb");
    if (!input) return false;
    FILE *output = fopen(dst, "wb");
    if (!output) { fclose(input); return false; }
    static byte buffer[65536] EXT_RAM_BSS;
    qboolean ok = true;
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), input)) != 0) {
        if (fwrite(buffer, 1, count, output) != count) { ok = false; break; }
    }
    if (ferror(input)) ok = false;
    if (fclose(input) != 0) ok = false;
    if (fclose(output) != 0) ok = false;
    return ok;
}

static qboolean SV_CopySaveFiles(char *src, char *dst)
{
    char name[MAX_OSPATH], target[MAX_OSPATH];
    const char *required[] = {"server.ssv", "game.ssv"};
    for (unsigned i = 0; i < 2; ++i) {
        if (!SV_SavePath(name, src, required[i]) || !SV_SavePath(target, dst, required[i])) return false;
        FS_CreatePath(target);
        if (!CopyFile(name, target)) return false;
    }
    if (!SV_SavePath(name, src, "")) return false;
    size_t prefix = strlen(name);
    if (!SV_SavePath(name, src, "*.sav")) return false;
    char *found = Sys_FindFirst(name, 0, 0);
    qboolean ok = true;
    while (found && ok) {
        // The iterator owns found; copy it before asking for another entry.
        if (strlen(found) >= sizeof(name) || strlen(found) <= prefix + 4) { ok = false; break; }
        strcpy(name, found);
        if (!SV_SavePath(target, dst, found + prefix)) { ok = false; break; }
        ok = CopyFile(name, target);
        size_t length = strlen(name), target_length = strlen(target);
        strcpy(name + length - 3, "sv2");
        strcpy(target + target_length - 3, "sv2");
        if (ok) ok = CopyFile(name, target);
        found = Sys_FindNext(0, 0);
    }
    Sys_FindClose();
    return ok;
}

static qboolean SV_SaveDirExists(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0;
}

qboolean SV_CopySaveGame (char *src, char *dst)
{
    char destination[MAX_OSPATH], staged[MAX_OSPATH], backup[MAX_OSPATH];
    if (!SV_ValidSaveDir(src) || !SV_ValidSaveDir(dst) || !Q_strcasecmp(src, dst)) return false;
    if (!SV_SavePath(destination, dst, NULL) || !SV_SavePath(staged, ".q2-stage", NULL) ||
        !SV_SavePath(backup, ".q2-backup", NULL)) return false;
    // Preserve interrupted operations for manual recovery rather than deleting
    // the only remaining complete slot after a power failure.
    if (SV_SaveDirExists(staged) || SV_SaveDirExists(backup)) {
        Com_Printf("Save staging/backup exists; recover it before saving again.\n");
        return false;
    }
    if (!SV_CopySaveFiles(src, ".q2-stage")) {
        SV_WipeSavegame(".q2-stage");
        rmdir(staged);
        return false;
    }
    qboolean had_destination = SV_SaveDirExists(destination);
    if (had_destination && rename(destination, backup) != 0) {
        SV_WipeSavegame(".q2-stage");
        rmdir(staged);
        return false;
    }
    if (rename(staged, destination) != 0) {
        if (had_destination && rename(backup, destination) != 0)
            Com_Printf("Save rollback failed; previous slot retained in .q2-backup.\n");
        SV_WipeSavegame(".q2-stage");
        rmdir(staged);
        return false;
    }
    if (had_destination) {
        SV_WipeSavegame(".q2-backup");
        if (rmdir(backup) != 0)
            Com_Printf("Previous save backup could not be removed.\n");
    }
    return true;
}

static void SV_SaveWrite(FILE *file, const void *data, size_t size)
{
    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        Com_Error(ERR_DROP, "Could not write native save");
    }
}

static void SV_SaveClose(FILE *file)
{
    if (fclose(file) != 0) Com_Error(ERR_DROP, "Could not close native save");
}

/*
==============
SV_WriteLevelFile

==============
*/
void SV_WriteLevelFile (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_DPrintf("SV_WriteLevelFile()\n");

	Com_sprintf (name, sizeof(name), "%s/current/%s.sv2", FS_Gamedir(), sv.name);
	f = fopen(name, "wb");
	if (!f)
	{
		Com_Error(ERR_DROP, "Failed to open %s", name);
		return;
	}
	SV_SaveWrite(f, sv.configstrings, sizeof(sv.configstrings));
	CM_WritePortalState (f);
	SV_SaveClose(f);

	Com_sprintf (name, sizeof(name), "%s/current/%s.sav", FS_Gamedir(), sv.name);
	ge->WriteLevel (name);
}

/*
==============
SV_ReadLevelFile

==============
*/
void SV_ReadLevelFile (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_DPrintf("SV_ReadLevelFile()\n");

	Com_sprintf (name, sizeof(name), "%s/current/%s.sv2", FS_Gamedir(), sv.name);
	f = fopen(name, "rb");
	if (!f)
	{
		Com_Error(ERR_DROP, "Missing level save companion: %s", name);
		return;
	}
	if (fread(sv.configstrings, 1, sizeof(sv.configstrings), f) != sizeof(sv.configstrings)) {
        fclose(f);
        Com_Error(ERR_DROP, "Truncated level configstrings");
    }
    for (unsigned i = 0; i < MAX_CONFIGSTRINGS; ++i) {
        // The HUD program intentionally spans CS_STATUSBAR's reserved rows.
        // Those rows may contain fragments without a local terminator; bound
        // their strings at CS_AIRACCEL, where independent records resume.
        size_t capacity = sizeof(sv.configstrings[i]);
        if (i >= CS_STATUSBAR && i < CS_AIRACCEL)
            capacity *= CS_AIRACCEL - i;
        if (!memchr(sv.configstrings[i], 0, capacity)) {
            fclose(f);
            Com_Error(ERR_DROP, "Unterminated level configstring");
        }
    }
	CM_ReadPortalState (f);
	SV_SaveClose(f);

	Com_sprintf (name, sizeof(name), "%s/current/%s.sav", FS_Gamedir(), sv.name);
	ge->ReadLevel (name);
}

/*
==============
SV_WriteServerFile

==============
*/
void SV_WriteServerFile (qboolean autosave)
{
	FILE	*f;
	cvar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	time_t	aclock;
	struct tm	*newtime;

	Com_DPrintf("SV_WriteServerFile(%s)\n", autosave ? "true" : "false");

	Com_sprintf (name, sizeof(name), "%s/current/server.ssv", FS_Gamedir());
	f = fopen (name, "wb");
	if (!f)
	{
		Com_Error(ERR_DROP, "Couldn't write %s", name);
		return;
	}
	// write the comment field
	memset (comment, 0, sizeof(comment));

	if (!autosave)
	{
		time (&aclock);
		newtime = localtime (&aclock);
		Com_sprintf (comment,sizeof(comment), "%2i:%i%i %2i/%2i  ", newtime->tm_hour
			, newtime->tm_min/10, newtime->tm_min%10,
			newtime->tm_mon+1, newtime->tm_mday);
		strncat (comment, sv.configstrings[CS_NAME], sizeof(comment)-1-strlen(comment) );
	}
	else
	{	// autosaved
		Com_sprintf (comment, sizeof(comment), "ENTERING %s", sv.configstrings[CS_NAME]);
	}

	SV_SaveWrite(f, comment, sizeof(comment));

	// write the mapcmd
	SV_SaveWrite(f, svs.mapcmd, sizeof(svs.mapcmd));

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	for (var = cvar_vars ; var ; var=var->next)
	{
		if (!(var->flags & CVAR_LATCH))
			continue;
		if (strlen(var->name) >= sizeof(name)-1
			|| strlen(var->string) >= sizeof(string)-1)
		{
			Com_Printf ("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}
		memset (name, 0, sizeof(name));
		memset (string, 0, sizeof(string));
		strcpy (name, var->name);
		strcpy (string, var->string);
		SV_SaveWrite(f, name, sizeof(name));
		SV_SaveWrite(f, string, sizeof(string));
	}

	SV_SaveClose(f);

	// write game state
	Com_sprintf (name, sizeof(name), "%s/current/game.ssv", FS_Gamedir());
	ge->WriteGame (name, autosave);
}

/*
==============
SV_ReadServerFile

==============
*/
qboolean SV_ReadServerFile (void)
{
    FILE *file;
    char path[MAX_OSPATH], name[MAX_OSPATH], string[128];
    char comment[32], mapcmd[MAX_TOKEN_CHARS];
    Com_sprintf(path, sizeof(path), "%s/current/server.ssv", FS_Gamedir());
    file = fopen(path, "rb");
    if (!file) return false;
    if (fread(comment, 1, sizeof(comment), file) != sizeof(comment) ||
        fread(mapcmd, 1, sizeof(mapcmd), file) != sizeof(mapcmd) ||
        !memchr(comment, 0, sizeof(comment)) || !memchr(mapcmd, 0, sizeof(mapcmd)) || !mapcmd[0])
        goto damaged;
    // Validate every fixed record before applying cvars or reinitializing.
    unsigned count = 0;
    for (;;) {
        size_t bytes = fread(name, 1, sizeof(name), file);
        if (!bytes && !ferror(file)) break;
        if (bytes != sizeof(name) || ++count > 1024 ||
            fread(string, 1, sizeof(string), file) != sizeof(string) ||
            !name[0] || !memchr(name, 0, sizeof(name)) || !memchr(string, 0, sizeof(string)))
            goto damaged;
        if (!strcmp(name, "maxclients") || !strcmp(name, "maxentities")) {
            char *end;
            double value = strtod(string, &end);
            int maximum = !strcmp(name, "maxclients") ? MAX_CLIENTS : MAX_EDICTS;
            if (!string[0] || *end || !(value >= 1 && value <= maximum) || value != (int)value) goto damaged;
        }
    }
    if (fseek(file, sizeof(comment) + sizeof(mapcmd), SEEK_SET) != 0) goto damaged;
    for (unsigned i = 0; i < count; ++i) {
        if (fread(name, 1, sizeof(name), file) != sizeof(name) ||
            fread(string, 1, sizeof(string), file) != sizeof(string)) goto damaged;
        Cvar_ForceSet(name, string);
    }
    if (fclose(file) != 0) return false;
    SV_InitGame();
    strcpy(svs.mapcmd, mapcmd);
    Com_sprintf(path, sizeof(path), "%s/current/game.ssv", FS_Gamedir());
    ge->ReadGame(path);
    return true;
 damaged:
    fclose(file);
    QG_SaveNotice("The save has a damaged server record. Load cancelled.");
    return false;
}

//=========================================================




/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f (void)
{
	SV_Map (true, Cmd_Argv(1), false );
}

/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
void SV_GameMap_f (void)
{
	char		*map;
	int			i;
	client_t	*cl;
	qboolean	*savedInuse;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: gamemap <map>\n");
		return;
	}

	Com_DPrintf("SV_GameMap(%s)\n", Cmd_Argv(1));

	FS_CreatePath (va("%s/current/", FS_Gamedir()));

	// check for clearing the current savegame
	map = Cmd_Argv(1);
	if (map[0] == '*')
	{
		// wipe all the *.sav files
		SV_WipeSavegame ("current");
	}
	else
	{	// save the map just exited
		if (sv.state == ss_game)
		{
			// clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			savedInuse = malloc(maxclients->value * sizeof(qboolean));
			for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
			{
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
				cl->edict->inuse = savedInuse[i];
			free (savedInuse);
		}
	}

	// start up the next map
	SV_Map (false, Cmd_Argv(1), false );

	// archive server state
	strncpy (svs.mapcmd, Cmd_Argv(1), sizeof(svs.mapcmd)-1);

	// copy off the level to the autosave slot
	if (!dedicated->value)
	{
		SV_WriteServerFile (true);
		if (!SV_CopySaveGame("current", "save0"))
			QG_SaveNotice("Could not update autosave. The previous slot was preserved.");
	}
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
void SV_Map_f (void)
{
	char	*map;
	char	expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strstr (map, "."))
	{
		Com_sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
		if (FS_LoadFile (expanded, NULL) == -1)
		{
			Com_Printf ("Can't find %s\n", expanded);
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
	SV_WipeSavegame("current");
	SV_GameMap_f ();
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/


/*
==============
SV_Loadgame_f

==============
*/
void SV_Loadgame_f (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	*dir;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: loadgame <directory>\n");
		return;
	}

	Com_Printf ("Loading game...\n");

	dir = Cmd_Argv(1);
	if (!SV_ValidSaveDir(dir) || !Q_strcasecmp(dir, "current"))
	{
		Com_Printf ("Bad savedir.\n");
		return;
	}

	// make sure the server.ssv file exists
	Com_sprintf (name, sizeof(name), "%s/%s/server.ssv", FS_Gamedir(), Cmd_Argv(1));
	f = fopen (name, "rb");
	if (!f)
	{
		Com_Printf ("No such savegame: %s\n", name);
		return;
	}
	fclose (f);

	// Reject before copying over current or changing the running server.
	Com_sprintf (name, sizeof(name), "%s/%s/game.ssv", FS_Gamedir(), Cmd_Argv(1));
	if (!Q2_SaveCheckBuild(name))
	{
		Com_Printf ("Cannot load save: incompatible firmware or damaged save.\nCreate a new save with this firmware.\n");
		QG_SaveNotice("This save is incompatible with this firmware, or is damaged. Create a new save with this firmware.");
		return;
	}

	if (!SV_CopySaveGame(Cmd_Argv(1), "current")) {
		QG_SaveNotice("Could not copy save. The previous slot was preserved.");
		return;
	}

	if (!SV_ReadServerFile()) return;

	// go to the map
	sv.state = ss_dead;		// don't save current level when changing
	SV_Map (false, svs.mapcmd, true);
}



/*
==============
SV_Savegame_f

==============
*/
void SV_Savegame_f (void)
{
	char	*dir;

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableValue("deathmatch"))
	{
		Com_Printf ("Can't savegame in a deathmatch\n");
		return;
	}

	if (!Q_strcasecmp (Cmd_Argv(1), "current"))
	{
		Com_Printf ("Can't save to 'current'\n");
		return;
	}

	if (maxclients->value == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
	{
		Com_Printf ("\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (!SV_ValidSaveDir(dir) || !Q_strcasecmp(dir, "current"))
	{
		Com_Printf ("Bad savedir.\n");
		return;
	}

	Com_Printf ("Saving game...\n");

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SV_WriteLevelFile ();

	// save server state
	SV_WriteServerFile (false);

	// copy it off
	if (!SV_CopySaveGame("current", dir)) {
		QG_SaveNotice("Could not save game. The previous slot was preserved.");
		return;
	}

	Com_Printf ("Done.\n");
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void)
{
	if (!svs.initialized)
	{
		Com_Printf ("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", sv_client->name);
	// print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf (sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient (sv_client);
	sv_client->lastmessage = svs.realtime;	// min case there is a funny zombie
}


/*
================
SV_Status_f
================
*/
void SV_Status_f (void)
{
	int			i, j, l;
	client_t	*cl;
	char		*s;
	int			ping;
	if (!svs.clients)
	{
		Com_Printf ("No server running.\n");
		return;
	}
	Com_Printf ("map              : %s\n", sv.name);

	Com_Printf ("num score ping name            lastmsg address               qport \n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		Com_Printf ("%3i ", i);
		Com_Printf ("%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == cs_connected)
			Com_Printf ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Printf ("ZMBI ");
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf ("%4i ", ping);
		}

		Com_Printf ("%s", cl->name);
		l = 16 - strlen(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");

		Com_Printf ("%7i ", svs.realtime - cl->lastmessage );

		s = NET_AdrToString ( cl->netchan.remote_address);
		Com_Printf ("%s", s);
		l = 22 - strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");
		
		Com_Printf ("%5i", cl->netchan.qport);

		Com_Printf ("\n");
	}
	Com_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < maxclients->value; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -9999999;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f (void)
{
	Com_Printf ("Server info settings:\n");
	Info_Print (Cvar_Serverinfo());
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
void SV_DumpUser_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Com_Printf ("userinfo\n");
	Com_Printf ("--------\n");
	Info_Print (sv_client->userinfo);

}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
void SV_ServerRecord_f (void)
{
	char	name[MAX_OSPATH];
	char	buf_data[32768];
	sizebuf_t	buf;
	int		len;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("serverrecord <demoname>\n");
		return;
	}

	if (svs.demofile)
	{
		Com_Printf ("Already recording.\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	Com_sprintf (name, sizeof(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("recording to %s.\n", name);
	FS_CreatePath (name);
	svs.demofile = fopen (name, "wb");
	if (!svs.demofile)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	SZ_Init (&svs.demo_multicast, svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));

	//
	// write a single giant fake message with all the startup info
	//
	SZ_Init (&buf, buf_data, sizeof(buf_data));

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawncount);
	// 2 means server demo
	MSG_WriteByte (&buf, 2);	// demos are always attract loops
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);
	// send full levelname
	MSG_WriteString (&buf, sv.configstrings[CS_NAME]);

	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++)
		if (sv.configstrings[i][0])
		{
			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, sv.configstrings[i]);
		}

	// write it to the demo file
	Com_DPrintf ("signon message length: %i\n", buf.cursize);
	len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, svs.demofile);
	fwrite (buf.data, buf.cursize, 1, svs.demofile);

	// the rest of the demo file will be individual frames
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
void SV_ServerStop_f (void)
{
	if (!svs.demofile)
	{
		Com_Printf ("Not doing a serverrecord.\n");
		return;
	}
	fclose (svs.demofile);
	svs.demofile = NULL;
	Com_Printf ("Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
void SV_KillServer_f (void)
{
	if (!svs.initialized)
		return;
	SV_Shutdown ("Server was killed.\n", false);
	NET_Config ( false );	// close network sockets
}

/*
===============
SV_ServerCommand_f

Let the game dll handle a command
===============
*/
void SV_ServerCommand_f (void)
{
	if (!ge)
	{
		Com_Printf ("No game loaded.\n");
		return;
	}

	ge->ServerCommand();
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_AddCommand ("demomap", SV_DemoMap_f);
	Cmd_AddCommand ("gamemap", SV_GameMap_f);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	if ( dedicated->value )
		Cmd_AddCommand ("say", SV_ConSay_f);

	Cmd_AddCommand ("serverrecord", SV_ServerRecord_f);
	Cmd_AddCommand ("serverstop", SV_ServerStop_f);

	Cmd_AddCommand ("save", SV_Savegame_f);
	Cmd_AddCommand ("load", SV_Loadgame_f);

	Cmd_AddCommand ("killserver", SV_KillServer_f);

	Cmd_AddCommand ("sv", SV_ServerCommand_f);
}
