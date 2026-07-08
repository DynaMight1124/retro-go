# Docker environment to build SM64 for Funkey

**Step 1: Copy code & Dockerfile.sm64 to your PC**

Either git clone or download the sourcecode, this example assumes the full code is in C:\SM64\source & "Dockerfile.sm64" is in the SM64 directory.

Note: You need to ensure "baserom.us.z64" is in the source directory also.

**Step 2: Run the Container from Powershell (Install Docker Desktop if not installed)**

docker run -it --rm -v "C:\SM64\source:/workspace" sm64-build

### Compile (Inside the Container)

Once inside the container, run these commands.

**1. Extract Assets (First time only)**
This uses the Python tool to pull data from your ROM.
## ./extract_assets.py --clean

**2. Compile and Build OPK**

## ./build_opk.sh funkey-s

Once completed, look for "sm64_us_v1.3_funkey-s.opk" in the source directory


