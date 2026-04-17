Amiga Cannonball-C
==================
 
Cannonball-C is a conversion of the Cannonball source code to ANSI C by djcc
Amiga 68k port of Cannonball-C by Modern Vintage Gamer

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

      
    
Credits & Support
----------------

    Conversion to C by djcc. 
    
    He can be contacted on the Reassembler forums. 
    http://reassembler.game-host.org/
    
    