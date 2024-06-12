
# Minimalistic E-Paper 3.52inch Display NTP-RTC Clock

## Hardware
1. Xiao ESP32 C3
2. Waveshare 3.52inch BiColor 360x240 E-paper display refresh time 1.5 secs.
3. BH1750 (For light sensing)
4. DS3231 (For time keeping)
5. LiFePO4 6000mAh 3.2V Battery
6. TP5000 2 in 1 charging module
7. 1 BMS (2.5V Low Cutoff)
8. USB C breakout or any receiving board
9. Other stuffs like wire, capacitor (104), resistor (1M Ohm), connectors, switch, LED (1) etc.

## FEATURES V1.0.0 (New Features Coming Soon (Might take a year or so))
1. Minimalistic design
2. Big Icons
3. Shows battery percentage and voltage
4. Updates after every 60 secs and sleeps for the rest.
5. Read below
6. Mammoth 6000mAh battery! (Your personal choice though)
   
## UPCOMING FEATURES
1. Will update schematics.
2. Web based configurations or Settings.
3. Dark Or Light modes.
4. Way to remove battery voltage display from settings <br>
~~5. Battery icon~~
6. (If you can optimise it further then let me know)
   
Clock based on Waveshare 3.52inch e-Paper HAT, 360 × 240. :leaves: Eco-Friendly!

The clock runs on a 6000mAh LiFePO4 cell ( :leaves: :leaves: Eco-Friendly af!). There is an RTC for super power saving operation, and everyday at a particular time it connects to NTP and updates itself. 

Also, it houses a LUX sensor (BH1750) for sleeping while it is dark (E-paper doesn't have a backlight, remember?) ( :leaves: :leaves: :leaves: Eco-Friendly af faka fak!)

Made of old delivery card boards. :exploding_head: One-Punch Eco-Friendly Boost Ultra Pro Max :leaves: to :infinity:

Supports 5V 1A charging with options upto 2A.

All USB-C operation.

This clock does not runs on GxEPD2 Library (while making this clock, this display was not supported). This runs on OEM provided basic library.

Battery life: <br>
a. Full: 06/06/2024, Dead: (Will update once empty).

## Picture(s)

![Clock](https://github.com/KamadoTanjiro-beep/E-Paper-Display-NTP-Clock/blob/main/src/epdClock.jpg)


## License

Distribute it freely but link back to this project or put some good words or attributes or donate (paypal link in profile) haha. You are own your own, I take no resposibility, if this thing explodes or does any damage on anything.

