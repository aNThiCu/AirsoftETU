Latest board is very promising , i have semi-abandoned the idea of separating the controls and power components from the sensors. The board is now a 4 layer consisting of GND-3.3V-GND-GND copper pours.
Pcb manufacturing at home sadly cannot fullfill my needs for this project , future versions will not be able to be manufactured in house as i have finished some of the testing needed for a complete product.

This revision still is in no way usable , as i haven't been able to test any of the new software implementations. I just finished the firmware and i am about to go on with assembling the rest of the PCB.
I've been using a board with no power electronics until now for testing the signal complementarity and overall logic of the system.

Hardware changes made from the last readme update:

 - added back the high side pmos , board cannot function without it as freewheel diode would blow
 - added back the mode selector phototransistors and led-added a schmitt trigger for hardware debounce
 - added proper gate drivers , MCP1416T-E
 - added some ESD protection diodes on the gates of the power FETS and 12v power rail
 - moved leds to the 3.3v rail
 - modified shape of PCB for better fitment

Software changes made from the last readme update:
   - configurable duty cycle, not tested actual usable range , over 90% is considered 100% due to dead_time limitations
   - configurable dead time ticks for pwm , 1 tick = 200ns , good for different types of power mosfets
   - implemented synchronous rectification to take the burden off the flywheel diode 
   - changed driving logic to permit both PWM and on/off
   - added burst mode capability
   - changed to platformio

What's coming next:
   - finishing testing with new pcb and firmware
   - troubleshooting indicators, buzzer for quick configs in the upcoming settings tab
   - folder with everything needed for direct manufacturing from a factory
   - might do a different type of trigger , either optical of magnetic , for longevity sake , i like the clicking feel of microswitches more though
   
What's been marked done from last readme update:
   - configurable mode selector switch with multiple modes
   - redo of shape for better fitment in V2 gearboxes
