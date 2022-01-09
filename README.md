# bannister
a WS1218B addressable LED, rcwl-0516 microwave motion sensor and arduino nano stair lighting project

this uses the FastLED library to control the LEDs.
It's easier to use the HSV (hue, saturation, value) color space when you want a smooth transition from color to color, or want to fade brightness in and out but maintain the same basic color.

   @brief Control bannister lights

   A string of WS2812B LEDs run along a stair bannister inside an aluminium profile
   Microwave motion sensors at the top and bottom of the stairs detect motion
   A LDR reports current ambient light intensity

   During daytime, or when light levels are high, motion will trigger display of
   an animated rainbow down the length of the stairs for some time.

   During the night, motion will trigger a sequence to illuminate the stairs safely
   for a reasonable time.

