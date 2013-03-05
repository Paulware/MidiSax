/*
 NAME:                 11  switches to MIDI Output 
              
 FILE SAVED AS:        Saxophone.ino
 FOR:                  Mega
 CLOCK:                16.00 MHz CRYSTAL                                        
 PROGRAMME FUNCTION:   11 switches representing saxophone valves
  
 HARDWARE NOTE:
 The Midi IN Socket is connected to the Miduino RX through an 6N139 opto-isolator
 *
 * To send MIDI, attach a MIDI out Female 180 Degree 5-Pin DIN socket to Arduino.
 * Socket is seen from solder tags at rear.
 * DIN-5 pinout is:                                         _______ 
 *    pin 2 - Gnd                                          /       \
 *    pin 4 - 220 ohm resistor to +5V                     | 1     3 |  MIDI jack
 *    pin 5 - Arduino Pin 1 (TX) via a 220 ohm resistor   |  4   5  |
 *    all other pins - unconnected                         \___2___/
 *
 */


#define startNote 60       // Start note of C
#define ON_Threshold 2 // Set noise threshold level before switching ON

//variables setup

unsigned long switchCombination;
byte oldswitch;
byte noteon;
byte n;
byte note;
byte midiByte;
byte MIDIchannel=0;
int velocity;
byte velocity_value;
byte oldvelocity;
byte channel;
byte vol;
byte status=0;
byte x;
byte Flag;
byte LedPin = 13;   // select the pin for the LED
byte count;
byte times;
byte sensitivity=3;
byte Transposition=0;  //Set key to C at initialization
byte OctaveSwitch;
byte ChordsOn;
byte Octave;
byte oldOctave;
byte oldChordsOn;
byte FunctionOn;
byte Panic;
byte Loudness=99;
byte Silent=1;
byte Porto=0;
byte PressureDivider=8;
byte Bend;
int PulseX;
int AccelX;
int BeginAccelX;
byte BendSensitivity=10;  // larger - less sensitive
byte WriteDelay=2;
byte PitchOn=0;
byte OnAttack=80;
byte NoiseLevel=50;

byte inputValves[] = {22,23,24,25,26,27,28,29,30,31,32 };

void setup() {
  
  for (int i=0; i<11; i++)
  {
    pinMode (inputValves[i], INPUT);
	digitalWrite (inputValves[i], HIGH ); // Set pull-up resistors high
  }
  /* 
  pinMode(4, INPUT); // 
  pinMode(5, INPUT); // Pitch On
  pinMode(6, INPUT); // Pitch X

  pinMode(2, INPUT); //  Set inputs for O1 and O2
  pinMode(3, INPUT); 

  pinMode(8, INPUT); //  V5  Set Inputs for 5 Valve Switches
  pinMode(9, INPUT); //  V4
  pinMode(10, INPUT);//  V3 
  pinMode(11, INPUT); // V2
  pinMode(12, INPUT);  //V1
  pinMode(15, INPUT);  //Chord 1
  pinMode(16, INPUT);  //Chord 2
  pinMode(17, INPUT);  //Function Button
  pinMode(18, INPUT);  //Panic Button
  pinMode(19, INPUT);  // Silent
  

  digitalWrite(4, HIGH); // Set inputs Pull-up resistors High
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);

  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);

  digitalWrite(8, HIGH); // Set inputs Pull-up resistors High
  digitalWrite(9, HIGH);
  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(15, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
  */

  pinMode(LedPin,OUTPUT);   // declare the LED's pin as output

  hereIam(3);  // flash 3 times for startup

  Serial.begin(31250);  //start serial with midi baudrate 31250
  Serial.flush();

}

void loop() {
  noteon=0;
  readSwitches();
  readValves();
  oldswitch = switchCombination;   // oldswitch holds the existing note
  oldOctave=Octave;  // holds existing octave
  oldChordsOn=ChordsOn;   // holds existing chords
  if(FunctionOn==0) {
    KeyFunction();
  }
  if(Panic==0){  // Panic switch - off all notes
    for(note=0;note<=127;note++){
      midiSend( (0x80 | (MIDIchannel)), note, 0x00); // Note off each note
      midiSend( (0x80 | (MIDIchannel+1)), note, 0x00); // Note off each note
      midiSend( (0x80 | (MIDIchannel+2)), note, 0x00); // Note off each note
    }
  }
  if(PitchOn==1){
    GetBase_Tilt();
  }
  
  GetPressure();
  oldvelocity=velocity;  //variable used to see if velocity has changed
  if(velocity > ON_Threshold){
    GetPressure();  // check again to eliminate spurious signals
    if(velocity > ON_Threshold){
      Pitch_Bend(MIDIchannel,0x00,64&0x007F);  //reset pitch bend back to zero for new note
      // velocity=OnAttack;   // Set high attack for better staccato
      getNoteOn(switchCombination);
      // GetPressure();
      //AdjustVolume();

    }
    noteon=1;
  }

  while(velocity > ON_Threshold){

    readValves();
    if (oldswitch!=switchCombination||oldOctave!=Octave||oldChordsOn!=ChordsOn){     // Check if note has changed
      delay(40);   // deglitch the valves with 40 ms delay
      readValves();
      if (Porto==0){ // Porto Off
        getNoteOff(oldswitch);    //  turn off the old note
        // velocity=OnAttack;   // Set high attack for better staccato
        getNoteOn(switchCombination); 
        // GetPressure();
        // AdjustVolume();
      }
      else { // Porto On
        // velocity=OnAttack;   // Set high attack for better staccato
        getNoteOn(switchCombination); 
        getNoteOff(oldswitch);    //  turn off the old note
        // GetPressure();
        // AdjustVolume();
      }
      oldswitch=switchCombination;     //  reset "oldswitch" to the existing note
      oldOctave=Octave;  //reset Octave
      oldChordsOn=ChordsOn; //reset Chords
      if(PitchOn==1){
        GetBase_Tilt();  // Check position for pitch bend at every new note
      }
      Pitch_Bend(MIDIchannel,0x00,64&0x007F);  //reset pitch bend back to zero for new note
    }
    if(PitchOn==1){
      PitchBender();
    }
    GetPressure();
    //  getNoteOn(switchCombination);  //kludge to stop off notes when playing

   // if(abs(velocity-oldvelocity)>sensitivity){  // don't change the volume for every little fluctuation
      AdjustVolume();
   //   oldvelocity=velocity;  // reset the velocity check variable
   // }
  }
  if(noteon==1){
    getNoteOff(switchCombination);  //pressure has dropped and note is still on - turn it off
    noteon=0;
  }     
}
//_______________________________________________________________________________________________

void GetPressure(){
  delay(1);
  velocity_value=max((analogRead(0)-NoiseLevel),8)/PressureDivider;
  velocity=abs(min(127,velocity_value));
}

void PitchBender() {
  delay(1);
  PulseX=pulseIn(6,HIGH);
  AccelX=((PulseX/10)-500)*8;
  if(AccelX>BeginAccelX) {  //bend up
    Bend=min(127,((AccelX-BeginAccelX)/BendSensitivity)+64);
  }
  else {  //bend down
    Bend=(64-min(64,(BeginAccelX-AccelX)/BendSensitivity));
  }
  Bend=Bend&0x007F;
  Pitch_Bend(MIDIchannel,0x00,Bend);
}

void GetBase_Tilt() {
  PulseX=pulseIn(6,HIGH);
  BeginAccelX=((PulseX/10)-500)*8;
}

void midiMsg(byte cmd, byte data1, byte data2) {
  
  // digitalWrite(LedPin, HIGH);
  delay(WriteDelay);
  Serial.print(cmd, BYTE);
  Serial.print(data1, BYTE);
  Serial.print(data2, BYTE);
  // digitalWrite(LedPin,LOW);
}

//  Send a three byte midi message  
void midiSend(byte status, byte data1, byte data2) {
    
  //  digitalWrite(LedPin,HIGH);  // indicate we're sending MIDI data
  delay(WriteDelay);
  Serial.print(status, BYTE);
  Serial.print(data1 + startNote, BYTE);
  Serial.print(data2 , BYTE);
  // digitalWrite(LedPin,LOW);  // indicate we're sending MIDI data
}
// Functions:

void midi_volume(byte channel, byte vol) {
    
  // digitalWrite(LedPin, HIGH);   // sets the LED on
  delay(WriteDelay);
  Serial.print(0xB0 | channel&0x0F, BYTE);  //  control change command 
  Serial.print(0x02, BYTE);  //  Breath Control command
  Serial.print(vol&0x7F, BYTE);  //  volume 0-127
  // delay(WriteDelay);
  Serial.print(0xB0 | channel&0x0F, BYTE);  //  control change command 
  Serial.print(0x07, BYTE);  //  Volume command
  Serial.print(vol&0x7F, BYTE);  //  volume 0-127
  // digitalWrite(LedPin, LOW);    // sets the LED off

}

void Pitch_Bend(byte channel, byte LSB, byte MSB) {
  if (ChordsOn==3) {  // Single Note
    midiMsg((0xE0 | channel), LSB, MSB);
  }
  else {
    midiMsg((0xE0 | channel), LSB, MSB);
    midiMsg((0xE0 | channel+1), LSB, MSB);
    midiMsg((0xE0 | channel+2), LSB, MSB);
  }
}


void AdjustVolume(){

  switch(ChordsOn) {
  case 3:  // Single Note
    midi_volume(MIDIchannel, velocity);  //adjusts the volume
    break;
  case 2:  // Major
    midi_volume(MIDIchannel, velocity);  //adjusts the volume
    midi_volume(MIDIchannel+1, velocity);  //adjusts the volume
    midi_volume(MIDIchannel+2, velocity);  //adjusts the volume
    break;
  case 1:  //Minor
    midi_volume(MIDIchannel, velocity);  //adjusts the volume
    midi_volume(MIDIchannel+1, velocity);  //adjusts the volume
    midi_volume(MIDIchannel+2, velocity);  //adjusts the volume
    break;
  case 0:  //Horn Lines
    midi_volume(MIDIchannel, velocity);  //adjusts the volume
    midi_volume(MIDIchannel+1, velocity);  //adjusts the volume 
    midi_volume(MIDIchannel+2, velocity);  //adjusts the volume
    break;
  }
}


//---------------------------------------------------------------------------------------------------
void readValves(){

  switchCombination = 0;
  for (int i=0; i<11; i++)
    if (digitalRead (inputValves[i] ) )
	  switchCombination = (switchCombination * 2 ) + 1;

  /*
  // Read Valve 1 to 5  switches
  switchCombination=digitalRead(8) + (digitalRead(9)<<1) + (digitalRead(10)<<2) + (digitalRead(11)<<3)  + (digitalRead(12)<<4); 

  // Read Octave Switches O1 and O2
  OctaveSwitch=digitalRead(2) + (digitalRead(3)<<1);
  switch(OctaveSwitch){
  case 2:
    Octave=-24;
    break;
  case 1:
    Octave=+24;
    break;
  default:
    Octave=0;
    break;
  }

  // read Chord Switches
  ChordsOn=digitalRead(15) + (digitalRead(16)<<1);
  */
}

void readSwitches() {
  // read Function Button
  FunctionOn=digitalRead(17);

  // read Panic Button
  Panic=digitalRead(18);

  // read Silent Switch
  Silent=digitalRead(19);
  
  // read Pitch On Switch
  PitchOn=digitalRead(5);
}

void KeyFunction() {

  switch(ChordsOn) { // Turn Portomento on and off
  case 1:
    Porto=1;
    break;
  case 2:
    Porto=0;
    break;
  }

  switch(switchCombination) {  // Key Tranposition

    case 29:  //  Natural keys B E A D G 
    Blip(Transposition+12);
    delay(1000);
    readValves();
    while(switchCombination==31) {  // wait for the key to be input
      Blip(Transposition+12);
      delay(1000);
      readValves();
    }
    switch(switchCombination)  {
    case 15:
      Transposition=-1;
      break;
    case 23:
      Transposition=4;
      break;
    case 27:
      Transposition=-3;
      break;
    case 29:
      Transposition=2;
      break;
    case 30:
      Transposition=-5;
      break;
    default:
      Transposition=0;
      break;
    }
    Blip(Transposition+12);
    delay(50); 
    Blip(Transposition+12);
    break;

  case 30:  // Flat keys Bb Eb Ab Db Gb 
    Blip(Transposition+12);
    delay(1000);
    readValves();
    while(switchCombination==31) {  // wait for the key to be input
      Blip(Transposition+12);
      delay(1000);
      readValves();
    }

    switch(switchCombination)  {
    case 15:
      Transposition=-2;
      break;
    case 23:
      Transposition=3;
      break;
    case 27:
      Transposition=-4;
      break;
    case 29:
      Transposition=1;
      break;
    case 30:
      Transposition=-6;
      break;
    default:
      Transposition=0;
      break;
    }
    Blip(Transposition+12); 
    delay(50); 
    Blip(Transposition+12);
    break;

  case 28:  //Keys C and F
    Blip(Transposition+12);
    delay(1000);
    readValves();
    while(switchCombination==31) {  // wait for the key to be input
      Blip(Transposition+12);
      delay(1000);
      readValves();
    }
    switch(switchCombination)  {
    case 15:
      Transposition=0;
      break;
    case 23:
      Transposition=5;
      break;
    default:
      Transposition=0;
      break;
    }
    Blip(Transposition+12); 
    delay(50); 
    Blip(Transposition+12);
    break;
  case 15: // midi Channel # 0
    MIDIchannel=0;
    Blip(12);
    delay(30);
    break;
  case 23: // midi Channel # 3
    MIDIchannel=3;
    Blip(12);
    delay(30);
    break;
  case 27: // midi Channel # 6
    MIDIchannel=6;
    Blip(12);
    delay(30);
    break;
  case 7: // midi Channel # 10
    MIDIchannel=10;
    Blip(12);
    delay(30);
    break;
  case 11: // midi Channel # 13
    MIDIchannel=13;
    Blip(12);
    delay(30);
    break;
  case 3: //MIDI Channel #10
    MIDIchannel=9;
    Blip(12);
    delay(30);
    break;
  }
}

void Blip(byte nt) {
  if (Silent==0) {
    digitalWrite( LedPin, HIGH );
    delay(300);
    digitalWrite( LedPin, LOW );
    delay(300);
  }
  else
  {
    midiSend((0x90 | MIDIchannel), nt, 20);
    // midi_volume(MIDIchannel, 30);  //adjusts the volume
    delay(100);
    midiSend((0x80 | MIDIchannel), nt, 0x00);
  }
}

void hereIam(byte times){

  for (x=1; x<=times; x++){
    digitalWrite( LedPin, HIGH );
    delay(300);
    digitalWrite( LedPin, LOW );
    delay(300);
  }
}

//____________________________________________________________________________________________________

void getNoteOn(byte var){

  switch (var) {

  case 31:
    note=0;
    break;

  case 3:
    note=1;
    break;

  case 11:
    note=2;
    break;

  case 19:
    note=3;
    break;

  case 7:
    note=4;
    break;

  case 15:
    note=5;
    break;

  case 23:
    note=6;
    break;

  case 29:
    note=7;
    break;

  case 17:
    note=8;
    break;

  case 5:
    note=9;
    break;

  case 13:
    note=10;
    break;

  case 21:
    note=11;
    break;

  case 28:
    note=12;
    break;

  case 0:
    note=13;
    break;

  case 8:
    note=14;
    break;

  case 16:
    note=15;
    break;

  case 4:
    note=16;
    break;

  case 12:
    note=17;
    break;

  case 20:
    note=18;
    break;

  case 30:
    note=19;
    break;

  case 18:
    note=20;
    break;

  case 6:
    note=21;
    break;

  case 14:
    note=22;
    break;

  case 22:
    note=23;
    break;

  default:
    break;  

  }

  switch(ChordsOn) {
  case 3:  //single note
    midiSend((0x90 | MIDIchannel), note+Transposition+Octave, velocity);
    break;
  case 2:  // Major
    midiSend((0x90 | MIDIchannel), note+Transposition+Octave, velocity);
    midiSend((0x90 | MIDIchannel+1), note+Transposition+Octave-5, velocity);
    midiSend((0x90 | MIDIchannel+2), note+Transposition+Octave-8, velocity);
    break;
  case 1:  //Minor
    midiSend((0x90 | MIDIchannel), note+Transposition+Octave, velocity);
    midiSend((0x90 | MIDIchannel+1), note+Transposition+Octave-5, velocity);
    midiSend((0x90 | MIDIchannel+2), note+Transposition+Octave-9, velocity);
    break;
  case 0:  //Horn Line
    midiSend((0x90 | MIDIchannel), note+Transposition+Octave, velocity);
    midiSend((0x90 | MIDIchannel+1), note+Transposition+Octave-5, velocity);
    midiSend((0x90 | MIDIchannel+2), note+Transposition+Octave-12, velocity);
    break;
  }

}

//____________________________________________________________________________________________________

void getNoteOff(byte var){

  switch (var) {
  case 31:
    note=0;
    break;

  case 3:
    note=1;
    break;

  case 11:
    note=2;
    break;

  case 19:
    note=3;
    break;

  case 7:
    note=4;
    break;

  case 15:
    note=5;
    break;

  case 23:
    note=6;
    break;

  case 29:
    note=7;
    break;

  case 17:
    note=8;
    break;

  case 5:
    note=9;
    break;

  case 13:
    note=10;
    break;

  case 21:
    note=11;
    break;

  case 28:
    note=12;
    break;

  case 0:
    note=13;
    break;

  case 8:
    note=14;
    break;

  case 16:
    note=15;
    break;

  case 4:
    note=16;
    break;

  case 12:
    note=17;
    break;

  case 20:
    note=18;
    break;

  case 30:
    note=19;
    break;

  case 18:
    note=20;
    break;

  case 6:
    note=21;
    break;

  case 14:
    note=22;
    break;

  case 22:
    note=23;
    break;

  default:
    break; 

  }

  switch(oldChordsOn) {
  case 3: //Single Note
    midiSend((0x80 | MIDIchannel), note+Transposition+oldOctave, 0x00);
    break;
  case 2: //Major
    midiSend((0x80 | MIDIchannel), note+Transposition+oldOctave, 0x00);
    midiSend((0x80 | MIDIchannel+1), note+Transposition+oldOctave-5, 0x00);
    midiSend((0x80 | MIDIchannel+2), note+Transposition+oldOctave-8, 0x00);
    break;
  case 1:  //Minor
    midiSend((0x80 | MIDIchannel), note+Transposition+oldOctave, 0x00);
    midiSend((0x80 | MIDIchannel+1), note+Transposition+oldOctave-5, 0x00);
    midiSend((0x80 | MIDIchannel+2), note+Transposition+oldOctave-9, 0x00);
    break;
  case 0:  //Horn Line
    midiSend((0x80 | MIDIchannel), note+Transposition+oldOctave, 0x00);
    midiSend((0x80 | MIDIchannel+1), note+Transposition+oldOctave-5, 0x00);
    midiSend((0x80 | MIDIchannel+2), note+Transposition+oldOctave-12, 0x00);
    break;
  }

}

