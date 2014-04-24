OculusCoin
==========

Example implementation for rendering Open Inventor scenes in the Oculus Rift.


Prerequisits
------------

- [Qt5](http://qt-project.org/)
- [Coin](https://bitbucket.org/Coin3D/coin/wiki/Home)
- [OVR SDK 0.3.1 preview](https://github.com/jherico/OculusSDK/tree/preview_031)


Build
-----

1. Edit the paths in `src\OculusCoin.pro`.
2. Run `qmake` to generate the makefiles.

If you build using QtCreator, you may need to

1. Add a definition for `COINDIR` to the build environment in the *build settings*.
2. Add the `bin` dir of your Coin installation to the `PATH` variable in the *run settings*.
