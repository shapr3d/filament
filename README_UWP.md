# Steps to run `shadowtest` sample on UWP using ANGLE:
1. Generate a Win32 project, which outputs the executables of tools that process meshes and materials during build. Run the following command:

    cmake -S . -B out-win32 -DFILAMENT_ENABLE_JAVA=NO -DFILAMENT_SKIP_SAMPLES=YES -DCMAKE_CONFIGURATION_TYPES="Debug;Release"

2. Open `out-win32\TNT.sln`, and build the following targets: cmgen, filamesh, glslminifier, matc, mipgen, resgen.
3. Generate UWP+ANGLE project by running the following command:

    cmake -S . -B out-uwp -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION="10.0" -DCMAKE_CONFIGURATION_TYPES="Debug;Release" -DUSE_STATIC_CRT=NO -DFILAMENT_ENABLE_JAVA=NO  -DFILAMENT_USE_EXTERNAL_GLES3=YES

4. Open `out-uwp\TNT.sln`, and build `shadowtest` target.
5. Copy DLL files from `third_party/angle/lib/winrt/Debug` to `out-uwp\samples\Debug\AppX` folder.
6. Copy `assets` and `default_env` folders from `out-uwp\samples` to `out-uwp\samples\Debug\AppX` folder.
7. Run `shadowtest` target.
