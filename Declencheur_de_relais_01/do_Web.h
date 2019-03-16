// ------------------------------------------------------------------------------- 
//   serveur WEB    serveur WEB     serveur WEB      serveur WEB      serveur WEB 
// ------------------------------------------------------------------------------- 

void do_Web(){
   WiFiClient client = server.available(); // Écouter les clients entrants

  if (client) {                           // si vous avez un client,
    Serial.println("Nouveau Client!");    // imprimer un message sur le port série
    int inChronoDebutConnexion= millis();   
    String currentLine = "";              // Variable de type String pour contenir
                                          // les données entrantes du client
    while (client.connected()) {          // boucle alors que le client est connecté

      if ((millis() - inChronoDebutConnexion) > 1000){ // Déconnecte les fureteurs qui restent
          client.stop();                               // connecter sans avoir d'échange à
          inChronoDebutConnexion= millis();            // faire comme Chrome                           
       }   
       
      if (client.available()) {           // s'il y a des octets à lire du client,
         char c = client.read();          // lire un octet, puis
         //Serial.write(c);               // Imprimez sur le moniteur série
         if (c == '\n') {                 // si l'octet est un caractère
                                          // de nouvelle ligne

           // si la ligne en cours est vide, vous avez
           // deux caractères de nouvelle ligne consécutifs.
           // c'est la fin de la requête HTTP du client,
           // alors envoyez une réponse:
           if (currentLine.length() == 0) {
             client.println("HTTP/1.1 200 OK");
             client.println("Content-Type: text/html\n");
             
             // Début du HTML
             client.println("<!DOCTYPE html><html>");
             client.println("<head>");
             client.println("<title>D&Eacute;CLENCHEUR Web Server</title>");
             client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
             client.println("<link rel=\"icon\" href=\"data:,\">");
             // CSS to style the on/off buttons 
             client.println("<style>html { font-family: Helvetica;}");
             client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
             client.println("text-decoration: none; font-size: 40px; margin: 2px; cursor: pointer;}");
             client.println("</style>");
             client.println("</head>");

             client.println("<table align=\"center\"  style=\"background-color:#5EA084\"><tr><td>");
             
             client.println("<table>");
             client.println("<tr><th colspan=\"2\"><h1>D&Eacute;CLENCHEUR</h1></th></tr>");
             
             client.println("<tr><td>");
             client.println(flVoltMesure);
             client.println("</td><td>");
             client.println("V - Voltage de la source d'alimentation");
             client.println("</td></tr>");
             
             client.println("<tr><td>");
             client.println(u32DelaiDeReaction);
             client.println("</td><td>");
             if (blUnitDlReactMicroScd){
                client.println("us ");
             }
             else {
                client.println("ms ");
             }
             client.println("D&eacute;lai de r&eacute;action");
             client.println("</td></tr>");
             
             client.println("<tr><td>");
             client.println(u32TempsRelaisActif);
             client.println("</td><td>");
             client.println("ms Dur&eacute;e d'activation du relais<br>");
             client.println("</td></tr>");
             
             client.println("<tr><td>");
             client.println(u32DelaiAvantRetour);
             client.println("</td><td>");
             client.println("ms D&eacute;lai avant le retour aux d&eacute;tecteurs");
             client.println("</td></tr>");
             
             client.println("<tr><th colspan=\"2\"><br><a href=\"/D\"><button class=\"button\">RELAIS</button></a><br><br></th></tr>");
             client.println("</table>");
             
             client.println("<form action=\"/action_page.php\">");
             client.println("<table><tr><td>D&eacute;lai de r&eacute;action:</td><td><input type=\"text\" name=\"dlyReact\"></td></tr>");
             client.println("<tr><td>Temps actif:</td><td><input type=\"text\" name=\"TmpsActif\"></td></tr>");
             client.println("<tr><td>D&eacute;lai de retour:</td><td><input type=\"text\" name=\"dlyRtour\"></td></tr>");

             if (blUnitDlReactMicroScd){
                client.println("<tr><td colspan=\"2\"><input type=\"radio\" name=\"unitDelaiReactMS\" value=\"1\" checked>D&eacute;lais de r&eacute;action en microseconde</td></tr>");
                client.println("<tr><td colspan=\"2\"><input type=\"radio\" name=\"unitDelaiReactMS\" value=\"0\" >D&eacute;lais de r&eacute;action en milliseconde</td></tr>");
             }
             else {
                client.println("<tr><td colspan=\"2\"><input type=\"radio\" name=\"unitDelaiReactMS\" value=\"1\" >D&eacute;lais de r&eacute;action en microseconde</td></tr>");
                client.println("<tr><td colspan=\"2\"><input type=\"radio\" name=\"unitDelaiReactMS\" value=\"0\" checked>D&eacute;lais de r&eacute;action en milliseconde</td></tr>");
             }
                          
             client.println("<tr><th colspan=\"2\"><br><input type=\"submit\" value=\"Soumettre\"><br><br></th></tr>");
             client.println("</table>");
             client.println("</form>");
             
             client.println("<table>");
             client.println("<tr><td><br>&nbsp;&nbsp;<a href=\"/P\">+ Augmenter le d&eacute;lai de r&eacute;action</a></td></tr>");
             client.println("<tr><td><br><br>&nbsp;&nbsp;<a href=\"/M\">- Diminuer le d&eacute;lai de r&eacute;action</a></td></tr>");
             client.println("</table>");

             client.println("<br><br></td></tr></table>");
             client.println("</body>\n</html>");
             // break out of the while loop:
             break;
           } 
           else 
           {    // si vous avez une nouvelle ligne, effacez currentLine:
            int posDlyReact = currentLine.indexOf("dlyReact=");
            int posTmpsActif = currentLine.indexOf("TmpsActif=");
            int posDlyRtour = currentLine.indexOf("dlyRtour=");
            int posUnitDelaiReactMS = currentLine.indexOf("unitDelaiReactMS=");
            int posHTTP = currentLine.indexOf("HTTP");
            if (currentLine.startsWith("GET /action_page.php?")){
              if (posDlyReact >0) { 
                Serial.println("");
                Serial.println("***" );
                Serial.print(posDlyReact);
                Serial.println(" position du dlyReact" );
                Serial.print(posTmpsActif);
                Serial.println(" position du TmpsActif" );
                Serial.print(posDlyRtour);
                Serial.println(" position du dlyRtour" );
                Serial.print(posUnitDelaiReactMS);
                Serial.println(" position du unitDelaiReactMS" );
                Serial.println(currentLine);

                String strDlyReact = currentLine.substring(posDlyReact+9,posTmpsActif-1);
                Serial.println(strDlyReact);
                String strTmpsActif = currentLine.substring(posTmpsActif+10,posDlyRtour-1);
                Serial.println(strTmpsActif);
                String strDlyRtour = currentLine.substring(posDlyRtour+9,posUnitDelaiReactMS-1);
                Serial.println(strDlyRtour);
                String strUnitDelaiReactMS = currentLine.substring(posUnitDelaiReactMS+17,posHTTP-1);
                Serial.println(strUnitDelaiReactMS);

                if (strDlyReact != ""){
                  u32DelaiDeReaction =  strDlyReact.toInt();
                  memFlashDataUser(); 
                }
                if (strTmpsActif != ""){
                  u32TempsRelaisActif =  strTmpsActif.toInt();
                  memFlashDataUser(); 
                }
                if (strDlyRtour != ""){
                  u32DelaiAvantRetour =  strDlyRtour.toInt();
                  memFlashDataUser(); 
                }
                if (strUnitDelaiReactMS != ""){
                  blUnitDlReactMicroScd = strUnitDelaiReactMS.toInt();
                  memFlashDataUser(); 
                }
            }
           } 
            currentLine = "";
           }
         } 
         else if (c != '\r') 
         {  // si vous avez autre chose qu'un caractère de retour de chariot,
          currentLine += c;      // l'ajouter à la fin de la currentLine
         }
         
         // Vérifiez si la demande du client
         if (currentLine.endsWith("GET /P")) {
            u32DelaiDeReaction ++; // Augmente la durée d'activation du relais
            memFlashDataUser(); 
            currentLine = "";   
         }
         if (currentLine.endsWith("GET /M")) {
            u32DelaiDeReaction --; // Diminue la durée d'activation du relais
            if (u32DelaiDeReaction == 4294967295){ u32DelaiDeReaction =0;}
            memFlashDataUser(); 
            currentLine = "";    
         }    
         if (currentLine.endsWith("GET /D")) {
           Serial.println(currentLine);
           blDeclencheRelais = 1; // 
           currentLine = "";    
         }
        } // # if (client.available())
     } // # fin du while (client.connected())
      
  } // # fin du if (client) 
  // close the connection:
  client.stop();
  //Serial.println("Client Disconnected.");
  delay(0);
  }
 
