Cannonball-C
============

See README.md for original version.

Cannonball-C is a conversion of the Cannonball source code to ANSI C. 
The code is intended as a base for less powerful platforms that have 
non-existent or poor C++ compilers, like old consoles, computers, etc.
    
Codebase is based on the source at https://github.com/djyt/cannonball taken 
early September 2015.
    
Conversion Notes
----------------
    
    - All code converted from C++ to C
    - Removed STL
    - Removed Boost (which disables Cannonboard support)
    - Removed directx, (which disables force feedback)
    - Everything else should still work (SDL, OpenGL, Layout support, etc)

Known Issues
----------------

    Still has issue in visual studio where sdl linker path is wrong. Don't know
    enough about cmake to know how to fix it in script.
    
        Manual fix: change lib/SDL-1.2.15/lib/${Configuration} 
                 to lib/SDL-1.2.15/lib/x86
                 after the solution is built
             
             
Multiplatform Notes
----------------

    Only tested on Windows. I converted the cmake files for other platforms 
    I don't really know if they still work             
    
Credits & Support
----------------

    Conversion to C by djcc. 
    
    He can be contacted on the Reassembler forums. 
    http://reassembler.game-host.org/
    
    