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
  5000;           // Minimum time between send (in milliseconds). We don't want to spam the gateway (30000 ms = 30s).

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

void setup()
{
  // initialize our digital pins internal pullup resistor so one pulse switches from high to low (less distortion)
  pinMode(DIGITAL_INPUT_SENSOR, INPUT_PULLUP);

  pulseCount = oldPulseCount = 0;

  // Fetch last known pulse count value from gw
  request(CHILD_ID, V_VAR1);

  lastSend = lastPulse = millis();

  attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), onPulse, RISING);
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

void onPulse()
{
  Serial.print("PulsGezien");
  pulseCount++;
  Serial.print("Nieuwe pulsecount:");
  Serial.println(pulseCount);
}
