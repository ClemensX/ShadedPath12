# ShadedPath12
DirectX 12 Framework and Applications

For the time being, this repo will be used to port my DX11/Oculus framework to DX12. Until it has at least limited public use the version will stay on 0.0.

Most development is done in C++ and uses VisualStudio 2015 Community with Win 10 SDK.
Target Platform is Windows 10 (64 bit only).
Some tools are Java based and use eclipse 4. This is mostly done for tools used in asset handling.

# Versions
0.0.1 - line rendering for Oculus Rift
0.0 - port / test / implement DX12 features. Expect drastic changes at any time. Will have very limited use to look at and may not even compile.

# Installation
clone https://github.com/ClemensX/ShadedPath12.git to local folder, e.g. F:\dev\dx12test
you will now have a sub folder ShadedPath12, this is where all my code is located.

You need to copy / extract the Oculus SDK 8.0 to the same parent folder, so that you have the OculusSDK folder directly below dx12test, next to ShadedPath12
Now start the solution file F:\dev\dx12test\ShadedPath12\ShadedPath12\ShadedPath12.sln

If you now compile/run the sample will run only in a window. To render to the Rift, go to the configuration properties/Debugging/Command Arguments and add -vr

Running the sample now should give you lots of lines to see inside the rift. 

