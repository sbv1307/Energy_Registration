# Energy registration

This Arduino project will monitor the open collector output on a number of Carlo Gavazzi energy meters Type EM23 DIN and/or Type EM111.
The pulses will be counted and turned into number of kilo watt hours (kWh).

Upon request, the pulses counted (presented in kWh) will be pushed to a Google Sheet through a local webhook (A web site which basically "translate" HPPT to HTTPS).

Each meter monitored, will be connected to a digital imput, as well as the interrupt pin (See Emergy Registration Schematics.pdf for details).

Upon a HTTP GET pushToGoogle request (http://<Arduino IP address>/pushToGoogle), the sketch will return a HTML webpage, showing the current meter valeus, 
and initiate a HTTP GET request towards a local webHook, with the meter values, as the GET query values.

A HTTP GET meterValue request (http://<Arduino IP address>/meterValue) will return a webpage showing the current meter valeus.
 
A HTTP GET meterValue?meter<n>=<nn>, will set the counter to the provided value for the specified energy meter.
The value <n> represent the energy meter 1-7 and nn the actual energy meter value as readden on the meter, with two dicmals but entered without the decimal seperator!
(http://<Arduino IP address>/meterValue?meter1=1234567 will set the value for energy meter number one to 12345.67)


Notes:
The sketch monitors interrupt pin 2 for a FALLING puls and then reads the defined channelPins and register pulses for each pin.
The interruptfunction has a catch: It enters a "while pin 2 is low" loop, and stays there, until the interrupt is released.
This is against all recomendations, saying Interruptfunctions should be very short. 

However, in order to handle the problem with overlapping pulses from more meters, the interruptfunction will read the channels over and over,
without re-reading channels that allready has been read active (low). This way all pulses on all channels will be read.

Example: Tree meters generate tree pulses as shown below:
```bash
          _______        _______
Meter #1:        |______|
             _______        _______
Meter #2:           |______|
                _______        _______
Meter #3:              |______|

```
A pulse from meter #1 will activate the Interrupt Service Rutine (ISR), which startes the "while pin 2 is low" loop. When the pulse on meter #2 appears, while the pulse on meter #1 is still active, the ISR will stay active, when the puls on meter #1 dissapears. Then same happens when the pulse from meter #3 appears.
The "while pin 2 is low" loop read all non-read input pins, meaning: when a pin has been read as active (LOW), then pin will be read as readden, and wil not be re-read.

When the puls (the puls in meter #3) is released, the ISR ends. Then main loop will update the counter for each readden input pin, at reset the the status for the input pin.


