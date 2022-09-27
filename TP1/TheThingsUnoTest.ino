#include <TheThingsNetwork.h>
 
#define loraSerial Serial1
#define debugSerial Serial
 
// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

// Set your AppEUI and AppKey
const char *appEui = "6081F96365141FAE";
const char *appKey = "C88D3902278CDF5481D2C3CBA7487A78";
 
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);
 
void setup()
{
  loraSerial.begin(57600);

  loraSerial.println("mac set deveui 6081F9FC1F9BAF8C");
  delay(500);
  loraSerial.println("mac save");
  
  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);
  debugSerial.begin(9600);

    while (!debugSerial) {
    ;
    }

  debugSerial.println("-- STATUS");
  ttn.showStatus();

}
 
void loop()
{
}
