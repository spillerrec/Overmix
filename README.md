Overmix
=======

Overmix can stitch fractions of smaller images together to create the original full image. It is specifically made for stitching anime screenshots, where a small portion of a scene is shown and the viewpoint slides to show the remaining area.

The idea behind Overmix is to increase the amount of images which is used to stitch it together, and use this to solve MPEG compression, color banding and on-screen text/logo issues.
Development is now geared towards understanding the more theoretical parts about Image Reconstruction and how this can be applied to increase quality even further.

### Features

- High quality 16-bit (or more) rendering, with dithering for 8-bit output
- Automatic detection of vertical and horizontal offsets, with sub-pixel precision
- Interlacing support
- Multi-threaded to fully exploit your computer's potential
- Support for 10-bit YUV input through the dump format (more below)
- Rendering pipeline supports chroma sub-sampling without upscaling
- Deconvolution for sharpening images
- Logo/credits detection and removal
- Steam minimization (colors kinda broken right now though...)
- Detection of cyclic animations
- Basic Super resolution, GUI kinda lacking right now

### Current work

- Separation of foreground and background in slides where foreground and background moves with different speeds
- Minimization of JPEG compression, as an intro to MPEG with motion compensation
- Revamp GUI so it is more easy to add advanced settings for operations
- Maybe including the de-mosaic censor technique in the GUI

### Future work

- Detection of zooming and rotation
- Figuring out how features such as animation detection and separation of fore/back-ground can be combined
- Support saving as 16-bit PNG (loading is supported)

### The Dump format

In order to get Hi10p video frames without them being converted to 8-bit RGB, the dump format was developed. This format supports up to 16-bit YUV images with chroma sub-sampling.

While Overmix supports ordinary PNG screenshots, using dump files is the recommended way of using Overmix with video sources. This is because most media players focus on quick rendering, and image quality tends to suffer. Secondly there are some theoretical implications which might have more importance once further work have been done on Overmix.
(Update: there is now a de-VLC option to improve the quality of screenshots taken from VLC.)

Several tools related to the format have been developed, most importantly an application to easily extract every unique frame in a video sequence. This and other tools such as Windows extensions can be found in the following repository: https://github.com/spillerrec/dump-tools

### Known issues

- Dehumidifier renderer does not "dehumidify" colors. (Forcing RGB mode with the new option might do the trick, but I haven't checked the code.)
- Progress bars are not implemented everywhere, and the "Cancel" option is rarely implemented even though the button appears.

### Building

*Dependencies*
- Qt5
- C++11
- libpng
- zlib
- lzma
- lcms2
- fftw3

*Building*

1. qmake
2. make