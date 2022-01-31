# Energy registration

This Arduino project will monitor the open collector output on a number of Carlo Gavazzi energy meters Type EM23 DIN and/or Type EM111.
The pulses will be counted and turned into number of kilo watt hours (kWh).

Upon request, the
 kWh counted will be pushed to a Google Sheet through a local webhook (A web site which basically "translate" HPPT to HTTPS).

Each meter monitored, will be connected to a digital imput, as well as the interrupt pin (See Emergy Registration Schematics for details).

Upon a HTTP GET request (http://<Arduino IP address>/pushToGoogle), the sketch will return a HTML webpage, showing the current meter valeus, 
and initiate a HTTP GET request towards a local webHook, with the meter values, as the GET query values.

A HTTP GET meterValue request (http://<Arduino IP address>/meterValue) will return a webpage showing the current meter valeus.
 
A HTTP GET meterValue?meter<n>=<n.nn>, where <n> represent the energy meter 1-7 and n.nn the actual energy meter value as readden on the meter,
will set the counter to the provided value for the specified energy meter.  (http://<Arduino IP address>/meterValue?meter1=12345.67)
 




Notes:
The sketch monitors interrupt pin 2 for a FALLING puls and then reads the defined channelPins and counts pulses for each pin.
The interruptfunction has a catch: It enters a "while pin 2 is low" loop, and stays there, until the interrupt is released.
This is against all recomendations, saying Interruptfunctions should be very short. 

However, in order to handle the problem with overlapping pulses from more meters, the interruptfunction will active .
Example: tree meters generate tree pulses as shown below:
```bash
          _______        _______
Meter #1:        |______|
             _______        _______
Meter #2:           |______|
                _______        _______
Meter #3:              |______|

```
Eventhoug Meter #1 triggers the puls count, pulses will be registrated as olng as one of the Meters generate a pulse and hold the Interrupt active.


