# timg

This is a terminal image viewer.

the most basic way to implement these is to make a lookup table of characters and then index it with the greyscale pixel values of the image.
another way is to find the nearest displayable color and use ansi escape codes to set the background color to it.

this takes a bit of a different approach. the first difference is that I do Error diffusion based dithering, which helps with the limited number of colors.
I also do an amount of work to try to figure out the optimal character to use for a given part of the image instead of using a lookup table (hence the fonts and font rendering library).

overall this was a cool experiment.

# build
run
```
make
```
