
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//needed for library
#include <DNSServer.h>

#include <WiFiManager.h>  
//#include <PGI.h>
//#include <wifi.h>

#define MASKED 1

#define MAX_FRAME_LEN 300
// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (14)
#define PN532_RESET (13)  // Not connected by default on the NFC Shield

#define LOCK (33)

#define LEDR (14)
#define LEDG (12)
#define LEDB (13)

//test  wifi
IPAddress ip(192, 168, 2, 50);

char ssid[] = "TheLabIOT";    // your network SSID (name) 
char pass[] = "Yaay!ICanTalkNow"; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;



// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);


uint8_t START_14443A[] = {0x4A, 0x01, 0x00}; //InListPassiveTarget
//   0x0 0x0 0xFF 0x17 0xE9 0xD4 0x40 0x1 0x0 0xA4 0x4 0x0 0xE 0x32 0x50 0x41 0x59 0x2E 0x53 0x59 0x53 0x2E 0x44 0x44 0x46 0x30 0x31 0x0 0x8F 0x0
	uint8_t START_14443B[] = {0x4A, 0x01, 0x03, 0x00}; //InListPassiveTarget
	uint8_t SELECT_APP[4][15] = {{0x40,0x01,0x00,0xA4,0x04,0x00,0x07,0xA0,0x00,0x00,0x00,0x03,0x10,0x10,0x00},// InDataExchange VISA
                                {0x40,0x01,0x00,0xA4,0x04,0x00,0x07,0xA0,0x00,0x00,0x00,0x04,0x10,0x10,0x00}, // InDataExchange MasterCard
                                {0x40,0x01,0x00,0xA4,0x04,0x00,0x07,0xA0,0x00,0x00,0x00,0x03,0x20,0x10,0x00}, // InDataExchange VISA Electron
                                {0x40,0x01,0x00,0xA4,0x04,0x00,0x07,0xA0,0x00,0x00,0x00,0x42,0x10,0x10,0x00}}; // InDataExchange CB (Carte Bleu)
	uint8_t READ_RECORD[] = {0x40, 0x01, 0x00, 0xB2, 0x01, 0x0C, 0x00}; //InDataExchange 
   int aid_found = 0;
	  
	uint8_t abtRx[MAX_FRAME_LEN];
	uint8_t abtTx[MAX_FRAME_LEN];
    
	uint8_t szRx = sizeof(abtRx);
	uint8_t szTx;

	
	unsigned char *res, output[50], c, amount[10],msg[100];
	unsigned int i, j, expiry, m, n;

//not used
void show(size_t recvlg, uint8_t *recv) {
	int i;

    for (i=0;i<(int) recvlg;i++) {
        Serial.printf("%02x",(unsigned int) recv[i]);
    }
    Serial.printf("\n");
}

void setup(void) {
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  //nfc.setPassiveActivationRetries(0xFF);

    uint8_t tmp[] =  {0x03, 0xFD, 0xD4, 0x12, 0x00, 0x1A, 0x00 };
    nfc.sendCommandCheckAck(tmp,7);
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");

// lock 
    pinMode(LOCK, OUTPUT);
//leds
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);

    WiFiManager wifiManager;

    wifiManager.autoConnect("AutoConnectAP");

}


void loop(void) {
  Serial.println("-------------");
    bool success;
  //             apdu[] ={0x00, 0xA4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
    //uint8_t apdu[20] = {0x00, 0xA4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
    //uint8_t apdu[] =  {0x00, 0xA4, 0x04, 0x00, 0x0E, 0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
        uint8_t apdu[] ={0x00, 0xa4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
    uint8_t berBuffer[255];
    uint8_t berLength=0;

    //nfc.readPassiveTargetID()
//    success = nfc.inListPassiveTarget();
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.inListPassiveTarget();//PN532_ISO14443B);// .readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

    if(success)
    {
        Serial.println("Found a card!");
        Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("UID Value: ");
        for (uint8_t i=0; i < uidLength; i++) 
        {
            Serial.print(" 0x");Serial.print(uid[i], HEX); 
        }
        Serial.println("");
        //green open
        digitalWrite(LEDG, HIGH);
        //red close
        digitalWrite(LEDR, LOW);
        //lock open
        digitalWrite(LOCK, HIGH);
        //wait 1sec
        delay(1000);
        //green open
        digitalWrite(LEDG, LOW);
        //red close
        digitalWrite(LEDR, HIGH);
        //lock close
        digitalWrite(LOCK, LOW);   


    return;

        Serial.println(":)");
        success = nfc.inDataExchange(apdu, sizeof(apdu), berBuffer, &berLength);
        if(success)
        {
            Serial.println("weeeee");
        }
    }
    Serial.println("+++++++++++++");
    return;

    Serial.printf("[-] Looking for known card types...\n");
    memset(&abtRx,255,MAX_FRAME_LEN);
    szRx = sizeof(abtRx);
    int numBytes;
    // 14443A Card
    if (nfc.inDataExchange(START_14443A, sizeof(START_14443A), abtRx, &szRx)) {
        if (numBytes > 0){
            Serial.printf("[+] 14443A card found!!\n");
        }
        else{
            // 14443B Card
            if (nfc.inDataExchange(START_14443B, sizeof(START_14443B), abtRx, &szRx)) {
                if (numBytes > 0){
                    Serial.printf("[+] 14443B card found!!\n");
                }
                else{
                    Serial.printf("[x] Card not found or not supported!! Supported types: 14443A, 14443B.\n");
                    return;    
                }
            }
            else{
                Serial.printf("nfc_perror START_14443B");
                return;
            }
        }
    }
    else{
        Serial.printf("nfc_perror START_14443A");
        delay(1000);
        return;
    }
    
    // Looking for a valid AID (Application Identifier): VISA, VISA Electron, Mastercard, CB
}