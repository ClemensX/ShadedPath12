# ShadedPath12
DirectX 12 Framework and Applications

**Update:** Now supporting Oculus SDK 1.10 with DirectX 12 render path.

# General Info

This framework is free to use for everybody! If you just run a precompiled example or copy the entire source code into your own project is 
completely up to you.

**This is for the C++ developers!** There are no plans to provide any other access layer to the engine framework. Writing an application requires you to subclass the application base class.

**Who is it for** The ideal consumer of this work is a C++ programmer wanting to write applications for the Oculus Rift. There are not too many examples out there of how to write DX12 code for the Rift. 
You could use this framework as a starting point. Maybe throw away 90 % of the code and do your own thing. Or you might be interested in a single feature and copy only that into your own projects.
If you just want to write applications and are not interested in game engine code please consider that this framework is still in it's early stages. While certainly you can alter the existing demo applications, 
even add new one, the more you deviate from the existing demos the higher your chances are to run into issues. 
It is my goal to have a framework you can use to write great Rift applications without needing to look into engine code. But I am simply not there yet.

Most development is done in C++ and uses VisualStudio 2015 Community with Win 10 SDK.
Target Platform is Windows 10 (64 bit only).
Some tools are Java based and use eclipse 4. This is mostly done for tools used in asset handling.

# Legal
I put this whole thing into Public Domain, with only slight exceptions. Meaning you can use almost everything in this project in any way you like. I took great care to provide code that is free of any copyrights from somebody else.

**Exceptions from Public Domain**

* The code I use to read texture files is from Microsoft. It is a version of the DDSTextureLoader that is used in many examples provided by Microsoft.
That is under the MIT License which gives very widely usage rights.
* I provide textures in pak file format. There are tools and code to put texture files into pak format, but not to extract them. This is by intention because I do not own all 
the rights of the texture files. This means that you can use the textures I provide in your own projects, but you are not allowed to extract single textures from the pak file, 
and sell them or provide as single texture files anywhere.
* you are not allowed to restrict anyone elses rights to this code. Meaning you are allowed to rebundle everything here and sell it in parts or in its entirety, but you are not allowed to come back here and try
to restrict others (and myself) from any usage. In other words: You cannot copyright what I have placed in public domain.

# Versions
* 0.1.2 - pak files, multi-thread rendering optimizations
* 0.1.1 - mass rendering / mutli-thread support
* 0.1.0 - lighting and sound
* 0.0.5 - animation and collada import
* 0.0.3 - added DDS texture support
* 0.0.2 - added more examples and shaders: draw crosses and 3D text
* 0.0.1 - line rendering for Oculus Rift
* 0.0 - port / test / implement DX12 features. Expect drastic changes at any time. Will have very limited use to look at and may not even compile.

# Installation
clone or download the master branch from https://github.com/ClemensX/ShadedPath12.git to local folder, e.g. F:\dev\dx12test
you will now have a sub folder ShadedPath12, this is where all my code is located. Download the data.zip file from the latest release on GitHub and extract to that folder. 
F:\dev\dx12test\ShadedPath now has these subfolders: data, tools and (another) ShadedPath12.

You need to copy / extract the Oculus SDK 1.10.1 to the same parent folder, so that you have the OculusSDK folder directly below dx12test, next to ShadedPath12.
There are some batch files to directly start some of the more intersting demos in folder ShadedPath/tools.
To use in Visual Studio C++ start the solution file F:\dev\dx12test\ShadedPath12\ShadedPath12\ShadedPath12.sln

See below for command line switches. Use them in VC++ via right click on project, choose Properties then: Configuration Properties/Debugging/Command Arguments. Here you can use all the switches described.

# Command Line Switches
* **-app=Name** Select which app to start (see list below).
* **-w=100** Set window width to 100 pixel
* **-h=80**  Set window height to 80 pixel
* **-vr** Enable VR rendering to Oculus Rift (runtime version 8+)   
* **-warp** Use Microsoft Warp SW driver instead of GPU vendor driver
* **-disableDX11Debug** Used on systems that don't have DX11 Debug enabled. (Rarely used.)

# Sample Apps
* **Logo**         The ShadedPath Logo as 3D text with some flying lights
* **HangOn**       Uses mass rendering API to draw lots of small objects
* **Soundtest**    Show use of background music and directional sound of 3D objects
* **ObjectViewer** Display one of the predefined objects with animation and lighting
* **TestTextures** Load 12 texture files and display each at 83.000 different world positions (a total of 1 Million billboards)
* **TestDotcross** Draw an increasing number of crosses. Single Thread Example that will show system degredation for generating and transmitting large amounts of objects to the GPU
* **TestLinetext** Optimized Multi Thread example of a geometry shader for drawing 3D text. Displays FPS, some engine info and 1000 lines of text. Should render with more than 300 FPS on any system supporting the Oculus Rift (in window mode)
* **Sample1** Draw a lot of lines to mark the floor and ceiling of the world, some lines of text and a coordinate system at the origin point.

# Features for 0.1.2

* Pak files: For legal reasons I could not provide all the texture files for the demo apps in earlier releases. But inside a custom format pak file this is no longer a problem. All texture files except for the default texture
are now loaded from the pak file. The default texture is used when a texture is not found. When you use your own textures you can either put them in a pak file yourself or just put the .dds files 
inside the texture folder. To put dds files in a pak file I have provided a Java project under tools/texture

* Optimizations for multi-thread rendering: The rendering code for objects has been changed a lot to remove most of the mutexes needed in earlier versions. 

# Features for 0.1.1
* Mass rendering: As a first step to support rendering of lots of small objects, thread support and a different approach to rendering objects was implemented. See classes WorldObjectEffect and WorldObject.
While currently some thousand objects can be rendered ok I am still not exactly where I want to be. Most of the performance gain by using multiple threads is eaten up because some classes proved to be not thread save and I had to protect critical sections with mutexes.
This is mainly for the camera class and some deep down rendering code. Also, I am not too satisfied with the current architecure for mass rendering. Expect more rewrites in the future to fix these shortcomings.

See HangOn.cpp for example mass rendering. A group of objects that share the same mesh are rendered by splitting the draw calls to any number of threads. All thread creation, maintenance and synchronization has been done with standard C++11 features.

# Features for 0.1.0
* Lighting: Ambient, Directional and Point Lights. See ObjectViewer.cpp
* Sound: PLay background music and associate WorldObjects with sound. Sound for objects has calculated volume drop based on distance and positional orientation through the available speakers. All sounds must be in .wav format

# Features for 0.0.5
* Mesh creation: See section below
* Changed billboard shader so that they face camera at each point in time, display 1.000.000 billboards
* Animation: objects can be animated: Move object according to keyframe animation and/or render object with bone animation. Bone Animation is purely CPU bound, that means that for each frame all vertices need to be recomputed by the CPU and transferred to the GPU.
* Lighting: ambient and directional lighting is in, see ObjectViewer for usage, use F1-F2 to change ambient lighting level when ObjectViewer runs
* added another sample app: ObjectViewer. Used to display one of these objects:
* **shaded2.b** big 3D text, stationary
* **house4_anim.b** very simple house that moves along a predefined path
* **joint5_anim.b** simple bone animated object, basically a bar with one joint in the middle
* **worm5.b** animated worm, a more complex mesh and bone structure (8 bones, more than 10.000 vertices)

# Features for 0.0.3
Added a texture shader to load DDS files and push them to GPU. This shader uses the DX12 DDSTextureLoader from Microsofts MiniEngine. DDSTextureLoader was
slightly changed to make it easier to use outside MiniEngine.

Added a billboard shader that uses pre-loaded textures and displays them at user defined position and size

# Features for 0.0.2
Added two geometry shaders:
* Dotcross: Very simple geometry shader to draw crosses consisting of two lines at any world position
* Linetext: Copy text to the GPU where a geometry shader draws each letter out of simple lines. Intended as a diagnostics tool, e.g. for displaying FPS or other
info text directly in the 3d world


# Features for 0.0.1
This is no end-user engine! The code is provided as a playground to test DirectX 12 and/or Rift coding.
Let the sample run. Look at the code. Use anything you find useful in your own projects.
* Shader Code: Only very simple line drawing currently :blush: This is because I want to gather experience with DX12 before I invest a lot of time in shaders that I would have to rewrite with every major engine change
More will definitely come.
* Engine / Sample separation. Look at Sample1.cpp to see what you can currently do with this engine and see how it is done.
* Oculus Rift support (head tracking and rendering). See vr.cpp
* Post Effect Shader: Copy rendered frame to texture - Rift support is built on top of this feature
* Use Threads to update GPU data. See LinesEffect::update()
* Synchronize GPU and CPU via Fences

# Mesh creation
Starting with version 0.0.5 the Java based Collada importer is provided. Meshes are created like this:

* Use blender 2.7 and export as collada.
* After installing JDK 8 compile the Java class 'ColladaImport' from folder tools\collada like so:
* javac -d bin src\de\fehrprice\collada\ColladaImport.java
* Use the provided **make.bat** to import all example collada files to custom .b and place the binary files in the data folder. Here they should be found at runtime