/*
 * TELESCOPE DEW HEATER
 * 
 * See https://plefunga.com/projects/telescope_dew_heater for more info on what this does or how it works.
 *
 *
 * pinout (for Arduino Uno):
 *    ambient temperature DHT sensor output in pin 8
 *    telescope temperature sensor output in A0
 *    relay switch pin in pin 12
 * 
 * I also tried to use classes for this because I thought it would make the program easier to read.
 * I hope I succeeded.

 * Copyright (c) Nathan Carter 2025
 * 
 * This program is distributed free of charge under GNU Public License 3.0 with absolutely NO WARRANTY.
 * Use this code at you own risk. I am not liable for any damage or injury as a result of using this code.
 *
 * If you redistribute any of this code (including the other files in this sketch), you must include this header.
 * 
 */


#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <SimpleDHT.h>
#include "temp.h"
#include "interval.h"

// initialise the DHT's
SimpleDHT11 ambient_dht(13);
//SimpleDHT11 telescope_dht(12);

Temp previous(0,0);
float aref_level = 3300.0;

int relaypin = 14;

Interval dhtUpdate(2000);
Interval blink(10000);

int currentTemp;
int telescopeTemp;
int humidity;
int dewpoint;

bool state = false;

bool is_connected = false;
bool mdns = false;

ESP8266WebServer server(80);


WiFiManager wm;


void handleRoot()
{
  server.send(200, "text/html", R"=====(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            @import url('https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap');
            :root{
                --frame-bg-colour: #292929;
                --bg-colour: #121212;
                --primary: #EF9A9A;
                --secondary: #9aefef;
                --primary-lower: #EF9A9A33;
                --white: #fff
            }
            html, body{
                background-color: var(--bg-colour);
                width: 100%;
                min-height: 100%;
                font-family: Inter, "sans-serif";
                overflow-x: hidden;
                padding: 0;
                margin: 0;
            }

            .card-container {
                display: grid;
                gap: 20px;
                margin: 20px;
                grid-template-areas:
                "data graph"
                "state graph";
                grid-template-columns: minmax(0, 1fr) minmax(0,1fr);
            }

            /* Responsive layout - makes a one column layout instead of a two-column layout */
            @media (max-width: 1000px) {
                .card-container {
                    grid-template-areas:
                    "data"
                    "state"
                    "graph"
                    "graph";
                    grid-template-columns: minmax(0, 1fr);
                }
            }

            .card-container > div {
                background-color: var(--frame-bg-colour);
                padding: 20px;
                border-radius: 10px;
                min-height: 150px;

                transition: background-color 0.5s;
            }

            .data-card {
                grid-area: data;
            }
            
            .graph-card {
                grid-area: graph;
            }

            .state-card {
                grid-area: state;
            }

            .center {
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100%;
                width: 100%;
            }
            
            .topbar {
                display: flex;
                justify-content:space-between;
                background-color: var(--frame-bg-colour);
                width: 100%;
            }

            .title {
                float:left;
            }

            .ip-addr {
                float:right;
            }

            .text-600 {
                font-weight: 600;
            }
            .text-300 {
                font-weight: 300;
            }

            .colour-white {
                color: var(--white);
            }

            .topbar > div {
                margin: auto 0;
            }

            .p-1 {
                padding: 5px;
            }

            .p-2 {
                padding: 10px;
            }

            .p-3 {
                padding: 15px;
            }

            .p-4 {
                padding: 20px;
            }

            .active {
                background-color: var(--primary-lower)!important;
                border-color: var(--primary)!important;
                border-width: 5px!important;
                border-style: solid;
                color: var(--primary)!important;
            }

            .data-container {
                display:grid;
                grid-template-columns: auto auto;
                row-gap: 5px;
                margin-top: 5px;
            }

            .data-left {
                text-align: left;
            }
            .data-right {
                text-align: right;
            }

            .heading-3 {
                font-size: 1.17em;
                margin-top: 5px;
                margin-bottom: 5px;
            }

            .heading-1 {
                font-size: 2em;
            }

            .graph {
                height: 100%;
            }

        </style>
    </head>

    <body>
        <div class="topbar">
            <div class="text-600 colour-white p-2 heading-1 title">Dew Heater</div>
            <div class="text-300 colour-white p-2 heading-3 ip-addr" id="ip-addr">0.0.0.0</div>
        </div>

        <div class="card-container">
            <div class="data-card">
                <div class="heading-3 text-600 colour-white">Current Data</div>
                <div class="data-container">
                    <div class="data-left colour-white text-300">Temperature:</div>
                    <div class="data-right colour-white text-300" id="ambient">-</div>
                    <div class="data-left colour-white text-300">Humidity:</div>
                    <div class="data-right colour-white text-300" id="humidity">-</div>
                    <div class="data-left colour-white text-300">Dew Point:</div>
                    <div class="data-right colour-white text-300" id="dewpoint">-</div>
                    <div class="data-left colour-white text-300">Telescope Temperature:</div>
                    <div class="data-right colour-white text-300" id="telescope_temp">-</div>
                </div>
            </div>
            <div class="state-card" id="state">
                <div class="heading-1 text-600 center colour-white" id="state-text">-</div>
            </div>
            <div class="graph-card">
                <div class="heading-3 text-600 colour-white">Data timeseries</div>
                <canvas class="graph" id="graph"></canvas>
            </div>
        </div>

        <script>
            var temps = [];
            var humidities = [];
            var dewpoints = [];
            var times = [];
            var telescope_temps = [];
            const ctx = document.getElementById('graph');
            const url = "/data";

            var fail_count = 0;

            async function get_data() {
                if(fail_count > 10) {
                    clearInterval(update_interval);
                    update_interval = setInterval(get_data, 10000);
                }
                
                try {
                    const response = await fetch(url);

                    if(!response.ok) {
                        console.log(response);
                        fail_count += 1;
                        return;
                    }

                    const json = await response.json();

                    document.getElementById("ambient").innerHTML = json.ambient_temp + "&deg;C";
                    document.getElementById("humidity").innerHTML = json.humidity + "%";
                    document.getElementById("dewpoint").innerHTML = json.dew_point + "&deg;C";
                    document.getElementById("telescope_temp").innerHTML = json.telescope_temp + "&deg;C";
                    
                    document.getElementById("ip-addr").innerHTML = json.ip_addr;


                    state = json.state;

                    if(state)
                    {
                        document.getElementById("state-text").classList.remove("colour-white");
                        document.getElementById("state").classList.add("active");
                        document.getElementById("state-text").innerHTML = "ON";
                    }
                    else
                    {
                        document.getElementById("state-text").classList.add("colour-white");
                        document.getElementById("state").classList.remove("active");
                        document.getElementById("state-text").innerHTML = "OFF";
                    }

                    chart.data.datasets[1].data.push(json.ambient_temp);
                    //humidities.push(json.humidity);
                    chart.data.datasets[2].data.push(json.dew_point);
                    chart.data.datasets[0].data.push(json.telescope_temp);
                    chart.data.labels.push((new Date()).toLocaleTimeString());

                    // intentially there is no way for it to get re-filled -- as it would make it look kinda bad.
                    if(json.dew_point < 0)
                    {
                        chart.data.datasets[2].fill = false;
                    }

                    chart.update();

                } catch (err) {
                    console.log(err);
                    return;
                }

                
            }

            var update_interval = setInterval(get_data, 2000);

            var chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        {
                            label: "Telescope Temperature",
                            data: [],
                            fill: false,
                            borderColor: "#EF9A9A",
                            tension: 0.3
                        },
                        {
                            label: "Ambient Temperature",
                            data: [],
                            fill: false,
                            borderColor: "#9aefef",
                            tension: 0.3
                        },
                        {
                            label: "Dew Point",
                            data: [],
                            fill: true,
                            backgroundColor: "#efef9a33",
                            borderColor: "#efef9a",
                            tension: 0.3
                        }
                    ]
                },
                options: {
                    animation: {
                        duration: 1000,
                    },
                    scales: {
                        y: {
                            beginAtZero: true,
                        }
                    },
                }
            });
        </script>
    </body>
</html>
)=====");
}

void handleData()
{
    server.send(200, "application/json", "{\"ambient_temp\":" + String(currentTemp) + ",\"humidity\":"+String(humidity) + ",\"dew_point\":" + String(dewpoint) + ",\"telescope_temp\":" + String(telescopeTemp) + ",\"state\":" + String(state) + ",\"ip_addr\":\"" + WiFi.localIP().toString() +"\"}");
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  wm.setConfigPortalTimeout(60);
  wm.setClass("invert");
  is_connected = wm.autoConnect("Dew Heater");

  if(!MDNS.begin("dewheater"))
  {
    Serial.println("Could not start MDNS.");
  }
  else
  {
    mdns = true;
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  if(mdns)
  {
    MDNS.addService("http", "tcp", 80);
  }
  
  pinMode(relaypin, OUTPUT);

  // lil flash to indicate that we are calibrating
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  Temp ambient(0, 0);
  Temp telescope(0, 0);

  // wait for 2 seconds to elapse from startup for the DHT
  while(millis() < 2000)
  {
    delay(1);
  }

  ambient.read(ambient_dht);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  while(ambient.error && millis() < 15000)
  {
    delay(2000);
    ambient.read(ambient_dht);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if(millis() < 15000)
  {
    aref_level = telescope.calibrate(ambient.temperature);
    Serial.println("Calibration: " + String(aref_level));
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  server.handleClient();
  if(mdns)
  {
    MDNS.update();
  }

  if(dhtUpdate.has_passed())
  {
    // create objects to store the variables in
    Temp ambient(0, 0);
    Temp telescope(0, 0);
    
    // read the data from the DHT11's
    ambient.read(ambient_dht);
    //telescope.read(telescope_dht);

    if(ambient.error)
    {
        ambient = previous;
    }

    // not a dht11, but TMP36
    telescope.read(aref_level);

    // calculate target temperature
    float target = dew_point(ambient.temperature, ambient.humidity) + 3;

    currentTemp = (int)ambient.temperature;
    humidity = (int)ambient.humidity;
    telescopeTemp = (int)telescope.temperature;
    dewpoint = (int)(target - 3);

    // turn on the heater if neccessary
    if(telescope.temperature <= target)
    {
      digitalWrite(relaypin, HIGH);
      state = true;
    }

    else
    {
      digitalWrite(relaypin, LOW);
      state = false;
    }

    // debug printing -- wrap in a function soon?
    char buffer[50];
    sprintf(buffer, "Ambient:\t%d*C\t%d%%\n\r", ambient.temperature, ambient.humidity);
    Serial.print(buffer);
    sprintf(buffer, "Telescope:\t%d*C\n\r", telescope.temperature);
    Serial.print(buffer); 
    sprintf(buffer, "Dew point: %d*C\n\r", (int)dewpoint);
    Serial.print(buffer); 
    sprintf(buffer, "Target: %d*C\n\n\r", (int)target);
    Serial.print(buffer); 

    previous = ambient;
  }

  if(blink.has_passed())
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(10);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  delay(1);
}
