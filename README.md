# ShadedPath12
DirectX 12 Framework and Applications

For the time being, this repo will be used to port my DX11/Oculus framework to DX12. Until it has at least limited public use the version will stay on 0.0.

Most development is done in C++ and uses VisualStudio 2015 Community with Win 10 SDK.
Target Platform is Windows 10 (64 bit only).
Some tools are Java based and use eclipse 4. This is mostly done for tools used in asset handling.

# Versions
* 0.0.2 - added more examples and shaders: draw crosses and 3D text
* 0.0.1 - line rendering for Oculus Rift
* 0.0 - port / test / implement DX12 features. Expect drastic changes at any time. Will have very limited use to look at and may not even compile.

# Installation
clone https://github.com/ClemensX/ShadedPath12.git to local folder, e.g. F:\dev\dx12test
you will now have a sub folder ShadedPath12, this is where all my code is located.

You need to copy / extract the Oculus SDK 8.0 to the same parent folder, so that you have the OculusSDK folder directly below dx12test, next to ShadedPath12
Now start the solution file F:\dev\dx12test\ShadedPath12\ShadedPath12\ShadedPath12.sln

If you now compile/run the sample will run only in a window. To render to the Rift, go to the configuration properties/Debugging/Command Arguments and add -vr

Running Sample1 now should give you lots of lines to see inside the rift. 

# Command Line Switches
* -app=Name Select which app to start (see list below).
* -w=100 Set window width to 100 pixel
* -h=80  Set window height to 809 pixel
* -vr Enable VR rendering to Oculus Rift (runtime version 8+)   
* -warp Use Microsoft Warp SW driver instead of GPU vendor driver
* -disableDX11Debug Used on systems that don't have DX11 Debug enabled. (Rarely used.)

# Sample Apps
* TestDotcross Draw an increasing number of crosses. Single Thread Example that will show system degredation for generating and transmitting large amounts of objects to the GPU
* TestLinetext - Optimized Multi Thread example of a geometry shader for drawing 3D text. Displays FPS, some engine info and 100 lines of text. Should render with more than 300 FPS on any system supporting the Oculus Rift (in window mode)
* Sample1 Draw a lot of lines to mark the floor and ceiling of the world, some lines of text and a coordinate system at the origin point.

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