#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <RFID.h>
#include <SoftwareSerial.h>


const int red = 7, green = 6, blue = 5;  //configuration des pins RGB pour la LED

LiquidCrystal_I2C lcd(0x27, 16, 2);  //initialisation de l'ecran LCD

SoftwareSerial espSerial(2, 3);  //Initialisation de liaison série avec l'ESP8266 avec les pin 2(relié au TX) et pin 3(relié au RX)


RFID rfid(10, 9);  // initialisation du RFID avec les pin 10 (relié au SDA), et pin 9 (relié au RST)
unsigned char status;
unsigned char str[MAX_LEN]; //stockage des données envoyée par la carte RFID


void setup() {
  off();  //eteitn la led
  SPI.begin(); //demarre la communication SPI (pour le RFID)
  rfid.init(); //demarre le module RFID
  pinMode(red, OUTPUT); //définit les pins correspondant a la LED RGB comme des sorties
  pinMode(green, OUTPUT); 
  pinMode(blue, OUTPUT); 

  espSerial.begin(9600); //demarre et definit la vitess de la communication série avec l'ESP8266 a 9600 Bd
  Serial.begin(9600);

  if (!i2CAddrTest(0x27)) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }
  lcd.init(); // init le LCD
  lcd.backlight(); //retroeclairage
  stand_down();  //Affichage le message de standby sur le LCD
}


//fonction red_on()
// input: neant
// output: neant
// objectif: allume la LED RGB en rouge
void red_on(){
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  digitalWrite(blue, HIGH);
}

//fonction green_on()
// input: neant
// output: neant
// objectif: allume la LED RGB en vert
void green_on(){
  digitalWrite(red, HIGH);
  digitalWrite(green, LOW);
  digitalWrite(blue, HIGH);
}

//fonction blue_on()
// input: neant
// output: neant
// objectif: allume la LED RGB en bleu
void blue_on(){
  digitalWrite(red, HIGH);
  digitalWrite(green, HIGH);
  digitalWrite(blue, LOW);
}

//fonction off()
// input: neant
// output: neant
// objectif: eteint la LED 
void off(){
  digitalWrite(red, HIGH);
  digitalWrite(green, HIGH);
  digitalWrite(blue, HIGH);
}

//fonction off()
// input: addr, l'addresse I2C a tester
// output: bool, true si l'addresse est valide, false sinon
// objectif: verifie si l'adresse I2C donnee est valide en effectuant y lui envoyant une resquete de transmission 
bool i2CAddrTest(uint8_t addr) {
  Wire.begin();  //demarre la communication I2C
  Wire.beginTransmission(addr);  //envoie une requete de transmission a l'adresse I2C

  if (Wire.endTransmission() == 0)
  return true;  //rend true si la transmission est reussie 
  return false;  //false sinon 
}


//fonction ShowCardType(unsigned char * type)
// input: type, pointeur vers un tableau contenant ls type de la carte RFID
// output: neant
// objectif: lie les 2 premiers octets du tableau et affiche son type sur la console série ou "Unknown"
// si le type n'est pas reconnu
void ShowCardType(unsigned char * type) {
  Serial.print("Card type: ");
  if (type[0] == 0x04 && type[1] == 0x00)
    Serial.println("MFOne-S50");
  else if (type[0] == 0x02 && type[1] == 0x00)
    Serial.println("MFOne-S70");
  else if (type[0] == 0x44 && type[1] == 0x00)
    Serial.println("MF-UltraLight");
  else if (type[0] == 0x08 && type[1] == 0x00)
    Serial.println("MF-Pro");
  else if (type[0] == 0x44 && type[1] == 0x03)
    Serial.println("MF Desire");
  else
    Serial.println("Unknown");
}


//fonction access_denied(String cardNumber)()
// input: cardNumber, le numero de la carte refusée sous forme de String
// output: neant
// objectif: refuse l'accès et transmet le message a inscrire dans les logs
// a l'ESP8266 via la communication série
void access_denied(String cardNumber){
  lcd.clear();  //vide l'ecran pour ne pas que les lettres se superpose
  lcd.setCursor(0, 0);  //postionne le curseur au début
  lcd.print("Acces refuse");  //ecrit un message (sans accent car pas prit en compte par le LCD)
  espSerial.println("Accès refusé pour la carte " + cardNumber); //envoie un message via la liaision série a l'ESP
  for(int i=0; i<=15; i++){  //fait clignoter la LED 15fois en rouge
    red_on();
    delay(100);
    off();
    delay(100);
  }
  stand_down();  //retablit l'etat de standby sur l'ecran LCD
}

//fonction access_denied(String cardNumber)()
// input: cardNumber, le numero de la carte autorisée sous forme de String
// output: neant
// objectif: accepte l'accès et transmet le message a inscrire dans les logs
// a l'ESP8266 via la communication série
void access_granted(String cardNumber){
  lcd.clear();  //vide l'ecran pour ne pas que les lettres se superpose
  lcd.setCursor(0, 0);  //postionne le curseur au début
  lcd.print("Acces autorise");//ecrit un message (sans accent car pas prit en compte par le LCD)
  espSerial.println("Accès autorisé pour la carte " + cardNumber); //envoie un message via la liaision série a l'ESP
  for(int i=0; i<=15; i++){ //fait clignoter la LED 15fois en vert
    green_on();
    delay(100);
    off();
    delay(100);
  }
  stand_down();  //retablit l'etat de standby sur l'ecran LCD
}

//fonction stand_down()
// input: neant
// output: neant
// objectif: reinitialise l'etat de standby (LED eteint et message qui 
// demande de presentez son badge sur le LCD)
void stand_down(){
  off();  //eteint LED
  lcd.clear();  //vide le LCD
  lcd.setCursor(0, 0);  //place le curseur sur la premiere ligne
  lcd.print("Presentez votre"); //et ecrit
  lcd.setCursor(0, 1);
  lcd.print("badge");
}

void loop() {

  if (rfid.findCard(PICC_REQIDL, str) == MI_OK) { //verifie si une carte est detectée par le lecteur RFID

    if (rfid.anticoll(str) == MI_OK) { //evite de lire plusieurs cartes en meme temsp
      String cardNumber = "";  //string pour stocker le numero du tag 
      for (int i = 0; i < 4; i++) {  //recupre les 4 octets correspondant au numero du tag en hexa
        cardNumber += String(0x0F & (str[i] >> 4), HEX);  // Partie haute de l'octet
        cardNumber += String(0x0F & str[i], HEX);         // Partie basse de l'octet
      }

      if(cardNumber=="2a4d0bb0"){
        access_denied(cardNumber);  //l'acces est refusé pour la carte "2a4d0bb0"
      }else if(cardNumber=="630c68ad"){
        access_granted(cardNumber); //l'acces est autorisé pour la carte "2a4d0bb0"
      }else{ 
        access_denied(cardNumber); ////l'acces est refusé pour toute carte inconnue
      }

    }
    rfid.selectTag(str);  //apres l'anticollision, selectionne le tag
  }
  rfid.halt(); //une fois la carte traitée on la met en veille
}



