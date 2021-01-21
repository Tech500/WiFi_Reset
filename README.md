# WiFi_Reset

ESP32:

   "WiFi_Reset.ino" by tech500.
            
   Sketch demonstrates starting WiFi, Disconnects of WiFi, and Resets of WiFi;
   plus logs WiFi Connects and WiFi Disconnects, additionally watchdog trigger events are logged.

   FTP to WiFi ipaddress.  FTP username is: "admin" no quotes and "12345" no quotes is the FTP password.
   FileZilla was used in testing this Sketch.  Hostname is the ip address in the Sketch.
   
   Uncomment this line:  //delay(50 * 1000);  //Uncomment to test watchdog; 50 second delay  to test
   watchdog.  Line is in the setup section of the the code.
