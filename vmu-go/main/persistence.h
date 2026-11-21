#pragma once
#include <stdio.h>
#include <string>

inline bool vmu_write_file_checked(const char *path, const void *data, size_t size)
{
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fwrite(data, 1, size, file) == size && fflush(file) == 0;
    if (fclose(file) != 0) ok = false;
    return ok;
}

// FAT cannot replace an existing file with rename. Keep the previous complete
// save as .bak until the new file has been closed and promoted successfully.
inline bool vmu_atomic_write(const char *path, const void *data, size_t size)
{
    std::string temp = std::string(path) + ".tmp";
    std::string backup = std::string(path) + ".bak";
    if (!vmu_write_file_checked(temp.c_str(), data, size)) {
        remove(temp.c_str());
        return false;
    }
    FILE *file;
    bool existed = false;
    if ((file = fopen(path, "rb"))) { existed = true; fclose(file); }
    if (existed) {
        remove(backup.c_str());
        if (rename(path, backup.c_str()) != 0) { remove(temp.c_str()); return false; }
    }
    if (rename(temp.c_str(), path) != 0) {
        if (existed) rename(backup.c_str(), path);
        remove(temp.c_str());
        return false;
    }
    return true;
}

inline bool vmu_file_has_size(const char *path, size_t size)
{
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    bool ok = fseek(file, 0, SEEK_END) == 0 && ftell(file) == (long)size;
    fclose(file);
    return ok;
}

inline bool vmu_recover_backup(const char *path, size_t size)
{
    if (vmu_file_has_size(path, size)) return true;
    std::string backup = std::string(path) + ".bak";
    if (!vmu_file_has_size(backup.c_str(), size)) return false;
    remove(path);
    return rename(backup.c_str(), path) == 0;
}
