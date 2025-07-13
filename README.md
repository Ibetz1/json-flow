# json-flow
A tool for visualizing the evolution of your project config.

# compilation
```
g++ ./src/*.cpp ./src/imgui/*.cpp ./src/platform/*.c -lglfw3 -lgdi32 -lopengl32 -lole32 -luuid -lcomdlg32 -lshell32 -DJF_DEBUG_HEAP -Isrc
```