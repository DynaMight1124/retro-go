#include "Core.h"

#include "Platform.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "String_.h"
#include "Errors.h"
#include "Utils.h"
#include "Input.h"
#include "Window.h"
#include "Gui.h"
#include "Inventory.h"
#include "Camera.h"
#include "Chat.h"
#include "InputHandler.h"
#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef UNIX_EPOCH_SECONDS
#define UNIX_EPOCH_SECONDS 1241100000ULL
#endif

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_Alloc(cc_uint32 numElements, cc_uint32 size, const char* place) {
    void* ptr = rg_alloc(numElements * size, MEM_SLOW);
    if (!ptr) RG_PANIC("Out of memory");
    return ptr;
}

void* Mem_TryAlloc(cc_uint32 numElements, cc_uint32 size) {
    return rg_alloc(numElements * size, MEM_SLOW);
}

void* Mem_AllocCleared(cc_uint32 numElements, cc_uint32 size, const char* place) {
    void* ptr = Mem_Alloc(numElements, size, place);
    memset(ptr, 0, numElements * size);
    return ptr;
}

void* Mem_TryAllocCleared(cc_uint32 numElements, cc_uint32 size) {
    void* ptr = Mem_TryAlloc(numElements, size);
    if (ptr) memset(ptr, 0, numElements * size);
    return ptr;
}

void* Mem_Realloc(void* mem, cc_uint32 numElements, cc_uint32 size, const char* place) {
    void* ptr = realloc(mem, numElements * size);
    if (!ptr) RG_PANIC("Out of memory during realloc");
    return ptr;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElements, cc_uint32 size) {
    return realloc(mem, numElements * size);
}

void Mem_Free(void* mem) {
    if (mem) free(mem);
}

void* Mem_Copy(void* dst, const void* src, unsigned numBytes) {
    return memcpy(dst, src, numBytes);
}

void* Mem_Set(void* dst, cc_uint8 value, unsigned numBytes) {
    return memset(dst, value, numBytes);
}

int Mem_Equal(const void* a, const void* b, cc_uint32 numBytes) {
    return memcmp(a, b, numBytes) == 0;
}

static void* temp_mem_ptr;
void* TempMem_Alloc(int size) {
    if (!temp_mem_ptr) temp_mem_ptr = Mem_Alloc(1, 64 * 1024, "temp mem");
    return temp_mem_ptr;
}

void TempMem_Free(void* mem) {
    // Keep it allocated for reuse
}


/*########################################################################################################################*
*---------------------------------------------------------Logging---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
    char buf[512];
    int count = min(len, 511);
    memcpy(buf, msg, count);
    buf[count] = '\0';
    RG_LOGI("%s", buf);
}

void Platform_LogConst(const char* message) {
    RG_LOGI("%s\n", message);
}

void Platform_Log1(const char* format, const void* a1) {
    cc_string msg; char msgBuffer[512];
    String_InitArray_NT(msg, msgBuffer);
    String_Format1(&msg, format, a1);
    msg.buffer[msg.length] = '\0';
    RG_LOGI("%s\n", msg.buffer);
}

void Platform_Log2(const char* format, const void* a1, const void* a2) {
    cc_string msg; char msgBuffer[512];
    String_InitArray_NT(msg, msgBuffer);
    String_Format2(&msg, format, a1, a2);
    msg.buffer[msg.length] = '\0';
    RG_LOGI("%s\n", msg.buffer);
}

void Platform_Log3(const char* format, const void* a1, const void* a2, const void* a3) {
    cc_string msg; char msgBuffer[512];
    String_InitArray_NT(msg, msgBuffer);
    String_Format3(&msg, format, a1, a2, a3);
    msg.buffer[msg.length] = '\0';
    RG_LOGI("%s\n", msg.buffer);
}

void Platform_Log4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4) {
    cc_string msg; char msgBuffer[512];
    String_InitArray_NT(msg, msgBuffer);
    String_Format4(&msg, format, a1, a2, a3, a4);
    msg.buffer[msg.length] = '\0';
    RG_LOGI("%s\n", msg.buffer);
}


/*########################################################################################################################*
*----------------------------------------------------------Time-----------------------------------------------------------*
*#########################################################################################################################*/
TimeMS DateTime_CurrentUTC(void) {
    return (TimeMS)(UNIX_EPOCH_SECONDS + (rg_system_timer() / 1000000));
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
    memset(t, 0, sizeof(*t));
}

cc_uint64 Stopwatch_Measure(void) {
    return rg_system_timer();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 start, cc_uint64 end) {
    return end - start;
}

int Stopwatch_ElapsedMS(cc_uint64 start, cc_uint64 end) {
    return (int)((end - start) / 1000);
}


/*########################################################################################################################*
*--------------------------------------------------------Filesystem-------------------------------------------------------*
*#########################################################################################################################*/
const cc_result ReturnCode_DirectoryExists = EEXIST;
const cc_result ReturnCode_FileNotFound    = ENOENT;
const cc_result ReturnCode_FileShareViolation = 1000000000;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

const char* Platform_AppNameSuffix = " retro-go";
cc_bool Platform_ReadonlyFilesystem = false;

static void GetFullPath(char* str, const cc_filepath* path) {
    const char* root = RG_BASE_PATH_ROMS "/classicube/data/";
    const char* filename = path->buffer;

    // Use saves for options and other mutable data
    if (strstr(filename, "options.txt") || strstr(filename, "options-default.txt")) {
        root = RG_BASE_PATH_CONFIG "/classicube/";
    } else if (strstr(filename, ".log") || strstr(filename, "maps/")) {
        root = RG_BASE_PATH_SAVES "/classicube/";
    }

    snprintf(str, 1024, "%s%s", root, filename);
}

CC_API void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
    int i, len = min(path->length, FILENAME_MAX - 1);
    for (i = 0; i < len; i++) dst->buffer[i] = path->buffer[i];
    dst->buffer[len] = '\0';
}

CC_API void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
    cc_string s = String_FromReadonly(path->buffer);
    String_Copy(dst, &s);
}

cc_result Directory_Create(const cc_filepath* path) {
    char str[1024]; GetFullPath(str, path);
    // Recursively create directory if needed (retro-go handles this or we need to ensure parents exist)
    if (mkdir(str, 0777) == -1 && errno != EEXIST) return errno;
    return 0;
}

cc_result Directory_Create2(const cc_filepath* path) {
    return Directory_Create(path);
}

cc_result Directory_Enum(const cc_string* path, void* obj, Directory_EnumCallback callback) {
    return ERR_NOT_SUPPORTED;
}

int File_Exists(const cc_filepath* path) {
    char str[1024]; GetFullPath(str, path);
    struct stat buffer;
    return stat(str, &buffer) == 0;
}

void Directory_GetCachePath(cc_string* path) { }

cc_result File_Open(cc_file* file, const cc_filepath* path) {
    char str[1024]; GetFullPath(str, path);
    FILE* f = fopen(str, "rb");
    *file = (cc_file)f;
    return f ? 0 : errno;
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
    char str[1024]; GetFullPath(str, path);
    FILE* f = fopen(str, "wb");
    *file = (cc_file)f;
    return f ? 0 : errno;
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
    char str[1024]; GetFullPath(str, path);
    FILE* f = fopen(str, "r+b");
    if (!f) f = fopen(str, "w+b");
    *file = (cc_file)f;
    return f ? 0 : errno;
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
    *bytesRead = fread(data, 1, count, (FILE*)file);
    if (*bytesRead < count && ferror((FILE*)file)) return errno;
    return 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
    *bytesWrote = fwrite(data, 1, count, (FILE*)file);
    if (*bytesWrote < count && ferror((FILE*)file)) return errno;
    return 0;
}

cc_result File_Close(cc_file file) {
    return fclose((FILE*)file) ? errno : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
    static cc_uint8 modes[] = { SEEK_SET, SEEK_CUR, SEEK_END };
    return fseek((FILE*)file, offset, modes[seekType]) ? errno : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
    long res = ftell((FILE*)file);
    if (res == -1) return errno;
    *pos = (cc_uint32)res; return 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
    FILE* f = (FILE*)file;
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long res = ftell(f);
    fseek(f, cur, SEEK_SET);
    if (res == -1) return errno;
    *len = (cc_uint32)res; return 0;
}


/*########################################################################################################################*
*---------------------------------------------------------Process---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Exit(int code) { rg_system_exit(); }
void Platform_Sleep(cc_uint32 ms) { rg_usleep(ms * 1000); }

void Process_Abort2(cc_result result, const char* msg) {
    RG_LOGE("ABORT: %s (0x%lx)\n", msg, result);
    rg_system_panic("ClassiCube", msg);
}

cc_bool Process_OpenSupported = false;
cc_result Process_StartOpen(const cc_string* args) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 ms) { rg_usleep(ms * 1000); }
void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) { *handle = NULL; }
void Thread_Detach(void* handle) { }
void Thread_Join(void* handle) { }

void* Mutex_Create(const char* name) { return NULL; }
void Mutex_Free(void* handle) { }
void Mutex_Lock(void* handle) { }
void Mutex_Unlock(void* handle) { }

void* Waitable_Create(const char* name) { return NULL; }
void Waitable_Free(void* handle) { }
void Waitable_Signal(void* handle) { }
void Waitable_Wait(void* handle) { }
void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) { }

/*########################################################################################################################*
*---------------------------------------------------------Other-----------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Init(void) { }
void Platform_Free(void) { }
void Platform_SetCurrentDirectory(const cc_string* path) { }
void Platform_GetExecutablePath(cc_string* path) { }
cc_result Platform_LoadDll(const cc_string* path, void** lib) { return ERR_NOT_SUPPORTED; }
cc_result Platform_GetDllExport(void* lib, const char* name, void** symbol) { return ERR_NOT_SUPPORTED; }
cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
    return false;
}

cc_result Process_Start(const cc_string* path, const cc_string* args) { return ERR_NOT_SUPPORTED; }

/*########################################################################################################################*
*---------------------------------------------------------Networking------------------------------------------------------*
*#########################################################################################################################*/
cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) { return ERR_NOT_SUPPORTED; }
cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) { return ERR_NOT_SUPPORTED; }
cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* bytesRead) { return ERR_NOT_SUPPORTED; }
cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* bytesWrote) { return ERR_NOT_SUPPORTED; }
void Socket_Close(cc_socket s) { }
cc_result Socket_Poll(cc_socket s, int timeoutMS, int mode, cc_bool* success) { return ERR_NOT_SUPPORTED; }
cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) { return ERR_NOT_SUPPORTED; }
cc_result Socket_ConnectAddr(cc_socket s, void* addr) { return ERR_NOT_SUPPORTED; }
cc_result Socket_GetLastError(cc_socket s) { return ERR_NOT_SUPPORTED; }
cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) { return false; }

cc_result SSL_Write(void* ctx, const cc_uint8* data, cc_uint32 count, cc_uint32* bytesWrote) { return ERR_NOT_SUPPORTED; }

void LauncherTheme_Load(void) { }

#if 0
static const BindMapping pad_defaults[BIND_COUNT] = {
	[BIND_LOOK_UP]      = { CCPAD_2, CCPAD_UP },
	[BIND_LOOK_DOWN]    = { CCPAD_2, CCPAD_DOWN },
	[BIND_LOOK_LEFT]    = { CCPAD_2, CCPAD_LEFT },
	[BIND_LOOK_RIGHT]   = { CCPAD_2, CCPAD_RIGHT },
	[BIND_FORWARD]      = { CCPAD_UP,    0 },  
	[BIND_BACK]         = { CCPAD_DOWN,  0 },
	[BIND_LEFT]         = { CCPAD_LEFT,  0 },  
	[BIND_RIGHT]        = { CCPAD_RIGHT, 0 },
	[BIND_JUMP]         = { CCPAD_1, 0 },
	[BIND_INVENTORY]    = { CCPAD_R, CCPAD_UP },
	[BIND_PLACE_BLOCK]  = { CCPAD_L, 0 },
	[BIND_DELETE_BLOCK] = { CCPAD_R, 0 },
	[BIND_HOTBAR_LEFT]  = { CCPAD_L, CCPAD_LEFT }, 
	[BIND_HOTBAR_RIGHT] = { CCPAD_L, CCPAD_RIGHT }
};
#endif

static void MapKey(int key, int pressed) {
	if (Input.Pressed[key] != pressed) {
		Input_Set(key, pressed);
	}
}

void Gamepads_Process(float delta) {
	static uint32_t last_joystick = 0;
	static cc_bool buildMode = false;
	static cc_bool startTriggeredModifier = false;

	uint32_t joystick = rg_input_read_gamepad();

	// Keyboard Emulation Pipeline (direct and bypasses options/bind bugs)
	int inMenu = (Gui.InputGrab != NULL);

	// D-pad mappings
	int upPressed = (joystick & RG_KEY_UP) != 0;
	int downPressed = (joystick & RG_KEY_DOWN) != 0;
	int leftPressed = (joystick & RG_KEY_LEFT) != 0;
	int rightPressed = (joystick & RG_KEY_RIGHT) != 0;

	// Option button strictly cycles Build / Destroy Mode
	uint32_t optionBtn = joystick & RG_KEY_OPTION;
	uint32_t lastOptionBtn = last_joystick & RG_KEY_OPTION;
	if (optionBtn && !lastOptionBtn) {
		buildMode = !buildMode;
		if (buildMode) {
			Chat_AddRaw("&a[Build Mode]");
		} else {
			Chat_AddRaw("&c[Destroy/Delete Mode]");
		}
	}

	int lookLeft = 0;
	int lookRight = 0;
	int lookUp = 0;
	int lookDown = 0;
	int walkForward = 0;
	int walkBackward = 0;
	int strafeLeft = 0;
	int strafeRight = 0;

	if (inMenu) {
		// Menu navigation
		MapKey(CCKEY_UP,    upPressed);
		MapKey(CCKEY_DOWN,  downPressed);
		MapKey(CCKEY_LEFT,  leftPressed);
		MapKey(CCKEY_RIGHT, rightPressed);
		
		// In-game movement keys must be released when entering menu
		MapKey('W', 0);
		MapKey('S', 0);
		MapKey('A', 0);
		MapKey('D', 0);
	} else {
		int startPressed = (joystick & RG_KEY_START) != 0;
		int lastStartPressed = (last_joystick & RG_KEY_START) != 0;

		if (startPressed && !lastStartPressed) {
			startTriggeredModifier = false;
		}

		if (startPressed) {
			// START modifier active:
			// D-pad Up/Down looks Up/Down, Left/Right strafes Left/Right
			lookUp = upPressed;
			lookDown = downPressed;
			strafeLeft = leftPressed;
			strafeRight = rightPressed;

			if (upPressed || downPressed || leftPressed || rightPressed) {
				startTriggeredModifier = true;
			}

			// START + A cycles Hotbar active slot
			int aPressed = (joystick & RG_KEY_A) != 0;
			int lastAPressed = (last_joystick & RG_KEY_A) != 0;
			if (aPressed && !lastAPressed) {
				int nextIndex = Inventory.SelectedIndex + 1;
				if (nextIndex > 8) nextIndex = 0;
				Inventory_SetSelectedIndex(nextIndex);
				startTriggeredModifier = true;
			}

			// START + B cycles Camera view perspective
			int bPressed = (joystick & RG_KEY_B) != 0;
			int lastBPressed = (last_joystick & RG_KEY_B) != 0;
			if (bPressed && !lastBPressed) {
				Camera_CycleActive();
				startTriggeredModifier = true;
			}
		} else {
			// Normal FPS Mode: D-pad Up/Down walks Forward/Backward, Left/Right turns Left/Right
			walkForward = upPressed;
			walkBackward = downPressed;
			lookLeft = leftPressed;
			lookRight = rightPressed;
		}

		// Map simulated movement keys
		MapKey('W', walkForward);
		MapKey('S', walkBackward);
		MapKey('A', strafeLeft);
		MapKey('D', strafeRight);

		// Update look binds directly
		if (lookUp) {
			Bind_IsTriggered[BIND_LOOK_UP] |= INPUT_DEVICE_NORMAL;
		} else {
			Bind_IsTriggered[BIND_LOOK_UP] &= ~INPUT_DEVICE_NORMAL;
		}

		if (lookDown) {
			Bind_IsTriggered[BIND_LOOK_DOWN] |= INPUT_DEVICE_NORMAL;
		} else {
			Bind_IsTriggered[BIND_LOOK_DOWN] &= ~INPUT_DEVICE_NORMAL;
		}

		if (lookLeft) {
			Bind_IsTriggered[BIND_LOOK_LEFT] |= INPUT_DEVICE_NORMAL;
		} else {
			Bind_IsTriggered[BIND_LOOK_LEFT] &= ~INPUT_DEVICE_NORMAL;
		}

		if (lookRight) {
			Bind_IsTriggered[BIND_LOOK_RIGHT] |= INPUT_DEVICE_NORMAL;
		} else {
			Bind_IsTriggered[BIND_LOOK_RIGHT] &= ~INPUT_DEVICE_NORMAL;
		}

		// Menu keys must be released when in-game
		MapKey(CCKEY_UP,    0);
		MapKey(CCKEY_DOWN,  0);
		MapKey(CCKEY_LEFT,  0);
		MapKey(CCKEY_RIGHT, 0);
	}

	// Action buttons mappings
	int enterPressed = inMenu ? ((joystick & RG_KEY_A) != 0) : 0;
	int spacePressed = 0;
	if (!inMenu) {
		int startHeld = (joystick & RG_KEY_START) != 0;
		if (!startHeld) {
			spacePressed = (joystick & RG_KEY_B) != 0;
		}
	}

	// Tapping START cycles the active hotbar slot on release, if no modifier was triggered
	int startPressed = (joystick & RG_KEY_START) != 0;
	int lastStartPressed = (last_joystick & RG_KEY_START) != 0;
	int escPressed = 0;
	if (!startPressed && lastStartPressed) {
		if (!startTriggeredModifier) {
			int nextIndex = Inventory.SelectedIndex + 1;
			if (nextIndex > 8) nextIndex = 0;
			Inventory_SetSelectedIndex(nextIndex);
		}
	}

	// MENU button opens/closes Pause Menu on tap (edge-triggered)
	int menuPressed = (joystick & RG_KEY_MENU) != 0;
	int lastMenuPressed = (last_joystick & RG_KEY_MENU) != 0;
	if (menuPressed && !lastMenuPressed) {
		escPressed = 1;
	}

	if (inMenu) {
		// In menus, B closes/backs out
		int bPressed = (joystick & RG_KEY_B) != 0;
		int lastBPressed = (last_joystick & RG_KEY_B) != 0;
		if (bPressed && !lastBPressed) {
			escPressed = 1;
		}
	}

	MapKey(CCKEY_ENTER, enterPressed);
	MapKey(CCKEY_SPACE, spacePressed);
	MapKey(CCKEY_ESCAPE, escPressed);

	// Inventory: Select button strictly opens/closes inventory when START is not held
	int selectPressed = (joystick & RG_KEY_SELECT) != 0;
	int lastSelectPressed = (last_joystick & RG_KEY_SELECT) != 0;
	int invPressed = 0;
	if (!inMenu) {
		int startHeld = (joystick & RG_KEY_START) != 0;
		if (!startHeld) {
			if (selectPressed && !lastSelectPressed) {
				invPressed = 1;
			}
		}
	} else {
		// In inventory menu, Select closes it too
		if (selectPressed && !lastSelectPressed) {
			invPressed = 1;
		}
	}
	
	// Map to inventory block menu
	MapKey(CCKEY_B, invPressed);

	// In-game action buttons (mouse clicks)
	int leftClick = 0;
	int rightClick = 0;
	if (!inMenu) {
		int triggerLeft = (joystick & RG_KEY_R) != 0;
		int triggerRight = (joystick & RG_KEY_L) != 0;
		
		int startHeld = (joystick & RG_KEY_START) != 0;
		int actionPressed = (joystick & RG_KEY_A) != 0;
		if (actionPressed && !startHeld) {
			if (buildMode) {
				rightClick = 1;
			} else {
				leftClick = 1;
			}
		}
		if (triggerLeft) leftClick = 1;
		if (triggerRight) rightClick = 1;
	}
	MapKey(CCMOUSE_L, leftClick);
	MapKey(CCMOUSE_R, rightClick);

	last_joystick = joystick;
}
