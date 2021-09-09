# Steps to make libs for Shapr3D
(replace `<your_angle_include_dir>` with the `/include` dir in your ANGLE working copy!)
1. Generate a Win32 project, which outputs the executables of tools that process meshes and materials during build. Run the following command:

```
    cmake -S . -B out-win32 `
        -DCMAKE_CONFIGURATION_TYPES="Debug;Release" `
        -DFILAMENT_ENABLE_JAVA=NO `
        -DFILAMENT_SKIP_SAMPLES=YES `
        -DFILAMENT_USE_EXTERNAL_GLES3=YES `
        -DOPENGL_INCLUDE_DIR=<your_angle_include_dir>
```

2. Open `out-win32\TNT.sln`, and build the following targets: cmgen, filamesh, glslminifier, matc, mipgen, resgen.
3. Generate UWP+ANGLE project by running the following command:

```
    cmake -S . -B out-uwp `
        -DCMAKE_SYSTEM_NAME=WindowsStore `
        -DCMAKE_SYSTEM_VERSION="10.0" `
        -DCMAKE_CONFIGURATION_TYPES="Debug;Release" `
        -DUSE_STATIC_CRT=NO `
        -DFILAMENT_ENABLE_JAVA=NO `
        -DFILAMENT_SKIP_SAMPLES=YES `
        -DFILAMENT_USE_EXTERNAL_GLES3=YES `
        -DOPENGL_INCLUDE_DIR=<your_angle_include_dir>
```

4. Open `out-uwp\TNT.sln`, and build INSTALL target.
5. Copy smol-v.lib to filament/out-uwp/install/lib/x86_64 (INSTALL target doesn't put 3rd party libs there)
