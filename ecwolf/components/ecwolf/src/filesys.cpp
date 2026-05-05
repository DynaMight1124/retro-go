#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include "filesys.h"
#include "version.h"
#include <rg_system.h>
#include <rg_storage.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

namespace FileSys {

static FString SpecialPaths[NUM_SPECIAL_DIRECTORIES];

FString GetDirectoryPath(ESpecialDirectory dir) { return SpecialPaths[dir]; }
void SetDirectoryPath(ESpecialDirectory dir, const FString &path) { SpecialPaths[dir] = path; }

void SetupPaths(int argc, const char * const *argv)
{
	FString &progDir = SpecialPaths[DIR_Program];
	FString &configDir = SpecialPaths[DIR_Configuration];
	FString &saveDir = SpecialPaths[DIR_Saves];
	FString &appsupportDir = SpecialPaths[DIR_ApplicationSupport];
	FString &documentsDir = SpecialPaths[DIR_Documents];
	FString &screenshotsDir = SpecialPaths[DIR_Screenshots];

    FString romPath = RG_BASE_PATH_ROMS;
    romPath += "/wolf3d";
    
    FString configPath = RG_BASE_PATH_CONFIG;
    configPath += "/wolf3d";

    FString savesPath = RG_BASE_PATH_SAVES;
    savesPath += "/wolf3d";

    rg_storage_mkdir(romPath.GetChars());
    rg_storage_mkdir(configPath.GetChars());
    rg_storage_mkdir(savesPath.GetChars());

    progDir = romPath;
    configDir = configPath;
    saveDir = savesPath;
    appsupportDir = configPath;
    documentsDir = romPath;
    screenshotsDir = savesPath;

    // Create subdirs
    FString s;
    s = screenshotsDir + "/screenshots";
    rg_storage_mkdir(s.GetChars());
    screenshotsDir = s;
}
FString GetSteamPath(ESteamApp game) { return ""; }
FString GetGOGPath(ESteamApp game) { return ""; }

} // namespace FileSys

File::File(const FString &filename)
{
	init(filename);
}

File::File(const File &dir, const FString &filename)
{
	FString path = dir.getPath();
	if (path.Len() > 0 && path[path.Len()-1] != '/')
		path += "/";
	path += filename;
	init(path);
}

void File::init(FString filename)
{
	this->filename = filename;
	struct stat st;
	existing = (stat(filename.GetChars(), &st) == 0);
	if (existing)
	{
		directory = S_ISDIR(st.st_mode);
		writable = (access(filename.GetChars(), W_OK) == 0);
		if (directory)
		{
			DIR *dir = opendir(filename.GetChars());
			if (dir)
			{
				struct dirent *ent;
				while ((ent = readdir(dir)) != NULL)
				{
					if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
						files.Push(ent->d_name);
				}
				closedir(dir);
			}
		}
	}
	else
	{
		directory = false;
		writable = true;
	}
}

FString File::getDirectory() const
{
	int pos = filename.LastIndexOf('/');
	if (pos != -1)
		return filename.Mid(0, pos);
	return ".";
}

FString File::getFileName() const
{
	int pos = filename.LastIndexOf('/');
	if (pos != -1)
		return filename.Mid(pos + 1);
	return filename;
}

FString File::getInsensitiveFile(const FString &filename, bool sensitiveExtension) const
{
	for (unsigned int i = 0; i < files.Size(); ++i)
	{
		if (files[i].CompareNoCase(filename) == 0)
			return files[i];
	}
	return "";
}

FILE *File::open(const char* mode) const
{
	return fopen(filename.GetChars(), mode);
}

void File::rename(const FString &newname)
{
	FString newpath = getDirectory() + "/" + newname;
	if (::rename(filename.GetChars(), newpath.GetChars()) == 0)
		filename = newpath;
}

bool File::remove()
{
	return (::remove(filename.GetChars()) == 0);
}
