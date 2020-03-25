/*
 * ClientHTTP example sketch
 * 
 * GET on a WiFiClientSecure with a CA certificate
 * 
 * This example sketch demonstrates how to use the ClientHTTP library
 * to fetch from a web server using GET. HTTPS (TLS 1.2) secure
 * communication is used with a CA certificate. The connection is
 * reused to fetch from the same server every 10 seconds.
 * 
 * Created by Jeroen Döll on 25 March 2020
 * This example is in the public domain
 */

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#if defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiClientSecure.h>

#include <ClientHTTP.h>

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

const char * ssid = STASSID;
const char * password = STAPSK;

// Use web browser to view and copy
// This certificate expires on 1/19/2038, 12:59:59 AM (Central European Standard Time)
const char * rootCACert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----
)EOF";


// Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;
ClientHTTP http(client);


void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial.printf("Connecting to %s", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#if defined(ESP8266)
  configTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "pool.ntp.org", "time.nist.gov");
#endif
#if defined(ESP32)
  configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "pool.ntp.org", "time.nist.gov");
#endif

  Serial.print("Waiting for time");
  time_t epoch;
  for(;;) {
    Serial.write('.');
    time(&epoch);
    if(epoch >= 3600) break;
    delay(500);
  }
  Serial.print("Current time: ");
  Serial.println(asctime(localtime(&epoch)));
  
#if defined(ESP8266)
  BearSSL::X509List cert(rootCACert);
  client.setTrustAnchors(&cert);
#endif
#if defined(ESP32)
  client.setCACert(rootCACert);
#endif

  http.setTimeout(10000);

  http.requestHeaders["Connection"] = "Keep-Alive";
  http.responseHeaders["Connection"];
}


void wait() {
  for(size_t index = 0; index < 10; index++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(); 
}

void loop() {
  if(!http.connected()) {
    if (!http.connect("jigsaw.w3.org", 443)) {
      Serial.print("Connection failed, waiting 10s before retrying");
      wait();
      return;
    }
  }

  if(!http.GET("/HTTP/connection.html")) {
    Serial.println("Failed to GET, waiting 10s before retrying");
    wait();
    return;
  }

  ClientHTTP::http_code_t status = http.status();

  if(status > 0) {
    Serial.printf("HTTP status code is %d\n", status);
    if(status == ClientHTTP::OK) {
      Serial.println("Page succesfully fetched!");
    }

    http.printTo(Serial);
    Serial.println();
  } else {
    Serial.printf("Request failed with error code %d\n", status);
  }

  if(http.responseHeaders["Connection"] != NULL && strcasecmp(http.responseHeaders["Connection"], "close") == 0) {
    Serial.println("Closing connection because the server requested to do so");
    http.stop();
  } else {
    Serial.println("Reusing the connection");
  }

  Serial.print("Wait 10s for the next loop");
  wait();
}
