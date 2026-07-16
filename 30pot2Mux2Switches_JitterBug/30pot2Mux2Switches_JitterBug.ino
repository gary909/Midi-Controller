/*
┏┳┓╻╺┳┓╻   ┏━╸┏━┓┏┓╻╺┳╸┏━┓┏━┓╻  ╻  ┏━╸┏━┓
┃┃┃┃ ┃┃┃   ┃  ┃ ┃┃┗┫ ┃ ┣┳┛┃ ┃┃  ┃  ┣╸ ┣┳┛
╹ ╹╹╺┻┛╹   ┗━╸┗━┛╹ ╹ ╹ ╹┗╸┗━┛┗━╸┗━╸┗━╸╹┗╸
#########################################################
V7.  
Is for trying to fix the jitter on the pots. the hardware
is showing only 0-1 points of jitter. 

Gemini is claiming:

Fix 1: Widen the "Deadband" (Easiest)
Right now, your code ignores a change of 1 on the MIDI scale 
(0-127). We need to make the code a bit more forgiving of 
electrical noise.

Try changing the > 1 in your if statement to > 2 or > 3:

C++
// Change this line:
if (abs(midiVal - pots[i].lastValue) > 2) { 
Pros: Takes two seconds to change.

Cons: You might lose a tiny bit of "fine" control. You will
 have to turn the knob slightly further before the synth 
 responds.

Fix 2: Hardware Filtering (Most Reliable)
If changing the code to > 2 makes the knobs feel too unresponsive, 
the issue is purely electrical. You can clean up the power 
fluctuations using capacitors.

Wire a 100nF (0.1µF) ceramic capacitor between the SIG pin of 
each MUX and Ground.

Wire a 10µF electrolytic capacitor across the 5V and GND rails 
on your breadboard/circuit board.
This acts like a shock absorber for the electricity, physically 
preventing the jitter before it even reaches the code.
#########################################################
V6: 
This version is running 30 pots and two switches (D6, D7):
16 through mux A0
14 through mux A1
Switch through D6, otherside: ground
Switch D7: Midi Send on/off switch
*/

#include <MIDI.h>

// Initialize the MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

// --- CONFIGURATION ---
// MUX Address Pins (Shared by both MUX chips)
const int s0 = 2; const int s1 = 3; const int s2 = 4; const int s3 = 5;

// Signal Pins
const int mux1Sig = A0;
const int mux2Sig = A1;

// Switch Pin
const int ringModSwitchPin = 6; 

// Structure for Potentiometers
struct Potentiometer {
  int muxPin;     // Which MUX chip (A0 or A1)
  int muxChannel; // Which input on the MUX (0-15)
  int ccNumber;
  int midiChannel;
  int lastValue;
};

// Define pots here (signal A0 or A1 / PIN Number / MIDI CC Number / MIDI Channel / placeholder init val -1 )
Potentiometer pots[] = {
  // MUX 1 (A0)
  {mux1Sig, 15, 1, 1, -1}, // Mod pin15 midiCC_1
  {mux1Sig, 14, 5, 1, -1},  // Porta pin14 midiCC_5

  {mux1Sig, 13, 29, 1, -1}, // OSC Bal pin13 midiCC_29

  {mux1Sig, 11, 24, 1, -1}, // OSC 1 Wave
  {mux1Sig, 10, 115, 1, -1}, // OSC 1 Coarse
  {mux1Sig, 9, 111, 1, -1}, // OSC 1 Fine
  {mux1Sig, 8, 113, 1, -1}, // OSC 1 PWM/Saw Detune/FM

  {mux1Sig, 7, 25, 1, -1}, // OSC 2 Wave
  {mux1Sig, 6, 116, 1, -1}, // OSC 2 Coarse
  {mux1Sig, 5, 112, 1, -1}, // OSC 2 Fine
  {mux1Sig, 4, 114, 1, -1}, // OSC 2 PWM
  
  // Ring MOD on/off defined with pin D6
  {mux1Sig, 12, 95, 1, -1}, // Ring Mod Amount

  {mux1Sig, 3, 70, 1, -1}, // LFO 1 AMT
  {mux1Sig, 2, 72, 1, -1}, // LFO 1 RATE
  {mux1Sig, 1, 54, 1, -1}, // LFO 1 WAV
  {mux1Sig, 0, 56, 1, -1}, // LFO 1 DEST

  // MUX 2 (A1)
  {mux2Sig, 7, 28, 1, -1},  // LFO 2 AMT
  {mux2Sig, 6, 73, 1, -1},  // LFO 2 RATE
  {mux2Sig, 5, 55, 1, -1},  // LFO 2 WAV

  {mux2Sig, 4, 74, 1, -1},  // FILTER CUTOFF
  {mux2Sig, 3, 71, 1, -1},  // FILTER RESONANCE
  {mux2Sig, 2, 47, 1, -1},  // FILTER ENV AMT

  {mux2Sig, 15, 85, 1, -1},  // VCF ENV Attack
  {mux2Sig, 14, 86, 1, -1},  // VCF ENV Decay
  {mux2Sig, 13, 87, 1, -1},  // VCF ENV Sustain
  {mux2Sig, 12, 88, 1, -1},   // VCF ENV Release

  {mux2Sig, 11, 81, 1, -1},  // VCA Attack
  {mux2Sig, 10, 82, 1, -1},  // VCA Decay
  {mux2Sig, 9, 83, 1, -1},  // VCA Sustain
  {mux2Sig, 8, 84, 1, -1}   // VCA Release
};

const int numPots = 30;
bool lastSwitchState = true; 

// --- SETUP ---
void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  // Set MUX address pins as outputs
  pinMode(s0, OUTPUT); pinMode(s1, OUTPUT); pinMode(s2, OUTPUT); pinMode(s3, OUTPUT);
  
  // Set up the switch pin with internal pull-up
  pinMode(ringModSwitchPin, INPUT_PULLUP);

  // midi  on/off switch
  pinMode(7, INPUT_PULLUP);
}

// Function to read from a specific MUX signal pin
int readMux(int sigPin, int channel) {
  digitalWrite(s0, channel & 0x01);
  digitalWrite(s1, (channel >> 1) & 0x01);
  digitalWrite(s2, (channel >> 2) & 0x01);
  digitalWrite(s3, (channel >> 3) & 0x01);
  return analogRead(sigPin);
}

// --- MAIN LOOP ---
void loop() {
  // Check the midi on/off (Pin 7). 
  // If switch is OPEN (HIGH) = ON /Else off
  bool controllerActive = digitalRead(7);

  if (controllerActive == HIGH) {
    // 1. Process all Pots
    for (int i = 0; i < numPots; i++) {
      int rawVal = readMux(pots[i].muxPin, pots[i].muxChannel);
      int midiVal = rawVal / 8; // Scale 0-1023 to 0-127

      // Send only if value changed by more than 1 to prevent jitter ##changed to '2' for testing
      if (abs(midiVal - pots[i].lastValue) > 2) {
        MIDI.sendControlChange(pots[i].ccNumber, midiVal, pots[i].midiChannel);
        pots[i].lastValue = midiVal;
      }
    }

    // 2. Process the Ring Mod Switch
    bool currentSwitchState = digitalRead(ringModSwitchPin); 
    
    if (currentSwitchState != lastSwitchState) {
      // If LOW (pressed), send 127; if HIGH (open), send 0
      int midiVal = (currentSwitchState == LOW) ? 127 : 0;
      
      MIDI.sendControlChange(96, midiVal, 1);
      lastSwitchState = currentSwitchState;
    }
  }
  
  delay(5); // Small stability delay
}