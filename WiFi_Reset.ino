///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    ESP8266/ESP32  
//
//   "WiFi_Reset.ino" by William Lucid on Github.com  by Tech500   09/12/2021 02:19 EDT
//
//   Start WiFi plus log disconnect and reflect
//
//   Sketch demonstrates starting WiFi, Disconnects of WiFi, and Resets of WiFi;
//   plus logs WiFi Connects and WiFi Disconnects, additionally watchdog trigger events are logged.
//
//   FTP to ipaddress.  FTP username is: "admin" no quotes and "12345" no quotes is the FTP password.
//   FileZilla was used in testing this Sketch.  Hostname is the ip address in the Sketch.
//
//   Sketch produces two files:  "Server.txt" and "wifi.txt.""
//
////////////////////////////////////////////////////////////////////////////////////////////////////////


/*

    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6

 */


#include <WiFiUdp.h>
#include <FTPServer.h>   //https://github.com/dplasa/FTPClientServer
#include <sys/time.h>
#include <time.h>   // time() ctime() --> Needed to sync time
#include <FS.h>

#ifdef ESP32       

  #include <WiFi.h>
  #include "SPIFFS.h"
  FTPServer ftpSrv(SPIFFS);
  
	
#elif defined ESP8266

  #include <ESP8266WiFi.h>
  #include "LittleFS.h"
	#define SPIFFS LittleFS
  FTPServer ftpSrv(LittleFS);
  
	
#endif
	
#include <Ticker.h>

// Replace with your network details
const char * ssid = "R2D2";
const char * password = "sissy4357";

Ticker secondTick;

//Are we currently connected?
boolean connected = false;

WiFiUDP udp;
// local port to listen for UDP packets
const int udpPort = 1337;
char incomingPacket[255];
char replyPacket[] = "Hi there! Got the message :-)";
const char * udpAddress1 = "us.pool.ntp.org";
const char * udpAddress2 = "time.nist.gov";

#define TZ "EST+5EDT,M3.2.0/2,M11.1.0/2"

int DOW, MONTH, DATE, YEAR, HOUR, MINUTE, SECOND;

char strftime_buf[64];

String dtStamp(strftime_buf);

int lc = 0;

time_t tnow = 0;

int started;

int reconnect;

volatile int watchdogCounter;
volatile int watchDog;

#ifdef ESP32       

	
	portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

	void IRAM_ATTR ISRwatchdog()
	{

		portENTER_CRITICAL_ISR(&mux);
		watchdogCounter++;
		portEXIT_CRITICAL_ISR(&mux);

		if (watchdogCounter == 45)
		{

			watchDog = 1;

		}

	}
	
#elif defined ESP8266

	void ISRwatchdog()
	{

		watchdogCounter++;

		if (watchdogCounter == 45)
		{

			watchDog = 1;

		}
	
	}
	
#endif
    
void setup()
{
  
  Serial.begin(115200);

  while(!Serial);

  Serial.println("\nDemonstration of WiFi connection events and Auto-reconnection to WiFi.");
  Serial.println("with Watchdog and WiFi events logged, plus FTP to view log files.\n");

  started = 0;

  wifi_Start();

  secondTick.attach(1, ISRwatchdog);  //watchdog ISR increase watchdogCounter by 1 every 1 second

  configTime(0, 0, udpAddress1, udpAddress2);
  setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 3);   // this sets TZ to Indianapolis, Indiana
  tzset();

  Serial.print("wait for first valid timestamp");

  while (time(nullptr) < 100000ul)
  {
    Serial.print(".");
    delay(5000);

  }

  Serial.println("");

  getDateTime();

  Serial.println(dtStamp);

  Serial.println("");

  SPIFFS.begin();

  Serial.println("File System opened!");
  Serial.println("");
  ftpSrv.begin("admin", "12345");    //username, password for ftp.

  //SPIFFS.format();

   started = 1;
  
}

void loop()
{

  delay(1);

  //udp only send data when connected
  if (connected)
  {

    //Send a packet
    udp.beginPacket(udpAddress1, udpPort);
    udp.printf("Seconds since boot: %u", millis() / 1000);
    udp.endPacket();

  }
  
  getDateTime();

  if (WiFi.status() != WL_CONNECTED)
  {

    watchdogCounter = 0;  //Resets the watchdogCounter
    
    getDateTime();

    //Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
    File logFile = SPIFFS.open("/WIFI.TXT", "a");

    if (!logFile)
    {
      Serial.println("File: '/WIFI.TXT' failed to open");
    }

    logFile.print("WiFi Disconnected:  ");

    logFile.print(dtStamp);

    logFile.print("   Connection result: ");

    logFile.println(WiFi.waitForConnectResult());
    logFile.close();
    
    wifi_Start();

  }

  if (reconnect == 1)
  {

    watchdogCounter = 0;  //Resets the watchdogCounter

    //Open "WIFI.TXT" for appended writing.   Client access ip address logged.
    File logFile = SPIFFS.open("/WIFI.TXT", "a");

    if (!logFile)
    {
      Serial.println("File: '/WIFI.TXT' failed to open");
    }

    getDateTime();

    logFile.print("WiFi Connected:       ");

    logFile.print(dtStamp);

    logFile.print("   Connection result: ");

    logFile.println(WiFi.waitForConnectResult());

    logFile.println("");

    logFile.close();

    reconnect = 0;

  }

  if (started == 1)
  {

    delay(2000);

    getDateTime();

    // Open a "log.txt" for appended writing
    File log = SPIFFS.open("/SERVER.TXT", "a");

    if (!log)
    {
      Serial.println("file 'SERVER.TXT' open failed");
    }

    log.print("Started Server:  ");
    log.print("  ");
    log.print(dtStamp);
    log.println("");
    log.close();

    started = 0;

  }

  for (int x = 1; x < 5000; x++)
  {
    ftpSrv.handleFTP();
  }
  
#ifdef ESP32       
	
	if ( watchDog == 1)
	{

		portENTER_CRITICAL(&mux);
		watchdogCounter--;
		portEXIT_CRITICAL(&mux);

		logWatchdog();

		ESP.restart();

	}

#elif defined ESP8266

	if ( watchDog == 1)
	{

		logWatchdog();

		ESP.restart();

	}
	
#endif

  getDateTime();

  watchdogCounter = 0;  //Resets the watchdogCounter 

}

String getDateTime()
{

  struct tm *ti;

  tnow = time(nullptr) + 1;
  //strftime(strftime_buf, sizeof(strftime_buf), "%c", localtime(&tnow));
  ti = localtime(&tnow);
  DOW = ti->tm_wday;
  YEAR = ti->tm_year + 1900;
  MONTH = ti->tm_mon + 1;
  DATE = ti->tm_mday;
  HOUR  = ti->tm_hour;
  MINUTE  = ti->tm_min;
  SECOND = ti->tm_sec;

  strftime(strftime_buf, sizeof(strftime_buf), "%a , %m/%d/%Y , %H:%M:%S %Z", localtime(&tnow));
  dtStamp = strftime_buf;
  return (dtStamp);

}

void logWatchdog()
{

  yield();

  Serial.println("");
  Serial.println("Watchdog event triggered.");

  // Open a "WIFI.TXT" for appended writing
  File log = SPIFFS.open("/WIFI.TXT", "a");

  if (!log)
  {
    Serial.println("file 'WIFI.TXT' open failed");
  }

  getDateTime();

  log.print("Watchdog Restart:   ");
  log.print("  ");
  log.print(dtStamp);
  log.println("");
  log.close();

  Serial.println("Watchdog Restart  " + dtStamp);
  Serial.println("\n");

}

void wifi_Start()
{

  //WiFi.mode(WIFI_OFF);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  //WiFi.persistent(true);

  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  
  //setting the addresses
  IPAddress ip(10, 0, 0,5);
  IPAddress gateway(10, 0, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(10, 0, 0, 1);

  WiFi.config(ip, gateway, subnet, dns);
 
  Serial.println("network...");

  while(WiFi.status() == WL_CONNECTION_LOST) {};
  
  while(WiFi.status() == WL_NO_SSID_AVAIL)
  {
     
    delay(10 * 1000);
    
    Serial.println("No SSID availible");
    
    watchdogCounter = 0;   
             
  }

  Serial.printf("\nConnection result: %d\n", WiFi.waitForConnectResult());
  Serial.print("IP Address:  ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength: ");
  Serial.println(WiFi.RSSI());
  Serial.println("WiFi Connected");

  reconnect = 1;
    
  watchdogCounter = 0;  //Resets the watchdogCounter
 
}


