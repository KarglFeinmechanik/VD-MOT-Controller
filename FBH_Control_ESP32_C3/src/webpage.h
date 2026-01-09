#pragma once

const char MAIN_page[] PROGMEM = R"HTML(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <title>FBH Controller VD-MOT</title>
        <link rel="icon" type="image/png" href="/favicon-32x32.png">
        <link rel="stylesheet" href="/style.css">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <script>
        let userDraggingSlider = false;
        let expertMode = false;
        let pendingValve = null;
        let currentLang = "de";
        window.addEventListener("DOMContentLoaded", () => {
            document.querySelectorAll("input[data-default]").forEach(el => {
                if (el.value.startsWith("__") && el.value.endsWith("__")) {
                    el.value = el.dataset.default;
                }
            });
        });
        const translations = {
            en:{ title:"Valve Controller", wifiSsid:"WiFi SSID", wifiPass:"WiFi Password",
                mqttServer:"MQTT Server", mqttUser:"MQTT User", mqttPass:"MQTT Password",
                mqttInterval:"MQTT Status Interval (100..60000 ms)", deleteFile: "Delete Selected File",
                sensorType:"Sensor Type", sensorDht:"DHT22 (one sensor)", sensorDs:"DS18B20 (multiple)",
                expertLabel:"Expert Mode", calTitle:"Calibration", humidityLabel:"Humidity",
                valveCalibration:"Valve Calibration", controlsTitle:"Controls", fsUploadBtn: "Upload File",
                valvePosLabel:"Valve Position", saveConfig:"Save Config", deleteAll: "DELETE ENTIRE FILESYSTEM",
                restart:"Restart Device", factoryReset:"Factory Reset", firmwareUpdate: "Firmware Update",
                statusTitle:"Status", calibratedLabel:"Calibrated", confirmUpdate: "Do you really want to update the firmware?",
                currentPosLabel:"Current Pos", targetPosLabel:"Target Pos", uploadFlash: "Upload & Flash",
                stallLabel:"Stall", faultLabel:"Fault", travelTimeLabel:"Travel Time", filesystem: "Filesystem",
                tempsLabel:"Temperatures", saveConfirm: "Save configuration?", fsUpload: "Upload Allowed Files",
                confirmRestart:"Do you really want to restart the device?", mqttClient:"MQTT Client ID",
                confirmFactory:"Do you REALLY want to erase all settings?", fsDelete:"Delete Individual Files",
                yes:"YES", no:"NO", savingMsg: "Saving…", restartingMsg: "Restarting…", factoryMsg: "Factory Reset…"
            },
            de:{ title:"Ventil-Steuerung", wifiSsid:"WiFi SSID", wifiPass:"WiFi Passwort",
                mqttServer:"MQTT Server", mqttUser:"MQTT Benutzer", mqttPass:"MQTT Passwort",
                mqttInterval:"MQTT Status-Intervall (100..60000 ms)", fsDelete:"Datei löschen",
                sensorType:"Sensortyp", sensorDht:"DHT22 (ein Sensor)", sensorDs:"DS18B20 (mehrere)",
                expertLabel:"Expertenmodus", calTitle:"Kalibrierung", deleteFile:"Ausgewählte Datei löschen",
                valveCalibration:"Ventilkalibrierung", controlsTitle:"Steuerung", deleteAll:"Dateisystem löschen",
                valvePosLabel:"Ventilposition", saveConfig:"Konfiguration speichern", fsUpload:"Datei hochladen",
                restart:"Gerät neu starten", factoryReset:"Werkseinstellungen", firmwareUpdate: "Firmware-Update",
                statusTitle:"Status", calibratedLabel:"Kalibriert", confirmUpdate: "Firmware wirklich aktualisieren?",
                currentPosLabel:"Ist-Pos", targetPosLabel:"Soll-Pos", uploadFlash: "Hochladen & Flashen",
                stallLabel:"Stall", faultLabel:"Fehler", travelTimeLabel:"Fahrzeit", filesystem:"Dateisystem",
                tempsLabel:"Temperaturen", saveConfirm: "Einstellungen Speichern?", fsUpload: "Passende Dateien hochladen",
                confirmRestart:"Gerät wirklich neu starten?", mqttClient: "MQTT Client ID",
                confirmFactory:"WIRKLICH alle Einstellungen löschen?", humidityLabel:"Luftfeuchtigkeit",
                yes:"JA", no:"NEIN", savingMsg: "Speichere…", restartingMsg: "Neustart…", factoryMsg: "Werkseinstellungen…"

            },
            fr:{ title:"Contrôleur de vanne", wifiSsid:"SSID WiFi", wifiPass:"Mot de passe WiFi",
                mqttServer:"Serveur MQTT", mqttUser:"Utilisateur MQTT", mqttPass:"Mot de passe MQTT",
                mqttInterval:"Intervalle MQTT (100..60000 ms)", humidityLabel:"Humidité",
                sensorType:"Type de capteur", sensorDht:"DHT22 (un capteur)", sensorDs:"DS18B20 (multiple)",
                expertLabel:"Mode expert", calTitle:"Calibration", deleteAll: "SUPPRIMER TOUT LE SYSTÈME",
                valveCalibration:"Calibration de la vanne", controlsTitle:"Contrôle", 
                valvePosLabel:"Position de la vanne", saveConfig:"Enregistrer", deleteFile: "Supprimer sélectionné",
                restart:"Redémarrer", factoryReset:"Réinitialisation usine", firmwareUpdate: "Mise à jour du firmware",
                statusTitle:"Statut", calibratedLabel:"Calibré", confirmUpdate: "Mettre à jour le firmware ?",
                currentPosLabel:"Position actuelle", targetPosLabel:"Position cible", fsDelete: "Supprimer un fichier",
                stallLabel:"Blocage", faultLabel:"Défaut", travelTimeLabel:"Temps de course", uploadFlash: "Téléverser & Flasher",
                tempsLabel:"Températures", saveConfirm: "Enregistrer ?", fsUploadBtn: "Téléverser",
                confirmRestart:"Voulez-vous vraiment redémarrer ?", filesystem: "Système de fichiers",
                confirmFactory:"Voulez-vous vraiment effacer tous les paramètres ?", fsUpload: "Télécharger un fichier",
                yes:"OUI", no:"NON", savingMsg: "Enregistrement…", restartingMsg: "Redémarrage…", factoryMsg: "Réinitialisation…"

            }
        };

        let t = translations[currentLang];
        function setLanguage(lang) {
            localStorage.setItem("fbh_lang", lang);  // ← MISSING!
            t = translations[lang];
            currentLang = lang;
            const map = {
                title:"title", lblWifiSsid:"wifiSsid", lblWifiPass:"wifiPass",
                lblMqttServer:"mqttServer", lblMqttClient: "mqttClient",
                lblMqttUser:"mqttUser", lblMqttPass:"mqttPass",
                lblMqttInterval:"mqttInterval", lblSensorType:"sensorType",
                expertLabel:"expertLabel", calTitle:"calTitle", btnCalib:"valveCalibration",
                controlsTitle:"controlsTitle", lblValvePos:"valvePosLabel",
                btnSaveFooter:"saveConfig",
                lblFwUpdate:"firmwareUpdate",
                btnFlash:"uploadFlash",
                btnRestartSingleFooter:"restart",
                btnRestartFooter:"restart",
                btnFactoryFooter:"factoryReset",
                lblFS:"filesystem",
                btnFSUpload:"fsUploadBtn",
                lblFSDelete:"fsDelete",
                btnDeleteFile:"deleteFile",
                btnDeleteFS:"deleteAll", lblHumidity: "humidityLabel",
                statusTitle:"statusTitle", lblCalibrated:"calibratedLabel",
                lblCurrentPos:"currentPosLabel", lblTargetPos:"targetPosLabel",
                lblStall:"stallLabel", lblFault:"faultLabel",
                lblTravelTime:"travelTimeLabel", lblTemps:"tempsLabel"
            };
            for (const id in map) {
                const el = document.getElementById(id);
                if (el && t[map[id]]) el.innerText = t[map[id]];
            }
            document.getElementById("optDHT").innerText = t.sensorDht;
            document.getElementById("optDS").innerText  = t.sensorDs;
            document.querySelectorAll(".lang-btn").forEach(btn =>
                btn.classList.toggle("active", btn.dataset.lang === lang)
            );
        }

        function initLanguage() {
            let saved = localStorage.getItem("fbh_lang");
            setLanguage(saved && translations[saved] ? saved : "en");
        }

        function toggleExpert() {
            expertMode = document.getElementById("expertSwitch").checked;
            document.getElementById("expertSection").style.display =
                expertMode ? "block" : "none";
            document.getElementById("btnRestartSingleFooter").style.display =
                expertMode ? "none" : "block";
            document.getElementById("expertFooterRow").style.display =
                expertMode ? "flex" : "none";
            document.getElementById("fwUpdateSection").style.display =
                expertMode ? "block" : "none";
            document.getElementById("fsSection").style.display =
                expertMode ? "block" : "none";
        }

        function previewValve(v) {
            userDraggingSlider = true;
            pendingValve = v;
        }

        function commitValve() {
            if (pendingValve !== null) {
                setValvePos(pendingValve);
                pendingValve = null;
            }
            userDraggingSlider = false;   // allow updates again
        }

        function showOverlay(msg) {
            document.body.classList.add("overlay-active");
            document.getElementById("overlayBox").innerText = msg;
            document.getElementById("overlayBlur").style.display = "flex";
        }

        // --- Restart ---
        function restartDevice() {
            if (!confirm(t.confirmRestart)) return;
            showOverlay(t.restart + "…");
            fetch("/restart");      // ← semicolon required
            waitForReboot();        // ← MUST be called on restart
        }

        // --- Factory Reset ---
        function factoryReset() {
            if (!confirm(t.confirmFactory)) return;
            showOverlay(t.factoryReset + "…");
            fetch("/factory_reset");   // remove waitForReboot() if you don't want auto reconnect
            // If you want auto reconnect, add: waitForReboot();
        }
        // --- Save Config ---
        document.addEventListener("DOMContentLoaded", () => {
            document.getElementById("mainForm").addEventListener("submit", function (e) {
                e.preventDefault();
                if (!confirm(t.saveConfirm)) return;
                showOverlay(t.savingMsg);
                fetch("/save", {
                    method: "POST",
                    body: new FormData(document.getElementById("mainForm"))
                });
                waitForReboot();   // must be here so GUI reloads after save
            });
        });

        function startCalibration(){
            fetch("/calibrate").then(() => {
                // Calibration is blocking on ESP — browser updates via updateStatus()
            });
        }

        function setValvePos(v) { fetch("/valve?pos=" + v); }

        function updateStatus() {       
            // --- update WiFi + MQTT only every 5 seconds ---
            let nowTime = Date.now();
            if (!window.lastSlowUpdate) window.lastSlowUpdate = 0;
            let slowUpdate = false;
            if (nowTime - window.lastSlowUpdate >= 5000) {
                window.lastSlowUpdate = nowTime;
                slowUpdate = true;
            }
            fetch("/status")
            .then(r => r.json())
            .then(j => {
                // --- basic state ---
                // ---- Calibration button state ----
                const btnCal = document.getElementById("btnCalib");
                if (btnCal) {
                    if (j.calrunning) {
                        btnCal.disabled = true;
                        btnCal.innerText = t.calTitle + "…";
                    } else {
                        btnCal.disabled = false;
                        btnCal.innerText = t.calTitle;
                    }
                }
                document.getElementById("btnRehome").disabled = !j.canRehome;
                // document.getElementById("calibrated").innerText = j.calibrated ? t.yes : t.no;
                document.getElementById("currentPos").innerText = j.currentPos.toFixed(1);
                document.getElementById("valveActual").value    = j.currentPos;
                document.getElementById("targetPos").innerText  = j.targetPos.toFixed(1);
                if (!userDraggingSlider) {
                    document.getElementById("valveSlider").value = j.targetPos;
                }
                document.getElementById("stall").innerText       = j.stall ? t.yes : t.no;
                document.getElementById("fault").innerText       = j.fault ? t.yes : t.no;
                document.getElementById("travelTime").innerText  = j.travelTime;
                document.getElementById("statusTitle").innerText = j.movement;
                // calibrated icon
                const calibIcon = document.getElementById("calibIcon");
                calibIcon.textContent = j.calibrated ? "✅" : "⚠️";
                document.getElementById("humidity").innerText    = j.humidity.toFixed(1);
                // ------- Temperature List -------
                const temps = Array.isArray(j.temps) ? j.temps : [];
                // main temp (first sensor)
                const tempMainEl = document.getElementById("tempMain");
                if (tempMainEl) {
                    if (temps.length > 0) {
                        tempMainEl.innerText = temps[0].toFixed(1);
                    } else {
                        tempMainEl.innerText = "--";
                    }
                }
                // full list
                let tempDiv = document.getElementById("temps");
                if (tempDiv) {
                    tempDiv.innerHTML = "";
                    temps.forEach((v, i) => {
                        tempDiv.innerHTML += `Sensor ${i+1}: ${v.toFixed(2)} °C<br>`;
                    });
                }
                // show extra temperatures only if more than 1 sensor
                const tempWrapper = document.getElementById("tempWrapper");
                if (tempWrapper) {
                    tempWrapper.style.display = (temps.length > 1) ? "block" : "none";
                }
                // --- slow section: WiFi + MQTT every 5s ---
                if (slowUpdate) {
                    document.getElementById("wifiSince").innerText = "WiFi: " + j.wifiSince;
                    let mqttIcon = document.getElementById("mqttStatusIcon");
                    if (mqttIcon) {
                        if (j.mqttStatus === "Connected") {
                            // green = enabled, grey = connected but disabled
                            mqttIcon.textContent = j.mqttEnabled ? "✅" : "☑️";
                            mqttIcon.style.filter = ""; // no grayscale tricks anymore
                        }
                        else if (j.mqttStatus === "Reconnecting" || j.mqttStatus === "Connecting") {
                            mqttIcon.textContent = "⏳";
                            mqttIcon.style.filter = "";
                        }
                        else {
                            mqttIcon.textContent = "❌";
                            mqttIcon.style.filter = "";
                        }
                    }
                }
                // ------- Update Interval -------
                let moving = (j.openPin === 1 || j.closePin === 1);
                let next   = moving ? 50 : 500;
                // Thin slider visual feedback (CSS class toggle)
                let actual = document.getElementById("valveActual");
                if (actual) actual.classList.toggle("moving", moving);
                setTimeout(updateStatus, next);
            });
        }

        document.addEventListener("DOMContentLoaded", ()=>{
            initLanguage();
            toggleExpert();
            updateStatus();
        });
        window.onload = () => {
            document.body.classList.remove("overlay-active");
        };

        // -------- Auto Reload After ESP Restart --------
        let reconnectTimer = null;
        function waitForReboot() {
            clearInterval(reconnectTimer);
            document.getElementById("overlayBox").innerText = "Reconnecting…";
            document.getElementById("overlayBlur").style.display = "flex";
            reconnectTimer = setInterval(() => {
                fetch("/", { method: "GET", cache: "no-store" })
                    .then(r => {
                        if (r.ok) {
                            clearInterval(reconnectTimer);
                            location.reload();
                        }
                    })
                    .catch(e => {
                        // still offline → wait
                    });
            }, 1500);
        }

        function startOta() {
            showOverlay("Uploading firmware…");

            setTimeout(() => {
                waitForReboot();   // Your existing reconnect logic
            }, 1000);

            return true; // allow form submission
        }

        function confirmOta() {
            if (!confirm(t.confirmUpdate)) return false;
            showOverlay(t.uploadFlash + "…");
            setTimeout(() => {
                waitForReboot();
            }, 1500);
           return true;
        }

        function deleteFile() {
            let name = document.getElementById("fileDeleteSelect").value;
            if (!confirm("Delete " + name + "?")) return;

            fetch("/delete_file?name=" + encodeURIComponent(name))
                .then(r => r.text())
                .then(t => alert(t));
        }

        function deleteFS() {
            if (!confirm("Delete ALL files in FS?")) return;

            fetch("/delete_fs")
                .then(r => r.text())
                .then(t => alert(t));
        }
                
        function resetCalibration() {
            if (!confirm("Reset calibration values?")) return;
            showOverlay("Resetting calibration…");
            fetch("/reset_calibration")
                .then(r => r.text())
                .then(t => {
                    alert(t); // optional
                    document.getElementById("calibIcon").textContent = "⚪";
                    updateStatus();
                });
        }

        function setCalibration() {
            if (!confirm("Set calibration values?")) return;
            showOverlay("Set calibration…");
            fetch("/set_calibration")
                .then(r => r.text())
                .then(t => {
                    alert(t); // optional
                    document.getElementById("calibIcon").textContent = "✅";
                    updateStatus();
                });
        }
        function startRehome() {
            if (!confirm("Rehome valve now?")) return;
            fetch("/rehome")
                .then(r => r.text())
                .then(t => {
                    if (t !== "OK") alert("Rehome failed: " + t);
                });
        }
        </script>
    </head>
    <body>
    <!-- TOP BAR -->
    <div class="top-status">
        <span id="fwVersion">Firmware: __FWVER__</span>
        <span class="mid-dot">·</span>
        <span id="wifiSince">WiFi: --:--:--</span>
        <span class="mid-dot">·</span>
        <span id="mqttStatusLine">
            MQTT: <span id="mqttStatusIcon">🔴</span>
        </span>
    </div>
    <div class="top-bar">
        <div class="lang-box">
            <img class="lang-btn" data-lang="de" src="https://flagcdn.com/w40/de.png" onclick="setLanguage('de')">
            <img class="lang-btn" data-lang="en" src="https://flagcdn.com/w40/us.png" onclick="setLanguage('en')">
            <img class="lang-btn" data-lang="fr" src="https://flagcdn.com/w40/fr.png" onclick="setLanguage('fr')">
        </div>
        <div class="expert-toggle">
            <span id="expertLabel">Expert Mode</span>
            <label class="switch">
                <input type="checkbox" id="expertSwitch" onchange="toggleExpert()">
                <span class="slider"></span>
            </label>
        </div>
    </div>
    <div class="center-container">
        <div class="card">
            <h2 id="title">Valve Controller</h2>
            <div class="card-body">
                <form id="mainForm" method="POST" action="/save">
                    <label id="lblWifiSsid">WiFi SSID</label>
                    <input type="text" name="ssid" value="__SSID__">
                    <label id="lblWifiPass">WiFi Password</label>
                    <input type="password" name="pass" value="__WPASS__">
                    <label id="lblMqttServer">MQTT Server</label>
                    <input name="mqtt" value="__MQTT__" data-default="ip:port">
                    <label id="lblMqttClient">MQTT Client ID</label>
                    <input name="mclient" value="__MCLIENT__" data-default="FBH-Controller">
                    <label id="lblMqttUser">MQTT User</label>
                    <input name="muser" value="__MUSER__" data-default="mqttuser">
                    <label id="lblMqttPass">MQTT Password</label>
                    <input type="password" name="mpass" value="__MPASS__">
                    <label id="lblMqttInterval">MQTT Status Interval</label>
                    <input name="statusInterval" value="__SINT__">
                    <label id="lblSensorType">Sensor Type</label>
                    <select name="sensor_type">
                        <option id="optDHT" value="DHT22" __SEL_DHT__></option>
                        <option id="optDS" value="DS18B20" __SEL_DS18__></option>
                    </select>
                    <div id="expertSection" style="display:none;">
                        <hr>
                        <h3 id="calTitle">Calibration</h3>
                        <div class="calib-grid">
                            <button type="button" id="btnCalib" 
                                    onclick="startCalibration()">
                            </button>
                            <button type="button" id="btnRehome"
                                    onclick="startRehome()">
                                Rehome
                            </button>
                            <button type="button" id="btnCalibSet"
                                    class="yellow"
                                    onclick="setCalibration()">
                                Set Calibration
                            </button>
                            <button type="button" id="btnCalibReset"
                                    class="red"
                                    onclick="resetCalibration()">
                                Reset Calibration
                            </button>
                        </div>
                    </div>
                    <hr>
                    <h3 id="controlsTitle">Controls</h3>
                    <label id="lblValvePos">Valve Position</label>
                    <input id="valveSlider" type="range" min="0" max="100"
                        oninput="previewValve(this.value)"
                        onmousedown="userDraggingSlider = true"
                        onmouseup="commitValve()"
                        ontouchstart="userDraggingSlider = true"
                        ontouchend="commitValve()">
                    <input id="valveActual" type="range" min="0" max="100"
                        disabled class="thin-slider">
                </form>
                <hr>
                <div id="fwUpdateSection" style="display:none;">
                    <h3 id="lblFwUpdate">Firmware Update</h3>
                    <form id="otaForm" method="POST" action="/update"
                        enctype="multipart/form-data"
                        onsubmit="return confirmOta();">
                        <input type="file" name="update" accept=".bin" required>
                        <button id="btnFlash" type="submit">Upload & Flash</button>
                    </form>
                </div>
                <!-- NEW: Filesystem Management (Expert Mode Only) -->
                <div id="fsSection" style="display:none;">
                    <hr>
                    <h3 id="lblFS">Filesystem</h3>
                    <label id="lblFSUpload">Upload Allowed Files</label>
                    <form id="fsUploadForm" method="POST" action="/upload_file" enctype="multipart/form-data">
                        <input type="file" name="file" accept=".jpg,.png,.css" required>
                        <button id="btnFSUpload" type="submit">Upload File</button>
                    </form>
                    <label id="lblFSDelete">Delete Individual Files</label>
                    <select id="fileDeleteSelect">
                        <option value="background.jpg">background.jpg</option>
                        <option value="favicon-32x32.png">favicon-32x32.png</option>
                        <option value="style.css">style.css</option>
                    </select>
                    <button id="btnDeleteFile" onclick="deleteFile()">Delete Selected File</button>
                    <hr>
                    <button id="btnDeleteFS" class="red" onclick="deleteFS()">DELETE ENTIRE FILESYSTEM</button>
                </div>
            </div>
            <!-- ATTACHED FOOTER (NOT SCROLLING) -->
            <div class="footer-box">
                <!-- Status placed inside footer -->
                <div class="status-footer">
                    <!-- First line: Status + Calibrated icon/text -->
                    <div class="row-2col status-topline">
                        <span class="status-left"><b id="statusTitle">Status</b></span>

                        <span class="cal-block">
                            CAL <span id="calibIcon">⚪</span>
                        </span>
                    </div>
                    <!-- Valve position: current / target -->
                    <div class="row-2col">
                        <span id="lblValvePos"><b>Valve Pos: current </b></span>
                        <span>
                            <span id="currentPos">--</span>% / <b>target</b>  
                            <span id="targetPos">--</span>%
                        </span>
                    </div>
                    <!-- Stall / Fault -->
                    <div class="row-2col">
                        <span>
                            <b><span id="lblStall">Stall</span>:</b>
                                <span id="stall">--</span> /    
                        </span>
                        <span>
                            <b><span id="lblFault">Fault</span>:</b>
                                <span id="fault">--</span>
                        </span>
                    </div>
                    <!-- Humidity / main temperature -->
                    <div class="row-2col">
                        <span>
                            <b><span id="lblHumidity">Humidity</span>:  </b>
                                <span id="humidity">--</span>%   /
                        </span>
                        <span>
                            <b><span id="lblHumidity">Temp</span>:  </b>
                            <span id="tempMain">--</span>°C
                        </span>
                    </div>
                    <!-- Travel time -->
                    <div class="row-2col">
                        <span><b id="lblTravelTime">Travel Time</b>:</span>
                        <span><span id="travelTime">--</span> ms</span>
                    </div>
                    <!-- Extra temperatures list (e.g. DS18B20 multi sensors) -->
                    <div class="row-1col" id="tempWrapper">
                        <div id="lblTempsRow"><b id="lblTemps">Temperatures</b></div>
                        <div id="tempsSection">
                            <div id="temps"></div>
                        </div>
                    </div>
                </div>
                <button id="btnSaveFooter" type="submit" form="mainForm">Save Config</button>
                <button id="btnRestartSingleFooter" type="button" onclick="restartDevice()">Restart Device</button>
                <div class="btn-row" id="expertFooterRow">
                    <button id="btnRestartFooter" type="button" onclick="restartDevice()">Restart</button>
                    <button id="btnFactoryFooter" type="button" class="red" onclick="factoryReset()">Factory Reset</button>
                </div>
            </div>
        </div>
    </div>
    <div id="overlayBlur">
    <div id="overlayBox">Working…</div>
    </div>
    </body>
    </html>
)HTML";

const char STYLE_css[] PROGMEM = R"CSS(
/* ------------------ GLOBAL ------------------ */
body {
    margin: 0;
    padding: 0;
    background: url("/background.jpg") no-repeat center center fixed !important;
    background-size: cover !important;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    overflow: hidden;
}

/* When overlay is active, disable UI interaction under it */
body.overlay-active {
    pointer-events: none;
}

/* Center card */
.center-container {
    display: flex;
    justify-content: center;
    align-items: flex-start;
    padding-top: 100px;
    height: calc(100vh - 120px);
}

/* Blur overlay container */
#overlayBlur {
    position: fixed;
    left:0;
    top:0;
    width:100%;
    height:100%;
    background: rgba(0,0,0,0.5);
    backdrop-filter: blur(5px);
    display: none;
    align-items: center;
    justify-content: center;
    z-index: 9999;
    pointer-events: all;
}

/* Message box */
#overlayBox {
    background: white;
    padding: 25px 40px;
    border-radius: 12px;
    font-size: 22px;
    font-weight: bold;
    text-align: center;
    box-shadow: 0 0 25px rgba(0,0,0,0.4);
}

/* ------------------ TOP BAR ------------------ */
.top-status {
    position: fixed;
    top: 8px;
    left: 8px;
    font-size: 14px;
    font-weight: 600;
    color: #fff;
    background: rgba(0,0,0,0.45);
    padding: 6px 12px;
    border-radius: 10px;
    display: flex;
    align-items: center;
    gap: 6px;
    z-index: 9999;
}

.top-status .mid-dot {
    opacity: 0.8;
    font-weight: bold;
}

.top-bar {
    position: fixed;
    top: 10px;
    right: 10px;
    display: flex;
    align-items: center;
    gap: 20px;
    z-index: 1000;
}

/* Language flags */
.lang-box img {
    width: 40px;
    height: 28px;
    opacity: 0.6;
    cursor: pointer;
    object-fit: cover;
    border-radius: 4px;
    transition: 0.2s;
}
.lang-box img.active {
    opacity: 1;
    transform: scale(1.05);
}

/* Expert switch */
.expert-toggle {
    display: flex;
    align-items: center;
    justify-content: space-between;
    width: 180px;               /* FIXED box width */
    padding: 6px 10px;
    background: rgba(255,255,255,0.8);
    border-radius: 12px;
    box-shadow: 0 2px 6px rgba(0,0,0,0.15);
}

.expert-toggle span {
    font-weight: 700;
    white-space: nowrap;
    text-align: left;
    flex: 0 1 auto;            /* << NO stretching */
    max-width: 120px;          /* << keeps text inside */
    overflow: hidden;
    text-overflow: ellipsis;   /* optional: avoids overflow */
}

.switch {
    position: relative;
    width: 42px;
    height: 22px;
    flex-shrink: 0;
    margin-top: 0px;
}

.slider {
    position: absolute;
    top: 50%;
    left: 0;
    width: 100%;
    height: 100%;
    background: #ccc;
    border-radius: 22px;
    transform: translateY(-50%);      /* PERFECT vertical centering */
    transition: 0.25s;
}

.slider:before {
    content: "";
    position: absolute;
    width: 16px;
    height: 16px;
    left: 3px;
    top: 50%;
    transform: translateY(-50%);      /* knob centered */
    background: white;
    border-radius: 50%;
    transition: 0.25s;
}

input:checked + .slider {
    background: #34c759;
}

input:checked + .slider:before {
    transform: translate(19px, -50%); /* moves knob horizontally */
}

/* ------------------ CARD ------------------ */
.card {
    width: min(600px, 92vw);
    background: rgba(255,255,255,0.72);
    backdrop-filter: blur(16px);
    border-radius: 20px;
    box-shadow: 0 4px 18px rgba(0,0,0,0.25);
    display: flex;
    flex-direction: column;
    max-height: calc(100vh - 160px);
    overflow: hidden;
}

.card h2 {
    text-align: center;
    padding: 20px 0 0 0;
    margin: 0;
}

/* Scrollable area */
.card-body {
    flex: 1;
    min-height: 0; /* Critical */
    overflow-y: auto;
    max-width: 100%;
    padding: 28px 35px 40px 28px;
    overflow-x: hidden !important;
    box-sizing: border-box;
}

/* Labels */
label {
    display: block;
    margin-top: 12px;
    font-weight: 600;
}

/* Inputs */
input, select {
    width: 100%;
    margin-top: 6px;
    padding: 10px;
    border-radius: 10px;
    border: 1px solid #d1d1d6;
    background: #fafafa;
    font-size: 15px;
}

/* Slider fix */
input[type=range] {
    width: 100%;
    margin: 0;
}

/* --- PERFECT thumb alignment for both sliders (0% left, 100% right) --- */
input[type="range"] {
    -webkit-appearance: none;
    appearance: none;
    padding: 0;
    border: none;
}

/* WebKit (Chrome/Edge/Opera/Safari) */
input[type="range"]::-webkit-slider-runnable-track {
    height: 4px;
    margin-left: 0;
    margin-right: 0;
}

input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 18px;    /* adjust if desired */
    height: 18px;
    border-radius: 50%;
    background: #fff;
    margin-top: -7px; /* vertical align */
}

/* Firefox */
input[type="range"]::-moz-range-track {
    height: 4px;
}

input[type="range"]::-moz-range-thumb {
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background: #fff;
}

/* Thin actual-position feedback slider */
.thin-slider {
    height: 6px;
    margin-top: 4px;
    opacity: 0.5;                  /* 50% transparent */
    pointer-events: none;
    appearance: none;
    background: #888;              /* default grey */
    border-radius: 4px;
}

.thin-slider.moving {
    background: #34c759 !important;
}

/* Chrome / Edge knob removal */
.thin-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 0;
    height: 0;
}

/* Firefox knob removal */
.thin-slider::-moz-range-thumb {
    width: 0;
    height: 0;
}

/* ------------------ FOOTER (inside card, attached) ------------------ */
.footer-box {
    background: rgba(255,255,255,0.72);
    backdrop-filter: blur(16px);
    padding: 16px 18px 16px 18px;
    border-top: 2px solid rgba(0,0,0,0.12);
    border-radius: 0 0 20px 20px;
    position: sticky;
    bottom: 0;
    z-index: 5;
}

/* Style the status block inside footer */
.status-footer {
    margin-bottom: 12px;
    font-size: 14px;
    line-height: 1.3;
}

.footer-box button {
    width: 100%;
    margin-top: 6px;
    margin-bottom: 6px;
}

.status-topline {
    display: flex;
    justify-content: space-between;
    align-items: center;
    width: 100%;
}

.cal-block {
    display: flex;
    align-items: center;
    gap: 6px;
    font-weight: bold;
    font-size: 14px;
}

/* ------------------ CALIBRATION BUTTON GRID ------------------ */
.calib-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;   /* 2 buttons per row */
    gap: 15px;
    width: 100%;                       /* 50% of scroll container */
    margin: 15px 0;                   /* spacing above/below */
}

/* Ensure buttons fill grid cells */
.calib-grid button {
    width: 100%;
}

#calibIcon {
    font-size: 16px;
}

/* Expert mode buttons row */
.btn-row {
    display: none;
    gap: 10px;
}
.btn-row button {
    width: 50%;
}

/* Buttons */
button {
    padding: 12px 18px;
    border-radius: 12px;
    border: none;
    cursor: pointer;
    background: #007aff;
    color: white;
    font-size: 16px;
    transition: 0.2s;
}
    
button:hover { background: #0a84ff; }
button.red { background: #ff3b30; }
button.red:hover { background: #ff453a; }
button.yellow { background: #ffcc00; color: #000; }
button.yellow:hover { background: #ffd60a; }

)CSS";

