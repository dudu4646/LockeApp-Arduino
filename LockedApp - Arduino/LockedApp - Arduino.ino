/*
21/10
sending wifi done
*/

#include <FirebaseESP8266.h>
#include <FirebaseESP8266HTTPClient.h>
#include <FirebaseJson.h>
#include <jsmn.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <string.h>

#define FIREBASE_HOST "https://lockedappproject.firebaseio.com/"
#define FIREBASE_AUTH "a9VdU3AGQC9wzu7NYn4W44mQhQRvVSwisbqMR8Mq"
//sets the pins
#define blue D5
#define green D6
#define red D7
#define RxD D0
#define TxD D1
//EEPROM ptr
#define ssid_ptr 100 
#define pass_ptr 121 
#define id_ptr 141
//Lock ID
#define LID "30"
//State
#define MISSING_SSID_PASS 0
#define HAVING_SSID_PASS 1
#define WIFI_DISCONNECT 2
#define WIFI_CONNECT 3
#define NEW_LOCK 4
#define BLUETOOTH_CONNECT 5

FirebaseData FBdata;
FirebaseJson snapshot;
FirebaseJsonObject jsonParseResult;
SoftwareSerial bluetoothSerial(TxD, RxD);

String path = "/locks/";
String data;
char ssid[21], pass[21], lockId[] = LID;
char state = 0, t;
char ch, pass_len, ssid_len;

void setup() {
	//	Serial.begin(9600);
	bluetoothSerial.begin(9600);
	EEPROM.begin(512);

	//reseting the EEPROM - LID, SSID and PASS 
//	reset(LID, "Dudu phone", "pass4646");

	pinMode(blue, OUTPUT);
	pinMode(green, OUTPUT);
	pinMode(red, OUTPUT);
	turn_RGB(0, 0, 0);
	path += LID;
	path += "/status";
	read_SSID_PASSWORD();
	bluetoothSerial.flush();
	t = 0;

}
void loop() {
	if (bluetoothSerial.available() > 0 || t!=0)
	{
		for (char tmp = 0; tmp < 2; tmp++) {
			turn_RGB(0, 0, 1);
			delay(300);
			turn_RGB(0, 0, 0);
		}
		if(bluetoothSerial.available())
			t = bluetoothSerial.read();
		switch (t) {

		case '1':
			bluetoothSerial.write(1);
			delay(15);
			bluetoothSerial.write(4);
			delay(15);
			bluetoothSerial.write("SYNC");
			t = 0;
			break;
		case '2':
			bluetoothSerial.write(1);
			delay(15);
			bluetoothSerial.write(strlen(lockId));
			delay(15);
			for (char i = 0; i < strlen(lockId); i++) {
				bluetoothSerial.write(lockId[i]);
				delay(15);
			}
			t = 0;
			break;
		case '3':
			bluetoothSerial.write(1);
			delay(15);
			bluetoothSerial.write(ssid_len);
			delay(15);
			for (char i = 0; i < ssid_len; i++) {
				bluetoothSerial.write(ssid[i]);
				delay(15);
			}
			t = 0;
			break;
		case '4':
			bluetoothSerial.write(1);
			delay(15);
			bluetoothSerial.write(1);
			delay(15);
			bluetoothSerial.write((WiFi.status() == WL_CONNECTED) ? '1' : '0');
			delay(15);
			t = 0;
			break;

		case '5':
			char ssidNum = WiFi.scanNetworks();
			bluetoothSerial.write(ssidNum);
			delay(15);
			for (char i = 0; i < ssidNum; i++) {
				String str = WiFi.SSID(i).c_str();
				delay(15);
				bluetoothSerial.write(str.length());
				delay(15);
				for (char j = 0; j < str.length(); j++) {
					bluetoothSerial.write(str[j]);
					delay(15);
				}
			}
			t = 0;
		}
	}
	else {
		switch (state) {
		case MISSING_SSID_PASS:
			for (char t = 0; t < 3; t++) {
				turn_RGB(1, 0, 0);
				delay(300);
				turn_RGB(0, 0, 0);
				delay(300);
			}
			read_SSID_PASSWORD();
			break;
		case HAVING_SSID_PASS:
			connect_to_WIFI_and_FB();
			break;
		case WIFI_CONNECT:
			read_lock_status();
			break;
		case NEW_LOCK:
			for (char i = 0; i < 2; i++) {
				turn_RGB(1, 1, 1);
				delay(300);
				turn_RGB(0, 0, 0);
				delay(300);
			}
			read_lock_status();
			break;
		}
	}

}

//connecting to WIFI and FIREBASE
void connect_to_WIFI_and_FB() {
	WiFi.begin(ssid, pass);
	//	Serial.print("connecting");
	while (WiFi.status() != WL_CONNECTED) {
		//		Serial.print(".");
		turn_RGB(1, 1, 0);
		delay(300);
		turn_RGB(0, 1, 1);
		delay(300);
		turn_RGB(1, 0, 1);
		delay(300);
	}
	Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
	Firebase.reconnectWiFi(true);
	state = WIFI_CONNECT;
	turn_RGB(0, 0, 0);
}

//read SSID and PASSWORD form EEPROM
void read_SSID_PASSWORD() {
	char len = 0;
	char ch;
	ch = EEPROM.read(ssid_ptr);
	while (ch != '\0' && len < 21) {
		ssid[len++] = ch;
		ch = EEPROM.read(ssid_ptr + len);
	}
	ssid[len] = '\0';
	ssid_len = len;
	len = 0;
	ch = EEPROM.read(pass_ptr);
	while (ch != '\0' && len < 21) {
		pass[len++] = ch;
		ch = EEPROM.read(pass_ptr + len);
	}
	pass[len] = '\0';
	pass_len = len;

	Serial.print("\n~~~~~~~~~~\n");
	Serial.println(ssid);
	Serial.println(pass);
	Serial.println("~~~~~~~~~~~");

	if ((ssid[0] != '\0' && pass[0] != '\0'))
		state = HAVING_SSID_PASS;
}

//write SSID and PASSWORD to EEPROM
void write_SSID_PASSWORD(char s[20], char p[20]) {
	char i;
	for (i = 0; s[i] != '\0'; i++)
		EEPROM.write(ssid_ptr + i, s[i]);
	EEPROM.write(ssid_ptr + i, s[i]);
	for (i = 0; p[i] != '\0'; i++)
		EEPROM.write(pass_ptr + i, p[i]);
	EEPROM.write(pass_ptr + i, p[i]);
	EEPROM.commit();
}

//write lock id to EEPROM
void write_lock_id(char id[6]) {
	char i;
	for (i = 0; i < id[i] != '\0'; i++)
		EEPROM.write(id_ptr + i, id[i]);
	EEPROM.write(id_ptr + i, id[i]);
}

//read lock status
void read_lock_status() {
	if (Firebase.getString(FBdata, path)) {
		state = WIFI_CONNECT;
		if (FBdata.stringData().equalsIgnoreCase("open"))
			turn_RGB(0, 1, 0);
		else if (FBdata.stringData().equalsIgnoreCase("close"))
			turn_RGB(1, 0, 0);
		else if (FBdata.stringData().equalsIgnoreCase("lock"))
			turn_RGB(0, 0, 1);
		else
			state = NEW_LOCK;
	}
	else
		if (WiFi.status() != WL_CONNECTED)
			state = HAVING_SSID_PASS;
		else
			state = NEW_LOCK;
}

//turn leds on/off
void turn_RGB(char r, char g, char b) {
	digitalWrite(red, r == 0 ? LOW : HIGH);
	digitalWrite(green, g == 0 ? LOW : HIGH);
	digitalWrite(blue, b == 0 ? LOW : HIGH);
}

void reset(char id[6], char ssid[21], char pass[21]) {
	write_lock_id(id);
	write_SSID_PASSWORD(ssid, pass);
}
