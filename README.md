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

### Current work

- Detection of cyclic animations
- Separation of foreground and background in slides where foreground and background moves with different speeds
- Super resolution

### Future work

- Detection of zooming and rotation
- Figuring out how features such as animation detection and separation of fore/back-ground can be combined
- Support 16-bit PNG for input and output
- The image viewer component is taken from another project, but I need to be merge the updates to fix controls, color management, etc.
- Have an output format to save the output of alignment, etc.

### The Dump format

In order to get Hi10p video frames without them being converted to 8-bit RGB, the dump format was developed. This format supports up to 16-bit YUV images with chroma sub-sampling.

While Overmix supports ordinary PNG screenshots, this using dump files the recommended way of using Overmix with video sources. This is because most media players focus on quick rendering, and image quality tends to suffer. Secondly there are some theoretical implications which might have more importance once further work have been done on Overmix.

Several tools related to the format have been developed, most importantly an application to easily extract every unique frame in a video sequence. This and other tools such as Windows extensions can be found in the following repository: https://github.com/spillerrec/dump-tools

### Known issues

- Chroma up-sampling currently required when using the "Average" renderer
- Dehumidifier renderer does not "dehumidify" colors
- Drop-down box to select scaling algorithm does not work
- Downscaling is not implemented correctly

### Building

*Dependencies*
- Qt5
- C++11
- libpng
- zlib

*Building*

1. qmake
2. make