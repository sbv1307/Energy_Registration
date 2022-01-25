/* Verify (compile) for Arduino Ethernet
 *  
 *  
 *  
 * This sketch reads an open collector output on a number of Carlo Gavazzi energy meters Type EM23 DIN and/or Type EM111.
 * The sketch monitors interrupt pin 2 for a FALLING puls and then reads the defined channelPins and counts pulses for each pin.
 * 
 * Upon a HTTP GET request (http://<Arduino IP address>/pushToGoogle), the sketch will return a webpage, showing the current meter valeus, 
 * and initiate a HTTP GET request towards a webHook (webHookServer), with the meter values, as the GET query values. 
 * 
 * A HTTP GET meterValue request (http://<Arduino IP address>/meterValue) will return a webpage showing the current meter valeus.
 * 
 * A HTTP GET meterValue?meter<n>=<n.nn>, where <n> represent the energy meter 1-7 and n.nn the actual energy meter value as readden on the meter,
 * will set the counter to the provided value for the specified energy meter.  (http://<Arduino IP address>/meterValue?meter1=12345.67)
 * 
 * For this sketch on privat network 192.168.10.0 and Ardhino IP address 192.168.10.146 some recogniced requests are:
 * http://192.168.10.146/pushToGoogle
 * http://192.168.10.146/meterValue
 * http://192.168.10.146/meterValue?meter1=12345.67
 * 
 * The code is made specific for the Arduino Ethernet REV3 board.
 * 
 * Created:
 * 2020-11-19
 */
#define SKETCH_VERSION "Carlo Gavazzi energy meter Type EM23 and/or Type EM111 DIN - Energy registrations - V0.1.3"

/*
 * Future modifications / add-on's
 * - When HTTP request has no or incorrect abs-path / function - load explnating HTML page for corret usage
 * - Make webHookServer IP address an port number configurable.
 * Version history
 * 0.1.3 - Due to capacity limitations in version 0.1.2, this version build upon version 0.1.1
 *       - This version is to trace why pulse registrations are lost.
 *       - Screesed in the HTML <form> element form Version 0.2.1 - Makes energymeter settings much easier
 *       - Dividing entered metervalues by 100, to avodi having to enter dicimal point (a dot) when enter metervalues
 *       - Corrected BUGs in getQuery(EthernetClient localWebClient)
 *       - Error dinvestigations marked: TO_BE_REMOVED
 *       - Added further #ifdef sections.
 *       - Inserted a function call to setMeterDataDefaults in function getQuery. If Metervalue for Meter 1 is nigative, metervalues are reset
 * 0.1.1 - Web server and web client funktionality added.
 * 0.1.0 - Initial commit - This versino is a merger of two lab tests: "EnergyRegistration" and "LocalWebHook-with-server-for-Arduino".
 * 
 **** Ardhino Pin definition/layout:***
 * Pin  0: Serial RX
 * Pin  1: Serial TX
 * Pin  2: Interrupt pin for counting pulses.
 * Pin  3: Channel pin 1: Input for reading Open Collector output on Type EM111 DIN energy meter (1000 pulses per kWh (PPKW))
 * Pin  4: Chip select (CS) for SD card (Disable SD card if not used - Undefined SD card could cause problems with the Ethernet interface.)
 * Pin  5: Channel pin 2: Input for reading Open Collector output on Type EM111 DIN energy meter (1000 pulses per kWh)
 * Pin  6: Channel pin 3: Input for reading Open Collector output on Type EM111 DIN energy meter (1000 pulses per kWh)
 * Pin  7: Channel pin 4: Input for reading Open Collector output on Type EM111 DIN energy meter (1000 pulses per kWh)
 * Pin  8: Channel pin 5: Input for reading Open Collector output on Type EM111 DIN energy meter (1000 pulses per kWh)
 * Pin  9: Default defined as LED_BUILTIN. Used for indicating pulses.
 * Pin 10: Chip select (CS) for SD card.
 * Pin 11: SPI.h library for MOSI (Master In Slave Out) 
 * Pin 12: SPI.h library for MISO (Master Out Slave In)
 * Pin 13: SPI.h library for SCK (Serial Clock).
 * Pin 14 (A0): Channel pin 6: Input for reading Open Collector output on Type EM23 DIN energy meter (100 pulses per kWh)
 * Pin 15 (A1): Channel pin 7: Input for reading Open Collector output on Type EM23 DIN energy meter (100 pulses per kWh)
 * Pin 16 (A2):
 * Pin 17 (A3):
 * Pin 18 (A4):
 * Pin 19 (A5):
 */



/* 
 * The interruptfunction has a catch: It enters a "while pin 2 is low" loop, and stays there, until the interrupt is released. Se further explanation in the README.md file.
 * 
 * Values are stored on SD in order to prevent data loss in case of a poweroutage or reststart.
 * 
 */
#include <SPI.h> 
#include <SD.h> 
#include "SDAnything.h"
#include <Ethernet.h>
#include <SPI.h>


//#define DEBUG        // If defined (remove // at line beginning) - Sketch await serial input to start execution, and print basic progress status informations
//#define WEB_DEBUG    // (Require definition of  DEBUG!) If defined - print detailed informatins about web server and web client activities
//#define COUNT_DEBUG  // (Require definition of  DEBUG!)If defined - print detailed informatins about puls counting. 
/*
 * ######################################################################################################################################
 *                       C  O  N  F  I  G  U  T  A  B  L  E       D  E  F  I  N  I  T  I  O  N  S
 * ######################################################################################################################################
*/

#define NO_OF_CHANNELS 7   // Number of energy meters connected
#define NO_OF_EM23_METERS 2  // Set the number of energy meters having 100 PPKW the rest will have 1000 PPKW

const int channelPin[NO_OF_CHANNELS] = {3,5,6,7,8,14,15};  // define which pin numbers are used for input channels

#define DATA_STRUCTURE_VERSION 1    // Version number to verify if data read from file corrospond to current structure defination.
#define DATA_FILE_NAME "data.dat"  //The SD Library uses short 8.3 names for files. 

#define RANDUM_PIN A4   // TO_BE_REMOVED In order to have the sequence of values generated by random() to differ, on subsequent executions of the sketch,
                        // randomSeed() is initialize with a random number, generator with a fairly random input, from an analogRead() on RANDUM_PIN
                        // This fukntionality is only for debugginhg purpose. Search TO_BE_REMOVED ir commented out in prod version.

#define INTERRUPT_PIN 2
#define CHIP_SELECT_PIN 4
#define HTTP_PORT_NUMBER 80
#define ONBOARD_WEB_SERVER_PORT 80

/*
 * Incapsulate strings i a P(string) macro definition to handle strings in PROGram MEMory (PROGMEM) to reduce valuable memory  
  MACRO for string handling from PROGMEM
  https://todbot.com/blog/2008/06/19/how-to-do-big-strings-in-arduino/
  max 149 chars at once ...
*/
char p_buffer[150];
#define P(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer)

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x63, 0x15 };  // MAC address for Arduino Ethernet board
IPAddress ip( 192, 168, 10, 146);                     // IP address for Arduino Ethernet board 
IPAddress webHookServer( 192, 168, 10, 102);          // Local IP address for web server on QNAP-NAS-2 (192.168.10.x) NAP-NAS-2 

EthernetClient webHookClient;
EthernetServer localWebServer(ONBOARD_WEB_SERVER_PORT);

/*
 *  #####################################################################################################################
 *                       V  A  R  I  A  B  L  E      D  E  F  I  N  A  I  T  O  N  S
 *  #####################################################################################################################
 */

int PPKW[NO_OF_CHANNELS];    //Variable for holding Puls Pr Kilo Watt (PPKW) for each channel (energy meter)

/*
 * Define structure for energy meter counters
 */
struct data_t
   {
     int structureVersion;
     double kWhTotal[NO_OF_CHANNELS];    // For counting total Kilo Watt hours (KWh) on each Chanel
     double kWhPeriod[NO_OF_CHANNELS];    // For counting number of KWh betseen every e-mail update (day) on each chanel
   } meterData;

volatile boolean channelState[NO_OF_CHANNELS];  // Volatile global vareiable used in interruptfunction

/*
 * ###################################################################################################
 *                       F  U  N  C  T  I  O  N      D  E  F  I  N  I  T  I  O  N  S
 * ###################################################################################################
*/
/*
 * >>>>>>>>>>>>>>>>>   S E T     C O N F I G U R A T I O N   D E F A U L T S  <<<<<<<<<<<<<<<<<<<<<<<<
 * Configuration is "hardcoded" to minimize sketch size
 */
 void setConfigurationDefaults() {
  for (int ii = 0; ii < NO_OF_CHANNELS; ii++) {
    if ( ii < NO_OF_CHANNELS - NO_OF_EM23_METERS)
      PPKW[ii] = 1000;
    else
      PPKW[ii] = 100;
    }
}

/*
 * >>>>>>>>>>>>>>>>>   S E T     M E T E R   D A T A     D E F A U L T   <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 * 
 * Inititialize the meterData structure and store it to SD Card.
 */
void setMeterDataDefaults() {
    meterData.structureVersion = 10 * DATA_STRUCTURE_VERSION + NO_OF_CHANNELS;
    for (int ii = 0; ii < NO_OF_CHANNELS; ii++) {
      meterData.kWhTotal[ii] = 0.00;    // TO_BE_REMOVED exchange "random( 1000, 9999999) / 100" with "0.00"
      meterData.kWhPeriod[ii] = 0.00;    // TO_BE_REMOVED exchange "random( 1000, 9999999) / 100" with "0.00"
    }
  int characters = SD_reWriteAnything(DATA_FILE_NAME, meterData);
}

/*
 * >>>>>>>>>>>>>>>>   G E T     Q U E R Y    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    
 */
void getQuery(EthernetClient localWebClient) {
  int meterNumber = localWebClient.parseInt();
  if ( 1 <= meterNumber && meterNumber <= NO_OF_CHANNELS ) {
    float meterValue = localWebClient.parseFloat()  / 100.00;
// TO_BE_REMOVED : to reset metervalues during test / evaluation
if ( meterNumber == 1 && meterValue < 0.0)
  setMeterDataDefaults();
     if ( 0.0 <= meterValue && meterValue < 99999.99) {
      meterData.kWhTotal[meterNumber - 1] = (double)meterValue;
                                                              #ifdef WEB_DEBUG
                                                              Serial.print(P("\n\nMeter Number: "));
                                                              Serial.print(meterNumber);
                                                              Serial.print(P(" Value: ")); 
                                                              Serial.println(meterData.kWhTotal[meterNumber - 1]);
                                                              #endif
    }
  }
}

/*
 * >>>>>>>>>>>>>>    U P D A T E     G O O G L E     S H E E T S    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 */
void updateGoogleSheets() {

  webHookClient.stop(); //close any connection before send a new request. This will free the socket on the WiFi shield

/*  
 * if there is a successful connetion post data. Otherwise print return code  
 */
  if ( int returncode = webHookClient.connect(webHookServer, HTTP_PORT_NUMBER) == 1) {
                                                              #ifdef WEB_DEBUG
                                                              Serial.println(P("connected... Sending:"));
                                                              #endif
    
    webHookClient.print(P("GET /energyRegistrations/updateEnergyRegistrations?function=updateSheet&dataString="));
    for ( int ii = 0; ii < NO_OF_CHANNELS; ii++) {
      webHookClient.print(meterData.kWhTotal[ii]);
      if ( ii < NO_OF_CHANNELS - 1) {
        webHookClient.print(P(","));
      } 
    }  
    webHookClient.println(P(" HTTP/1.0"));
    webHookClient.println();

    while ( !webHookClient.available() && webHookClient.connected()) {
      ; // Wait for webHook Server to return data while webHook Client is connected
    }
    
    /*
     * Print response from webhook to Serial
     */
    while (webHookClient.available()) {
      char c = webHookClient.read();
                                                              #ifdef WEB_DEBUG
                                                              Serial.print(c);
                                                              #endif
    }
    
                                                              #ifdef WEB_DEBUG
                                                              Serial.println(P("\ndisconnecting..."));
                                                              #endif
    webHookClient.stop();
  } 
                                                              #ifdef WEB_DEBUG
                                                              else {
                                                                Serial.print(P("connection failed: "));
                                                                Serial.println(returncode);
                                                              }
                                                              #endif
}
/*
 * ######################  I N T E R R U P T    F U N C T I O N  -   B E G I N     ####################
 * When a LOW pulse triggers the interrupt pin, stastus on all channals are read, as long as the pulse on the interrupt pin 
 * is active (LOW) AND the channal hasn't change to LOW.
 * Explanation:
 * When more pulses for different energy meters arrives and overlapping each other , the pulse that triggers the read of Chanels 
 * will stay active LOW. When reading the channals over and over with out "resettng" the state, that way all pulses will be read.
 */

void readChannelPins() {
  while ( !digitalRead(INTERRUPT_PIN)) { 
    for (int ii = 0; ii < NO_OF_CHANNELS; ii++) {
      if (channelState[ii]) {
        channelState[ii] = digitalRead(channelPin[ii]);
      }
    }
  }
}
 
/*
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 *                       S E T U P      B e g i n
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 */
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
                                                              #ifdef DEBUG
                                                              while (!Serial) {
                                                                ;  // Wait for serial connectionsbefore proceeding
                                                              }
                                                              Serial.println(P(SKETCH_VERSION));
                                                              Serial.println(P("Hit [Enter] to start!"));
                                                              while (!Serial.available()) {
                                                                ;  // In order to prevent unattended execution, wait for [Enter].
                                                              }
                                                              #endif
  for (int ii=0; ii < NO_OF_CHANNELS; ii++) {  // Initialize input channels - and counters
    pinMode(channelPin[ii], INPUT_PULLUP);
    channelState[ii] = HIGH;
  }


  setConfigurationDefaults();  // Initialize PPKW values


/*
 * Setup SD card communication ad see if card is present and can be initialized
 */
  if ( !SD.begin(CHIP_SELECT_PIN)) {
                                                              #ifdef DEBUG
                                                              Serial.println(P("SD Card failed, or not present"));
                                                              #endif
    while (true) {  // Flashing builtin indicating error
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);  // Dont't do anyting more
    }
  }

  char charactersRead = SD_readAnything( DATA_FILE_NAME, meterData);  // Read data counters or set data counters to default if not present or incorrect structure version
  if ( meterData.structureVersion != 10 * DATA_STRUCTURE_VERSION + NO_OF_CHANNELS) 
    setMeterDataDefaults();

// TO_BE_REMOVED : Reset couters
                                                              #ifdef COUNT_DEBUG
                                                                  for (int ii = 0; ii < NO_OF_CHANNELS; ii++) {
                                                                    meterData.kWhTotal[ii] = 0.00;
                                                                    meterData.kWhPeriod[ii] = 0.00;
                                                                  }
                                                              #endif
// TO_BE_REMOVED  (END)


/*
 * Initiate Ethernet connection
 * Static IP is used to reduce sketch size
 */
  Ethernet.begin(mac, ip);
                                                              #ifdef DEBUG
                                                              Serial.print(P("Ethernet ready at :"));
                                                              Serial.println(Ethernet.localIP());
                                                              #endif
  localWebServer.begin();
                                                              #ifdef DEBUG
                                                              Serial.print(P("Listening on port: "));
                                                              Serial.println(ONBOARD_WEB_SERVER_PORT);
                                                              #endif

  // Initialize and arm interrupt.
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), readChannelPins, FALLING);

  digitalWrite(LED_BUILTIN, LOW);
}

/*
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 *                       L O O P     B e g i n
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 */
void loop() {
  boolean reqToPush = false;
  
/*
 * >>>>>>>>>>>>>>>>>>>>>>>>>  C o u n i n g    m o d u l e   -   B E G I N     <<<<<<<<<<<<<<<<<<<<<<<
 */

 
  for (int ii = 0; ii < NO_OF_CHANNELS; ii++) {
    if ( !channelState[ii]) {
      channelState[ii] = HIGH;
      digitalWrite(LED_BUILTIN, HIGH);
      meterData.kWhTotal[ii] += 1.000 / (double)PPKW[ii];
      meterData.kWhPeriod[ii] += 1.000 / (double)PPKW[ii];
      int characters = SD_reWriteAnything(DATA_FILE_NAME, meterData);
                                                              #ifdef COUNT_DEBUG
                                                              if ( ii == 0)
                                                                Serial.println();
                                                              Serial.print(P("Total kWh for channel "));
                                                              Serial.print( ii +1);
                                                              Serial.print(P(": "));
                                                              Serial.println(meterData.kWhTotal[ii], 3);
                                                              #endif
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  
/*
 * >>>>>>>>>>>>>>>>>>>>>>> W E B     S E R V E R     B E G I N    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 */
  EthernetClient localWebClient = localWebServer.available();
  
  if (localWebClient) {
    char keyWord1[14] = "pushToGoogle";
    int  keyWordPtr1 = 0;
    char keyWord2[12] = "meterValue";
    int  keyWordPtr2 = 0;
    while (localWebClient.connected())     {
      if (localWebClient.available()) {
        char c = localWebClient.read();
                                                              #ifdef WEB_DEBUG
                                                              Serial.print(c);
                                                              #endif

        if ( c == '\n') {
                                                              #ifdef WEB_DEBUG
                                                              Serial.println("\n\n>> New Line found - no further entreputation done\n\n");
                                                              #endif
          break;
        }

        if ( c == keyWord1[keyWordPtr1] )        // analize characterstream for presence of keyWord1 "pushToGoogle"
          keyWordPtr1++;
        else
          keyWordPtr1 = 0;

        if ( c == keyWord2[keyWordPtr2] )        // analize characterstream for presence of keyWord2 "meterValue"
          keyWordPtr2++;
        else
          keyWordPtr2 = 0;
      }

      
      if ( keyWordPtr1 == 12 || keyWordPtr2 == 10 ) {   // 12 / 10  på hinanden følgende karakterer i "keyWord" er fundet
        if ( keyWordPtr1 == 12) {
          reqToPush = true;                       // need to finish current web corrosponcence before pushing updates to wards Google. Setting flag to push data to webHook when finished.
        } else {
          char c = localWebClient.read();
          if ( c == '?') {                  //A "?" after the keyword/ function (abs_path) indicate a meterValue has a quey. A meter number and value-
            getQuery(localWebClient);
          }
        }

        // "200 OK" means the resource was located on the server and the browser (or service consumer) should expect a happy response
                                                              #ifdef WEB_DEBUG
                                                              Serial.println(P("\n\nGot Keyword sending: 200 OK\n\n"));
                                                              #endif
/*        
 *   Send a standard http response header
 */
        localWebClient.println(P("HTTP/1.1 200 OK"));
        localWebClient.println(P("Content-Type: text/html"));
        localWebClient.println(P("Connnection: close")); // do not reuse connection
        localWebClient.println();
          
/* 
 *  send html page, displaying meter values:
 *  
 *  
 */
        if ( reqToPush == true)
          localWebClient.println(P("<HTML><head><title>PostToGoogle</title></head><body><h1>Posting following to to Google Sheets</h1>"));
        else
          localWebClient.println(P("<HTML><head><title>MeterValues</title></head><body><h1>Energy meter values</h1>"));

        for ( int ii = 0; ii < NO_OF_CHANNELS; ii++) {
          localWebClient.println(P("<br><b>Meter </b>"));
          localWebClient.println(ii + 1);
          localWebClient.println(P("<b>:  </b>"));
          localWebClient.println(meterData.kWhTotal[ii], 3);
// TO_BE_REMOVED : print 3 digits                      ^^^          
          localWebClient.print(P("<form action='/meterValue?' method='GET'>New value: <input type=text name='meter"));
          char meter[2];
          sprintf(meter, "%i", ii + 1);
          localWebClient.print(meter);
          localWebClient.println(P("'/> <input type=submit value='Opdater'/></form>"));
// No endtag for <br> !?          localWebClient.println(P("</br>"));

        }
        // max length:    -----------------------------------------------------------------------------------------------------------------------------------------------------  (149 chars)
        localWebClient.println(P("</body></html>"));

        // Exit the loot
        break;
      }
    }

/*    
 *     Not verified if localWebClient.flush() actually empties the stream, therefor explicie read all characters.
 */
                                                              #ifdef WEB_DEBUG  // TO_BE_REMOVED
                                                              Serial.println(">> Reading available chars ><<<");
                                                              #endif

    while ( localWebClient.available()) {
      char c = localWebClient.read();
                                                              #ifdef WEB_DEBUG  // TO_BE_REMOVED
                                                              Serial.print(c);
                                                              #endif
    }
                                                              #ifdef WEB_DEBUG  // TO_BE_REMOVED
                                                              Serial.println("<<<< Available chars read >>>");
                                                              #endif
    localWebClient.flush();
    localWebClient.stop();
  }  //>>>>>>>>>>>>>>>>>>>>>>> W E B     S E R V E R     E N D    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  /*
   * >>>>>>>>>>>>>>>>>>> U P D A T E     G O O G L E   S H E E T S    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
   */
  if ( reqToPush == true) {
    updateGoogleSheets();
  }
}
