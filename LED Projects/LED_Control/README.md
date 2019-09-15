LED Control
===============

LED Control is a utility library designed to make it easy to  generate patterns and animations on LED strips, making use of the excellent FastLED library (https://fastled.io) to handle low level command and control of a wide variety of LEDs.
 
All patterns and animations are based on a simple state machine where patterns are assigned to all attached LED strips and a custom update function is called each clock cycle to carry out the desired animation.
