#include <Wire.h>               // Biblioteca para comunicação I2C
#include <Adafruit_GFX.h>       // Biblioteca base para displays
#include <Adafruit_SSD1306.h>   // Biblioteca para o display OLED SSD1306
#include <ESP8266WiFi.h>        // Biblioteca para conexão Wi-Fi
#include <ESP8266HTTPClient.h>  // Biblioteca para requisições HTTP

#define SCREEN_WIDTH 128        // Largura do display OLED
#define SCREEN_HEIGHT 64       // Altura do display OLED

// Cria um objeto para o display OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Definindo os pinos dos sensores e LED
#define MQ135_PIN A0           // Pino analógico para o MQ-135
#define MQ7_PIN D0             // Pino analógico para o MQ-7
#define LED_PIN D4             // Pino digital para o LED

// Configurações Wi-Fi
const char* SSID = "TTFIBRA-ABADE";
const char* PASSWORD = "abade2605";

// Configurações do Supabase
const char* SUPABASE_URL = "https://krccnsbfofpowxdzspwc.supabase.co/rest/v1/sensores";  // URL do seu projeto
const char* SUPABASE_API_KEY = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtyY2Nuc2Jmb2Zwb3d4ZHpzcHdjIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzQwMjQyNjksImV4cCI6MjA0OTYwMDI2OX0.5r-KaYdn6UEdaVnQOaN3nDp4SCIs6WH6DUecnzhRXko";  // Sua chave anon do Supabase

// Configurações do ThingSpeak
const char* THINGSPEAK_API_KEY = "YIEBLY8CUWNS85AF"; // Sua chave do ThingSpeak
const char* THINGSPEAK_SERVER = "http://api.thingspeak.com/update";

// Lista para armazenar as leituras (gráfico)
int readings[128];
int readingIndex = 0;

// Função para ler o valor do sensor MQ-135
int readMQ135() {
  return analogRead(MQ135_PIN);  // Lê o valor do pino analógico (0-1023)
}

// Função para ler o valor do sensor MQ-7
int readMQ7() {
  return analogRead(MQ7_PIN);  // Lê o valor do pino analógico do MQ-7
}

// Função para avaliar a qualidade do ar
String evaluateAirQuality(int value) {
  if (value > 700) {
    return "Péssima";
  } else if (value > 500) {
    return "Ruim";
  } else if (value > 300) {
    return "Moderada";
  } else {
    return "Boa";
  }
}

// Função para desenhar o gráfico no display OLED
void drawGraph() {
  display.fillRect(0, 32, 128, 32, BLACK);  // Limpa a área do gráfico
  for (int i = 1; i < 128; i++) {
    int y1 = 64 - (readings[(readingIndex + i - 1) % 128] / 16);  // Normaliza as leituras para o display
    int y2 = 64 - (readings[(readingIndex + i) % 128] / 16);
    if (y1 >= 32 && y1 < 64 && y2 >= 32 && y2 < 64) {
      display.drawLine(i - 1, y1, i, y2, WHITE);
    }
  }
}

// Função para enviar os dados para o Supabase
void sendToSupabase(int mq135Value, int mq7Value, String airQuality) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = SUPABASE_URL;

    // Criando o corpo da requisição em formato JSON
    String jsonBody = "{\"mq135\":" + String(mq135Value) + ", \"mq7\":" + String(mq7Value) + ", \"qualidade\":\"" + airQuality + "\", \"timestamp\": \"NOW()\"}";

    // Configurando o cabeçalho da requisição HTTP
    http.begin(url);  // URL do Supabase
    http.addHeader("Content-Type", "application/json");  // Cabeçalho para indicar que o corpo é em JSON
    http.addHeader("Authorization", SUPABASE_API_KEY);  // Chave de autorização para o Supabase

    // Enviando o POST para o Supabase
    int httpResponseCode = http.POST(jsonBody);

    if (httpResponseCode > 0) {
      Serial.print("[HTTP] Response Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("[HTTP] Error Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("[WiFi] Not connected");
  }
}

// Função para enviar dados ao ThingSpeak
void sendToThingSpeak(int value1, int value2) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client; // Cria uma instância de WiFiClient
    String url = String(THINGSPEAK_SERVER) + "?api_key=" + THINGSPEAK_API_KEY + "&field1=" + value1 + "&field2=" + value2;

    http.begin(client, url); // Usando o WiFiClient como primeiro parâmetro
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("[HTTP] Response Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("[HTTP] Error Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("[WiFi] Not connected");
  }
}

void setup() {
  // Inicializa a comunicação serial
  Serial.begin(115200);

  // Inicializa o display OLED com endereço I2C 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Troque para 0x3D se necessário
    Serial.println("Falha ao iniciar o display OLED");
    for (;;);
  }
  display.clearDisplay();

  // Conecta ao Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("[WiFi] Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");

  // Mostra mensagem de conexão no display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Conectado ao Wi-Fi!");
  display.display();
}

void loop() {
  // Lê os valores dos sensores MQ-135 e MQ-7
  int mq135Value = readMQ135();
  int mq7Value = readMQ7();

  // Avalia a qualidade do ar
  String airQuality = evaluateAirQuality(mq135Value);

  // Exibe os dados no display OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("MQ-135: ");
  display.println(mq135Value);
  display.print("MQ-7: ");
  display.println(mq7Value);
  display.print("Qualidade do Ar: ");
  display.println(airQuality);
  display.display();

  // Envia os dados para o Supabase
  sendToSupabase(mq135Value, mq7Value, airQuality);

  // Envia os dados para o ThingSpeak
  sendToThingSpeak(mq135Value, mq7Value);

  // Atualiza a lista de leituras para o gráfico
  readings[readingIndex] = mq135Value;
  readingIndex = (readingIndex + 1) % 128;

  // Desenha o gráfico de leituras no display
  drawGraph();

  delay(10000);  // Aguarda 10 segundos antes de nova leitura
}
