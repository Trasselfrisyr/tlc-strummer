# tlc-strummer
USB MIDI chord strummer for Teensy LC

Simple chord strummer with a 8x3 keyboard based on the circle of fifths, like the Omnichord. For strumming the chords, a set of eight touch sensitive pads, strings or similar is used. In my build I simply used a 90 deg pin header for touch electrodes.

Chords are selected as follows:

Columns from left to right selects the key
Bb, F, C, G, D, A, E, B

Rows select the kind of chord you play
top row     - major
mid row     - minor
bottom row  - 7th

Additional chord combinations are
top+bottom  - major 7th
mid+bottom  - minor 7th
top+mid     - diminished
all three   - augmented

How to use:

Connect USB to computer or other host device with a midi synth software installed and running. 

Select "Teensy MIDI" as MIDI input in synth software if necessary.

Press and hold chord buttons and strum with a finger over the touch pads to play.
