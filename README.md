# tlc-strummer

This is the firmware for T. Chordstrum - USB MIDI chord strummer for Teensy LC and 3.2

https://hackaday.io/project/25111-t-chordstrum

Chord strummer with a 12x3 keyboard based on the circle of fifths, like the Omnichord. For strumming the chords, a set of eight touch sensitive pads is used. 

Chords are selected as follows:

Columns from left to right selects the key
Db, Ab, Eb, Bb, F, C, G, D, A, E, B, F#

Rows select the kind of chord you play
top row     - major,
mid row     - minor,
bottom row  - 7th

Additional chord combinations are
top+bottom  - major 7th,
mid+bottom  - minor 7th,
top+mid     - diminished,
all three   - augmented


Using the T. Chordstrum:

1. As a USB MIDI controller

Connect the T. Chordstrum via micro USB cable to a computer or other host device with a midi synth software installed and running.
Select "Teensy MIDI" as MIDI input in synth software if necessary.
Press and hold chord buttons and strum with a finger over the touch pads to play.



2. With built in audio (Teensy 3.2 only)

Connect micro USB port of Teensy to a 5V USB power source like a power bank, USB charger or a computer. You may also power the Cordstrum using a 3.6V to 6V DC power source (like a 3xAAA battery holder with switch) soldered to the dedicated input pin holes located under the Teensy on the T. Cordstrum pcb. If using this alternate way for power, do not connect USB power at the same time, unless Vin is separated from Vusb on the Teensy pcb (see Teensy documentation).
Connect the T. Chordstrum 3.5mm line level audio output to powered computer speakers, mixer line input, or other amplification device with line level input. The line level output of the Teensy is not able to directly power headphones or speakers.
Mind that the instrument emits a short burst of noise when powering up. If possible, connect the audio cable or switch the amplifier on after the power on LED blinks of the Teensy.
The default setting of the audio is playing the Omnichord style strummed sound, with no backing chord or rhythm activated. To change these settings, refer to the settings chart. You press and hold the “set” button of the Chordstrum, and then momentarily press the button corresponding to the setting you want. Release the “set” key when you are done changing settings.
There are 16 rhytm patterns available. They may or may not be accurate to their descriptions :) They are named as follows: Hard rock (default), Disco, Reggae, Rock, Samba, Rumba, Cha-Cha, Swing, Bossa Nova, Beguine, Synthpop, Boogie, Waltz, Jazz rock, Slow rock and Oxygene.


These are the audio capabilities of the Teensy 3.2 version (may change in later versions):

Strummed sound, Omnichord like sinewave sound or Autoharp like plucked string sound (Karplus-Strong algorithm). Adjustable volume. Press and hold a chord key, strum with a finger over the touch pads of the strumming area to play.
Backing chord sound. Default off. Plays when a chord button is pressed and held. This is also played when rhythm is activated, but in a gated pattern, with a separate control for switching on and off. It is on by default. Volume for the backing chord can also be adjusted.
Rhythm. The Chordstrum uses samples from the Keio (Korg) Minipops 7 for a nice retro vibe. You can adjust tempo and volume, and choose between 16 different rhythm patterns (bass and gated backing chord patterns follow this selection).
Bass sound. This follows the rhythm pattern, can be switched off/on and adjusted in volume.


To switch between autoharp and omnichord type sound, press setting switch and top row (major) C together.

To switch on/off a backing chord that is played at key press, press setting switch and mid row (minor) C together. Adjust volume for backing chord using set+Fm (down) and set+Gm (up). (Default on.)

To switch on/off rhythm part, press set+C7. Rhythm part volume is adjusted down with set+F7 and up with set+G7. Next pattern is selected with set+E7 and previous pattern with set+D7. Tempo is controlled with set+Dm (down) and set+Em (up).

As default, with rhythm on, a backing chord is played, gated in a suiting pattern. To switch this off and on, use set+Am.

For the bassline patterns of the rhythm function, use set+D and set+E for volume, and set+A for on/off.

Reverse the strumming direction with set+B.

Settings chart:

[      ][      ][      ][      ][str v-][str mod][str v+][bas v-][bas 1/0][bas v+][revers][      ]

[      ][      ][      ][      ][bac v-][bac 1/0][bac v+][tempo-][bac gat][tempo+][      ][      ]

[      ][      ][      ][      ][rtm v-][rtm 1/0][rtm v+][r pat-][       ][r pat+][      ][      ]




