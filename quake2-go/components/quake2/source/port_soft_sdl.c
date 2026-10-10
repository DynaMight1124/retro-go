
#include <stdio.h>

#include "ref_soft/r_local.h"
#include "client/keys.h"

#include "quake2.h"


#include <SDL.h>

static SDL_Color colors[256];
static SDL_Palette* sdlPalette = NULL;


int _old_mouse_buttonstate = 0;

static int _width = 640;
static int _height = 480;

static SDL_Surface *_screenSurface = NULL;
static SDL_Surface *surface = NULL;
static SDL_Window* _window = NULL;

/*****************************************************************************/

static void setupWindow(qboolean fullscreen)
{
  if (_window)
  {
    SDL_DestroyWindow(_window);
  }

  if (surface)
  {
    SDL_FreeSurface(surface);
  }

  int flags = SDL_WINDOW_SHOWN;
  if (fullscreen)
  {
    flags |= SDL_WINDOW_FULLSCREEN;
  }
  _window = SDL_CreateWindow("Quake2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, flags);
  _screenSurface = SDL_GetWindowSurface(_window);
  surface = SDL_CreateRGBSurface(0, _width, _height, 8, 0, 0, 0, 0);
  SDL_SetSurfacePalette(surface, sdlPalette);

  vid.rowbytes = surface->pitch;
  vid.buffer = surface->pixels;
}


rserr_t SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width = 0;
  int height = 0;

	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d\n", width, height );


  _width = width;
  _height = height;

  *pwidth = width;
	*pheight = height;

  setupWindow(fullscreen);	

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	return rserr_ok;
}

void SWimp_Shutdown( void )
{
    //TODO: check if there are buffers need to be freed

	SDL_Quit();
}


int SWimp_Init( void *hInstance, void *wndProc )
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    Sys_Error("VID: Couldn't load SDL: %s", SDL_GetError());
  }

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  sdlPalette = SDL_AllocPalette(256);

  setupWindow(false);

	return true;
}

static qboolean SWimp_InitGraphics( qboolean fullscreen )
{
    vid.rowbytes = surface->pitch;
	  vid.buffer = surface->pixels;

    return rserr_ok;
}

void SWimp_SetPalette( const unsigned char *palette )
{
	for (int i=0; i<256; ++i)
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
        colors[i].a = SDL_ALPHA_OPAQUE;
        palette++;
    }
    
    if (sdlPalette)
    {
        SDL_SetPaletteColors(sdlPalette, colors, 0, 256);
        SDL_SetSurfacePalette(surface, sdlPalette);
    }
}

void SWimp_BeginFrame( float camera_seperation )
{
}


/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void SWimp_EndFrame (void)
{
  SDL_BlitSurface(surface, NULL, _screenSurface, NULL);
  SDL_UpdateWindowSurface(_window);
}


void SWimp_AppActivate( qboolean active )
{

}


int ConvertToQuakeKey(unsigned int keysym)
{
  int key;
  
  key = 0;
  switch(keysym) {
  case SDLK_KP_9:            key = K_KP_PGUP; break;
  case SDLK_PAGEUP:        key = K_PGUP; break;
      
  case SDLK_KP_3:            key = K_KP_PGDN; break;
  case SDLK_PAGEDOWN:        key = K_PGDN; break;
    
  case SDLK_KP_7:            key = K_KP_HOME; break;
  case SDLK_HOME:            key = K_HOME; break;
    
  case SDLK_KP_1:            key = K_KP_END; break;
  case SDLK_END:            key = K_END; break;
    
  case SDLK_KP_4:            key = K_KP_LEFTARROW; break;
  case SDLK_LEFT:            key = K_LEFTARROW; break;
    
  case SDLK_KP_6:            key = K_KP_RIGHTARROW; break;
  case SDLK_RIGHT:        key = K_RIGHTARROW; break;
    
  case SDLK_KP_2:            key = K_KP_DOWNARROW; break;
  case SDLK_DOWN:            key = K_DOWNARROW; break;
    
  case SDLK_KP_8:            key = K_KP_UPARROW; break;
  case SDLK_UP:            key = K_UPARROW; break;
    
  case SDLK_ESCAPE:        key = K_ESCAPE; break;
    
  case SDLK_KP_ENTER:        key = K_KP_ENTER; break;
  case SDLK_RETURN:        key = K_ENTER; break;
    
  case SDLK_TAB:            key = K_TAB; break;
    
  case SDLK_F1:            key = K_F1; break;
  case SDLK_F2:            key = K_F2; break;
  case SDLK_F3:            key = K_F3; break;
  case SDLK_F4:            key = K_F4; break;
  case SDLK_F5:            key = K_F5; break;
  case SDLK_F6:            key = K_F6; break;
  case SDLK_F7:            key = K_F7; break;
  case SDLK_F8:            key = K_F8; break;
  case SDLK_F9:            key = K_F9; break;
  case SDLK_F10:            key = K_F10; break;
  case SDLK_F11:            key = K_F11; break;
  case SDLK_F12:            key = K_F12; break;
    
  case SDLK_BACKSPACE:        key = K_BACKSPACE; break;
    
  case SDLK_KP_PERIOD:        key = K_KP_DEL; break;
  case SDLK_DELETE:        key = K_DEL; break;
    
  case SDLK_PAUSE:        key = K_PAUSE; break;
    
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:        key = K_SHIFT; break;
    
  case SDLK_LCTRL:
  case SDLK_RCTRL:        key = K_CTRL; break;
    
  case SDLK_LGUI:
  case SDLK_RGUI:
  case SDLK_LALT:
  case SDLK_RALT:            key = K_ALT; break;
    
  case SDLK_KP_5:            key = K_KP_5; break;
    
  case SDLK_INSERT:        key = K_INS; break;
  case SDLK_KP_0:            key = K_KP_INS; break;
    
  case SDLK_KP_MULTIPLY:        key = '*'; break;
  case SDLK_KP_PLUS:        key = K_KP_PLUS; break;
  case SDLK_KP_MINUS:        key = K_KP_MINUS; break;
  case SDLK_KP_DIVIDE:        key = K_KP_SLASH; break;

  default: /* assuming that the other sdl keys are mapped to ascii */
    if (keysym < 128)
      key = keysym;
    break;
  }

  return key;        
}

void HandleInput(void)
{
  SDL_Event event;
  
  while (SDL_PollEvent(&event))
  {
    unsigned int key = 0;
    
    switch(event.type) {
    case SDL_KEYDOWN:
      key = ConvertToQuakeKey(event.key.keysym.sym);
      if (key) {
        Quake2_SendKey(key, true);
      }
      break;
    case SDL_KEYUP:
        key = ConvertToQuakeKey(event.key.keysym.sym);
        if (key) {
            Quake2_SendKey(key, false);
        }
        break;
    case SDL_QUIT:
        ri.Cmd_ExecuteText(EXEC_NOW, "quit");
        break;
    }
  }

  int mouse_buttonstate = 0;
  int bstate = SDL_GetMouseState(NULL, NULL);
  if (SDL_BUTTON(1) & bstate)
      mouse_buttonstate |= (1 << 0);
  if (SDL_BUTTON(3) & bstate) /* quake2 has the right button be mouse2 */
      mouse_buttonstate |= (1 << 1);
  if (SDL_BUTTON(2) & bstate) /* quake2 has the middle button be mouse3 */
      mouse_buttonstate |= (1 << 2);
  if (SDL_BUTTON(6) & bstate)
      mouse_buttonstate |= (1 << 3);
  if (SDL_BUTTON(7) & bstate)
      mouse_buttonstate |= (1 << 4);

    
    for (int i = 0; i < 3; i++)
    {
      if ( (mouse_buttonstate & (1<<i)) &&
        !(_old_mouse_buttonstate & (1<<i)) )
      {
        Quake2_SendKey(K_MOUSE1 + i, true);
      }

      if ( !(mouse_buttonstate & (1<<i)) &&
        (_old_mouse_buttonstate & (1<<i)) )
      {
          Quake2_SendKey(K_MOUSE1 + i, false);
      }
    }

    _old_mouse_buttonstate = mouse_buttonstate;
}


int QG_Milliseconds(void)
{	
	return SDL_GetTicks();
}

void QG_GetMouseDiff(int* dx, int* dy)
{
  SDL_GetRelativeMouseState(dx, dy);
}

void QG_CaptureMouse(void)
{
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

void QG_ReleaseMouse(void)
{
  SDL_SetRelativeMouseMode(SDL_FALSE);
}

int main(int argc, char **argv)
{
	int time, oldtime, newtime;

	Quake2_Init(argc, argv);

    oldtime = Quake2_Milliseconds ();
    while (1)
    {
      HandleInput();
      
      do {
        newtime = Quake2_Milliseconds ();
        time = newtime - oldtime;
      } while (time < 1);

      Quake2_Frame(time);
      
      oldtime = newtime;
    }

    return 0;
}