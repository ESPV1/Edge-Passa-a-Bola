#include <WiFi.h> // lib wifi
#include <PubSubClient.h> //
#include <stdio.h>

// SSID, senha do wifi, IP do broker e porta
const char* default_SSID = "Wokwi-GUEST"; // SSID do Wokwi
const char* default_PASSWORD = ""; // wifi do Wokwi não tem senha
const char* default_BROKER_MQTT = "54.172.140.81"; // IP do Broker MQTT
int default_BROKER_PORT = 1883; // porta do Broker MQTT

// pinos pir e LED
#define default_pinPir 13
#define default_pinUserLED 4
#define default_pinSystemLED 19

// tópicos
const char* default_TOPICO_SUBSCRIBE = "/TEF/device001/cmd"; // tópico subscriber
const char* default_TOPICO_PUBLISH_1 = "/TEF/device001/attrs"; // tópico publisher
const char* default_TOPICO_PUBLISH_2 = "/TEF/device001/attrs/p"; // tópico publisher
const char* default_ID_MQTT = "fiware_001"; // ID MQTT
const char* topicPrefix = "device001";   // prefixo do tópico

// transformando em variáveis
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;

char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);

int pinPir = default_pinPir;
int pinUserLED = default_pinUserLED;
int pinSystemLED = default_pinSystemLED;

WiFiClient espClient;
PubSubClient MQTT(espClient);

char outputState = '0';

// inicia conexão wifi
void initWiFi() {
  delay(10);
  Serial.println("========== conexao wifi ==========");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  connectWiFi();
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

bool status = 0;

void setup() {
  // pisca
  initOutput();

  // inicia monitor serial
  Serial.begin(115200);

  // conecta wifi
  initWiFi();

  // conecta mqtt
  initMQTT();

  delay(5000);

  // publica tópico inicial
  MQTT.publish(TOPICO_PUBLISH_1, "s|on");
}

void loop() {
  // checa as conexões
  verifyMQTTAndWiFiConnection();

  // envia estado do LED para os inscritos
  sendOutputStateMQTT();

  // lê pier e envia para o tópico caso o gol seja marcado
  readGoal();

  // mantém o mqtt durante o loop
  MQTT.loop();
}

void initOutput() {
  pinMode(pinPir, OUTPUT);
  pinMode(pinUserLED, OUTPUT);
  pinMode(pinSystemLED, OUTPUT);

  // acende LED
  digitalWrite(pinUserLED, HIGH); 

  // controla on/off do led
  boolean toggle = false;

  // pisca led
  for (int i = 0; i <= 10; i++) {
    toggle = !toggle;
    digitalWrite(pinUserLED, toggle);
    delay(200);
  }
}

void sendOutputStateMQTT() {
  if (outputState == '1') {
    MQTT.publish(TOPICO_PUBLISH_1, "s|on");
    Serial.println("- Led Ligado");
  }

  if (outputState == '0') {
    MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    Serial.println("- Led Desligado");
  }
  Serial.println("- Estado do LED onboard enviado ao broker!");
  delay(100);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;

  // insere os caracteres recebidos na string 'msg'
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  // printa a mensagem recebida
  Serial.print("- Mensagem recebida: ");
  Serial.println(msg);

  // forma o padrão de tópico para comparação
  String onTopic = String(topicPrefix) + "@on|";
  String offTopic = String(topicPrefix) + "@off|";

  // compara com o tópico recebido
  if (msg.equals(onTopic)) {
    digitalWrite(pinUserLED, HIGH);
    outputState = '1';
  }

  if (msg.equals(offTopic)) {
    digitalWrite(pinUserLED, LOW);
    outputState = '0';
  }
}

void connectWiFi() {
  // se o wifi estiver conectado retorna
  if (WiFi.status() == WL_CONNECTED) return;

  // tenta conectar ao wifi com SSID e senha
  WiFi.begin(SSID, PASSWORD);

  // enquanto não conecta printa loading
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  // quando consegue conectar printa sucesso SSID e IP
  Serial.println();
  Serial.print("Conectado com sucesso na rede: ");
  Serial.println(SSID);
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());

  // garantir que o LED inicie desligado
  digitalWrite(pinUserLED, LOW);
}

void connectMQTT() {
  Serial.println("========== conexao mqtt =========");
  while (!MQTT.connected()) {
    Serial.print("Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Haverá nova tentativa de conexão em 2s");
      delay(2000);
    }
  }
}

void verifyMQTTAndWiFiConnection() {
  if (!MQTT.connected()) connectMQTT();
  if(WiFi.status() != WL_CONNECTED) connectWiFi();
}

void readGoal() {
  int pirValue = digitalRead(pinPir);

  if ((pirValue == HIGH) && (status == 0)){
    status = 1;
    Serial.println("Gol Marcado!");

    digitalWrite(pinSystemLED, HIGH);

    // manda pros inscritos
    MQTT.publish(TOPICO_PUBLISH_2, "Gol Marcado!");
  }
  else if ((pirValue == LOW) && (status == 1)){
    status = 0;
    digitalWrite(pinSystemLED, LOW);
  }

  delay(100);
}