// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <MySensors.h>

#define DIGITAL_INPUT_SENSOR 3                  // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)

#define PULSE_FACTOR 1000                       // Nummber of blinks per m3 of your meter (One rotation/liter)

#define CHILD_ID 1                              // Id of the sensor child

unsigned long SEND_FREQUENCY =
  30000;           // Minimum time between send (in milliseconds). We don't want to spam the gateway (30000 ms = 30s).

MyMessage volumeMsg(CHILD_ID, V_VOLUME);
MyMessage lastCounterMsg(CHILD_ID, V_VAR1);

double ppl = ((double)PULSE_FACTOR) / 1000;      // Pulses per liter

volatile unsigned long pulseCount = 0;
volatile unsigned long lastBlink = 0;
bool pcReceived = false;
unsigned long oldPulseCount = 0;
unsigned long newBlink = 0;
double volume = 0;
double oldvolume = 0;
unsigned long lastSend = 0;
unsigned long lastPulse = 0;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
// the following variables are unsigned long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup()
{
  // initialize our digital pins internal pullup resistor so one pulse switches from high to low (less distortion)
  pinMode(DIGITAL_INPUT_SENSOR, INPUT);

  pulseCount = oldPulseCount = 0;

  // Fetch last known pulse count value from gw
  request(CHILD_ID, V_VAR1);

  lastSend = lastPulse = millis();
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Water Meter", "1.1");

  // Register this device as Watersensor
  present(CHILD_ID, S_WATER);
}

void loop()
{
  unsigned long currentTime = millis();

  // read the state of the switch into a local variable:
  int reading = digitalRead(DIGITAL_INPUT_SENSOR);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        pulseCount++;
        Serial.print("Pulse!");
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;

  // Only send values at a maximum frequency or woken up from sleep
  if (currentTime - lastSend > SEND_FREQUENCY) {
    lastSend = currentTime;

    if (!pcReceived) {
      //Last Pulsecount not yet received from controller, request it again
      request(CHILD_ID, V_VAR1);
      return;
    }

    // Pulse count has changed
    if (pulseCount != oldPulseCount) {
      oldPulseCount = pulseCount;

      Serial.print("pulsecount:");
      Serial.println(pulseCount);

      send(lastCounterMsg.set(pulseCount));                  // Send  pulsecount value to gw in VAR1

      double volume = ((double)pulseCount / ((double)PULSE_FACTOR));
      if (volume != oldvolume) {
        oldvolume = volume;

        Serial.print("volume:");
        Serial.println(volume, 3);

        send(volumeMsg.set(volume, 3));               // Send volume value to gw
      }
    }
  }
}

void receive(const MyMessage &message) //Haal 1 keer de waarden op van gateway
{
  if (message.type == V_VAR1) {
    unsigned long gwPulseCount = message.getULong();
    pulseCount += gwPulseCount; //pulseCount = 162516
    Serial.print("Received last pulse count from gw:");
    Serial.println(pulseCount);
    pcReceived = true;
  }
}
