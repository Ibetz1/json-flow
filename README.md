# 🌀 JSN flow  
**A powerful tool for visualizing the evolution of your project's configuration files over time.**

---

## 🚀 What is JSN flow

Modern software projects often rely heavily on structured configuration files — yet tracking changes to these configs is a nightmare. `JSN flow` solves that.

**JSN Flow** is a standalone desktop visualizer that lets you:
- Instantly see how your project config files evolve over time.
- Compare diffs between versions with a color-coded tree view.
- Track file history using a timeline-based UI.
- Navigate deeply nested keys with ease.
- Spot regressions and misconfigurations visually.
- Instantly roll back changes from ages ago

Whether you're debugging a bug introduced 3 versions ago or documenting a feature rollout, **JSN flow** gives you insight into your config timeline with surgical clarity.

Built entirely from scratch with **OpenGL**, native filesystem access, and zero runtime dependencies.

---

## 🧰 Features

- ✅ Full timeline of changes to config files, auto-organized by version
- ✅ Inline diffing of values with color indicators for **added**, **removed**, and **changed** keys
- ✅ Filter and search through keys and values across all timeline versions
- ✅ Intuitive interface with clickable timelines and collapsible trees
- ✅ Fully local — no internet access or background daemons required
- ✅ Cross-platform potential (currently Windows-focused)

---

## 🔧 Installation (Windows)

### 1. Dependencies

You will need:

- **g++ (MinGW)** or any gnu compiler with C++17 support  
- **GLFW** (as `glfw3`)  
- Windows system libraries:  
  - `gdi32`, `opengl32`, `ole32`, `uuid`, `comdlg32`, `shell32` (already available on most systems)

---

### 2. Clone the Repository

`git clone https://github.com/Ibetz1/json-flow.git`

### 3. Build the Project
```
cd json-flow
g++ ./src/*.cpp ./src/imgui/*.cpp ./src/platform/*.c -lglfw3 -lgdi32 -lopengl32 -lole32 -luuid -lcomdlg32 -lshell32 -DJF_DEBUG_HEAP -Isrc -std=c++17
```
