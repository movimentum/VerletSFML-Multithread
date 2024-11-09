# Verlet Multi thread

![image](images/image_1.png)

## Compilation

 [CMake](https://cmake.org/) needs to be installed.
 Git needs to be installed as it fetches [SFML](https://www.sfml-dev.org/) during cmake build.

Create a `build` directory

```bash
mkdir build
cd build
```

**Configure** and **build** the project

```bash
cmake ..
cmake --build .
```

On **Windows** it will build in **debug** by default. To build in release you need to use this command

```bash
cmake --build . --config Release
```

That's it, just run `build/bin/Release/Verlet-Multithread.exe` (on Windows).

Note, you might want to install [SFML](https://www.sfml-dev.org/) manually and add the `res` directory and the SFML dlls in the Release or Debug directory for the executable to run, as original repository suggests.


