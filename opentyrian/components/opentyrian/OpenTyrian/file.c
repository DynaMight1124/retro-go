/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "file.h"
#include "opentyr.h"
#include "varz.h"

#include "SDL.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <rg_system.h>
#include <rg_storage.h>

const char *custom_data_dir = NULL;

static FILE *try_fopen( const char *dir, const char *file, const char *mode )
{
	char path[512];
	if (dir && dir[0] != '\0' && dir[strlen(dir) - 1] == '/')
		snprintf(path, sizeof(path), "%s%s", dir, file);
	else
		snprintf(path, sizeof(path), "%s/%s", dir, file);
	return fopen(path, mode);
}

// prepend directory and fopen with case-insensitive fallback
FILE *dir_fopen( const char *dir, const char *file, const char *mode )
{
	if (!dir || !file)
		return NULL;

	FILE *f = try_fopen(dir, file, mode);
	if (f != NULL)
		return f;

	// Fallback 1: Try uppercase filename (common in DOS archives)
	char alt_file[256];
	size_t len = strlen(file);
	if (len < sizeof(alt_file))
	{
		for (size_t i = 0; i <= len; i++)
			alt_file[i] = (char)toupper((unsigned char)file[i]);

		if (strcmp(file, alt_file) != 0)
		{
			f = try_fopen(dir, alt_file, mode);
			if (f != NULL)
				return f;
		}

		// Fallback 2: Try lowercase filename
		for (size_t i = 0; i <= len; i++)
			alt_file[i] = (char)tolower((unsigned char)file[i]);

		if (strcmp(file, alt_file) != 0)
		{
			f = try_fopen(dir, alt_file, mode);
			if (f != NULL)
				return f;
		}
	}

	return NULL;
}

static bool check_data_dir( const char *path )
{
	if (!path || path[0] == '\0')
		return false;

	const char *probe_files[] = {
		"palette.dat",
		"tyrian1.lvl",
		"tyrian.hdt",
		"tyrian.cdt",
	};

	for (size_t i = 0; i < COUNTOF(probe_files); ++i)
	{
		FILE *f = dir_fopen(path, probe_files[i], "rb");
		if (f)
		{
			fclose(f);
			return true;
		}
	}
	return false;
}

// finds the Tyrian data directory
const char *data_dir( void )
{
	static char dir[256] = "";
	if (dir[0] != '\0')
		return dir;

	rg_app_t *app = rg_system_get_app();
	if (app && app->romPath && strlen(app->romPath) > 0)
	{
		struct stat st;
		if (stat(app->romPath, &st) == 0)
		{
			if (S_ISDIR(st.st_mode))
			{
				if (check_data_dir(app->romPath))
				{
					strncpy(dir, app->romPath, sizeof(dir) - 1);
					dir[sizeof(dir) - 1] = '\0';
					RG_LOGI("Tyrian data found in romPath dir: '%s'\n", dir);
					return dir;
				}
			}
			else
			{
				char temp[256];
				strncpy(temp, app->romPath, sizeof(temp) - 1);
				temp[sizeof(temp) - 1] = '\0';
				char *slash = strrchr(temp, '/');
				if (slash)
					*slash = '\0';
				if (check_data_dir(temp))
				{
					strncpy(dir, temp, sizeof(dir) - 1);
					dir[sizeof(dir) - 1] = '\0';
					RG_LOGI("Tyrian data found in romPath parent: '%s'\n", dir);
					return dir;
				}
			}
		}
	}

	const char *dirs[] =
	{
		custom_data_dir,
		RG_BASE_PATH_ROMS "/opentyrian",
		RG_BASE_PATH_ROMS "/opentyrian/data",
		"data",
		".",
	};

	for (size_t i = 0; i < COUNTOF(dirs); ++i)
	{
		if (dirs[i] == NULL)
			continue;

		if (check_data_dir(dirs[i]))
		{
			strncpy(dir, dirs[i], sizeof(dir) - 1);
			dir[sizeof(dir) - 1] = '\0';
			RG_LOGI("Tyrian data found in: '%s'\n", dir);
			return dir;
		}
	}

	strncpy(dir, RG_BASE_PATH_ROMS "/opentyrian", sizeof(dir) - 1);
	dir[sizeof(dir) - 1] = '\0';
	RG_LOGW("Tyrian data not found; defaulting to: '%s'\n", dir);
	return dir;
}

// warn when dir_fopen fails
FILE *dir_fopen_warn(  const char *dir, const char *file, const char *mode )
{
	FILE *f = dir_fopen(dir, file, mode);
	
	if (f == NULL)
		fprintf(stderr, "warning: failed to open '%s': %s\n", file, strerror(errno));
	
	return f;
}

// die when dir_fopen fails
FILE *dir_fopen_die( const char *dir, const char *file, const char *mode )
{
	FILE *f = dir_fopen(dir, file, mode);
	
	if (f == NULL)
	{
		fprintf(stderr, "error: failed to open '%s': %s\n", file, strerror(errno));
		fprintf(stderr, "error: One or more of the required Tyrian " TYRIAN_VERSION " data files could not be found.\n"
		                "       Please read the README file.\n");
		JE_tyrianHalt(1);
	}
	
	return f;
}

// check if file can be opened for reading
bool dir_file_exists( const char *dir, const char *file )
{
	FILE *f = dir_fopen(dir, file, "rb");
	if (f != NULL)
	{
		efclose(f);
	}
	return (f != NULL);
}

// returns end-of-file position
long ftell_eof( FILE *f )
{
	SDL_LockDisplay();
	long pos = ftell(f);
	
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	
	fseek(f, pos, SEEK_SET);
	SDL_UnlockDisplay();
	return size;
}

int efeof ( FILE * stream )
{
	SDL_LockDisplay();
	int ret = feof ( stream );
	SDL_UnlockDisplay();
	return ret;	
}

int efputc ( int character, FILE * stream )
{
	SDL_LockDisplay();
	int ret = fputc ( character, stream );
	SDL_UnlockDisplay();
	return ret;	
}

int efgetc ( FILE * stream )
{
	SDL_LockDisplay();
	int ret = fgetc ( stream );
	SDL_UnlockDisplay();
	return ret;	
}

size_t eefwrite ( const void * ptr, size_t size, size_t count, FILE * stream )
{
	SDL_LockDisplay();
	size_t ret = fwrite ( ptr, size, count, stream );
	SDL_UnlockDisplay();
	return ret;		
}

int efclose ( FILE * stream )
{
	SDL_LockDisplay();
	int ret = fclose ( stream );
	SDL_UnlockDisplay();
	return ret;	
}

long int eftell ( FILE * stream )
{
	SDL_LockDisplay();
	long int ret = ftell ( stream );
	SDL_UnlockDisplay();
	return ret;
}

int efseek( FILE * stream, long int offset, int origin )
{
	SDL_LockDisplay();
	int ret = fseek ( stream, offset, origin );
	SDL_UnlockDisplay();
	return ret;
}

// endian-swapping fread that dies if the expected amount cannot be read
size_t efread( void *buffer, size_t size, size_t num, FILE *stream )
{
	SDL_LockDisplay();
	size_t num_read = fread(buffer, size, num, stream);
	SDL_UnlockDisplay();

	switch (size)
	{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		case 2:
			for (size_t i = 0; i < num; i++)
				((Uint16 *)buffer)[i] = SDL_Swap16(((Uint16 *)buffer)[i]);
			break;
		case 4:
			for (size_t i = 0; i < num; i++)
				((Uint32 *)buffer)[i] = SDL_Swap32(((Uint32 *)buffer)[i]);
			break;
		case 8:
			for (size_t i = 0; i < num; i++)
				((Uint64 *)buffer)[i] = SDL_Swap64(((Uint64 *)buffer)[i]);
			break;
#endif
		default:
			break;
	}
	
	if (num_read != num)
	{
		fprintf(stderr, "error: An unexpected problem occurred while reading from a file.\n");
		fprintf(stderr, "read bytes: %d, expected: %d\n", num_read, num);
		JE_tyrianHalt(1);
	}

	return num_read;
}

// endian-swapping fwrite that dies if the expected amount cannot be written
size_t efwrite( const void *buffer, size_t size, size_t num, FILE *stream )
{
	void *swap_buffer = NULL;
	
	switch (size)
	{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		case 2:
			swap_buffer = malloc(size * num);
			for (size_t i = 0; i < num; i++)
				((Uint16 *)swap_buffer)[i] = SDL_SwapLE16(((Uint16 *)buffer)[i]);
			buffer = swap_buffer;
			break;
		case 4:
			swap_buffer = malloc(size * num);
			for (size_t i = 0; i < num; i++)
				((Uint32 *)swap_buffer)[i] = SDL_SwapLE32(((Uint32 *)buffer)[i]);
			buffer = swap_buffer;
			break;
		case 8:
			swap_buffer = malloc(size * num);
			for (size_t i = 0; i < num; i++)
				((Uint64 *)swap_buffer)[i] = SDL_SwapLE64(((Uint64 *)buffer)[i]);
			buffer = swap_buffer;
			break;
#endif
		default:
			break;
	}
	
	SDL_LockDisplay();
	size_t num_written = fwrite(buffer, size, num, stream);
	SDL_UnlockDisplay();

	if (swap_buffer != NULL)
		free(swap_buffer);
	
	if (num_written != num)
	{
		fprintf(stderr, "error: An unexpected problem occurred while writing to a file.\n");
		JE_tyrianHalt(1);
	}
	
	return num_written;
}
