#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <MIDIUSB.h>

#define LED     LED_BUILTIN // Pin for heartbeat LED (shows code is working)
#define CHANNEL 1  // MIDI channel number

Adafruit_Trellis trellis;

#define ANALOG_INPUT

uint8_t       heart        = 0;  // Heartbeat LED counter
unsigned long prevReadTime = 0L; // Keypad polling timer
uint8_t       mod;
uint8_t       vel;
uint8_t       fxc;
uint8_t       rate;  

uint8_t note[] = {
  60, 61, 62, 63,
  56, 57, 58, 59,
  52, 53, 54, 55,
  48, 49, 50, 51
};

// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, (byte)(0x90 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, (byte)(0x80 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, (byte) (0xB0 | channel), control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  
  Serial.begin(9600);
  Serial.println("Trellis Demo");
  
  pinMode(LED, OUTPUT);
  trellis.begin(0x70); // Pass I2C address
#ifdef __AVR__
  // Default Arduino I2C speed is 100 KHz, but the HT16K33 supports
  // 400 KHz.  We can force this for faster read & refresh, but may
  // break compatibility with other I2C devices...so be prepared to
  // comment this out, or save & restore value as needed.
  TWBR = 12;
#endif
  trellis.clear();
  trellis.writeDisplay();
  
#ifdef ANALOG_INPUT
  mod = map(analogRead(0), 0, 1023, 0, 127);
  vel = map(analogRead(1), 0, 1023, 0, 127);
  fxc = map(analogRead(2), 0, 1023, 0, 127);
  rate = map(analogRead(3),0, 1023, 0, 127);
  controlChange(CHANNEL,  1, mod);
  controlChange(CHANNEL, 11, vel);
  controlChange(CHANNEL, 12, fxc);
  controlChange(CHANNEL, 13, rate);
#endif
}

void loop() {
  

  unsigned long t = millis();
  if((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if(trellis.readSwitches()) {  // Button state change?

      for(uint8_t i=0; i<16; i++) { // For each button...
        if(trellis.justPressed(i)) {
          noteOn(CHANNEL, note[i], 127);
        
          Serial.print("v"); Serial.println(i);
          trellis.setLED(i);
        } else if(trellis.justReleased(i)) {
          noteOn(CHANNEL, note[i], 0);
          trellis.clrLED(i);
        }
      }
      trellis.writeDisplay();
    }
#ifdef ANALOG_INPUT
    uint8_t newModulation = map(analogRead(0), 0, 1023, 0, 127);
    if(mod != newModulation) {
      mod = newModulation;
      controlChange(CHANNEL, 1, mod);
      Serial.println("Mod 1");
    }
    uint8_t newVelocity = map(analogRead(1), 0, 1023, 0, 127);
    if(vel != newVelocity) {
      vel = newVelocity;
      controlChange(CHANNEL, 11, vel);
      Serial.println("Mod 2");
    }
    uint8_t newEffect = map(analogRead(2), 0, 1023, 0, 127);
    if(fxc != newEffect) {
      fxc = newEffect;
      controlChange(CHANNEL, 12, fxc);
      Serial.println("Mod 3");
    }
    uint8_t newRate = map(analogRead(3), 0, 1023, 0, 127);
    if(rate !=newRate) {
      rate = newRate;
      controlChange(CHANNEL, 13, rate);
      Serial.println("Mod 4");
    }
#endif
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32); // Blink = alive
    MidiUSB.flush();
  }
  (void)MidiUSB.read(); // Discard incoming MIDI messages
}
