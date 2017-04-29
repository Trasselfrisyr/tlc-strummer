/////////////////////////////////////////////////////////////////////////
//                                                                     //
//   Teensy 3.2 chord strummer with built in audio output              //
//   by Johan Berglund, April 2017                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

#define MIDI_CH 1            // MIDI channel
#define VELOCITY 64          // MIDI note velocity (64 for medium velocity, 127 for maximum)
#define START_NOTE 60        // MIDI start note (60 is middle C)
#define PADS 8               // number of touch electrodes
#define LED_PIN 13           // LED to indicate midi activity
#define BRIGHT_LED 1         // LED brightness, 0 is low, 1 is high
#define TOUCH_THR 1200       // threshold level for capacitive touch (lower is more sensitive)

#define AUTOHARP 0
#define OMNICHORD 1

#define ATTACK 1             // tone attack value in ms
#define RELEASE 1000         // tone release value in ms

#define CHECK_INTERVAL 4     // interval in ms for sensor check

unsigned long currentMillis = 0L;
unsigned long statusPreviousMillis = 0L;

byte colPin[12]          = {15,20,21,5,6,7,8,9,10,11,12,14};// teensy digital input pins for keyboard columns (just leave unused ones empty)
byte colNote[12]         = {1,8,3,10,5,0,7,2,9,4,11,6};     // column to note number                                            
                                                            // column setup for omnichord style (circle of fifths)
                                                            // chord    Db, Ab, Eb, Bb,  F,  C,  G,  D,  A,  E,  B, F#
                                                            // col/note  1,  8,  3, 10,  5,  0,  7,  2,  9,  4, 11,  6
                                                            // for chromatic order, C to B, straight order 0 to 11
                                                            // original tlc strummer design only use pins 5 through 12 (chords Bb to B)

byte rowPin[3]           = {4,3,2};                         // teensy output pins for keyboard rows

                                                            // chord type   maj, min, 7th
                                                            // row            0,   1,   2
                                                            
                                                            // chordType 0 to 7, from binary row combinations
                                                            // 0 0 0 silent
                                                            // 0 0 1 maj
                                                            // 0 1 0 min
                                                            // 0 1 1 dim  (maj+min keys)
                                                            // 1 0 0 7th
                                                            // 1 0 1 maj7 (maj+7th keys)
                                                            // 1 1 0 m7   (min+7th keys)
                                                            // 1 1 1 aug  (maj+min+7th)

byte sensorPin[8]       = {1,0,23,22,19,18,17,16};          // teensy lc touch input pins
byte activeNote[8]      = {0,0,0,0,0,0,0,0};                // keeps track of active notes

byte sensedNote;               // current reading
byte patch = OMNICHORD;        // built in audio sound setting (default)
byte backing = 0;              // backing chord on/off 0/1 (default)  
byte prevP = 0;                // setting combo edge trackings
byte prevB = 0; 
byte prevBup = 0;
byte prevBdn = 0;
int noteNumber;                // calculated midi note number
int chord = 0;                 // chord key setting (base note of chord)
int chordType = 0;             // chord type (maj, min, 7th...)

int chordNote[8][8] = {        // chord notes for each pad/string
  {-1,-1,-1,-1,-1,-1,-1,-1 },  // silent
  { 0, 4, 7,12,16,19,24,28 },  // maj 
  { 0, 3, 7,12,15,19,24,27 },  // min 
  { 0, 3, 6, 9,12,15,18,21 },  // dim 
  { 0, 4, 7,10,12,16,19,22 },  // 7th 
  { 0, 4, 7,11,12,16,19,23 },  // maj7
  { 0, 3, 7,10,12,15,19,22 },  // m7  
  { 0, 4, 8,12,16,20,24,28 }   // aug  
};

float midiToFreq[128];         // for storing pre calculated frequencies for note numbers
float bacLevel = 0.3;

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=85,38
AudioSynthWaveformSine   sine2;          //xy=90,87
AudioSynthWaveformSine   sine3;          //xy=97,137
AudioSynthWaveformSine   sine4;          //xy=105,186
AudioSynthWaveformSine   sine5;          //xy=114,234
AudioSynthWaveformSine   sine6;          //xy=127,286
AudioSynthWaveformSine   sine7;          //xy=140,337
AudioSynthWaveformSine   sine8;          //xy=150,386
AudioSynthKarplusStrong  string1;        //xy=161,464
AudioSynthKarplusStrong  string2;        //xy=174,509
AudioSynthKarplusStrong  string3;        //xy=187,554
AudioSynthKarplusStrong  string4;        //xy=205,598
AudioSynthKarplusStrong  string5;        //xy=214,641
AudioSynthKarplusStrong  string6;        //xy=218,683
AudioSynthKarplusStrong  string7;        //xy=233,726
AudioSynthKarplusStrong  string8;        //xy=241,768
AudioSynthWaveform       waveform1;      //xy=256,836
AudioSynthWaveform       waveform2;      //xy=257,875
AudioSynthWaveform       waveform3;      //xy=260,912
AudioSynthWaveform       waveform4;      //xy=263,948
AudioEffectFade          fade1;          //xy=283,62
AudioEffectFade          fade2;          //xy=290,105
AudioEffectFade          fade3;          //xy=299,150
AudioEffectFade          fade4;          //xy=317,199
AudioEffectFade          fade5;          //xy=331,244
AudioEffectFade          fade6;          //xy=346,293
AudioEffectFade          fade7;          //xy=359,335
AudioEffectFade          fade8;          //xy=375,381
AudioMixer4              mixer1;         //xy=537,138
AudioMixer4              mixer2;         //xy=577,240
AudioMixer4              mixer3;         //xy=727,183
AudioMixer4              mixer4;         //xy=801,490
AudioMixer4              mixer5;         //xy=828,609
AudioMixer4              mixer8;         //xy=872,810
AudioMixer4              mixer6;         //xy=1015,538
AudioEffectFade          fade9;          //xy=1100,214
AudioEffectFade          fade10;         //xy=1150,318
AudioEffectFade          fade11;         //xy=1201,687
AudioMixer4              mixer7;         //xy=1353,276
AudioOutputAnalog        dac1;           //xy=1509,245
AudioConnection          patchCord1(sine1, fade1);
AudioConnection          patchCord2(sine2, fade2);
AudioConnection          patchCord3(sine3, fade3);
AudioConnection          patchCord4(sine4, fade4);
AudioConnection          patchCord5(sine5, fade5);
AudioConnection          patchCord6(sine6, fade6);
AudioConnection          patchCord7(sine7, fade7);
AudioConnection          patchCord8(sine8, fade8);
AudioConnection          patchCord9(string1, 0, mixer4, 0);
AudioConnection          patchCord10(string2, 0, mixer4, 1);
AudioConnection          patchCord11(string3, 0, mixer4, 2);
AudioConnection          patchCord12(string4, 0, mixer4, 3);
AudioConnection          patchCord13(string5, 0, mixer5, 0);
AudioConnection          patchCord14(string6, 0, mixer5, 1);
AudioConnection          patchCord15(string7, 0, mixer5, 2);
AudioConnection          patchCord16(string8, 0, mixer5, 3);
AudioConnection          patchCord17(waveform1, 0, mixer8, 0);
AudioConnection          patchCord18(waveform2, 0, mixer8, 1);
AudioConnection          patchCord19(waveform3, 0, mixer8, 2);
AudioConnection          patchCord20(waveform4, 0, mixer8, 3);
AudioConnection          patchCord21(fade1, 0, mixer1, 0);
AudioConnection          patchCord22(fade2, 0, mixer1, 1);
AudioConnection          patchCord23(fade3, 0, mixer1, 2);
AudioConnection          patchCord24(fade4, 0, mixer1, 3);
AudioConnection          patchCord25(fade5, 0, mixer2, 0);
AudioConnection          patchCord26(fade6, 0, mixer2, 1);
AudioConnection          patchCord27(fade7, 0, mixer2, 2);
AudioConnection          patchCord28(fade8, 0, mixer2, 3);
AudioConnection          patchCord29(mixer1, 0, mixer3, 0);
AudioConnection          patchCord30(mixer2, 0, mixer3, 1);
AudioConnection          patchCord31(mixer3, fade9);
AudioConnection          patchCord32(mixer4, 0, mixer6, 0);
AudioConnection          patchCord33(mixer5, 0, mixer6, 1);
AudioConnection          patchCord34(mixer8, fade11);
AudioConnection          patchCord35(mixer6, fade10);
AudioConnection          patchCord36(fade9, 0, mixer7, 0);
AudioConnection          patchCord37(fade10, 0, mixer7, 1);
AudioConnection          patchCord38(fade11, 0, mixer7, 2);
AudioConnection          patchCord39(mixer7, dac1);
// GUItool: end automatically generated code



// Pointers
AudioSynthWaveformSine*  osc[8]{&sine1,&sine2,&sine3,&sine4,&sine5,&sine6,&sine7,&sine8};
AudioEffectFade*         fader[8]{&fade1,&fade2,&fade3,&fade4,&fade5,&fade6,&fade7,&fade8};
AudioSynthKarplusStrong* str[8]{&string1,&string2,&string3,&string4,&string5,&string6,&string7,&string8};
AudioSynthWaveform*      bac[4]{&waveform1,&waveform2,&waveform3,&waveform4};
AudioEffectFade*         oscFade = &fade9;
AudioEffectFade*         strFade  =&fade10;
AudioEffectFade*         bacFade  =&fade11;

// SETUP
void setup() {
  for (int i = 0; i < 12; i++) {
     pinMode(colPin[i],INPUT_PULLUP);
  }
    for (int i = 0; i < 3; i++) {
     pinMode(rowPin[i],OUTPUT);
     digitalWrite(rowPin[i],LOW);
  }
  pinMode(LED_PIN, OUTPUT);
  for(int i=0;i<128;i++) {  // set up table, midi note number to frequency
      midiToFreq[i] = numToFreq(i);
  }
  AudioMemory(20);
  dac1.analogReference(INTERNAL);   // normal volume
  //dac1.analogReference(EXTERNAL); // louder
  mixer1.gain(0, 0.27);
  mixer1.gain(1, 0.27);
  mixer1.gain(2, 0.27);
  mixer1.gain(3, 0.27);
  mixer2.gain(0, 0.27);
  mixer2.gain(1, 0.27);
  mixer2.gain(2, 0.27);
  mixer2.gain(3, 0.27);
  mixer3.gain(0, 0.5);
  mixer3.gain(1, 0.5);
  mixer4.gain(0, 0.27);
  mixer4.gain(1, 0.27);
  mixer4.gain(2, 0.27);
  mixer4.gain(3, 0.27);
  mixer5.gain(0, 0.27);
  mixer5.gain(1, 0.27);
  mixer5.gain(2, 0.27);
  mixer5.gain(3, 0.27);
  mixer6.gain(0, 0.5);
  mixer6.gain(1, 0.5);
  mixer7.gain(0, 1);
  mixer7.gain(1, 1);
  mixer7.gain(2, 0.1);
  delay(100);
  for (int i=0; i < 4; i++){
    bac[i]->begin(WAVEFORM_SAWTOOTH);
    bac[i]->amplitude(bacLevel);
  }
  if (patch == OMNICHORD){
    oscFade->fadeIn(1);
    strFade->fadeOut(1);
  } else {
    oscFade->fadeOut(1);
    strFade->fadeIn(1);
  }
  bacFade->fadeOut(1);
}

// MAIN LOOP
void loop() {
  currentMillis = millis();
  if ((unsigned long)(currentMillis - statusPreviousMillis) >= CHECK_INTERVAL) {
    if (BRIGHT_LED) digitalWrite(LED_PIN, LOW);                          // led off for high brightness
    readKeyboard();                                                      // read keyboard input and replay active notes (if any) with new chording
    for (int scanSensors = 0; scanSensors < PADS; scanSensors++) {       // scan sensors for changes and send note on/off accordingly
      sensedNote = (touchRead(sensorPin[scanSensors]) > TOUCH_THR);      // read touch pad/pin/electrode/string/whatever
      if (sensedNote != activeNote[scanSensors]) {
        noteNumber = START_NOTE + chord + chordNote[chordType][scanSensors];
        if ((noteNumber < 128) && (noteNumber > -1) && (chordNote[chordType][scanSensors] > -1)) {    // we don't want to send midi out of range or play silent notes
          digitalWrite(LED_PIN, HIGH);                                // sending midi, so light up led
          if (sensedNote){
              usbMIDI.sendNoteOn(noteNumber, VELOCITY, MIDI_CH);      // send Note On, USB MIDI
              internalSineNoteOn(noteNumber-12, scanSensors);         // play sine note with teensy audio (built in DAC)
              internalStringNoteOn(noteNumber-12, scanSensors);       // play string note with teensy audio (built in DAC)
          } else {
              usbMIDI.sendNoteOff(noteNumber, VELOCITY, MIDI_CH);     // send note Off, USB MIDI
              internalNoteOff(noteNumber-12, scanSensors);            // note off for internal audio (fade out)
          }
          if (!BRIGHT_LED) digitalWrite(LED_PIN, LOW);                // led off for low brightness
        }  
        activeNote[scanSensors] = sensedNote;         
      }  
    }
    statusPreviousMillis = currentMillis;                             // reset interval timing
  }
}
// END MAIN LOOP

// Check chord keyboard, if changed shut off any active notes and replay with new chord
void readKeyboard() {
  int readChord = 0;
  int readChordType = 0;
  for (int row = 0; row < 3; row++) {     // scan keyboard rows
    enableRow(row);                       // set current row low
    if (row == 0) settingCheckPatch();
    if (row == 1) settingCheckBacking();
    for (int col = 0; col < 12; col++) {  // scan keyboard columns
      if (!digitalRead(colPin[col])) {    // is scanned pin low (active)?
        readChord = colNote[col];         // set chord base note
        readChordType |= (1 << row);      // set row bit in chord type
      }
    }
  }  
  if ((readChord != chord) || (readChordType != chordType)) { // have the values changed since last scan?
    for (int i = 0; i < PADS; i++) {
       noteNumber = START_NOTE + chord + chordNote[chordType][i];
       if ((noteNumber < 128) && (noteNumber > -1) && (chordNote[chordType][i] > -1)) {      // we don't want to send midi out of range or play silent notes
         if (activeNote[i]) {
          digitalWrite(LED_PIN, HIGH);                        // sending midi, so light up led
          usbMIDI.sendNoteOff(noteNumber, VELOCITY, MIDI_CH); // send Note Off, USB MIDI
          internalNoteOff(noteNumber-12, i);            // note off for internal audio (fade out)
          if (!BRIGHT_LED) digitalWrite(LED_PIN, LOW);        // led off for low brightness
         }
       }
    }
    for (int i = 0; i < PADS; i++) {
      noteNumber = START_NOTE + readChord + chordNote[readChordType][i];
      if ((noteNumber < 128) && (noteNumber > -1) && (chordNote[readChordType][i] > -1)) {    // we don't want to send midi out of range or play silent notes
        if (i < 4) internalBackingChordOn(noteNumber-12, i);
        if (activeNote[i]) {
          digitalWrite(LED_PIN, HIGH);                        // sending midi, so light up led
          usbMIDI.sendNoteOn(noteNumber, VELOCITY, MIDI_CH);  // send Note On, USB MIDI
          internalSineNoteOn(noteNumber-12, i);               // play sine note with teensy audio (built in DAC)
          internalStringNoteOn(noteNumber-12, i);             // play string note with teensy audio (built in DAC)
          if (!BRIGHT_LED) digitalWrite(LED_PIN, LOW);        // led off for low brightness
        }
      }
    }
    if (backing) {
      if (chordNote[readChordType][1] > -1) bacFade->fadeIn(100); else bacFade->fadeOut(500);
    }
    chord = readChord;
    chordType = readChordType;
  }
}

// Set selected row low (active), others to Hi-Z
void enableRow(int row) {
  for (int rc = 0; rc < 3; rc++) {
    if (row == rc) {
      pinMode(rowPin[rc], OUTPUT);
      digitalWrite(rowPin[rc], LOW);
    } else {
      digitalWrite(rowPin[rc], HIGH);
      pinMode(rowPin[rc], INPUT); // Put to Hi-Z for safety against shorts
    }
  }
  delayMicroseconds(30); // wait before reading ports (let ports settle after changing)
}

// MIDI note number to frequency calculation
float numToFreq(int input) {
    int number = input - 21; // set to midi note numbers = start with 21 at A0 
    number = number - 48; // A0 is 48 steps below A4 = 440hz
    return 440*(pow (1.059463094359,number));
}

// play a sine wave sound using the teensy audio library
void internalSineNoteOn(int note, int num) {
  if (midiToFreq[note] > 20.0) {
    osc[num]->frequency(midiToFreq[note]);
    fader[num]->fadeIn(ATTACK);
  }
}

void internalNoteOff(int note, int num) {
  if (midiToFreq[note] > 20.0) {
    fader[num]->fadeOut(RELEASE);
  }
}

// play a string pluck sound using the teensy audio library Karplus-Strong algorithm
void internalStringNoteOn(int note, int num) {
  if (midiToFreq[note] > 20.0) str[num]->noteOn(midiToFreq[note], 1.0);
}

void internalBackingChordOn(int note, int num) {
  if (midiToFreq[note] > 20.0) bac[num]->frequency(midiToFreq[note]);
}

void settingCheckPatch(){
  byte reading = !digitalRead(colPin[5])&&!digitalRead(colPin[8]); // if C and A (top row) is pressed simultaneously 
  if (reading != prevP){
    if (reading){
      patch = !patch; // switch patch
      xfade();        // xfade over to new patch setting
    }
  }
  prevP = reading;
}

void settingCheckBacking(){
  byte reading = !digitalRead(colPin[5])&&!digitalRead(colPin[8]); // if C and A (mid row) is pressed simultaneously 
  if (reading != prevB){
    if (reading){
      backing = !backing;   // switch backing setting
      fadeBacking();        // fade backing chord in or out
    }
  }
  prevB = reading;
  byte readingUp = !digitalRead(colPin[5])&&!digitalRead(colPin[9]); // if C and E (mid row) is pressed simultaneously 
  if (readingUp != prevBup){
    if (readingUp){
      if (bacLevel < 1) bacLevel = bacLevel+0.1; //change amplitude for backing chord
        for (int i=0; i < 4; i++){
          bac[i]->amplitude(bacLevel);
        }
    }
  }
  prevBup = readingUp;
  byte readingDn = !digitalRead(colPin[5])&&!digitalRead(colPin[7]); // if C and D (mid row) is pressed simultaneously 
  if (readingDn != prevBdn){
    if (readingDn){
      if (bacLevel > 0.1) bacLevel = bacLevel-0.1; //change amplitude for backing chord
      for (int i=0; i < 4; i++){
        bac[i]->amplitude(bacLevel);
      }
    }
  }
  prevBdn = readingDn;
}

void xfade(){
  if (patch == OMNICHORD){ // xfade to OMNICHORD sound
    oscFade->fadeIn(500);
    strFade->fadeOut(500);
  } else {                 // xfade to AUTOHARP sound
    oscFade->fadeOut(500);
    strFade->fadeIn(500);
  }
}

void fadeBacking(){
  if (!backing) bacFade->fadeOut(500); else delay(500);
}

