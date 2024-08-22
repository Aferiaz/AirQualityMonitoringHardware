/*
	Air Quality Monitoring System
	Code
	4/3/2023
	
	Andreas Josef Diaz
	Marc Jefferson Obeles
	Justin Gabriel Sy
*/

// Private Credentials and Connections
// Change information as needed

/* DEBUG */
#define DEBUG

/* Box Information */
#define BOX_3

/* WiFi Information */
#define SECRET_SSID ""
#define SECRET_PASS ""

/* Google Script Information */
String GOOGLE_SCRIPT_ID = "";
String PASSWORD = "";

/* ThingSpeak Information */
#if defined(BOX_1)
  const unsigned long ChannelNumber = ;
  const char* 				WriteAPIKey   = "";
#elif defined(BOX_2)
  const unsigned long ChannelNumber = ;
  const char* 				WriteAPIKey   = "";
#elif defined(BOX_3)
  const unsigned long ChannelNumber = ;
  const char* 				WriteAPIKey   = "";
#endif

/* Time Information */
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800; // GMT+8
const int   daylightOffset_sec = 0;
