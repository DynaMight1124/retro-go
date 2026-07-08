import os
import subprocess
import sys

def run_command(cmd, cwd=None):
    print(f"Running: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, cwd=cwd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        sys.exit(1)

def main():
    if not os.path.exists('baserom.us.z64'):
        print("Error: baserom.us.z64 not found!")
        print("Please place the Super Mario 64 US ROM in this directory and name it 'baserom.us.z64'.")
        sys.exit(1)

    print("=== Step 1: Extracting N64 ROM Assets ===")
    print("Skipping Python extraction. This is handled by Docker beforehand.")

    print("\n=== Step 2: Generating Header Files ===")
    run_command([sys.executable, 'generate_headers.py'])

    print("\n=== Step 3: Fixing Constants ===")
    run_command([sys.executable, 'fix_const.py'])

    print("\n=== Step 4: Converting Textures to C Arrays ===")
    run_command([sys.executable, 'convert_textures.py'])

    print("\n=== Step 5: Generating Mario Animations and Demo Data ===")
    run_command([sys.executable, 'gen_assets.py'])

    # generate_skybox_c.py is handled automatically by CMakeLists.txt when building
    # but we can also manually process any missing skyboxes just to be thorough.
    print("\n=== Step 6: Verifying Skyboxes ===")
    skyboxes = ["bbh", "bidw", "bitfs", "bits", "ccm", "cloud_floor", "clouds", "ssl", "water", "wdw"]
    for skybox in skyboxes:
        png_path = f"textures/skyboxes/{skybox}.png"
        c_path = f"textures/skyboxes/{skybox}_skybox.c"
        if os.path.exists(png_path) and not os.path.exists(c_path):
            run_command([sys.executable, 'tools/generate_skybox_c.py', png_path, c_path])

    print("\n=== Step 7: Generating Ending Cake Textures ===")
    if os.path.exists("levels/ending/cake.png"):
        run_command([sys.executable, 'tools/generate_cake_c.py', 'levels/ending/cake.png', 'levels/ending/cake.inc.c'])
    if os.path.exists("levels/ending/cake_eu.png"):
        run_command([sys.executable, 'tools/generate_cake_c.py', 'levels/ending/cake_eu.png', 'levels/ending/cake_eu.inc.c', '--eu'])

    print("\n=== Step 8: Generating Text Data ===")
    os.makedirs("text/us", exist_ok=True)
    # Generate define_courses.inc.c
    run_command(["xtensa-esp-elf-gcc", "-E", "-Wno-trigraphs", "-I", "text/us/", "-DVERSION_US", "text/define_courses.inc.c", "-o", "text/us/define_courses.pre.c"])
    run_command([sys.executable, 'tools/textconv.py', 'charmap.txt', 'text/us/define_courses.pre.c', 'text/us/define_courses.inc.c'])
    
    # Generate define_text.inc.c
    run_command(["xtensa-esp-elf-gcc", "-E", "-Wno-trigraphs", "-I", "text/us/", "-DVERSION_US", "text/define_text.inc.c", "-o", "text/us/define_text.pre.c"])
    run_command([sys.executable, 'tools/textconv.py', 'charmap.txt', 'text/us/define_text.pre.c', 'text/us/define_text.inc.c'])

    print("\n=== Step 9: Copying Sound Assets ===")
    import shutil
    sound_files = [
        "sound_data.ctl.inc.c",
        "sound_data.tbl.inc.c",
        "sequences.bin.inc.c",
        "bank_sets.inc.c"
    ]
    for sf in sound_files:
        src = f"build/us/sound/{sf}"
        dst = f"sound/{sf}"
        if os.path.exists(src):
            print(f"Copying {sf} to sound/")
            shutil.copy2(src, dst)
        else:
            print(f"Warning: {src} not found. Did you run generate_sound.sh inside Docker?")

    print("\n=== Asset Preparation Complete! ===")
    print("You can now build the project using:")
    print("python rg_tool.py --target <your_target> release launcher sm64-go")

if __name__ == '__main__':
    main()
