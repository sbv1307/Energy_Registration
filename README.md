# **Energy registration**
###### **Version 2.0.0**

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


 Version history:
  * 2.0.0 - Arduino SW project now merged with KiCad Hardware, and this version surpports the new HW design
 *         1 - LED_BUILTIN on Pin #9 has stopped woring. Pin A3 will be used instead. LED_PIN changed to LED_PIN - 
 *             HOWEVER!!! It has turned out, that it was set incrrrectly by the entrepreter. Hovering over the LED_BULTIN variable indicate it expands to 13 and not 9.
 *             Setting LED_PIN to 9, makes the LED light.
 *             Since the Hardware prototype, was made to use PIN 17, Pin 17 will be used for LED_PIN in this version.
 *         2 - Channels extenced to 8 instead of 7. Noe includeing Pin A2 
 * 0.2.3 - BUILTIN_LED has stopped working - NEED INVESTIGATION AND CODE IMPLEMTATION
 * 0.2.2 - Post powerup data to google sheets (data, and the comment (Power Up))
 * 0.2.1 - Cleaning up entries used for verifying pulscounts - No functional changes.
 * 0.2.0 - Meters, which only gives 100 pulses pr. kWh, were registered as if they gave a thousenth. Might be an issue by adding "1.000 / (double)PPKW[ii]" (0.01) to the previous counts.
 *         Since 1.000 / "(double)PPKW[ii]" might now be exactly 0.01 but maybe 0.009nnnnnnnnn, which could sum up the deffrence.
 *         SO - This version 0.2.0 will count pulses (integers). 
 *         Notes to be taken here - Mixing the types long and int, can give strange results (sometimes).
 *          - Implement: Remove SD update after each pulsecount, marked: "// V0.1.4_change (1)" in ver. 0.1.4
 *          - Implement: Flashing LED_BUILTI handled by calls to millis(), marked : "// V0.1.4_change (3)" in ver. 0.1.4.
 *          - SD Updates are done for every 10 pulses or every 30 minutes (only if pulses have been registred).
 * 0.1.4 - In an attempt to investigate lost registrations, writing every update to SD will be changed (changes marked // V0.1.4_change):
 *         1 - Remove SD update after each pulsecount marked: // V0.1.4_change (1)
 *         2 - SD is updated after each call to HTTP GET meterValue request (http://<Arduino IP address>/meterValue) marked: // V0.1.4_change (2)
 *         3 - Flashing the LED_BUILTI has handeled by the time it took to write to SD. Flashing LED_BUILTI will be handled by calls to millis() marked : // V0.1.4_change (3)
 * 
 * 0.1.3 - Due to capacity limitations in version 0.1.2, this version build upon version 0.1.1
 *       - This version is to trace why pulse registrations are lost.
 *       - Screesed in the HTML <form> element form Version 0.2.1 - Makes energymeter settings much easier
 *       - Dividing entered metervalues by 100, to avodi having to enter dicimal point (a dot) when enter metervalues
 *       - Corrected BUGs in getQuery(EthernetClient localWebClient)
 *       - Error dinvestigations marked: TO_BE_REMOVED
 *       - Added further #ifdef sections.
 *       - Inserted a function call to setMeterDataDefaults in function getQuery. If Metervalue for Meter 1 is nigative, metervalues are reset
 * 0.1.2 - N O T E !!!! - Changes exceeded limit for: Low memory available, stability problems may occur.
 *         Change in HTML presentation of energy meter values. Describing text. e.g. "Værksted" added to the informaiotn: "Meter <n>".
 *       - Introduces IP configuration for Arduino Ethernet shield attached to Arduino UNO. Define ARDUINO_UNO_DEBUG
 * 0.1.1 - Web server and web client funktionality added.
 * 0.1.0 - Initial commit - This versino is a merger of two lab tests: "EnergyRegistration" and "LocalWebHook-with-server-for-Arduino".
