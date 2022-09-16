
# Arduino Keyboard

I found the model online and I need to find the link.

https://www.thingiverse.com/thing:2923851

Uses the PCF8575 library.
Also, may have a dependency on the Teensyduino library for the Keyboard object.

In this project I began learning about I2C
Learned how to solder a keyboard matrix

I spent a fair amount of time trying to find were an issue was between the two halfs only to find out that there was a short at 
the jack that I could only find after desoldering everything.

TODO:

-Move the ctrl key somewhere else
-Add a shift to the left half
-Add function keys
-Add hold/press interpretation
-Change interpretation for mod keys to allow changes to them on different layers
-Clean up the code generally

Strech goal of n-key rollover


