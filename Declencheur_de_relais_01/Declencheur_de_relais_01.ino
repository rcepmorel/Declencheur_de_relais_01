/*      DÉCLENCHEUR DE RELAIS
* 
* Auteur : Richard Morel
*     2018-11-09
* 
* Modification
*     2018-11-17
*     - Mesure du voltage d'entrée
*     - Ajout du clignotement de la DEL bleue
*       Relié sur la broche D33
*    2018-11-30
*     - Ajout du déclencheur de relais
*       Relié sur le GPIO 32
*     - Ajout du détecteur du rayon laser pour déclencher le relais
*       sur la coupure du faisceau
*       Relié sur la GPIO 13
*     - Ajout d'un interrupteur pour déclencher le relais
*       Relié sur la GPIO 25
*     - La DEL bleue suit l'état du détecteur laser
*    2018-11-01
*     - Ajout du serveur WEB
*     - Déplacer l'interrupteur pour déclencher le relais
*       sur le GPIO 26
*     - Remplacement de la DEL bleue par une DEL RGB
*     - La broche «rouge» de la DEL RGB se relie sur GPIO 25
*     - La broche «verte» de la DEL RGB se relie sur GPIO 33
*     2019-01-13
*     - Quelques corrections pour donner suite aux avertissements du compilateur
*     
*/

// ----------------------------------------------------------------------------- 
//             Importation des fichiers et définition des variables
// ----------------------------------------------------------------------------- 

//******* Affichage DEL ********
// Afin d'ajuster au besoin l'intensité et la couleur de la DEL, le programme
// fait appel une fonction (ledcWrite) qui module la sortie PWM des broches
// selon la valeur fournie à la fonction
#define LEDC_CHANNEL_0_VERT     0
#define LEDC_CHANNEL_1_ROUGE    1

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT  13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

#define DELROUGE  25 // DEL rouge reliée au GPIO25
#define DELVERTE  33 // DEL verte reliée au GPIO33

byte byBrightnessVert;
byte byBrightnessRouge;

//******* Convertisseurs Analogique/Numérique ******
#include <driver/adc.h>   
//ADC1_CHANNEL_7 D35 35

//*******   EEPROM   *****************
// Inclure la librairie de lecture et d'écriture de la mémoire flash
#include "EEPROM.h"

// définie le nombre de Bytes que l'on veut accéder
#define EEPROM_SIZE 64

//******  WIFI *********************
#include "connect_id_Wifi.h"
#include <WiFi.h>
WiFiServer server(80);

const char* chStatusWifi[] ={"WL_IDLE_STATUS", "WL_NO_SSID_AVAIL", "-", 
                       "WL_CONNECTED ", "WL_CONNECT_FAILED ", "-",
                       "WL_DISCONNECTED"};

String stWifiConnectionNetwork = "Connecté";

// ***** Mesure de voltage ************  
float flCalibrationDiviseurTsn = 8.02/7.97; // Valeur réelle diviser par
                                            // flVoltMesure lorsque 
                                            // flCalibrationDiviseurTsn
                                            // égal à 1

int inSeuilDeVoltage = 7;              // Niveau d'alerte de basse tension 
                                       // de l'alimentation d'entrée

float flReading;
float flVoltAlimCtrlReduit;
float flVoltMesure;


//*****  Interrupteurs, détecteurs et sorties ******** 
#define PIN_ACTIVE_RELAIS      32      // Sortie pour déclencher le relais
#define PIN_DETECTEUR_LASER_1  17      // Détecte une MALT à l'entrée D17
                                       // Détecteur fixe sur la plaquette
#define PIN_BOUTON_DECLENCHEUR 26      // Détecte une MALT à l'entrée D26

boolean blEtatLaser1;
boolean blTamponEtatLaser1;
boolean blEtatBtnDeclencheur; 
boolean blTamponEtatBouton    = 1;
boolean blDeclencheRelais     = 0;
boolean blUnitDlReactMicroScd = 0;     // Unité du délai de réaction 
                                       // 1 => microseconde
                                       // 0 => milliseconde

// La variable suivante est de type «uint64_t» car le temps, 
// mesuré en millisecondes, deviendra rapidement un nombre grand
uint64_t u64HorodatageBtnDeclencheur = 0;  // La dernière fois que la broche
                                           // de sortie a été basculée
                                         
uint32_t u32DelaiReActInterup = 3000;  // Délai de réactivation
                                       // Pas plus d'un changement d'état 
                                       // au 3 secondes pour éliminer
                                       // les intermittences    
                                   

uint64_t u64HorodatageRayonCoupe = 0;  // La dernière fois que le rayon
                                       // laser a été coupé
uint32_t u32DelaiReActRayon = 3000;    // Pas plus d'un changement d'état 
                                       // au 3 secondes pour éliminer
                                       // les intermittences    

// Les prochaines variables sont de type «unsigned int»
// Ceci évite des valeurs négatives lors de la première lecture du EEPROM
// au SETUP avant qu'il y est eu une première écriture
// Sinon, il pourrait y avoir blocage du programme
// Ces données sont aléatoires avant la première écriture                                
uint32_t u32DelaiDeReaction  = 0;      // Durée d'attente avant d'activer le relais
uint32_t u32TempsRelaisActif = 2000;   // Durée d'activation du relais en milliseconde
uint32_t u32DelaiAvantRetour = 1000;   // Délai en milliseconde avant de redonner
                                       // la main à la suite du programme après
                                       // l'activation du relais


//******** Divers ***************
int inChronoDebutIndicModReseau = 0;   // Chrono Indique le mode réseau
int inChronoDebutIndicBattFbl   = 0;   // Chrono Indique batterie faible

 
// ------------------------------------------------------------------------------- 
// FONCTION  FONCTION   FONCTION    FONCTION    FONCTION    FONCTION    FONCTION
// ------------------------------------------------------------------------------- 

//*****  Activation du DEL   *****************
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  if (value > valueMax){value = valueMax;}
  uint32_t duty = (8191 / valueMax) * value;
  
  // write duty to LEDC
  ledcWrite(channel, duty);
}

//******  Enregistrement dans la mémoire flash *******
void memFlashDataUser(){
  EEPROM.writeInt(0,  u32DelaiDeReaction);
  EEPROM.writeInt(4,  u32TempsRelaisActif);
  EEPROM.writeInt(8,  u32DelaiAvantRetour);
  EEPROM.writeInt(12, blUnitDlReactMicroScd);
  EEPROM.commit();
}

//***  Attente selon le délai de réaction ****
void delaiDeReaction(){
  if (blUnitDlReactMicroScd){
    delayMicroseconds(u32DelaiDeReaction);   // délai avant d'activer
                                             // le relais (en microseconde)
  }
  else {
    delay(u32DelaiDeReaction);               // délai avant d'activer
                                             // le relais (en milliseconde)
  } 
}

//******   Activation du relais **************
void declencheRelais(){
  digitalWrite(PIN_ACTIVE_RELAIS, HIGH);     // Déclenche le relais
  delay(u32TempsRelaisActif);                // Attente en milliseconde 
  digitalWrite(PIN_ACTIVE_RELAIS, LOW);      // Relâche le relais
  delay(u32DelaiAvantRetour);                // Attente en milliseconde                    
}

//*****  Choix d'affichage de la couleur du DEL *******
void AfficheVertCWRougePA(){
   if (stWifiConnectionNetwork=="Non connecté au réseau") {
      byBrightnessVert  = 0 ;
      byBrightnessRouge = 255 ;
      // set the brightness on LEDC channel 0
      ledcAnalogWrite(LEDC_CHANNEL_0_VERT, byBrightnessVert);
      // set the brightness on LEDC channel 1
      ledcAnalogWrite(LEDC_CHANNEL_1_ROUGE,byBrightnessRouge);
  }
  else
  {
      byBrightnessVert  = 255;
      byBrightnessRouge = 0 ;
      ledcAnalogWrite(LEDC_CHANNEL_0_VERT, byBrightnessVert);
      ledcAnalogWrite(LEDC_CHANNEL_1_ROUGE,byBrightnessRouge);
  }
}


 // *******  html Serveur Web ******** 
 #include "do_Web.h"
 
// -------------------------------------------------------------------------------
// SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP
// ------------------------------------------------------------------------------- 
void setup() {
  Serial.begin(115200);
  Serial.println();


  //**********  EEPROM *************
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  
  Serial.println();
 
  // Lecture des valeurs sauvegardées dans le EEPROM 
  u32DelaiDeReaction     = EEPROM.readInt(0);
  u32TempsRelaisActif    = EEPROM.readInt(4); 
  u32DelaiAvantRetour    = EEPROM.readInt(8);
  blUnitDlReactMicroScd  = EEPROM.readInt(12);  

  if (u32DelaiDeReaction == 4294967295){ // valeur par défaut du ESP
      u32DelaiDeReaction  = 0;           // Changer sinon le programme est figé  
      u32TempsRelaisActif = 1000;        // pour longtemps 
      u32DelaiAvantRetour = 0;
      
      memFlashDataUser();
      
      u32DelaiDeReaction     = EEPROM.readInt(0);  // Validation 
      u32TempsRelaisActif    = EEPROM.readInt(4); 
      u32DelaiAvantRetour    = EEPROM.readInt(8);
      blUnitDlReactMicroScd  = EEPROM.readInt(12);  
  }
  Serial.print(u32DelaiDeReaction);
  Serial.println(" delaiDeReaction " );

  Serial.print(u32TempsRelaisActif);
  Serial.println(" tempsRelaisActif " );

  Serial.print(u32DelaiAvantRetour);
  Serial.println(" delaiAvantRetour " );

  Serial.print(blUnitDlReactMicroScd);
  Serial.println(" Bool unité délai de réaction " );

  //**********  ENTRÉES - SORTIES *************
  pinMode(PIN_ACTIVE_RELAIS, OUTPUT );
  digitalWrite(PIN_ACTIVE_RELAIS, LOW);

  pinMode(PIN_DETECTEUR_LASER_1, INPUT);
  pinMode(PIN_BOUTON_DECLENCHEUR, INPUT_PULLUP);   // active la résistance
                                                   // de pull-up interne sur le +3V

  //**********  CONVERTISSEURS ANALOGIQUE/NUMÉRIQUE *************
  //     Configuration pour faire des mesures sur D35
  //     (voltage de la source d'alimentation) ******
  adc1_config_width(ADC_WIDTH_BIT_12);         // Définie la résolution (0 à 4095)
  adc1_config_channel_atten(ADC1_CHANNEL_7,    // Le voltage maximum au GPIO
                            ADC_ATTEN_DB_11);  // est de 3.3V

  // *********** WIFI ****************************
  // Configuration d'une adresse IP statique
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(ssid, password);
  delay(1000);
  uint8_t retries = 15;
 
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print("..");
      Serial.print(WiFi.status());
      Serial.print("-");
      Serial.println(retries);
      retries--;
      if (retries == 0) {
        int intStatus = WiFi.status();
        if (intStatus==255){intStatus=2;}
        Serial.print("..");
        Serial.print(WiFi.status());
        Serial.print("-");
        Serial.println(retries);
        Serial.print("WiFi status :");
        Serial.print(WiFi.status());
        Serial.print("->");
        Serial.println(chStatusWifi[intStatus]); 
        stWifiConnectionNetwork = "Non connecté au réseau";
        break;
      }
  }

    Serial.println(stWifiConnectionNetwork);
  if (stWifiConnectionNetwork=="Non connecté au réseau") {
     Serial.println("- * Activation du point d'accès Wifi * - ");
     WiFi.softAP(ssid_AP, password_AP);
     IPAddress myIP = WiFi.softAPIP();
     Serial.print("AP IP address: ");
     Serial.println(myIP);
  }
  else{
  Serial.print("WiFi status :");
  Serial.println(WiFi.status());
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  }
    
   server.begin();
   
  //**********  DEL *************
  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_0_VERT, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(DELVERTE, LEDC_CHANNEL_0_VERT);
  ledcSetup(LEDC_CHANNEL_1_ROUGE, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(DELROUGE, LEDC_CHANNEL_1_ROUGE);

  AfficheVertCWRougePA();
  
  //***** Divers ******
  inChronoDebutIndicModReseau = millis();
  Serial.println("PRÊT");
}

// ------------------------------------------------------------------------------- 
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP 
// ------------------------------------------------------------------------------- 
void loop() {  
  //******* Lecture des entrées numériques  ******  
  blEtatLaser1 = digitalRead(PIN_DETECTEUR_LASER_1);
 if (blEtatLaser1 != blTamponEtatLaser1){
    if ((millis() - u64HorodatageRayonCoupe) > u32DelaiReActRayon) {
        blTamponEtatLaser1 = blEtatLaser1;         // Mémorise le nouvel état
        u64HorodatageRayonCoupe = millis();
    }
  }

  blEtatBtnDeclencheur = digitalRead(PIN_BOUTON_DECLENCHEUR);
  if (blEtatBtnDeclencheur != blTamponEtatBouton){
     if ((millis() - u64HorodatageBtnDeclencheur) > u32DelaiReActInterup) {
        blTamponEtatBouton = blEtatBtnDeclencheur; // Mémorise le nouvel état
        u64HorodatageBtnDeclencheur = millis();
      }
  }

  //***********  ACTIONS *****************
  if (!blTamponEtatLaser1){;   // si l'état du détecteur est 0 (faisceau coupé)
    delaiDeReaction();
    declencheRelais();
    blTamponEtatLaser1 = 1;
  }
  
  if (!blTamponEtatBouton){;   // si l'état du bouton est 0
    delaiDeReaction();
    declencheRelais();
    blTamponEtatBouton = 1;
  }

  if (blDeclencheRelais){;     // si la commande web D reçue
    blDeclencheRelais = 0;
    delaiDeReaction();
    declencheRelais();
  }

  // ******* ********   Lecture du voltage d'alimentation   **********************
  //    
  // Méthode de G6EJD  (applicable si l'atténuation est 11dB
  // et la résolution est 12 bits)
  flReading = analogRead(35);
  flVoltAlimCtrlReduit = -0.000000000000016*pow(flReading,4)
                               +0.000000000118171*pow(flReading,3)
                               -0.000000301211691*pow(flReading,2)
                               +0.001109019271794*flReading+0.034143524634089;
  flVoltMesure = flVoltAlimCtrlReduit*122/22;     // Diviseur de tension
                                                        // 100K, 22K
  flVoltMesure = flVoltMesure*flCalibrationDiviseurTsn; // correction attribuable aux 
                                                        // valeurs imprécises du diviseur 
                                                        // de tension et au changement
                                                        // de ESP32
  
  // Serial.print(flReading); Serial.print(" DigitalValueVoltAlimCtrl ");
  // Serial.print(flVoltAlimCtrlReduit); Serial.print(" flVoltAlimCtrlReduit  ");
  // Serial.print(flVoltMesure); Serial.println(" flVoltMesure"); 
  // Serial.println(" "); 
  
  Serial.print(blEtatLaser1); Serial.print(" État détecteur Laser,");
  Serial.print(blEtatBtnDeclencheur); Serial.print(" État Interrupteur,");
  Serial.print(flVoltMesure); Serial.println(" Voltage d'alimentation,");

  // *****************   Traitement de l'affichage du DEL  ************************
  //     
  if (flVoltMesure > inSeuilDeVoltage ){          // Voltage d'alimentation > 7 volts 
     AfficheVertCWRougePA();
  }else{                                          // Voltage d'alimentation < 7 volts 
    if ((millis() - inChronoDebutIndicModReseau) > 1000){
       if (inChronoDebutIndicBattFbl == 0) {
          inChronoDebutIndicBattFbl= millis();    // Démarre le chrono Indique
                                                  // batterie faible
       }   
       if ((millis() - inChronoDebutIndicBattFbl) < 1000){ // DEL ORANGE pendant
                                                           // 1 seconde
          ledcAnalogWrite(LEDC_CHANNEL_0_VERT,   32);
          ledcAnalogWrite(LEDC_CHANNEL_1_ROUGE, 128);
       } 
       else{
          AfficheVertCWRougePA();                 // Vert Client Wifi
                                                  // Rouge Point d'accès
          inChronoDebutIndicModReseau = millis(); // Démarre le chrono
                                                  // Indication le mode réseau
          inChronoDebutIndicBattFbl = 0;          // Remet à zéro le chrono
                                                  // Indication batterie faible
      }
    }
  }

  do_Web();
  
} // # Fin du
