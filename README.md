### PSX-Inspired Fixed-Point 3D Renderer (Strict C89)

A lightweight, high-performance 3D software-assisted renderer written in **strict ANSI C (C89)**, heavily inspired by the hardware architecture and constraints of the original PlayStation 1 (PSX). 

This project completely avoids modern high-level rendering frameworks, external float libraries, and modern C features. Instead, it features a highly disciplined codebase where all components are engineered under strict mid-80s standards, recreating the iconic 32-bit retro aesthetic through pure fixed-point mathematics, custom low-level pipelines, and Modern OpenGL. 
<img width="1280" height="720" alt="Screenshot From 2026-11-03 00-46-51" src="https://github.com/user-attachments/assets/3ddf3c33-011c-410d-8863-29de52797331" />

### 🚀 Key Technical Features

### 1. Pure ANSI C (C89) Compliance & Data-Oriented Architecture

* **Strict C89 Standard:** No mixed declarations, no C99 types, and no // inline comments. Pure compatibility designed to compile on anything from modern Linux machines to legacy compiler tools.
* **No Global State:** The architecture relies strictly on passing pointers to isolated structures (struct gpu_data *, struct gte_transform *) through an object-like C interface, ensuring clean memory boundaries and deterministic behavior.

### 2. Hardware Emulation & Retro Aesthetics

* **Fixed-Point Math Engine (Q3.12):** Vertex transformations, matrix multiplications, and vector operations skip floats entirely, perfectly mirroring the processing characteristics of the PSX's GTE (Geometry Transformation Engine).
* **Affine Texture Mapping (noperspective):** Explicitly disables modern perspective-correct texture interpolation via GLSL modifiers to induce the classic "texture warping" and swimming geometry effect.
* **Color Quantization (RGB555):** Recreates 15-bit color precision inside custom shaders, inducing the classic retro step-gradient "color banding" over rasterized surfaces.
* **True CLUT (Color LookUp Table) Emulation:** Emulates original indexed 4-bit and 8-bit texture rendering using precise pixel fetches (usampler2D and texelFetch) against a dynamic texture palette atlas.

### 3. Modern Low-Level GPU Pipeline (OpenGL 4.5+ DSA)

* **Direct State Access (DSA):** Leverages modern API features (glCreateBuffers, glNamedBufferStorage) to feed data directly into GPU VRAM buffers, entirely bypassing the legacy global state binding anti-pattern (*Bind-to-Edit*).
* **Immutable Buffer Allocation:** Employs immutable storage mechanics for optimal CPU-to-GPU data streaming during runtime frame aggregation.

### 📁 Project Architecture

The codebase is meticulously decoupled into specialized, independent modules: 

* /gte - Fixed-point 3D vertex projection, camera-space calculations, and hardware-style near clipping.
* /gpu - Modern OpenGL state management, allocation of vertex storage arrays, and streaming.
* /math3d - Custom C89 fixed-point 3D math library (matrix manipulation, vector translations, trigonometric approximations).
* /renderer - Framebuffer allocation, view scaling management, and frame presentation.
* /platform - Window creation and user input processing wrapper utilizing SDL2.

### 📌 Project Status & Compromises

This is a **purely hobbyist and passion-driven project** built to explore legacy graphics programming and hardware constraints. As such, the codebase is **under continuous, active development** and should be considered a work in progress. 

To achieve a balance between authentic retro hardware behavior and modern operating system/GPU architectures, several **architectural compromises** had to be made: 

* **Geometry Submission & Batching (Rejected Linked Lists vs. Monolithic Stream):** The original PSX GPU was fundamentally a 2D rasterizer with zero awareness of 3D space; geometry data was never stored in VRAM. Instead, the CPU/GTE calculated 2D screen-space projections in Main RAM and fed them packet-by-packet to the GPU using transactional DMA commands. While it was theoretically possible to replicate this by emulating the original hardware's linked lists (Ordering Tables) and submitting individual primitive draw calls, doing so on modern PC architectures would introduce a devastating driver overhead by forcing the GPU to draw each triangle separately. To optimize throughput, **this approach was explicitly rejected in favor of a high-performance Dynamic Batching pipeline**. The engine utilizes a single monolithic Vertex Array Object (VAO) and a single Vertex Buffer Object (VBO). Every frame, all transformed primitives are aggregated into a single host-side array, streamed to the GPU in a single transaction via glBufferSubData, and rendered using a **single unified draw call per frame**, after which the geometry counter is zeroed for the next cycle.
* **Primitive Topology & Layout:** The PSX hardware relied on dynamic linked lists for depth sorting and primitive submission. Modern desktop GPUs do not natively support arbitrary runtime linked-list traversal for drawing (the legacy concept of a glDrawList does not exist in modern Core Profiles). To completely bypass the performance penalties of individual primitive drawing, the engine adaptively flattens the logical ordering structures into rigid, sequential vertex arrays perfectly tailored for modern macro draw operations (glDrawArrays).
* **Frame Timing & Delta Limits:** Authentic PSX software locked its execution loop directly to hardware video display interrupts (V-Blanking at a rigid 25 FPS PAL / 30 FPS NTSC), processing application logic entirely on a per-frame basis. Because modern operating systems and high-refresh-rate displays break this hardware-interlocked behavior, the engine implements a modern high-precision system timer loop (delta-time stepping) to accurately clamp the execution delta and recreate smooth retro pacing on variable modern hardware.
* **Hybrid Rasterization Pipeline:** While vertex transformation, matrix mathematics, and texture palettization perfectly emulate standard 1990s fixed-point logic, the actual rasterization stage is handled via Modern OpenGL (4.5+ DSA) rather than a pure, pixel-by-pixel software rasterizer. This keeps the engine performant on modern computer architectures while maintaining the classic PSX visual flaws.
* **Math Precision Limits:** Implementing a strict Q3.12 fixed-point math engine in C89 means dealing with intentional integer overflows, saturation clamping, and limited trigonometric resolution, which may occasionally cause aggressive jittering or clipping artifacts.

### 🖥️ Quick Start (Linux / Fedora)

Ensure you have SDL2, GLEW, PNG-LIB, and make installed on your system. 

bash

# Clone the repository
git clone https://github.com/programistac/psir.git

cd psir

# Build the project using the provided Makefile
make

# Run the executable
./psx_demo

# Optional: Clean up built objects and executables
make clean

### 🛠️ Upcoming Roadmap

* [x] Pure ANSI C (C89) Core with Modern OpenGL (4.5+ DSA) pipeline.
* [x] Fixed-point 3D math and perspective vertex projection.
* [ ] Per-vertex lighting implementation (Flat/Gouraud shading emulation via fixed-point logic).
* [ ] Custom **Binary 3D Model/Map Format** (.bin parsers) designed for direct memory streaming (zero-copy).
* [ ] Programmatic **Ordering Table (OT)** sorting algorithm to manage polygon depth without a hardware Z-buffer.
* [ ] **Back-Face Culling** implementation using fixed-point cross product vector math.
* [ ] Dynamic shader loading from local file assets.

### 📄 License & Credits

This project is licensed under the **GNU General Public License v3 (GPLv3)**. 

### Additional Terms under GPLv3 Section 7b:

If you modify this Program or any material containing it, you must prominently retain the original author credits ("Credits" or "Authors" sections) within the documentation, repository homepage, and/or the graphical user interface of the derivative work. 

**Original Author:** [programistac / ansiccode@googlemail.com]
