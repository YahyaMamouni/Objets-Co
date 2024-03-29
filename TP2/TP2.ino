#include "MKRWAN.h"

#include <Crypto.h>
#include <AES.h>
#include <string.h>

//compteKey[16] cotain 16 byte key(128 bit) for encryption
byte compteKey[16]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//key[16] cotain 16 byte key(128 bit) for encryption
byte key[16] = {0xf6, 0xc7, 0xd9, 0xe5, 0x0f, 0x8a, 0x1b, 0x3b, 0x3e, 0x6b, 0x2d, 0x6f, 0x79, 0x90, 0x77, 0xc2};
//plaintext[16] contain the text we need to encrypt
byte plaintext[16]={0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
//cypher[16] stores the encrypted text
byte cypher[16];
//enctyptedKey[16] stores the encrypted key
byte enctyptedKey[16];
//creating an object of AES128 class
AES128 aes128;

byte iv[16] = {0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55};


//#define LORA_DEBUG Serial
#include <TemperatureZero.h>

TemperatureZero TempZero = TemperatureZero();

/* --------------------------------------------------
 *  Add features to the MKRWAN.h lib
 * -------------------------------------------------- 
 */

 class MyLoRaModem : public LoRaModem {
    // some change you need to make in the (stupid) arduino library
    // line 265 - make private to protected for stream, rx,tx ...
    // line 708 - make private to protected for changeMode, join ...
    

    public:
      // Override the joinOTAA to permit joining from the credentials stored in modem NVM
      // or to store them into the NVM on the first attempt
      int joinOTAA(const char *appEui, const char *appKey, const char *devEui = NULL) {
         YIELD();
         rx.clear();
         changeMode(OTAA);
         if ( appEui != NULL ) set(APP_EUI, appEui);
         if ( devEui != NULL ) set(DEV_EUI, devEui);
         if ( appKey != NULL ) set(APP_KEY, appKey);
         network_joined = join(10000);
         delay(1000);
         return network_joined;
      }

      // store the credential and return false when failed
      bool storeCredential(const char *appEui, const char *appKey, const char *devEui) {
         YIELD();
         rx.clear();
         if ( appEui != NULL ) if ( ! set(APP_EUI, appEui) ) return false;
         if ( devEui != NULL ) if ( ! set(DEV_EUI, devEui) ) return false;
         if ( appKey != NULL ) if ( ! set(APP_KEY, appKey) ) return false;
         return true;
      }


      // Access to the modem NVM user storage zone to read and write custom data
      // will be used to store the device state
      bool readNVM(int adr, int * v) { 
        sendAT(GF("$NVM "), adr);
        if (waitResponse("+OK=") == 1) {
          *v = stream.readStringUntil('\r').toInt();
          return true;
        } else return false;
      }

      bool writeNVM(int adr, int v) { 
        sendAT(GF("$NVM "), adr, ",", v);
        if (waitResponse("+OK") == 1) {
          return true;
        } else return false;
      }

      // Extend the feature to add the lock key feature added in the custom firmware
      bool lockKeys() {
        sendAT(GF("$APKACCESS"));
        if (waitResponse("+OK") == 1) {
          return true;
        } else return false;
      }
  
 };
 MyLoRaModem modem;


/* --------------------------------------------------
 *  Manage the device state from the NVM
 * -------------------------------------------------- 
 */
#define MAGIC 0xA5
#define FACTORY_STATE   0x23
#define RUN_STATE       0x47
#define UNKNOWN_STATE   0x00

// the NVM will use 3 bytes to store the state safely
// First is MAGIC and can be changed when the version is
// modified and we want to be back on factory state
// Second byte is the state, better using random value than 0 and 1
// Third byte is a checksum of the 2 others, to make sure we did not
// had a bad luck with random values. (imagine you make a million of devices)

// Change the factory state in the NVM
bool setFactoryMode(bool state) {
  int s = (state)?FACTORY_STATE:RUN_STATE;
  int c = (MAGIC+s) & 0xFF;
  if ( modem.writeNVM(0,MAGIC) && modem.writeNVM(1,s) && modem.writeNVM(2,c) ) return true;
  return false;
}

// Read the factory state from the NVM 
bool isFactoryMode() {
  int m,c;
  int state = UNKNOWN_STATE;
  if( modem.readNVM(0,&m) ) {
    if ( m == MAGIC ) {
      if( modem.readNVM(1,&state) ) {
        if ( modem.readNVM(2,&c) ) {
          if ( c = (m + state) & 0xFF ) {
            // valid data
            return (state==FACTORY_STATE);
          }
        }
      }
    }
  }
  // The state read failed, we need to build the state and default is FACTORY
  setFactoryMode(true);
  return true;
}

/* --------------------------------------------------
 *  Manage credential setup
 * -------------------------------------------------- 
 */
// When in factory mode we are expecting the user to setup the 
// credential, for this we create a set of commands
// AT? - return the help
// AT+D=1234567812345678 // the DevEUI 16 hex char
// AT+A=1234567812345678 // the AppEUI 16 hex char
// AT+K=12345678123456781234567812345678 // the AppKey 32 hex char
// AT+S - save credentials 

// Extract the param, verify the hex char format and the size
// start is the starting char in the buffer
// len is the expected lenght for the param
// return "" when invalid
String getParam(String buf, int start, int len) {
  buf = buf.substring(start);
  if ( buf.length() != len ) return "";
  for ( int i = 0 ; i < len ; i++ ) {
    char c = buf.charAt(i);
    if ( ( c >= '0' && c <= '9') || ( c >= 'a' && c <= 'f') ||( c >= 'A' && c <= 'F') ) {
      // valid hex char
      if ( ( c >= 'a' && c <= 'f') ) {
        c = c-'a'+'A';
        buf.setCharAt(i,c);
      }
    } else return "";
  }
  return buf;
}


// Manage the AT commands
void setupCredentials() {
  bool quit = false;
  String buf;
  String devEUI, appEUI, appKEY;
  while ( !quit ) {
    char lastRead='\0';
    if ( Serial.available() > 0 ) {
      lastRead = (char)Serial.read();
      Serial.print(lastRead);
      if ( lastRead != '\r' && lastRead != '\n') buf += lastRead;
    }
    if ( lastRead == '\n' ) {
      //Serial.println(buf);
      if ( buf.startsWith("AT?") ) {
        // Print help
        Serial.println("AT? - return the help");
        Serial.println("AT+D=xxx - set the DevEUI");
        Serial.println("AT+A=xxx - set the AppEUI");
        Serial.println("AT+K=xxx - set the AppKEY");
        Serial.println("AT+S - save credential & run");
      }
      if ( buf.startsWith("AT+D=") ) {
        // set the DevEUI
        devEUI = getParam(buf,5,16);
        if ( devEUI.length() > 0 ) {
          Serial.println("OK");
        } else {
          Serial.println("KO");
        }
      }
      if ( buf.startsWith("AT+A=") ) {
        // set the AppEUI
        appEUI = getParam(buf,5,16);
        if ( appEUI.length() > 0 ) {
          Serial.println("OK");
        } else {
          Serial.println("KO");
        }
      }
      if ( buf.startsWith("AT+K=") ) {
        // set the AppKey
        appKEY = getParam(buf,5,32);
        if ( appKEY.length() > 0 ) {
          Serial.println("OK");
        } else {
          Serial.println("KO");
        }
      }
      if ( buf.startsWith("AT+S") ) {
        // save is everything has been setup
        if ( appKEY.length() == 32 && devEUI.length() == 16 && appEUI.length() == 16 ) {
          if( modem.storeCredential(appEUI.c_str(), appKEY.c_str(), devEUI.c_str() ) ) {
            // swith out of factory mode
            if ( setFactoryMode(false) ) {
              if ( modem.lockKeys() ) {
                 Serial.println("OK"); 
                 return;  
              } 
            }
          }
          Serial.println("ERR");                      
        } else {
          Serial.println("KO");
        }
      }
      buf = "";
    }
  }
  
}



void setup() {

  // Init serial, wait for initialization or 2s as in production
  // the serial will not be connected to the device
  Serial.begin(115200);
  uint32_t s = millis();
  while ( !Serial && (millis() - s) < 2000 );
  //while ( !Serial );
 
  // Setup modem, basically configure the Serial lines
  if ( !modem.begin(EU868)) {
    Serial.println(F("Failed to init modem"));
    while(true);
  }
Serial.println("1");
  // check if factory mode of not
  if ( isFactoryMode() ) {
    Serial.println("We are in factory Mode");
    setupCredentials();
    NVIC_SystemReset();
  }
Serial.println("2");

  // When we reach this point it means the setup has been completed
  // we can join from the credentials stored in NVM
  if ( ! modem.joinOTAA(NULL,NULL,NULL) ) {
    // join failed, retry in 30 seconds - not best approach but less lines to write above
    delay(30000);
    NVIC_SystemReset();
  }

  // Add some communication settings
  modem.setPort(1);
  modem.dataRate(3);

  // init temperature
  TempZero.init();
  pinMode(LED_BUILTIN,OUTPUT);
}

#define EMIT_RATE 30000

void loop() {
  static uint32_t lastEmit = millis() - EMIT_RATE; 

  
  static int8_t frame[1] = {0};
  static uint8_t output[1] = {0};

  if ( (millis() - lastEmit) > EMIT_RATE ) {
    // Read the internal temperature and store it in the first byte of the frame
    float t = TempZero.readInternalTemperature();
    frame[0] = (byte)t;

    // Set the key for AES encryption
    aes128.setKey(key,16);
    
    // Encrypt the key with AES
    aes128.encryptBlock(enctyptedKey,compteKey);
    
    // Create a counter block with a value of 1
    byte one[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    
    // Increment the counter in the key
    compteKey[15]++;
    if (compteKey[15]==255) compteKey[16]++;

    // Set the key for AES encryption and encrypt the counter block
    aes128.setKey(key,16);
    aes128.encryptBlock(cypher,iv);

    // XOR the first byte of the frame with the first byte of the encrypted counter block
    output[0] = frame[0] ^ cypher[0];
    
    modem.beginPacket();
    modem.write((uint8_t *)output,1);
    modem.endPacket(false);
    lastEmit = millis();
  }

  digitalWrite(LED_BUILTIN,HIGH);
  delay(10);
  digitalWrite(LED_BUILTIN,LOW);
  delay(500);
}

// USE FCNT TO INCREMENT COUNTER
