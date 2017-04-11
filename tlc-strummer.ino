/////////////////////////////////////////////////////////////////////////
//                                                                     //
//   Teensy LC chord strummer by Johan Berglund, April 2017            //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#define MIDI_CH 1            // MIDI channel
#define VELOCITY 64          // MIDI note velocity (64 for medium velocity, 127 for maximum)
#define START_NOTE 60        // MIDI start note (middle C)
#define PADS 8               // number of touch electrodes
#define LED_PIN 13           // LED to indicate midi activity (just a short blip)

#define CHECK_INTERVAL 5     // interval in ms for sensor check

unsigned long currentMillis = 0L;
unsigned long statusPreviousMillis = 0L;

byte colPin[8]          = {5,6,7,8,9,10,11,12};             // teensy digital input pins for keyboard columns
                                                            // (not using pin 2 so you can use same hardware as original version)
byte colNote[8]         = {10,5,0,7,2,9,4,11};              // column to note number                                            
                                                            // column setup for omnichord style (circle of fifths)
                                                            // chord    Db, Ab, Eb, Bb,  F,  C,  G,  D,  A,  E,  B, F#
                                                            // col/note  1,  8,  3, 10,  5,  0,  7,  2,  9,  4, 11,  6
                                                            // for chromatic order, C to B, straight order 0 to 11

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

byte sensorPin[8]       = {1,0,23,22,19,18,17,16}; // teensy lc touch input pins

byte activeNote[8]      = {0,0,0,0,0,0,0,0}; // keeps track of active notes
byte sensedNote;            // current reading
int noteNumber;             // calculated midi note number
int chord = 0;              // chord key setting (base note of chord)
int chordType = 0;          // chord type (maj, min, 7th...)
int octave = 0;             // octave setting
int transposition = 0;      // transposition setting 


int chordNote[8][16] = {                               //chords for up to 16 "strings" (pads), only first 8 used here
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },  //silent
  { 0, 4, 7,12,16,19,24,28,31,36,40,43,48,52,55,60 },  //maj 
  { 0, 3, 7,12,15,19,24,27,31,36,39,43,48,51,55,60 },  //min 
  { 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,45 },  //dim 
  { 0, 4, 7,10,12,16,19,22,24,28,31,34,36,40,43,46 },  //7th 
  { 0, 4, 7,11,12,16,19,23,24,28,31,35,36,40,43,47 },  //maj7
  { 0, 3, 7,10,12,15,19,22,24,27,31,34,36,39,43,46 },  //m7  
  { 0, 4, 8,12,16,20,24,28,32,36,40,44,48,52,56,60 }   //aug  
};

// SETUP
void setup() {
  for (int i = 0; i < 8; i++) {
     pinMode(colPin[i],INPUT_PULLUP);
  }
    for (int i = 0; i < 3; i++) {
     pinMode(rowPin[i],OUTPUT);
     digitalWrite(rowPin[i],LOW);
  }
  pinMode(LED_PIN, OUTPUT);
}

// MAIN LOOP
void loop() {
  currentMillis = millis();
  if ((unsigned long)(currentMillis - statusPreviousMillis) >= CHECK_INTERVAL) {
    setNoteParamsPlay();                                                 // adjust note selection parameters with note-offs for previous params and note-ons for new
    for (int scanSensors = 0; scanSensors < PADS; scanSensors++) {       // scan sensors for changes and send note on/off accordingly
      sensedNote = (touchRead(sensorPin[scanSensors]) > 1800);           // read touch pad
      if (sensedNote != activeNote[scanSensors]) {
        noteNumber = START_NOTE + chord + chordNote[chordType][scanSensors] + octave + transposition;
        if ((noteNumber < 128) && (noteNumber > -1) && (chordNote[chordType][scanSensors] > -1)) {    // we don't want to send midi out of range or play silent notes
          if (sensedNote){
              digitalWrite(LED_PIN, HIGH);
              usbMIDI.sendNoteOn(noteNumber, VELOCITY, MIDI_CH);      // send Note On, USB MIDI
              digitalWrite(LED_PIN, LOW);
          } else {
              digitalWrite(LED_PIN, HIGH);
              usbMIDI.sendNoteOff(noteNumber, VELOCITY, MIDI_CH);     // send note Off, USB MIDI
              digitalWrite(LED_PIN, LOW);
          }
        }  
        activeNote[scanSensors] = sensedNote;         
      }  
    }
    statusPreviousMillis = currentMillis;                                 // reset interval timing
  }
}
// END MAIN LOOP

// Check chord keyboard and potentiometers, if changed shut off any active notes and replay with new settings
void setNoteParamsPlay() {
  int rePlay = 0;
  int readChord = 0;
  int readChordType = 0;
  for (int row = 0; row < 3; row++) {     // scan keyboard rows
    enableRow(row);                       // set current row low
    for (int col = 0; col < 8; col++) {   // scan keyboard columns
      if (!digitalRead(colPin[col])) {    // is scanned pin low (active)?
        readChord = colNote[col];         // set chord base note
        readChordType |= (1 << row);      // set row bit in chord type
      }
    }
  }
  if ((readChord != chord) || (readChordType != chordType)) {   // have the values changed since last scan?
    rePlay = 1;
  }  
  if (rePlay) {
    for (int i = 0; i < PADS; i++) {
       noteNumber = START_NOTE + chord + chordNote[chordType][i] + octave + transposition;
       if ((noteNumber < 128) && (noteNumber > -1) && (chordNote[chordType][i] > -1)) {      // we don't want to send midi out of range or play silent notes
         if (activeNote[i]) {
          digitalWrite(LED_PIN, HIGH);
          usbMIDI.sendNoteOff(noteNumber, VELOCITY, MIDI_CH); // send Note Off, USB MIDI
          digitalWrite(LED_PIN, LOW);
         }
       }
    }
    for (int i = 0; i < PADS; i++) {
      noteNumber = START_NOTE + readChord + chordNote[readChordType][i] + octave + transposition;
      if ((noteNumber < 128) && (noteNumber > -1) && (chordNote[readChordType][i] > -1)) {    // we don't want to send midi out of range or play silent notes
        if (activeNote[i]) {
          digitalWrite(LED_PIN, HIGH);
          usbMIDI.sendNoteOn(noteNumber, VELOCITY, MIDI_CH);  // send Note On, USB MIDI
          digitalWrite(LED_PIN, LOW);
        }
      }
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

