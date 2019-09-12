/////////////////////////////////////////////////////////////////////////
//                                                                     //
//   Teensy 3.2 chord strummer with built in audio output              //
//   by Johan Berglund, April 2017                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//#define MPR121 // Uncomment if using optional MPR121 touch sensor board 

//Compile with USB Type set to "Serial + MIDI + Audio"

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#if defined(MPR121)
#include <Adafruit_MPR121.h>
#endif

// WAV files converted to code by wav2sketch
// Minipops 7 samples downloaded from http://samples.kb6.de 

#include "AudioSampleBd808w.h"
#include "AudioSampleBongo2mp7w.h"
#include "AudioSampleClavemp7w.h"
#include "AudioSampleCowmp7w.h"
#include "AudioSampleCymbal1mp7w.h"
#include "AudioSampleGuiramp7w.h"
#include "AudioSampleMaracasmp7w.h"
#include "AudioSampleQuijadamp7w.h"

#include "patterns.h"        // Patterns for rhythm section

#define MIDI_CH 1            // MIDI channel
#define VELOCITY 64          // MIDI note velocity (64 for medium velocity, 127 for maximum)
#define START_NOTE 60        // MIDI start note (60 is middle C)
#if defined(MPR121)
#define PADS 12              // number of touch electrodes (MPR121)
#define MPR_PWR_PIN 23       // MPR121 connects to Teensy pins 18 (SDA), 19 (SCL), 23 (3.3V) and 16 (GND)
#define MPR_GND_PIN 16
#else
#define PADS 8               // number of touch electrodes (built in)
#endif
#define SET_PIN 24           // pin for settings switch
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
unsigned long stepTimerMillis = 0L;
unsigned long stepInterval = 150;   // step interval in ms, one step is a 1/16 note http://www.dvfugit.com/beats-per-minute-millisecond-delay-calculator.php


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

byte activeNote[12]      = {0,0,0,0,0,0,0,0,0,0,0,0};       // keeps track of active notes

byte sensedNote;               // current reading
byte chordButtonPressed;
byte patch = OMNICHORD;        // built in audio sound setting (default)
#if defined(MPR121)
byte reverse = 1;              // reverse strumming direction (default setting if MPR121)
#else
byte reverse = 0;              // reverse strumming direction
#endif
byte backing = 0;              // backing chord on/off 1/0 (default)
byte rhythm = 0;               // rhythm on/off  
byte gated = 1;                // gated chords if rhythm on
byte bass = 1;                 // bassline if rhythm is on
byte mode = 0;
int prevKey = -1;              // edge tracking for setting keys (-1 is no key pressed)
int prevRow = -1;
int patNum = 0;                // selected rhythm pattern
int currentStep = 0;
int noteNumber;                // calculated midi note number
int chord = 0;                 // chord key setting (base note of chord)
int chordType = 0;             // chord type (maj, min, 7th...)

int chordNote[8][16] = {                               //chord notes for each pad/string (up to 16)
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0, 4, 7,12,16,19,24,28,31,36,40,43,48,52,55,60 },  //maj 
  { 0, 3, 7,12,15,19,24,27,31,36,39,43,48,51,55,60 },  //min 
  { 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,45 },  //dim 
  { 0, 4, 7,10,12,16,19,22,24,28,31,34,36,40,43,46 },  //7th 
  { 0, 4, 7,11,12,16,19,23,24,28,31,35,36,40,43,47 },  //maj7
  { 0, 3, 7,10,12,15,19,22,24,27,31,34,36,39,43,46 },  //m7  
  { 0, 4, 8,12,16,20,24,28,32,36,40,44,48,52,56,60 }   //aug  
};

int chordNoteTers[8][16] = {                               //chord notes for each pad/string (up to 16)
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0,-8,-5,12, 4, 7,24,16,19,36,28,31,48,40,43,60 },  //maj <
  { 0,-9,-5,12, 3, 7,24,15,19,36,27,31,48,39,43,60 },  //min <
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0,-8,-2,12, 4,10,24,16,22,36,28,34,48,40,46,60 },  //7th <
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent 
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
};

int chordNoteGrund[8][16] = {                               //chord notes for each pad/string (up to 16)
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0, 4, 7,12,16,19,24,28,31,36,40,43,48,52,55,60 },  //maj 
  { 0, 3, 7,12,15,19,24,27,31,36,39,43,48,51,55,60 },  //min 
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0, 4,10,12,16,22,24,28,34,36,40,46,48,52,58,60 },  //7th 
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent  
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent 
};

int chordNoteKvint[8][16] = {                               //chord notes for each pad/string (up to 16)
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0, 4,-5,12,16, 7,24,28,19,36,40,31,48,52,43,60 },  //maj 
  { 0, 3,-5,12,15, 7,24,27,19,36,39,31,48,51,43,60 },  //min 
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  { 0, 4,-2,12,16,10,24,28,22,36,40,34,48,52,46,60 },  //7th 
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent  
  {-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111,-111 },  //silent 
};

float midiToFreq[128];         // for storing pre calculated frequencies for note numbers
float strLevel = 0.8;          // strummer volume level
float bacLevel = 0.3;          // backing chord volume level
float rtmLevel = 0.6;          // rhythm volume level
float basLevel = 0.6;          // bassline volume level

#if defined(MPR121)
uint16_t touchValue;
Adafruit_MPR121          capTouch = Adafruit_MPR121();      // MPR121 connected to pins otherwise used for built in cap touch 
#else
byte sensorPin[8]       = {1,0,23,22,19,18,17,16};          // teensy touch input pins
#endif

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
AudioPlayMemory          playMem1;       //xy=261,1004
AudioPlayMemory          playMem2;       //xy=262,1041
AudioSynthWaveform       waveform4;      //xy=263,948
AudioPlayMemory          playMem3;       //xy=263,1076
AudioPlayMemory          playMem4;       //xy=265,1112
AudioPlayMemory          playMem5;       //xy=266,1145
AudioPlayMemory          playMem6;       //xy=268,1178
AudioPlayMemory          playMem7;       //xy=271,1211
AudioPlayMemory          playMem8;       //xy=273,1244
AudioEffectFade          fade1;          //xy=283,62
AudioEffectFade          fade2;          //xy=290,105
AudioEffectFade          fade3;          //xy=299,150
AudioSynthWaveform       waveform5;      //xy=303,1365
AudioEffectFade          fade4;          //xy=317,199
AudioEffectFade          fade5;          //xy=331,244
AudioEffectFade          fade6;          //xy=346,293
AudioEffectFade          fade7;          //xy=359,335
AudioEffectFade          fade8;          //xy=375,381
AudioMixer4              mixer1;         //xy=537,138
AudioMixer4              mixer2;         //xy=577,240
AudioSynthWaveformSine   sine9;          //xy=578,314
AudioSynthWaveformSine   sine10;         //xy=591,354
AudioFilterStateVariable filter1;        //xy=593,1323
AudioSynthWaveformSine   sine11;         //xy=605,391
AudioSynthWaveformSine   sine12;         //xy=618,426
AudioSynthKarplusStrong  string9;        //xy=640,673
AudioSynthKarplusStrong  string10;       //xy=646,708
AudioSynthKarplusStrong  string11;       //xy=652,742
AudioSynthKarplusStrong  string12;       //xy=662,775
AudioEffectFade          fade13;         //xy=709,320
AudioMixer4              mixer3;         //xy=727,183
AudioEffectFade          fade14;         //xy=743,357
AudioEffectFade          fade15;         //xy=753,394
AudioEffectFade          fade16;         //xy=786,427
AudioMixer4              mixer4;         //xy=801,490
AudioMixer4              mixer5;         //xy=828,609
AudioMixer4              mixer12;        //xy=855,276
AudioMixer4              mixer8;         //xy=872,810
AudioMixer4              mixer13;        //xy=876,690
AudioMixer4              mixer9;         //xy=893,1013
AudioMixer4              mixer10;        //xy=905,1109
AudioEffectFade          fade12;         //xy=959,1227
AudioMixer4              mixer6;         //xy=1015,538
AudioEffectFade          fade9;          //xy=1100,214
AudioMixer4              mixer11;        //xy=1104,1035
AudioEffectFade          fade10;         //xy=1150,318
AudioEffectFade          fade11;         //xy=1201,687
AudioMixer4              mixer7;         //xy=1353,276
AudioOutputUSB           usb1;           //xy=1503.333333333333,413.33333333333326
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
AudioConnection          patchCord20(playMem1, 0, mixer9, 0);
AudioConnection          patchCord21(playMem2, 0, mixer9, 1);
AudioConnection          patchCord22(waveform4, 0, mixer8, 3);
AudioConnection          patchCord23(playMem3, 0, mixer9, 2);
AudioConnection          patchCord24(playMem4, 0, mixer9, 3);
AudioConnection          patchCord25(playMem5, 0, mixer10, 0);
AudioConnection          patchCord26(playMem6, 0, mixer10, 1);
AudioConnection          patchCord27(playMem7, 0, mixer10, 2);
AudioConnection          patchCord28(playMem8, 0, mixer10, 3);
AudioConnection          patchCord29(fade1, 0, mixer1, 0);
AudioConnection          patchCord30(fade2, 0, mixer1, 1);
AudioConnection          patchCord31(fade3, 0, mixer1, 2);
AudioConnection          patchCord32(waveform5, 0, filter1, 0);
AudioConnection          patchCord33(fade4, 0, mixer1, 3);
AudioConnection          patchCord34(fade5, 0, mixer2, 0);
AudioConnection          patchCord35(fade6, 0, mixer2, 1);
AudioConnection          patchCord36(fade7, 0, mixer2, 2);
AudioConnection          patchCord37(fade8, 0, mixer2, 3);
AudioConnection          patchCord38(mixer1, 0, mixer3, 0);
AudioConnection          patchCord39(mixer2, 0, mixer3, 1);
AudioConnection          patchCord40(sine9, fade13);
AudioConnection          patchCord41(sine10, fade14);
AudioConnection          patchCord42(filter1, 0, fade12, 0);
AudioConnection          patchCord43(sine11, fade15);
AudioConnection          patchCord44(sine12, fade16);
AudioConnection          patchCord45(string9, 0, mixer13, 0);
AudioConnection          patchCord46(string10, 0, mixer13, 1);
AudioConnection          patchCord47(string11, 0, mixer13, 2);
AudioConnection          patchCord48(string12, 0, mixer13, 3);
AudioConnection          patchCord49(fade13, 0, mixer12, 0);
AudioConnection          patchCord50(mixer3, fade9);
AudioConnection          patchCord51(fade14, 0, mixer12, 1);
AudioConnection          patchCord52(fade15, 0, mixer12, 2);
AudioConnection          patchCord53(fade16, 0, mixer12, 3);
AudioConnection          patchCord54(mixer4, 0, mixer6, 0);
AudioConnection          patchCord55(mixer5, 0, mixer6, 1);
AudioConnection          patchCord56(mixer12, 0, mixer3, 2);
AudioConnection          patchCord57(mixer8, fade11);
AudioConnection          patchCord58(mixer13, 0, mixer6, 2);
AudioConnection          patchCord59(mixer9, 0, mixer11, 0);
AudioConnection          patchCord60(mixer10, 0, mixer11, 1);
AudioConnection          patchCord61(fade12, 0, mixer11, 2);
AudioConnection          patchCord62(mixer6, fade10);
AudioConnection          patchCord63(fade9, 0, mixer7, 0);
AudioConnection          patchCord64(mixer11, 0, mixer7, 3);
AudioConnection          patchCord65(fade10, 0, mixer7, 1);
AudioConnection          patchCord66(fade11, 0, mixer7, 2);
AudioConnection          patchCord67(mixer7, dac1);
AudioConnection          patchCord68(mixer7, 0, usb1, 0);
AudioConnection          patchCord69(mixer7, 0, usb1, 1);
// GUItool: end automatically generated code




// Pointers
AudioSynthWaveformSine*     osc[12]   {&sine1,&sine2,&sine3,&sine4,&sine5,&sine6,&sine7,&sine8,&sine9,&sine10,&sine11,&sine12};
AudioEffectFade*            fader[12] {&fade1,&fade2,&fade3,&fade4,&fade5,&fade6,&fade7,&fade8,&fade13,&fade14,&fade15,&fade16};
AudioSynthKarplusStrong*    str[12]   {&string1,&string2,&string3,&string4,&string5,&string6,&string7,&string8,&string9,&string10,&string11,&string12};
AudioSynthWaveform*         bac[4]    {&waveform1,&waveform2,&waveform3,&waveform4};
AudioPlayMemory*            rtm[8]    {&playMem1,&playMem2,&playMem3,&playMem4,&playMem5,&playMem6,&playMem7,&playMem8};
AudioSynthWaveform*         bas     = &waveform5;
AudioEffectFade*            oscFade = &fade9;
AudioEffectFade*            strFade = &fade10;
AudioEffectFade*            bacFade = &fade11;
AudioEffectFade*            basFade = &fade12;
AudioFilterStateVariable*   basFltr = &filter1;


// SETUP
void setup() {
  pinMode(LED_PIN, OUTPUT);
  #if defined(MPR121)
  pinMode(MPR_PWR_PIN, OUTPUT);
  pinMode(MPR_GND_PIN, OUTPUT);
  digitalWrite(MPR_PWR_PIN, HIGH);
  digitalWrite(MPR_GND_PIN, LOW);
  delay(100);
  if (!capTouch.begin(0x5A)) { // if MPR121 board is not found, just do nothing
    while (1);
  }
  blink121(); // indicate running with 1-2-1 LED flash if using MPR121
  #else
  blink32();  // indicate running with 3-2 LED flash if running default Teensy 3.2 touch sensing
  #endif
  for (int i = 0; i < 12; i++) {
     pinMode(colPin[i],INPUT_PULLUP);
  }
    for (int i = 0; i < 3; i++) {
     pinMode(rowPin[i],OUTPUT);
     digitalWrite(rowPin[i],LOW);
  }  
  for(int i=0;i<128;i++) {  // set up table, midi note number to frequency
      midiToFreq[i] = numToFreq(i);
  }
  pinMode(SET_PIN, INPUT_PULLUP);
  AudioMemory(50);
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
  mixer12.gain(0, 0.27);
  mixer12.gain(1, 0.27);
  mixer12.gain(2, 0.27);
  mixer12.gain(3, 0.27);
  mixer3.gain(0, 0.5);
  mixer3.gain(1, 0.5);
  mixer3.gain(2, 0.5);
  mixer4.gain(0, 0.27);
  mixer4.gain(1, 0.27);
  mixer4.gain(2, 0.27);
  mixer4.gain(3, 0.27);
  mixer5.gain(0, 0.27);
  mixer5.gain(1, 0.27);
  mixer5.gain(2, 0.27);
  mixer5.gain(3, 0.27);
  mixer13.gain(0, 0.27);
  mixer13.gain(1, 0.27);
  mixer13.gain(2, 0.27);
  mixer13.gain(3, 0.27);
  mixer6.gain(0, 0.5);
  mixer6.gain(1, 0.5);
  mixer6.gain(2, 0.5);
  mixer8.gain(0, 0.27);
  mixer8.gain(1, 0.27);
  mixer8.gain(2, 0.27);
  mixer8.gain(3, 0.27);
  mixer9.gain(0, 0.5);
  mixer9.gain(1, 0.5);
  mixer9.gain(2, 0.5);
  mixer9.gain(3, 0.5);
  mixer10.gain(0, 0.5);
  mixer10.gain(1, 0.5);
  mixer10.gain(2, 0.5);
  mixer10.gain(3, 0.5);
  mixer11.gain(0, rtmLevel);
  mixer11.gain(1, rtmLevel);
  mixer11.gain(2, basLevel);
  mixer7.gain(0, strLevel);
  mixer7.gain(1, strLevel);
  mixer7.gain(2, 0.27);
  mixer7.gain(3, 0.5);
  delay(100);
  basFltr->frequency(220);
  basFade->fadeOut(1);
  bas->begin(WAVEFORM_SAWTOOTH);
  bas->amplitude(0.8);
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
    if (!digitalRead(SET_PIN)) {
      readSettings();                                                    // settings key pressed, read settings from keyboard
    } else {
      readKeyboard();                                                    // read keyboard input and replay active notes (if any) with new chording
    }
    #if defined(MPR121)
    touchValue = capTouch.touched();
    #endif
    for (int scanSensors = 0; scanSensors < PADS; scanSensors++) {       // scan sensors for changes and send note on/off accordingly
      #if defined(MPR121)
      sensedNote = ((touchValue >> scanSensors) & 0x01);                 // read touch pad/pin/electrode/string/whatever (MPR121 cap touch)
      #else
      sensedNote = (touchRead(sensorPin[scanSensors]) > TOUCH_THR);      // read touch pad/pin/electrode/string/whatever (internal cap touch)
      #endif
      if (sensedNote != activeNote[scanSensors]) {
        if (reverse){
          noteNumber = START_NOTE + chord + omniChordNote(chordType,PADS-1-scanSensors,chord);
        } else {
          noteNumber = START_NOTE + chord + omniChordNote(chordType,scanSensors,chord);
        }
        if ((noteNumber < 128) && (noteNumber > -1) && (omniChordNote(chordType,scanSensors,chord) > -110)) {    // we don't want to send midi out of range or play silent notes
          digitalWrite(LED_PIN, HIGH);                                // sending midi, so light up led
          if (sensedNote){
              usbMIDI.sendNoteOn(noteNumber, VELOCITY, MIDI_CH);      // send Note On, USB MIDI
              internalSineNoteOn(noteNumber-12, scanSensors);         // play sine note with teensy audio (built in DAC)
              #if defined(MPR121)
              internalStringNoteOn(noteNumber-24, scanSensors);       // play string note with teensy audio (built in DAC)
              #else
              internalStringNoteOn(noteNumber-12, scanSensors);       // play string note with teensy audio (built in DAC)
              #endif
          } else {
              usbMIDI.sendNoteOff(noteNumber, VELOCITY, MIDI_CH);     // send note Off, USB MIDI
              internalSineNoteOff(scanSensors);                       // note off for internal audio (fade out)
          }
          if (!BRIGHT_LED) digitalWrite(LED_PIN, LOW);                // led off for low brightness
        }  
        activeNote[scanSensors] = sensedNote;         
      }  
    }
    statusPreviousMillis = currentMillis;                             // reset interval timing
  }
  if (rhythm) {
    if ((unsigned long)(currentMillis - stepTimerMillis) >= stepInterval) {
      for (int i = 0; i < 8; i++){
        if (bitRead(pattern[patNum][currentStep],7-i)) playRtm(i);
      }
      if (gated){
        if (chordButtonPressed && gatePattern[patNum][currentStep]) bacFade->fadeIn(10); else bacFade->fadeOut(200); // play backing chord if a chord key is pressed
      }
      if (bass){ //play bassline notes
        if (chordButtonPressed && (bassPattern[patNum][currentStep] > -110)){
          internalBassNoteOn(START_NOTE + chord + chordNote[chordType][bassPattern[patNum][currentStep]]-24);
          basFade->fadeIn(1);
        }  else {
          basFade->fadeOut(200);
        }
      }
      stepTimerMillis = currentMillis;
      currentStep++;
      if ((currentStep == 16) || pattern[patNum][currentStep] == 255) currentStep = 0; // start over at step 0 if we passed 15 or next step pattern value is 255 (reset)
    }
  }  
  while (usbMIDI.read()) {
    // read & ignore incoming messages
  }
}
// END MAIN LOOP

// Check chord keyboard, if changed shut off any active notes and replay with new chord
void readKeyboard() {
  int readChord = 0;
  int readChordType = 0;
  for (int row = 0; row < 3; row++) {     // scan keyboard rows
    enableRow(row);                       // set current row low
    for (int col = 0; col < 12; col++) {  // scan keyboard columns
      if (!digitalRead(colPin[col])) {    // is scanned pin low (active)?
        readChord = colNote[col];         // set chord base note
        readChordType |= (1 << row);      // set row bit in chord type
      }
    }
  }  
  if ((readChord != chord) || (readChordType != chordType)) { // have the values changed since last scan?
    for (int i = 0; i < PADS; i++) {
       if (reverse) {
         noteNumber = START_NOTE + chord + omniChordNote(chordType,PADS - 1 - i,chord);
       } else {
         noteNumber = START_NOTE + chord + omniChordNote(chordType,i,chord);
       }
       if ((noteNumber < 128) && (noteNumber > -1) && (omniChordNote(chordType,i,chord) > -110)) {      // we don't want to send midi out of range or play silent notes
         if (activeNote[i]) {
          digitalWrite(LED_PIN, HIGH);                        // sending midi, so light up led
          usbMIDI.sendNoteOff(noteNumber, VELOCITY, MIDI_CH); // send Note Off, USB MIDI
          internalSineNoteOff(i);                             // note off for internal audio (fade out)
          if (!BRIGHT_LED) digitalWrite(LED_PIN, LOW);        // led off for low brightness
         }
       }
    }
    for (int i = 0; i < PADS; i++) {
      if (reverse) {
        noteNumber = START_NOTE + readChord + omniChordNote(readChordType,PADS - 1 - i,readChord);
      } else {
        noteNumber = START_NOTE + readChord + omniChordNote(readChordType,i,readChord);
      }
      if ((noteNumber < 128) && (noteNumber > -1) && (omniChordNote(readChordType,i,readChord) > -110)) {    // we don't want to send midi out of range or play silent notes
        if (reverse && ((PADS - 1 - i) < 4)) internalBackingChordOn(noteNumber-12, PADS - 1 - i);
        if (!reverse && (i < 4)) internalBackingChordOn(noteNumber-12, i); 
        if (activeNote[i]) {
          digitalWrite(LED_PIN, HIGH);                        // sending midi, so light up led
          usbMIDI.sendNoteOn(noteNumber, VELOCITY, MIDI_CH);  // send Note On, USB MIDI
          internalSineNoteOn(noteNumber-12, i);               // play sine note with teensy audio (built in DAC)
          internalStringNoteOn(noteNumber-12, i);             // play string note with teensy audio (built in DAC)
          if (!BRIGHT_LED) digitalWrite(LED_PIN, LOW);        // led off for low brightness
        }
      }
    }
    chordButtonPressed =  (omniChordNote(readChordType,1,readChord) > -110);
    if (backing && !(rhythm && gated)) {
       if (chordButtonPressed) bacFade->fadeIn(50); else bacFade->fadeOut(500); // play backing chord if a chord key is pressed
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

void internalSineNoteOff(int num) {
  fader[num]->fadeOut(RELEASE);
}

// play a string pluck sound using the teensy audio library Karplus-Strong algorithm
void internalStringNoteOn(int note, int num) {
  if (midiToFreq[note] > 20.0) str[num]->noteOn(midiToFreq[note], 1.0);
}

void internalBackingChordOn(int note, int num) {
  if (midiToFreq[note] > 20.0) bac[num]->frequency(midiToFreq[note]);
}

void internalBassNoteOn(int note) {
  if (midiToFreq[note] > 20.0) bas->frequency(midiToFreq[note]);
}

// play rhythm samples
void playRtm(int i){
  switch(i){
    case 0:
      rtm[i]->play(AudioSampleGuiramp7w); //GU
      break;
    case 1:
      rtm[i]->play(AudioSampleBongo2mp7w); //BG2
      break;
    case 2:
      rtm[i]->play(AudioSampleBd808w); // BD
      break;
    case 3:
      rtm[i]->play(AudioSampleClavemp7w); // CL
      break;
    case 4:
      rtm[i]->play(AudioSampleCowmp7w); // CW
      break;     
    case 5:
      rtm[i]->play(AudioSampleMaracasmp7w); // MA
      break;  
    case 6:
      rtm[i]->play(AudioSampleCymbal1mp7w); // CY
      break;    
    case 7:
      rtm[i]->play(AudioSampleQuijadamp7w); // QU
      break;           
  }
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

void blink121(){
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
}

void blink32(){
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
}

// Change settings using keyboard keys
void readSettings() {
  int readKey = -1;
  int readRow = -1;
  for (int row = 0; row < 3; row++) {     // scan keyboard rows
    enableRow(row);                       // set current row low
    for (int col = 0; col < 12; col++) {  // scan keyboard columns
      if (!digitalRead(colPin[col])) {    // is scanned pin low (active)?
        readKey = col;                    // set pressed key
        readRow = row;                    // set row bit in chord type
      }
    }
  }  
  if ((readKey != prevKey) || (readRow != prevRow)) { // have the values changed since last scan?
    if (readKey > -1) { // if there is a key pressed
      if (readRow == 0) {
        // switch case
        switch(readKey){
          case 0:
            //do stuff
            break;
          case 1:
            mode = !mode; //switch between standard T.Chordstrum note order and Omnichord style note order
            break;
          case 2:
            //do stuff
            break;
          case 3:
            //do stuff
            break;
          case 4:
            if (strLevel > 0.2) strLevel -= 0.1; //strummer vol -
            mixer7.gain(0, strLevel);
            mixer7.gain(1, strLevel);
            break;
          case 5:
            patch = !patch;                      // switch patch
            xfade();                             // xfade over to new patch setting
            break;
          case 6:
            if (strLevel < 1.0) strLevel += 0.1; //strummer vol +
            mixer7.gain(0, strLevel);
            mixer7.gain(1, strLevel);
            break;
          case 7:
            if (basLevel > 0.2) basLevel -=0.1; //bassline vol -
            mixer11.gain(2, basLevel);
            break;
          case 8:
            bass = !bass;                       // switch bass setting
            if (!bass) basFade->fadeOut(200);   // fade bass out if switched off
            break;
          case 9:
            if (basLevel < 1) basLevel +=0.1;   //bassline vol +
            mixer11.gain(2, basLevel);
            break;
          case 10:
            reverse = !reverse;
            break;
          case 11:
            //do stuff
            break;
        }
      } else if (readRow == 1) {
        // switch case
        switch(readKey){
          case 0:
            //do stuff
            break;
          case 1:
            //do stuff
            break;
          case 2:
            //do stuff
            break;
          case 3:
            //do stuff
            break;
          case 4:
            if (bacLevel > 0.2) bacLevel -= 0.1; // change amplitude for backing chord
            for (int i=0; i < 4; i++){
              bac[i]->amplitude(bacLevel);
            }
            break;
          case 5:
            backing = !backing;                  // switch backing setting
            if (!backing) bacFade->fadeOut(500); // fade backing chord out if switched off
            break;
          case 6:
            if (bacLevel < 0.9) bacLevel += 0.1; // change amplitude for backing chord
            for (int i=0; i < 4; i++){
              bac[i]->amplitude(bacLevel);
            }
            break;
          case 7:
            if (stepInterval < 300) stepInterval += 5; //tempo dn (min 50 bpm)
            break;
          case 8:
            gated = !gated;
            break;
          case 9:
            if (stepInterval > 50) stepInterval -= 5; //tempo up (max 200 bpm)
            break;
          case 10:
            //do stuff
            break;
          case 11:
            //do stuff
            break;
        }
      } else if (readRow == 2) {
        // switch case
        switch(readKey){
          case 0:
            //do stuff
            break;
          case 1:
            //do stuff
            break;
          case 2:
            //do stuff
            break;
          case 3:
            //do stuff
            break;
          case 4:
            if (rtmLevel > 0.2) rtmLevel -= 0.1; //rtm vol -
            //mixer7.gain(3, rtmLevel);
            mixer11.gain(0, rtmLevel);
            mixer11.gain(1, rtmLevel);
            break;
          case 5:
            rhythm = !rhythm;                    //rtm on/off
            if (rhythm) currentStep = 0;
            break;
          case 6:
            if (rtmLevel < 1) rtmLevel += 0.1;   //rtm vol+
            //mixer7.gain(3, rtmLevel);
            mixer11.gain(0, rtmLevel);
            mixer11.gain(1, rtmLevel);
            break;
          case 7:
            if (patNum > 0) patNum--;            // select previous pattern
            break;
          case 8:
            //do stuff
            break;
          case 9:
            if (patNum < 15) patNum++;           // select next pattern
            break;
          case 10:
            //do stuff
            break;
          case 11:
            //do stuff
            break;
        }
      }
    }
    prevKey = readKey;
    prevRow = readRow;
  }
}

int omniChordNote(int type, int note, int chord){
   // chord    Db, Ab, Eb, Bb,  F,  C,  G,  D,  A,  E,  B, F#
   // col/note  1,  8,  3, 10,  5,  0,  7,  2,  9,  4, 11,  6
  int outnote = -111;
  if (mode){ // Omnichord style note order
    switch(chord){
      case 1: //Db
        outnote = chordNoteKvint[type][note];
        break;
      case 8: //Ab
        outnote = chordNoteGrund[type][note];
        break;
      case 3: //Eb
        outnote = chordNoteTers[type][note]; // <
        break;
      case 10: //Bb
        outnote = chordNoteGrund[type][note]; // <
        break;
      case 5: //F
        outnote = chordNoteTers[type][note]; // <
        break;
      case 0: //C
        outnote = chordNoteKvint[type][note]; // <
        break;
      case 7: //G
        outnote = chordNoteGrund[type][note]; // <
        break;
      case 2: //D
        outnote = chordNoteTers[type][note]; // <
        break;
      case 9: //A
        outnote = chordNoteGrund[type][note]; // <
        break;
      case 4: //E
        outnote = chordNoteTers[type][note]; // <
        break;
      case 11: //B
        outnote = chordNoteKvint[type][note]; // <
        break;
      case 6: //F#
        outnote = chordNoteGrund[type][note];
        break;
    }
  } else outnote = chordNote[type][note]; // T.Chordstrum standard note order
  return outnote;
}

