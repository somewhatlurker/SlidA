## SlidA

Code based on [Thinithm](https://github.com/somewhatlurker/thinithm) for Diva Arcade FT only sliders.

* Code quality is non-existent
* Only has minimal testing

Hardware is based on ATmega32u4 Arduinos -- (Pro) Micro or Leonardo. (may work on other boards, but I don't know)  
Also requires 3\*MPR121 and (optionally) 32\*WS2812 compatible LEDs (60/meter premade strip should work well).  
Button input is handled through keyboard buttons, but using an separate button encoder is recommended.

Cheap generic MPR121 modules can be used off a <3.3V supply only, so I suggest to pair them with a 3.3V Pro Micro.  
Adafruit MPR121 modules work off 5V and can be used with any compatible board.

Make sure you set the address of your second and third MPR121s:  
1. (For generic modules only) Cut between the ADD pads on the rear side.
2. Solder a jumper from ADD to 3V for the second MPR121, and SDA for the third.

Requires [QuickMpr121](https://github.com/somewhatlurker/QuickMpr121) and FastLED.

Set pins in pins.h or use the defaults (based around Pro Micro):  
|    Function    |    Pin    |
|    --------    |    ---    |
|   MPR121 SDA   |    SDA    |
|   MPR121 SCL   |    SCL    |
|   MPR121 IRQ   |     4     |
|   Keyboard W   |     5     |
|   Keyboard A   |     6     |
|   Keyboard S   |     7     |
|   Keyboard D   |     8     |
|Keyboard Return |     9     |
|Slider LED Data |    10     |

Slider sensors should be 32 conductive rectangles (eg. copper tape or PCB fills) against the rear side of ~3mm acrylic.  
They are connected sequentially to the MPR121s from left to right.

For now, MIT license (subject to change for future versions)