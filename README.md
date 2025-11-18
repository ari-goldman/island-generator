# OpenGL Island Generator

### Features
- Perlin noise starting island
- Simulated hydraulic erosion computed on the GPU
- Ray-marched clouds tha integrate into the scene with optional fractal brownian motion
- Water... mildly messed up, I attempted to make a tessellation shader to draw it but did not get it to a place I wanted to
- Triplanar mapping

---

### Demo

-> [Video demo on YouTube](https://youtu.be/pFPAKgLexl0)

---

### Use

#### To run (Windows and Linux only):
[install the GLFW library](https://www.glfw.org/download) if you you don't have it
```bash
$ make #compiles code and makes executable
$ ./island_generator #or .\island_generator.exe on Windows
$ make clean #clears junk files and executable
```

#### Controls:
- Click and drag or arrow keys to orbit camera
- E: perform 1 erode step (128 particles) (hold to erode more)
- Space: generate a new island
- T: toggle triplanar mapping (default = on)
- F: toggle fractal brownian motion for clouds (default = off) (WARNING: very slow)
- C or Shift+C: raise/lower clouds
- Page up/down: zoom in/out (or shift + up/down arrows)
- Escape: safely exit the program

---

### Credits
- Willem A. (Vlakkies) Schreuder: CSCIx239.h library
- Ryo Suzuki: PerlinNoise.hpp

---

> April 2025
