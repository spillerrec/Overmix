Overmix
=======

[![Discord](https://img.shields.io/discord/550072474855800837)](https://discord.gg/YvvfDbT)
[![GitHub license](https://img.shields.io/badge/license-GPLv3-blue.svg?style=flat-square)](https://www.gnu.org/licenses/gpl-3.0.txt)
[![GitHub release](https://img.shields.io/badge/release-v0.3.0-blue.svg?style=flat-square)](https://github.com/spillerrec/Overmix/releases/tag/v0.3.0)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/7062/badge.svg)](https://scan.coverity.com/projects/spillerrec-overmix)
[![CircleCI](https://circleci.com/gh/spillerrec/Overmix.svg?style=svg)](https://circleci.com/gh/spillerrec/Overmix)

Overmix can stitch fractions of smaller images together to create the original full image. It is specifically made for stitching anime screenshots, where a small portion of a scene is shown and the viewpoint slides to show the remaining area.

The idea behind Overmix is to increase the amount of images which is used to stitch it together, and use this to solve MPEG compression, color banding and on-screen text/logo issues.
Development is now geared towards understanding the more theoretical parts about Image Reconstruction and how this can be applied to increase quality even further.

### Features

- High quality 16-bit (or more) rendering, with dithering for 8-bit output
- Automatic detection of vertical and horizontal offsets, with sub-pixel precision
- Interlacing support
- Multi-threaded to fully exploit your computer's potential
- Support for 10-bit YUV input
- Rendering pipeline supports chroma sub-sampling without upscaling
- Deconvolution for sharpening images
- Logo/credits detection and removal
- Steam minimization (colors kinda broken right now though...)
- Detection of cyclic animations
- Basic Super resolution, GUI kinda lacking right now

### Current work

- Separation of foreground and background in slides where foreground and background moves with different speeds
- MPEG2 decoder for minimizing MPEG2 compression artefacts, which should help especially with motion compensation.
- Revamp GUI so it is more easy to add advanced settings for operations
- Command line interface

### Future work

- Detection of zooming and rotation
- Figuring out how features such as animation detection and separation of fore/back-ground can be combined

### How to contribute

Even if you know nothing about programming, there are several ways to contribute:

- If it currently does not solve all your needs, make a feature request on the [issue tracker](https://github.com/spillerrec/Overmix/issues), or comment on an existing one.
- If you fail to stitch an image properly, create an issue and share a link to the input images. Either I can help find you the right settings, or identify a current limitation of Overmix.
- If you can't figure out how to use some part of the program even after checking the wiki (or even it was just difficult), create an issue/bug report. This mean that either the interface is not intuitive enough, documentation is lacking, or the documentation is not clear enough.

If you do not want to create a Github user, feel free to send me an email at spillerrec@gmail.com about anything.

### Avoid using screenshots

Video is not as RGB directly, instead it uses a color transformation to seperate luminance and color information. The color information is then downsampled as it usually isn't noticable. Depending on your video player, it might use lower quality approximations to improve performance/batery life.
VLC in particular causes bad results, but mpv can also cause screenshots to be in lower quality than what you see on the screen. By using the built-in video importer you will get the full qualty (in 10bit for those videos as well), while also getting every frame which will further improve quality.

### The Dump format

In order to dump Hi10p video frames without them being converted to 8-bit RGB, the dump format was developed. This format supports up to 16-bit YUV images with chroma sub-sampling. Now only needed if you want to store the dumped frames.
Several tools related to the format have been developed, most importantly an application to easily extract every unique frame in a video sequence. This and other tools such as Windows extensions can be found in the following repository: https://github.com/spillerrec/dump-tools

### Known issues

- Dehumidifier renderer does not "dehumidify" colors. (Forcing RGB mode with the new option might do the trick, but I haven't checked the code.)
- Progress bars are not implemented everywhere, and the "Cancel" option is rarely implemented even though the button appears.

### Building

*Dependencies*
- Qt5
- C++14 (generic lambdas, `std::make_unique<>`)
- cmake (for compiling)
- ffmpeg
- zlib
- lzma
- libpng
- png++ (header only)
- libjpeg
- fftw3
- pugixml
- lcms2 (required for GUI)
- boost
- eigen3
- [QCustomPlot](http://www.qcustomplot.com/) 2.x (required for GUI)
- google/benchmark (required for unit-benchmarking)

Optional:
- waifu2x-converter-cpp (with opencv enabled)

Linux only:
- xcb
- qt5x11extras

*Building*

1. `cmake -DCMAKE_BUILD_TYPE=release`
2. `make`

It is recormended to build in a seperate folder, as cmake polutes the all the directories otherwise. You can do it like this:

1. `mkdir release`
2. `cd release`
3. `cmake ../ -DCMAKE_BUILD_TYPE=release`
4. `make`
