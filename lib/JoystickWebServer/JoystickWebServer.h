#pragma once
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "Joystick.h"

class JoystickWebServer {
public:
    JoystickWebServer(Joystick& jsRef, const char* ssid, const char* password, IPAddress apIP = IPAddress(192,168,4,1))
    : js(jsRef), wifiSSID(ssid), wifiPass(password), apIP(apIP), server(80) {}

    void begin() {
        if(!LittleFS.begin(true)){ // true = format if mount fails
            Serial.println("LittleFS Fehler");
            return;
        }

        esp_log_level_set("wifi", ESP_LOG_VERBOSE);
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
        WiFi.softAP(wifiSSID, wifiPass);

        Serial.println("AP gestartet. IP: " + WiFi.softAPIP().toString());
       
        // Statische Dateien
        server.serveStatic("/css/materialize.min.css", LittleFS, "/css/materialize.min.css");
        server.serveStatic("/js/materialize.min.js", LittleFS, "/js/materialize.min.js");
        server.serveStatic("/css/icons.css", LittleFS, "/css/icons.css");
        server.serveStatic("/fonts/MaterialIcons-Regular.woff2", LittleFS, "/fonts/MaterialIcons-Regular.woff2");

        // Webseiten
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req){ req->send(200,"text/html",buildPage()); });

        // Kalibrierung
        server.on("/calibrateCenter", HTTP_GET,[this](AsyncWebServerRequest* req){
            js.calibrateCenter();
            req->send(200,"text/plain","Mitte gespeichert");
        });

        server.on("/calibrateMin", HTTP_GET,[this](AsyncWebServerRequest* req){
            if(js.calibrateMin()) req->send(200,"text/plain","Minimum gespeichert");
            else req->send(200,"text/plain","Fehler: Min muss ≥0.5 V unter Mitte liegen!");
        });

        server.on("/calibrateMax", HTTP_GET,[this](AsyncWebServerRequest* req){
            if(js.calibrateMax()) req->send(200,"text/plain","Maximum gespeichert");
            else req->send(200,"text/plain","Fehler: Max muss ≥0.5 V über Mitte liegen!");
        });

        server.on("/resetCalibration", HTTP_GET, [this](AsyncWebServerRequest* req){
            js.resetCalibration();
            req->send(200, "text/plain", "Kalibrierung zurückgesetzt!");
        });

        server.on("/values", HTTP_GET,[this](AsyncWebServerRequest* req){
            String json;
            if(js.isCalibrated()){
                json = "{\"norm\":\""+String(js.getValue(),2)+"\",\"volt\":\""+String(js.getVoltage(),2)+"\"}";
            } else {
                json = "{\"norm\":\"NaN\",\"volt\":\"NaN\"}";
            }
            req->send(200,"application/json",json);
        });

        server.begin();
        Serial.println("Webserver gestartet.");
    }

private:
    Joystick& js;
    const char* wifiSSID;
    const char* wifiPass;
    IPAddress apIP;
    AsyncWebServer server;

    String buildPage(){
 String statusText = js.isCalibrated() ? "Kalibriert ✅" : "Nicht kalibriert ❌";
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>Joystick Kalibrierung</title>
<link rel="stylesheet" href="/css/materialize.min.css">
<link rel="stylesheet" href="/css/icons.css">
<style>
body{display:flex;min-height:100vh;flex-direction:column;} 
main{flex:1 0 auto;} 
.center-card{max-width:720px;margin:24px auto;}
#bar{width:100%;height:36px;background:#eceff1;border-radius:6px;position:relative;overflow:hidden;}
#leftFill{height:100%;background:#e53935;width:0%;position:absolute;right:50%;transition:width 0.2s ease;}
#rightFill{height:100%;background:#43a047;width:0%;position:absolute;left:50%;transition:width 0.2s ease;}
#centerLine{position:absolute;top:0;bottom:0;left:50%;width:2px;background:#37474f;}
</style>
</head>
<body>
<nav>
  <div class="nav-wrapper teal darken-2">
    <a href='#' class='brand-logo center'>Bugstrahlruder</a>
  </div>
</nav>

<main>
<div class='container'>
<div class='card center-card'>
  <div class='card-content'>
    <span class='card-title'>Kalibrierung & Live-Werte</span>
    <p>Status: <span id='statusText'>)" + statusText + R"rawliteral(</span></p>
    <p id='normText'>Normiert: —</p>
    <p id='voltText'>Spannung: — V</p>
    <div id='bar'>
      <div id='leftFill'></div>
      <div id='rightFill'></div>
      <div id='centerLine'></div>
    </div>

    <!-- Erste Zeile: Min, Mitte, Max -->
    <div class="row" style="display:flex; justify-content:space-between; gap:8px; margin-top:16px;">
      <a id="btnMin" class="waves-effect waves-light btn grey darken-1" style="flex:1;">
        <i class="material-icons arrow_back"></i>Min
      </a>
      <a id="btnCenter" class="waves-effect waves-light btn grey darken-1" style="flex:1;">
        <i class="material-icons lens"></i>Mitte
      </a>
      <a id="btnMax" class="waves-effect waves-light btn grey darken-1" style="flex:1;">
        <i class="material-icons arrow_forward"></i>Max
      </a>
    </div>

    <!-- Zweite Zeile: Reset -->
    <div class="row" style="display:flex; justify-content:center; margin-top:8px;">
      <a id="btnReset" class="waves-effect waves-light btn grey darken-1" style="flex:0 0 40%;">
        <i class="material-icons">Reset</i>Reset
      </a>
    </div>

  </div>
</div>
</div>
</main>

<footer class='page-footer teal lighten-2'>
  <div class='container'>Bugstrahlruder Joystick Kalibrierung</div>
</footer>

<script src="/js/materialize.min.js"></script>
<script>
// Live Werte aktualisieren
async function updateValues(){
    try{
        const r = await fetch('/values');
        const d = await r.json();
        const norm = parseFloat(d.norm), volt = parseFloat(d.volt);
        if(!isNaN(norm)){
            document.getElementById('statusText').innerText='Kalibriert ✅';
            document.getElementById('normText').innerText='Normiert: '+norm.toFixed(2);
            document.getElementById('voltText').innerText='Spannung: '+volt.toFixed(2)+' V';
            if(norm<0){
                document.getElementById('leftFill').style.width=Math.abs(norm*50)+'%';
                document.getElementById('rightFill').style.width='0%';
            } else {
                document.getElementById('rightFill').style.width=(norm*50)+'%';
                document.getElementById('leftFill').style.width='0%';
            }
        } else {
            document.getElementById('statusText').innerText='Nicht kalibriert ❌';
            document.getElementById('normText').innerText='Normiert: —';
            document.getElementById('voltText').innerText='Spannung: —';
            document.getElementById('leftFill').style.width='0%';
            document.getElementById('rightFill').style.width='0%';
        }
    } catch(e){
        document.getElementById('statusText').innerText='Fehler ❌';
    }
}

setInterval(updateValues,500);

// Kalibrierung Buttons mit Toast Meldungen
document.addEventListener("DOMContentLoaded",()=>{
    document.getElementById("btnCenter").onclick = async () => {
        const res = await fetch("/calibrateCenter");
        const text = await res.text();
        M.toast({html:text, classes:"blue"});
        updateValues();
    };
    document.getElementById("btnMin").onclick = async () => {
        const res = await fetch("/calibrateMin");
        const text = await res.text();
        M.toast({html:text, classes:text.includes("Fehler")?"red":"green"});
        updateValues();
    };
    document.getElementById("btnMax").onclick = async () => {
        const res = await fetch("/calibrateMax");
        const text = await res.text();
        M.toast({html:text, classes:text.includes("Fehler")?"red":"green"});
        updateValues();
    };
    document.getElementById("btnReset").onclick = async () => {
        const res = await fetch("/resetCalibration");
        const text = await res.text();
        M.toast({html:text, classes:"orange"});
        updateValues();
    };
});
</script>
</body>
</html>
)rawliteral";
    return html;
    }
};
