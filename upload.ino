#include "ESP8266WiFi.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// WiFi parameters
#define WLAN_SSID ""   //insert SSID
#define WLAN_PASS ""   //insert password

// Adafruit IO
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME ""  //insert username
#define AIO_KEY ""       //insert key

int ledPin = 5; // D1 = GPIO5

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

//on choisit le feed sur lequel nous publierons les logs
Adafruit_MQTT_Publish Attendance = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/RFID_Attendance.");

String message = "";  //variable dans laquelle nous stockerons le message a envoyer 

/* Setup code */
void setup() {
  pinMode(ledPin, OUTPUT);  //configure la broche de la LED comme sortie
  digitalWrite(ledPin, LOW);  //et l'eteint
  Serial.begin(9600); //initie la communication serie a 9600
  delay(1000); 

  // Connect to WiFi access point.
  Serial.println();
  delay(10);
  Serial.print("Connexion à ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);  //connexion au wifi donné


  //attend que la connexion soit etablie
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  digitalWrite(ledPin, HIGH);  //allume la LED pour indiquer la bonne connexion
  Serial.println();
  Serial.println("Wi-Fi Connecté Succès !");
  Serial.print("NodeMCU IP Address : ");  //affiche l'IP
  Serial.println(WiFi.localIP());

  // Connect to Adafruit IO
  connect();
}

// Connect to Adafruit IO via MQTT
void connect() {
  Serial.println("Connecting to Adafruit IO... ");
  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println("Wrong protocol"); break;
      case 2: Serial.println("ID rejected"); break;
      case 3: Serial.println("Server unavailable"); break;
      case 4: Serial.println("Bad user/pass"); break;
      case 5: Serial.println("Not authed"); break;
      case 6: Serial.println("Failed to subscribe"); break;
      default: Serial.println("Connection failed"); break;
    }
    if (ret >= 0)
      mqtt.disconnect();  //en cas d'echec on se deconnecte
    Serial.println("Retrying connection..."); //et on reesaye 5 secondes plus tard
    delay(5000);
  }

  //une fois connecté
  Serial.println("Adafruit IO Connected!");
}

void loop() {
  // on ping Adafruit IO pour garder la connexion active
  if (!mqtt.ping(3)) {
    //si on est plus connecté
    if (!mqtt.connected())
      connect();//on relance la connexion
  }

  if (Serial.available()) { //verifie si des données sont disponibles sur le port serie
    char lettre = Serial.read();  //lit un caractere et le stock

    if(lettre != '\n'){//tant que "entrée" n'est pas pressé
      message+= lettre;  //on ajoute les caractes dans le message a upload
    }else{
      Serial.println("Received from Arduino: " + message);  //on affiche le message recu depuis l'arduino

      if (!Attendance.publish(message.c_str())) { //on tente d'upload vers le cloud 
        Serial.println("Failed");  //Si l'envoi echoue
      } else {
        Serial.println("Sent!");  //si l'envoie réussit
      }
      message = ""; //reset le message
    }
    
  }
}
