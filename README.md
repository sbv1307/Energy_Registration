# Energy registration

This Arduino project will monitor the open collector output on a number of Carlo Gavazzi energy meters Type EM23 DIN and/or Type EM111.
The pulses will be counted and turned into number of kilo watt hours (kWh).

Upon request, the kWh counted will be pushed to a Google Sheet through a webhook (A web site which basically "translate" HPPT to HTTPS).

Each meter monitored, will be connected to a digital imput, as well as the interrupt pin.


Notes:
The sketch monitors interrupt pin 2 for a FALLING puls and then reads the defined channelPins and counts pulses for each pin.
The interruptfunction has a catch: It enters a "while pin 2 is low" loop, and stays there, until the interrupt is released.
This is against all recomendations, saying Interruptfunctions should be very short. 

However, in order to handle the problem with overlapping pulses from more meters, the interruptfunction will active .
Example: tree meters generate tree pulses as shown below:
          _______        _______
Meter #1:        |______|
             _______        _______
Meter #2:           |______|
                _______        _______
Meter #3:              |______|
Eventhoug Meter #1 triggers the puls count, pulses will be registrated as olng as one of the Meters generate a pulse and hold the Interrupt active.


