///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    ESP32:
//
//   "WiFi_Reset_plus_logging.ino" by tech500 on Github.com
//            
//   Start WiFi plus log disconnect and reflect
//
//   Sketch demonstrates starting WiFi, Disconnects of WiFi, and Resets of WiFi;
//   plus logs WiFi Connects and WiFi Disconnects, additionally watchdog trigger events are logged.
//
//   FTP to WiFi ipaddress.  FTP username is: "admin" no quotes and "12345" no quotes is the FTP password.
//   FileZilla was used in testing this Sketch.  Hostname is the ip address in the Sketch.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////// 

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266FtpServer.h> 
#include <sys/time.h>
#include <time.h>   // time() ctime() --> Needed to sync time
#include <FS.h>
#include <SPIFFS.h>
#include <Ticker.h>

FtpServer ftpSrv;

String LISTEN_PORT = "8030";

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
int totalwatchdogCounter;
int numberOfInterrupts = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR ISRwatchdog()
{
   
    portENTER_CRITICAL_ISR(&mux);
    watchdogCounter++;
    portEXIT_CRITICAL_ISR(&mux);

}

void setup()
{
	//Execute oncee...
	
	Serial.begin(115200);
	
	started = 0;   

	wifi_Start();
	
	secondTick.attach(1, ISRwatchdog);  //watchdog ISR increase watchdogCounter by 1 every 1 second

	configTime(0, 0, udpAddress1, udpAddress2);
	setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 3);   // this sets TZ to Indianapolis, Indiana
	tzset();

	Serial.print("wait for first valid timestamp ");

	while (time(nullptr) < 100000ul)
	{
		Serial.print(".");
		delay(5000);
    

	}

  Serial.println("");

  #ifdef ESP32       //esp32 we send true to format spiffs if cannot mount
  if (SPIFFS.begin(true))
  {
  #elif defined ESP8266
  if (SPIFFS.begin())
  
  #endif
    Serial.println("SPIFFS opened!");
    Serial.println("");
    ftpSrv.begin("admin", "12345");    //username, password for ftp.  set ports in ESP8266FtpServer.h  (default 21, 50009 for PASV)
  
  }
	
	//SPIFFS.format();
	
	//delay(50 * 1000);  //Uncomment to test watchdog; 50 second delay
	
	WiFi.disconnect();   //Just for testing; otherwise comment out or remove

}

void loop()
{

	//Execute repeatly...
	
	delay(1);
	
	//udp only send data when connected
	if (connected)
	{

		//Send a packet
		udp.beginPacket(udpAddress1, udpPort);
		udp.printf("Seconds since boot: %u", millis() / 1000);
		udp.endPacket();
	
	}

	if(WiFi.status() != WL_CONNECTED) 
	{

		getDateTime();

		//Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
		File logFile = SPIFFS.open("/WIFI.TXT", "a");

		if (!logFile)
		{
			Serial.println("File: '/WIFI.TXT' failed to open");
		}

		logFile.print("WiFi Disconnected:    ");

		logFile.print(dtStamp);

		logFile.print("   Connection result: ");   //Diagnostic test

		logFile.println(WiFi.waitForConnectResult());   //Diagnostic test

		logFile.close();

		reconnect = 0;

		wifi_Start();

	}

	if((WiFi.status() != WL_CONNECTED))  
	{

		exit;
			
	}
	else if(reconnect == 1)
	{

		watchdogCounter = 0;  //Resets the watchdogCount

		//Open "WIFI.TXT" for appended writing.   Client access ip address logged.
		File logFile = SPIFFS.open("/WIFI.TXT", "a");

		if (!logFile)
		{
			Serial.println("File: '/WIFI.TXT' failed to open");
		}

		getDateTime();

		logFile.print("WiFi Reconnected:     ");

		logFile.print(dtStamp);

		logFile.print("   Connection result: ");   //Diagnostic test

		logFile.println(WiFi.waitForConnectResult());   //Diagnostic test

		logFile.println("");

		logFile.close();

    Serial.println("\nConnected to network!\n");

		reconnect = 0;

	}

  if (started == 1)
  {

    delay(2000);

    getDateTime();

    // Open a "log.txt" for appended writing
    File log = SPIFFS.open("/SERVER.TXT", FILE_APPEND);

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
	
	if (watchdogCounter > 45) 
	{

		portENTER_CRITICAL(&mux);
		watchdogCounter--;
		portEXIT_CRITICAL(&mux);

		numberOfInterrupts++;
		Serial.print("An interrupt has occurred. Total: ");
		Serial.println(numberOfInterrupts);  

		Serial.println("");
		Serial.println("Watchdog event triggered.");

		// Open a "log.txt" for appended writing
		File log = SPIFFS.open("/WATCHDOG.TXT", "a");

		if (!log)
		{
			Serial.println("file 'WATCHDOG.TXT' open failed");
		}

		getDateTime();

		log.print("Watchdog Restart:  ");
		log.print("  ");
		log.print(dtStamp);
		log.println("");
		log.close();

		Serial.println("Watchdog Restart  " + dtStamp);

		watchdogCounter = 0;

		WiFi.disconnect();

		ESP.restart();

	}
	
	if(SECOND % 30 == 0)   //Feed watchdog every 30 seconds
	{
		watchdogCounter = 0;
	}

  delay(5000);

  getDateTime();

  Serial.println(dtStamp);

  delay(10 * 1000);

  watchdogCounter = 0;

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

void wifi_Start()
{

	// Replace with your network details
  const char * ssid = "R2D2";
  const char * password = "sissy4357";

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

	//Server settings
  #define ip {10,0,0,110}
  #define subnet {255,255,255,0}
  #define gateway {10,0,0,1}
  #define dns {10,0,0,1}

	WiFi.config(ip, gateway, subnet, dns);

	// Starting the web server
	//server.begin();
	Serial.println("Attempting to connect to Network...");

	// Printing the ESP IP address
	Serial.print("Server IP:  ");
	Serial.println(WiFi.localIP());
	Serial.print("Port:  ");
	Serial.print(LISTEN_PORT);
	Serial.println("\n");



	WiFi.waitForConnectResult();

	Serial.printf("Connection result: %d\n", WiFi.waitForConnectResult());

	if ((WiFi.status() != WL_CONNECTED))
	{

		delay(20 * 1000);

		wifi_Start();

		watchdogCounter = 0;

	}

	if((WiFi.status() == WL_CONNECTED))
	{

		reconnect = 1;  

		watchdogCounter = 0;

		
	}

}
	
