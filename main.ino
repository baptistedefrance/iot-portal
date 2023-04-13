#include <Servo.h> // import du module Servo
#include <SPI.h> // SPI
#include <MFRC522.h> // RFID
#define SS_PIN 10
#define RST_PIN 9
#define Max_Acces 3
#define Gate_PIN 3
#define AccesFlag_PIN 2
#include "IRremote.h"
#include <RTClib.h>

RTC_DS3231 rtc;
//télécomande
IRrecv irrecv(A1);
decode_results results;
    
// Déclaration 
MFRC522 rfid(SS_PIN, RST_PIN); 

// Tableau contentent l'ID
byte nuidPICC[4];

Servo porte;
byte Count_acces = 0;
byte CodeVerif = 0;
byte Code_Acces[4] = {0x15, 0x14, 0x13, 0xD3};


const int MAX_VALUES = 100;
int values[MAX_VALUES];


// Fonction mesurant la distance en cm au détecteur
long readUltrasonicDistance(int pinTrigger, int pinEcho)
{
  pinMode(pinTrigger, OUTPUT);  // Clear the trigger
  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);
  pinMode(pinEcho, INPUT);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return 0.01723 * pulseIn(pinEcho, HIGH);
}


// Déclaration des variables utilisées
long distance;
int lum;
int compt=0;


// Définition des pins
int pinLED1 = 4; 
int pinLED2 = 7;
int pinLED3 = 8;
int pinServo = 9;
int pinBuzzer = 6;
int pinTrigger = 3;
int pinEcho = 2;

int seuilDistance = 30;
long tempsAttente = 5000;



// Fonction setup
void setup() {

  Serial.begin(9600); // ouvre le port série
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  irrecv.enableIRIn();
  pinMode(12, OUTPUT);  // initialise les broches


  // Init RS232
  Serial.begin(9600);

  // Init SPI bus
  SPI.begin(); 

  // Init MFRC522 
  rfid.PCD_Init(); 
  
 
  pinMode(pinLED1,OUTPUT); // définit le port LED sur OUTPUT
  pinMode(pinLED2,OUTPUT); // définit le port LED sur OUTPUT
  pinMode(pinLED3,OUTPUT); // définit le port LED sur OUTPUT
  pinMode(pinBuzzer,OUTPUT); // définit le port Buzzer sur OUTPUT
  pinMode(pinTrigger,OUTPUT); // définit le port Trigger sur OUTPUT
  pinMode(pinEcho,INPUT); // définit le port Echo sur INPUT
  porte.attach(pinServo); 
  Serial.begin(9600); // ouvre la connexion Serial sur 9600 bauds
  while (!Serial); // attend que le Serial soit actif

}


// Fonction loop
void loop() {
  if (irrecv.decode(&results)) {
        if (results.value == 4294967295) {
          Serial.println(results.value);
          Serial.println("action bouton télécommande ");
          digitalWrite(12, HIGH);
          actionporte();

        }
        if (results.value == 16724175) {  digitalWrite(12, LOW); }
  
        irrecv.resume();
    }



  // Initialisé la boucle si aucun badge n'est présent 
  if ( !rfid.PICC_IsNewCardPresent())
    return;

  // Vérifier la présence d'un nouveau badge 
  if ( !rfid.PICC_ReadCardSerial())
    return;

  // Enregistrer l'ID du badge (4 octets) 
  for (byte i = 0; i < 4; i++) 
  {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  
  // Affichage de l'ID 
  Serial.println("Un badge est détecté");
  Serial.println(" L'UID du tag est:");
  for (byte i = 0; i < 4; i++) 
  {
    Serial.print(nuidPICC[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Re-Init RFID
  rfid.PICC_HaltA(); // Halt PICC
  rfid.PCD_StopCrypto1(); // Stop encryption on PCD

  
  // lecture de la distance de l'objet
  distance = readUltrasonicDistance(pinTrigger,pinEcho); 
  Serial.println("Distance detectee : " + String(distance) + " cm");
  for (byte i = 0; i < 4; i++) 
  {
    Serial.print(nuidPICC[i], HEX);
    Serial.print(" ");
  }
  
  CodeVerif= GetAccesState(Code_Acces,nuidPICC); 
  if (CodeVerif!=1)
  {
    Count_acces+=1;
    if(Count_acces==Max_Acces)
    {
     // Dépassement des tentatives (clignotement infinie) 
     while(1)
     {
      digitalWrite(AccesFlag_PIN, HIGH);
      delay(200); 
      digitalWrite(AccesFlag_PIN, LOW);
      delay(200); 
      // Affichage 
      Serial.println("Alarme!");
      digitalWrite(pinBuzzer,HIGH);
      Serial.println("Buzzer !");
      delay(200);
      digitalWrite(pinBuzzer,LOW);
     }
    }
    else
    {
      // Affichage 
      Serial.println("Code érroné");
    
      // Un seul clignotement: Code erroné 
      digitalWrite(AccesFlag_PIN, HIGH);
      delay(1000); 
      digitalWrite(AccesFlag_PIN, LOW);
    }
  }
  else
  {
    // Affichage 
    Serial.println("-------------------------------------------");
    Serial.println("Ouverture de la porte");
    Serial.println("-------------------------------------------");
    
    // Ouverture de la porte & Initialisation 
    digitalWrite(Gate_PIN, HIGH);
    delay(3000); 
    digitalWrite(Gate_PIN, LOW);
    Count_acces=0; 
    if (distance < seuilDistance) {
   
      actionporte();
    
    
  }
  
  // sinon ne rien faire
  else {
    porte.write(0);
    Serial.println("-------------------------------------------");
    Serial.println("Aucun objet detecte");
    Serial.println("-------------------------------------------");
    delay(200);
  }
  }


  // si la distance est inférieure à un certain seuil, actionner la porte
  

}


// Fonction d'action sur la porte 
void actionporte() {
  
  // Le Buzzer sonne
  digitalWrite(pinBuzzer,HIGH);
  Serial.println("Buzzer !");
  delay(200);
  digitalWrite(pinBuzzer,LOW);
  
  // Rotation du servomoteur


  addValue(1);
  printIfOne();

  

  Serial.println("-------------------------------------------");
  Serial.println("Ouverture de la porte");
  Serial.println("-------------------------------------------");
  porte.write(25);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(50);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(75);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(100);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(125);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(150);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(180);
  action_lum_analog();
  delay(500);
  extinctionlum();

  // Détection de la lumière et allumage des LED
  lum = analogRead(0);
  Serial.println("Lumiere : " + String(lum));
  
  //actionlumiere();
  action_lum_analog();
  
  // Attendre que le véhicule rentre
  delay(tempsAttente);
  int count = 0;
  while (readUltrasonicDistance(pinTrigger,pinEcho) < seuilDistance) {
    Serial.println("bloquer");
    Serial.println(distance);
    porte.write(150);    
    count += 1;
  }
  Serial.println(count);

  digitalWrite(pinBuzzer,HIGH);
  Serial.println("Buzzer !");
  delay(200);
  digitalWrite(pinBuzzer,LOW);

  // Fermeture du portail
  Serial.println("-------------------------------------------");
  Serial.println("Fermeture de la porte et extinction des LED");
  Serial.println("-------------------------------------------");
  porte.write(150);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(125);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(100);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(75);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(50);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(25);
  action_lum_analog();
  delay(500);
  extinctionlum();
  porte.write(0);
  action_lum_analog();
  delay(500);
  extinctionlum();
  
  // Extinction des LED
  digitalWrite(pinLED1,LOW);
  digitalWrite(pinLED2,LOW);
  digitalWrite(pinLED3,LOW);
  delay(1000);
}
void addValue(int value) {
  static int index = 0;     // Stocker l'indice actuel du tableau en mémoire statique

  if (index < MAX_VALUES) { // Vérifier si le tableau n'est pas plein
    values[index] = value;  // Ajouter la valeur au tableau
    index++;                // Augmenter l'indice
  }

  //printValues();            // Imprimer le tableau à chaque fois qu'une valeur est ajoutée
}
void printValues() {
  Serial.print("Values: [ ");

  for (int i = 0; i < MAX_VALUES; i++) {
    Serial.print(values[i]);
    Serial.print(" ");
  }

  Serial.println("]");
}
void printIfOne() {
  int count = 0;                  // Compteur pour le nombre de 1 trouvés dans le tableau

  for (int i = 0; i < MAX_VALUES; i++) {
    if (values[i] == 1) {
      count++;                    // Incrémenter le compteur à chaque fois qu'un 1 est trouvé
      if (count == 1) {           // Si c'est le premier 1 trouvé, imprimer la date et l'heure
        printDateTime();
      }
      Serial.print("ouverture - ");
      printDateTime();            // Imprimer la date et l'heure à côté de chaque occurrence de "ouverture"
    }
  }
}

void printDateTime() {
  DateTime now = rtc.now();       // Récupérer la date et l'heure actuelles depuis le module RTC
  Serial.print("Date/heure : ");
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}


void extinctionlum(){
  // Extinction des LED
  digitalWrite(pinLED1,LOW);
  digitalWrite(pinLED2,LOW);
  digitalWrite(pinLED3,LOW);
  delay(1000);
}


// Fonction d'activation de la lumière
void actionlumiere() {
  if (lum > 800) {
    Serial.println("Extinction des 3 LEDS");
    digitalWrite(pinLED1,LOW);
    digitalWrite(pinLED2,LOW);
    digitalWrite(pinLED3,LOW);
  }
  else if (lum > 500) {
    Serial.println("Allumage d'une LED");
    digitalWrite(pinLED1,HIGH);
    digitalWrite(pinLED2,LOW);
    digitalWrite(pinLED3,LOW);
  }
  else if (lum > 250) {
    Serial.println("Allumage de 2 LED");
    digitalWrite(pinLED1,HIGH);
    digitalWrite(pinLED2,HIGH);
    digitalWrite(pinLED3,LOW);
  }
  else if (lum >0) {
    Serial.println("Allumage des 3 LEDS");
    digitalWrite(pinLED1,HIGH);
    digitalWrite(pinLED2,HIGH);
    digitalWrite(pinLED3,HIGH);
  }
}

// 
void action_lum_analog() {
  int conv_lum;
  conv_lum = 255 - int(float(lum)/1023*255);
  analogWrite(pinLED1,conv_lum);
  analogWrite(pinLED2,conv_lum);
  analogWrite(pinLED3,conv_lum);
}


byte GetAccesState(byte *CodeAcces,byte *NewCode) 
{
  byte StateAcces=0; 
  if ((CodeAcces[0]==NewCode[0])&&(CodeAcces[1]==NewCode[1])&&
  (CodeAcces[2]==NewCode[2])&& (CodeAcces[3]==NewCode[3]))
    return StateAcces=1; 
  else
    return StateAcces=0; 
}


