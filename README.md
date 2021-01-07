# Timeborne
3d real-time sci-fi strategy game

![GitHub Logo](/Documents/Milestones/git_image.png)
## Setup
* Set up the framework in Internal/Framework as described on https://github.com/TamasSzep/Framework2 .
* Unpack the following ZIPs with Total Commander:
	* External.zip -> .
	* Resources_Levels.zip -> Resources
	* (Patches.zip -> .)
* Also unpack the fonts from framework: Internal/Framework/Resources_Fonts.zip -> <Timeborne_Root>/Resources
* Build Project/Timeborne.sln with Visual Studio 2019.
	* The framework does NOT have to be built when building Timeborne, since all required projects are included in Timeborne.sln.
## FAQ
* The DirectX11 graphics device could not be created in debug configuration: try removing D3D11_CREATE_DEVICE_DEBUG when creating the graphics device, e.g. by commenting it out in CreateGraphicsDevice(...).
