# Hardcore

Hardcore is a low-level graphics engine implemented using the Vulkan API. The project started as a way for me to learn and get experience using Vulkan and developing graphical applications, however I plan to keep developing it and even use it in some future projects. The engine is still extremely early in development, as I only work on it when I can and I still have some things to learn regarding Vulkan and graphical programming in general.

# Features

(To be expanded)

# API

(This is only a simple description of how the engine is used and some of the design choices)

There is no GUI, using the engine is similar to using a C++ library, with only a few differences.

As mentioned earlier, the engine is low-level, with the goal being giving as much control as possible to the client developer, so that optimizations may be easier to implement. This is to say, using the engine should be easier than using Vulkan directly, but nothing is assumed about client applications, other than running in real time.

[TO ADD]: # (Some utilities such as a dedicated physics thread are provided, but can be disabled.)

Resources, specifically those on the GPU, are implemented as classes, being allocated/created when the class is instantiated, and freed/destroyed when the class instance is destroyed. The goal of this design is to hopefuly make managing the resources intuitive and easy to use. Additionally, API functions deal mainly with context changes instead of things that are typically done every frame. (i.e. when doing something like drawing a mesh, as long as the resource class instances being used haven't been destroyed and the respective pipelines or tasks haven't been disabled or destroyed, then the mesh will keep getting drawn every frame, without the client calling any functions.)

# Some implementation details

- To handle window creation and mangement GLFW is used
- Vulkan calls are made using the C library
- Device memory management is handled entirely by the engine, without the use of any library
- Shader features are extracted using SPIRV-Cross

(To be expanded)

# Demos

In the demo folder of the repository are some working examples of small programs using the engine.

[TO ADD]: # (Video examples of the demos and other uses of the engine can be seen in a Youtube playlist)
