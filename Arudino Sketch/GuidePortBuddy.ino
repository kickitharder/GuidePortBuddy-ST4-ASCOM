#define VERSION "GuidePortBuddy V0.200930#"
/*
GUIDE PORT BUDDY
================

This Arduino sketch allows direct control of the ST-4 port of a telescope mount to apply pulses for guiding corrections.
It allows guiding software to control directly the directional lines via an ASCOM driver and send pulses quickly.

The pinout of the 6P6C RJ11 plug which is inserted into the mount's ST-4 guide port is below.  The line is activated by
grounding it.

 123456         1   No connection
||||||||        2   Ground
|      |        3   West  (-X)
| ==== |        4   South (-Y)
+------+        5   North (+Y)
  |  |          6   East  (+X)

The advantages of using this setup over a standard ST4 cable from a guide camera are as follows:

1)  The guide correction pulse is activated more quickly
2)  Guide corrections can be applied in both X and Y directions at the same time
3)  The telescope's drive can be stopped (and reactivated)
*/
#define WEST    3
#define SOUTH   4
#define NORTH   5
#define EAST    6
#define LED     13

char resp;                                        // Command and response to be sent back to Serial port
byte movingNS, movingEW;                          // Flags to indicate is Dec or RA axis are moving
unsigned long timerNS, timerEW, timerPause;       // Timers

void setup() {
  pinMode(WEST, INPUT_PULLUP);                  // Make ST-4 lines high resistance to ensure no pulse are active
  pinMode(EAST, INPUT_PULLUP);
  pinMode(SOUTH, INPUT_PULLUP);
  pinMode(NORTH, INPUT_PULLUP);
  pinMode(LED, OUTPUT);                         // LED is used to indicate ST-4 lines are active
  Serial.begin(115200);                         // Fastest Baud rate possible would be best to reduce time to activate ST-4 lines
  timerPause = movingNS = movingEW = 0;         // Initialise variables
  timerNS = timerEW = 1;                        // Setting timers to 1 inactivates them
  delay(2);
}

void loop() {
  if (Serial.available()){                        // Has something been received on the Serial port?
    resp = Serial.read();
    switch (resp){
      case 'W': moveEW(WEST);     break;          // Send pulse to move mount west for given number of milliseconds
      case 'E': moveEW(EAST);     break;          // Send pulse to move mount east for given number of milliseconds
      case 'S': moveNS(SOUTH);    break;          // Send pulse to move mount south for given number of milliseconds
      case 'N': moveNS(NORTH);    break;          // Send pulse to move mount north for given number of milliseconds
      case 'w': stopEW();         break;          // Stop moving west
      case 'e': stopEW();         break;          // Stop moving east
      case 's': stopNS();         break;          // Stop moving south
      case 'n': stopNS();         break;          // Stop moving north
      case 'P': pause();          break;          // Pause the mount to stop it tracking
      case 'X': stopAll();        break;          // Stop any guiding pulses 
      case 'x': stopAll();        break;
      case 'M': isMoving();       break;          // Reports if any guiding pulses are still active
      case 'L': timeLeft();       break;          // Returns time left before millis() clocks round to 0
      case 'R': reset();          break;          // Resets the sketch - helps prevent millis() from clocking back to 0
      case 'A': resp = 'A';       break;          // Reports if GuidePortBuddy is active and repsonding
      case 'T': Serial.println(timerNS); break;
      case 'V': Serial.print(VERSION); resp = 0 ;break;
                                                  // Return to the name and version of this sketch
      case '?': help();           break;          // Help info
      default: resp = 0; break;                   // What has been received is bad - ignore it.
    }
    if(resp){
      Serial.print(resp);                         // Send a repsonse if it exists.
      Serial.print("#");
    }
  }
  if (timerPause){                                // If in Pause mode, a timer for blinking the LED is active
    if(millis() > timerPause){
      timerPause = millis() + 500;                // Blink the LED every 0.5 sec to indicate in Pause mode
      digitalWrite(LED, 1 - digitalRead(LED));
    }
  } else{
    if (movingEW) if (timerEW) if(millis() > timerEW) stopEW();   // If the EW timer has expired, turn off East/West pulse
    if (movingNS) if (timerNS) if(millis() > timerNS) stopNS();   // If the NS timer has expired, turn off North/South pulse

    if (movingEW || movingNS) digitalWrite(LED, HIGH);            // If a pulse active, light the LED
    else digitalWrite(LED, LOW);                                  // else turn off the LED
  }
}

void moveEW(byte direction){          // Direction is EAST or WEST
  if(timerPause) stopNS();            // If parked, rest NORTH & SOUTH lines
  stopEW();                           // Stop any current EAST or WEST pulse
  pinMode(direction, OUTPUT);         // Activate EAST or WEST line
  digitalWrite(direction, LOW);       // Ground the line to send a pulse
  timerEW = getDuration();            // See if there is a duration in the command for pulse
  movingEW = 1;                       // If there is no error then the mount is now moving East or West
}

void moveNS(byte direction){          // Direction is NORTH or SOUTH
  if(timerPause) stopEW();            // If parked, reset EAST & WEST lines
  stopNS();                           // Stop any current NORTH or SOUTH pulse
  pinMode(direction, OUTPUT);         // Activate NORTH or SOUTH line
  digitalWrite(direction, LOW);       // Ground the line to send a pulse
  timerNS = getDuration();            // See if there is a duration in the command of pulse
  movingNS = 1;                       // It there is no error then the mount is now moving North or South
}

void stopEW(){
  pinMode(EAST, INPUT_PULLUP);        // Deactivate lines using the Arduino pull-up resistors 
  pinMode(WEST, INPUT_PULLUP);
  movingEW = timerPause = 0;          // Mount is no longer moving or paused
  timerEW = 1;                        // Stop the pulse timer
}

void stopNS(){
  pinMode(NORTH, INPUT_PULLUP);       // Deactivate lines using the Arduino's pull-up resistors
  pinMode(SOUTH, INPUT_PULLUP);
  movingNS = timerPause = 0;          // Mount is no longer moving or paused
  timerNS = 1;                        // Stop the pulse timer
}

void pause(){                         // This pause the telescope mount
  timerEW = timerNS = 1;              // Stop the pulse timers
  pinMode(EAST, OUTPUT);              // Pausing is done by grounding all of the pulse lines to make the active
  digitalWrite(EAST, LOW);
  pinMode(WEST, OUTPUT);
  digitalWrite(WEST, LOW);
  pinMode(NORTH, OUTPUT);
  digitalWrite(NORTH, LOW);
  pinMode(SOUTH, OUTPUT);
  digitalWrite(SOUTH, LOW);
  timerPause = 1;                     // Pause LED blink timer is made active
  movingNS = movingEW = 0;            // The mount is now classed as not moving
}

void stopAll(){
  stopEW();                           // Stop all pulses and allow the mount to track normally
  stopNS();
}

void isMoving(){                        // Reports if a pulse is in progress
  resp = '0';                           // Default resp is '0' meaning no guiding pulses are active
  if(movingEW || movingNS) resp = '1';  // '1' means one or more guiding pulses are active
  if(timerPause) resp = '2';            // '2' means the mount is in Pause mode
}

void timeLeft(){
  unsigned long t = -millis();          // Return the time left before milis() clocks round to 0
  int i = 0;
  while (t >= 86400000) {
    t -= 86400000;
    i++;
  }
  Serial.print("Days:");
  Serial.print(i);
  i = 0;
  while (t >= 3600000) {
    t -= 3600000;
    i++;
  }
  Serial.print(" Hours:");
  Serial.print(i);
  while (t >= 60000) {
    t -= 60000;
    i++;
  }
  Serial.print(" Minutes:");
  Serial.print(i);
  i = 0;
  while (t >=1000) {
    t -= 1000;
    i++;
  }
  Serial.print(" Seconds:");
  Serial.print(i);
  Serial.print(" Millis:");
  Serial.print(t);
  Serial.print('#');  
  resp = 0;
}

void reset(){
  Serial.print(resp);                       // This restarts the sketch so that millis() is reset
  Serial.print("#");                        // timer might then last 50 days!
  Serial.flush();
  asm volatile ("jmp 0");
}

unsigned long getDuration(){
 unsigned long value = timerPause = 0;  // Cancel pause LED blink timer as a request to start a guiding pulse has been received
  value = Serial.parseInt();
  if (value) return value + millis();
  return 0;
}

void help(){
  Serial.println(VERSION);
  Serial.println();
  Serial.println("N      Send continuous North pulse");
  Serial.println("S      Send continuous South pulse");
  Serial.println("E      Send continuous East pulse");
  Serial.println("W      Send continuous West pulse");
  Serial.println("NXXXXX Send North pulse for XXXXX ms");
  Serial.println("SXXXXX Send South pulse for XXXXX ms");
  Serial.println("EXXXXX Send East pulse for XXXXX ms");
  Serial.println("WXXXXX Send West pulse for XXXXX ms");
  Serial.println("n      Stop North pulse");
  Serial.println("s      Stop South pulse");
  Serial.println("e      Stop East pulse");
  Serial.println("w      Stop West pulse");
  Serial.println("P      Pause mount - pulses sent all direction");
  Serial.println("X      Stop any active pulses");
  Serial.println("x      Stop and active pulses");
  Serial.println("M      Returns 0 = no active pulses, 1 = active pulses, 2 = Paused");
  Serial.println("L      Time left before clock resets to 0");
  Serial.println("R      Reset the sketch and restart clock from 0");
  Serial.println("A      Returns 'A' if GuidePortBuddy is repsonding");
  
}