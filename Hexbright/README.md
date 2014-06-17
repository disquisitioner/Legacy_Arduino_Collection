HexBright
=========

[HexBright](http://hexbright.com) is the world's first hackable, open source flashlight. It combines solid construction, a powerful 500 lumen LED, rechargable lithium-ion battery, and (best of all) an Arduino that you can program yourself to create whatever operating modes or behaviors you like.  All the standard factory-installed code is available for modification, and there's quite a bit of community-contributed software as well.

The official HexBright repository is at [https://github.com/hexbright](https://github.com/hexbright).

##Content##

**My Variation**

I was pretty happy with the standard control program, which primarily allows cycling from off through low, medium, and high power modes and then back to off via quick clicks of the main switch.  I wanted to make it easier to turn the light off from any mode, so modified the control program so that a long press from any mode (low, medium, or high) would also turn the light off.  You'll find my program here as `hexbright_djb`.

**Community Code**

I've grabbed a few of community versions for reference and probably future modifications.  You'll find them here as `hexbright_factory` (the version that comes pre-installed on HexBright) and `hexbright4` (which uses the built-in accelerometer to enable interesting additional control modes). Newer versions might be available on the official repository, so if you're interested in doing anything with these you should check there first.

