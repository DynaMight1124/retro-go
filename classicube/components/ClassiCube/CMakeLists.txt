set(srcs
    "src/Animations.c"
    "src/Audio.c"
    "src/AxisLinesRenderer.c"
    "src/Bitmap.c"
    "src/Block.c"
    "src/BlockPhysics.c"
    "src/Builder.c"
    "src/Camera.c"
    "src/Certs.c"
    "src/Chat.c"
    "src/Commands.c"
    "src/Deflate.c"
    "src/Drawer.c"
    "src/Drawer2D.c"
    "src/Entity.c"
    "src/EntityComponents.c"
    "src/EntityRenderers.c"
    "src/EnvRenderer.c"
    "src/Event.c"
    "src/ExtMath.c"
    "src/FancyLighting.c"
    "src/Formats.c"
    "src/Game.c"
    "src/GameVersion.c"
    "src/Generator.c"
    "src/Graphics_SoftGPU.c"
    "src/Gui.c"
    "src/HeldBlockRenderer.c"
    "src/Http_Worker.c"
    "src/Input.c"
    "src/InputHandler.c"
    "src/Inventory.c"
    "src/IsometricDrawer.c"
    "src/Lighting.c"
    "src/Logger.c"
    "src/LScreens.c"
    "src/LWeb.c"
    "src/LWidgets.c"
    "src/MapRenderer.c"
    "src/MenuOptions.c"
    "src/Menus.c"
    "src/Model.c"
    "src/Options.c"
    "src/PackedCol.c"
    "src/Particle.c"
    "src/Physics.c"
    "src/Picking.c"
    "src/Protocol.c"
    "src/Queue.c"
    "src/Resources.c"
    "src/Screens.c"
    "src/SelectionBox.c"
    "src/SelOutlineRenderer.c"
    "src/Server.c"
    "src/SSL.c"
    "src/Stream.c"
    "src/String.c"
    "src/SystemFonts.c"
    "src/TexturePack.c"
    "src/TouchUI.c"
    "src/Utils.c"
    "src/Vectors.c"
    "src/Vorbis.c"
    "src/Widgets.c"
    "src/World.c"
    "src/Platform_RetroGo.c"
    "src/Window_RetroGo.c"
    "src/Audio_RetroGo.c"
)

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "src"
                       REQUIRES retro-go)

target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    CC_BUILD_RETROGO 
    CC_BUILD_LOWMEM
    CC_BUILD_REDUCED_BGRA
    CC_BUILD_COOPTHREADED
    CC_GFX_BACKEND=CC_GFX_BACKEND_SOFTGPU
    CC_AUD_BACKEND=CC_AUD_BACKEND_RETROGO
    CC_DISABLE_LAUNCHER
    CC_DISABLE_NETWORKING
)

rg_setup_compile_options(-O3 -ffast-math)

