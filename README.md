# PSP-EV3-Remote
Okay so this is a homebrew program that runs on the PSP that records joystick data, calculates how much speed to assign to each wheel and sends it to a EV3 running EV3Dev via a script that relays the data to the EV3

#Relay
So there's a relay that's being written in Python that relays the calculated speeds to the EV3. 
The reason for the need for a relay is because the PSP only has WiFi whereas the EV3 only has Bluetooth. Though you can plug in a usb WiFi adapter to the EV3, we didn't have one on hand and it'd stick out of the brick shape of the EV3
