# Godot MPV bindings

Uses MPV to play a video in an image texture. The video is decoded on the GPU, but the image texture is updated thru the
CPU.
Ideally we could use the texture directly without going this indirect route.  
This is not really meant to be shipped as a self contained game, since it requires MPV to be installed on the system.

Tested with godot 4.1.

## compile

Only tested on linux, might work on other platforms as well.

```bash
mkdir build
cd build
cmake ..
cmake --build . --target godot-mpv -j8
```

If all went well, you should have a `libgodot-mpv.so` in the `sample/godot-mpv` directory.  
Copy this directory into your own project.