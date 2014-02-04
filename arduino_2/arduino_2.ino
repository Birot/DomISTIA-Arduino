#include <SPI.h>
#include <Ethernet.h>
#include <String.h>
#include <aJSON.h>

// ---------------------------------- CONFIGURATION DE L'ARDUINO DUEMILANOVE
// adresse MAC de l'Arduino DUEMILANOVE
byte macArduino[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0x1D, 0xA7 };
String strMacArduino="90:A2:DA:00:1D:A7";
// l'adresse IP de l'Arduino
IPAddress ipArduino(172,20,82,167);
String stringIpArduino = "172.20.82.167";
// son identifiant=IP
String idArduino="pierreRo";
// port du serveur Arduino
int portArduino=102;
// description de l'Arduino
String descriptionArduino="Une voiture puissante";
// le serveur Arduino travaillera sur le port 102
EthernetServer server(portArduino);
// IP du serveur d'enregistrement
IPAddress ipServeurEnregistrement(172,20,82,166); 
// port du serveur d'enregistrement
int portServeurEnregistrement=100;
// le client Arduino du serveur d'enregistrement
EthernetClient clientArduino;

// initialisation
void setup() {
  // Le moniteur s�rie permettra de suivre les �changes
  Serial.begin(9600);
  // d�marrage de la connection Ethernet
  Ethernet.begin(macArduino,ipArduino);  
  // m�moire disponible
  Serial.print(F("Memoire disponible : "));
  Serial.println(freeRam());
}

// boucle infinie
void loop()
{
  boolean enregistrementFait=false;
  // tant que l'arduino ne s'est pas enregistr� aupr�s du serveur, on ne peut rien faire
  while(! enregistrementFait){
    // on se connecte au serveur d'enregistrement
    Serial.println(F("Connexion au serveur d'enregistrement..."));
    if (connecte(&clientArduino,ipServeurEnregistrement,portServeurEnregistrement)) {
      // suivi
      Serial.println(F("Connecte..."));
      // on envoie l'adresse Mac de l'Arduino, une description de ses fonctionnalit�s, son port de service
      String msg="{\"id\":\""+idArduino+"\",\"desc\":\""+descriptionArduino+"\",\"mac\":\""+strMacArduino+"\",\"port\":"+portArduino+",\"ip\":\""+stringIpArduino+"\"}";
      clientArduino.println(msg);
      Serial.print(F("enregistrement : "));
      Serial.println(msg);
      // enregistrement fait
      enregistrementFait=true;
      // on ferme la connexion
      delay(1);
      clientArduino.stop();
      // suivi
      Serial.println(F("Enregistrement termine..."));
    } 
    else {
      // suivi
      Serial.println(F("Echec de la connexion..."));
    }
  }
  // suivi
  Serial.print(F("Demarrage du serveur sur l'adresse IP "));
  Serial.print(Ethernet.localIP());
  Serial.print(F(" et le port "));
  Serial.println(portArduino);

  // on lance le serveur
  server.begin();
  // on attend ind�finiment les clients
  // on les traite un par un
  while(true){
    // m�moire disponible
    Serial.print(F("Memoire disponible avant service : "));
    Serial.println(freeRam());
    // on attend un client
    EthernetClient client=server.available();
    while(! client  ){
      delay(1);
      client=server.available();
    }
    // si et tant que ce client est connect�
    while(client.connected()){
      Serial.println(F("client connecte...")); 
      // m�moire disponible
      Serial.print(F("Memoire disponible debut boucle : "));
      Serial.println(freeRam());

      // on lit la commande de ce client
      String commande=lireCommande(&client);
      Serial.print(F("commande : ["));
      Serial.print(commande);
      Serial.println(F("]"));
      // si la commande est vide, on ne la traite pas
      if(commande.length()==0){
        continue;
      }
      // sinon on la traite
      traiterCommande(&client,commande);
      // m�moire disponible
      Serial.print(F("Memoire disponible fin boucle : "));
      Serial.println(freeRam());

      // on passe � la commande suivante
      delay(1);
    }
    // on est d�connect� - on passe au client suivant
    Serial.println(F("client deconnecte..."));     
    client.stop();
    // m�moire disponible
    Serial.print(F("Memoire disponible fin service : "));
    Serial.println(freeRam());
  }
  // on ne rebouclera jamais sur le loop (normalement)
  Serial.println(F("loop"));
}

// traitement d'une commande
void traiterCommande(EthernetClient *client, String commande){
  // on d�code la commande pour voir quelle action est demand�e
  // on peut d�clarer dynamiquement un tableau de caract�res
  int l = commande.length();
  char cmd[l+1];
  commande.toCharArray(cmd, l+1);
  // on parse la commande
  aJsonObject* json=aJson.parse(cmd);
  // pb ?
  if(json==NULL){
    sendReponse(client,reponse(NULL,"100",NULL));
    return;
  }
  // attribut id
  aJsonObject* id = aJson.getObjectItem(json, "id"); 
  if(id==NULL){
    // suppression json
    aJson.deleteItem(json);
    // on envoie la r�ponse
    sendReponse(client,reponse(NULL,"101",NULL));
    // retour
    return;
  }
  // on m�morise l'id
  char* strId=id->valuestring;
  // attribut action
  aJsonObject* action = aJson.getObjectItem(json, "ac"); 
  if(action==NULL){
    // suppression json
    aJson.deleteItem(json);
    // on envoie la r�ponse
    sendReponse(client,reponse(NULL,"102",NULL));
    // retour
    return;
  }
  // on m�morise l'action
  char* strAction=action->valuestring;
  // on r�cup�re les parametres
  aJsonObject* parametres = aJson.getObjectItem(json, "pa");
  if(parametres==NULL){
    // suppression json
    aJson.deleteItem(json);
    // on envoie la r�ponse
    sendReponse(client,reponse(NULL,"103",NULL));
    // retour
    return;
  }
  // c'est bon - on traite l'action
  // echo
  if(strcmp("ec",strAction)==0){
    // traitement    
    doEcho(client, strId);
    // suppression json
    aJson.deleteItem(json);
    // retour
    return;
  }
  // clignotement
  if(strcmp("cl",strAction)==0){
    // traitement
    doClignoter(client,strId,parametres);
    // suppression json
    aJson.deleteItem(json);
    // retour
    return;
  }
  // pinWrite
  if(strcmp("pw",strAction)==0){
    // traitement
    doPinWrite(client,strId,parametres);
    // suppression json
    aJson.deleteItem(json);
    // retour
    return;
  }
  // pinRead
  if(strcmp("pr",strAction)==0){
    // traitement
    doPinRead(client,strId,parametres);
    // suppression json
    aJson.deleteItem(json);
    // retour
    return;
  }

  // ici, l'action n'est pas reconnue
  // suppression json
  aJson.deleteItem(json);
  // r�ponse
  sendReponse(client, reponse(strId,"104",NULL));
  // retour
  return;
}

// connexion au serveur
int connecte(EthernetClient *client, IPAddress serveurIP, int serveurPort) {
  // on se connecte au serveur apr�s 1 seconde d'attente
  delay(1000);
  return client->connect(serveurIP, serveurPort);
}

// lecture d'une commande du serveur
String lireCommande(EthernetClient *client){
  String commande="";

 // on rend la commande
  while(client->available()){
    char c = client->read();
    commande+=c;
  }
  return commande;
}

// formatage json de la r�ponse au client
String reponse(String id, String erreur, String etat){
  // cha�ne json de la r�ponse au client
  // attribut id
  if(id==NULL){
    id="";
  }
  // attribut erreur
  if(erreur==NULL){
    erreur="";
  }
  // attribut etat - pour l'instant dictionnaire vide
  if(etat==NULL){
    etat="{}";
  }
  // construction de la r�ponse
  String reponse="{\"id\":\""+id+"\",\"er\":\""+erreur+"\",\"et\":\""+etat+"\"}";

  // r�sultat
  return reponse;
}

// echo
void doEcho(EthernetClient *client, char * strId){
  // on fait l'�cho
  sendReponse(client, reponse(strId,"0",NULL));
}

//clignoter
void doClignoter(EthernetClient *client,char * strId, aJsonObject* parametres){
  // exploitation des parametres
  // il faut une pin
  aJsonObject* pin = aJson.getObjectItem(parametres, "pin");
  if(pin==NULL){
    // r�ponse d'erreur
    sendReponse(client, reponse(strId,"202",NULL));
    return;
  }
  // valeur de la pin � eteindre
  int led=atoi(pin->valuestring);
  Serial.print(F("clignoter led="));
  Serial.println(led);
  // il faut la dur�e d'un clignotement
  aJsonObject* dur = aJson.getObjectItem(parametres, "dur");
  if(dur==NULL){
    // r�ponse d'erreur
    sendReponse(client, reponse(strId,"211",NULL));
    return;
  }
  // valeur de la dur�e
  int duree=atoi(dur->valuestring);
  Serial.print(F("duree="));
  Serial.println(duree);
  // il faut le nombre de clignotements
  aJsonObject* nb = aJson.getObjectItem(parametres, "nb");
  if(nb==NULL){
    // r�ponse d'erreur
    sendReponse(client, reponse(strId,"212",NULL));
    return;
  }
  // valeur du nombre de clignotements
  int nbClignotements=atoi(nb->valuestring);
  Serial.print(F("nb="));
  Serial.println(nbClignotements);

  // on rend la r�ponse tout de suite
  sendReponse(client, reponse(strId,"0",NULL));

  // on op�re le clignotement
  pinMode(led, OUTPUT);
  for(int i=0;i<nbClignotements;i++)
  {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(duree);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(duree);               // wait for a second
  }

}


// pinWrite
 void doPinWrite(EthernetClient *client, char * strId, aJsonObject* parametres){
 // il faut une pin
 aJsonObject* pin = aJson.getObjectItem(parametres, "pin");
 if(pin==NULL){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"201",NULL));
 return;
 }
 // num�ro de la pin � �crire
 int pin2= atoi(pin->valuestring);
 // suivi
 Serial.print(F("pw pin="));
 Serial.println(pin2);
 // il faut une valeur
 aJsonObject* val = aJson.getObjectItem(parametres, "val");
 if(val==NULL){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"202",NULL));
 return;
 }
 // valeur � �crire
int val2=atoi(val->valuestring);
 // suivi
 Serial.print(F("pw val="));
 Serial.println(val2);
 // il faut un mode d'�criture
aJsonObject* mod = aJson.getObjectItem(parametres, "mod");
 if(mod==NULL){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"203",NULL));
 return;
 }
 char *mod2 =mod->valuestring;
 // ce doit �tre a (analogique) ou b (binaire)
 if(strcmp("b",mod2)!=0 && strcmp("a",mod2)!=0){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"204",NULL));
 return;
 }
 // c'est bon pas d'erreur
 sendReponse(client,reponse(strId,"0",NULL));
 // suivi
 Serial.print(F("pw mod="));
 Serial.println(mod2);
 
 // on �crit sur la pin
  pinMode(pin2, OUTPUT);
  if(strcmp("b",mod2)==0){
    
    if(val2==1){
      digitalWrite(pin2,HIGH);
    }else{
       
          digitalWrite(pin2,LOW);
       
    }
      
  }else{
    analogWrite(pin2, val2); 
  }
 }
 
 //pinRead
 void doPinRead(EthernetClient *client,char * strId, aJsonObject* parametres){
 // exploitation des parametres
 // il faut une pin
aJsonObject* pin = aJson.getObjectItem(parametres, "pin");
 if(pin==NULL){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"302",NULL));
 return;
 }
 // num�ro de la pin � lire
  int pin2= atoi(pin->valuestring);
 // suivi
 Serial.print(F("pr pin="));
 Serial.println(pin2);
 // il faut un mode d'�criture
 aJsonObject* mod = aJson.getObjectItem(parametres, "mod");
 if(mod==NULL){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"303",NULL));
 return;
 }
 char *mod2 =mod->valuestring;
 // ce doit �tre a (analogique) ou b (binaire)
 if(strcmp(mod2,"a")!=0 && strcmp(mod2,"b")!=0){
 // r�ponse d'erreur
 sendReponse(client, reponse(strId,"304",NULL));
 return;
 }
 // c'est bon pas d'erreur  
 // suivi
 Serial.print(F("pr mod="));
 Serial.println(mod2);
 // on lit la pin
 int valeur =0;
 if(strcmp("b",mod2)==0){
    valeur=digitalRead(pin2);
      
  }else{
    valeur=analogRead(pin2); 
  }
 // on rend le r�sultat
 String json="{\"pin"+String(pin2)+"\":\""+valeur+"\"}";  
 sendReponse(client, reponse(strId,"0",json));
 }

// envoi r�ponse
void sendReponse(EthernetClient *client, String message){
  // envoi de la r�ponse
  client->println(message);
 
  // suivi
  Serial.print(F("reponse="));
  Serial.println(message);

}

// mémoire libre
int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

