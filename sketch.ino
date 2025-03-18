// Bibliotecas que serão utilizadas para o display e/ou para a publish
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);


#define BTM 25 //Botão que alterna entre modo automático, edição e manual
#define BTE 26 //Botão para recolher ou estender o varal no modo manual
#define BTR 12 //Botão de Redução do valor do parâmetro de luminosidade do modo de edição
#define BTA 14 //Botão de Adição do valor do parâmetro de luminosidade do modo de edição
#define Chu 35 //Sensor de chuva (umidade)
#define Lum 34 //Sensor de luminosidade
#define MT0 0 //LED do motor
#define MT1 2 // LED do motor
#define FC_Fora 19 //Fim de curso fora
#define FC_Dentro 18 //Fim de curso dentro


enum EstadosModoManual {
  Recolhido = 0,
  Estendendo = 1,
  Estendido = 2,
  Recolhendo = 3
} estadoModoManual;

enum EstadosModoAutomatico {
  Monitorando = 0,
  Recolhendo_AT = 1,
  Estendendo_AT = 2
} estadoModoAutomatico;

enum EstadosModoDeFuncionamento{
  Manual = 0,
  Edicao = 1,
  Automatico = 2,
  Aumenta = 3,
  Diminui = 4
} estadoModoDeFuncionamento;

//parâmetros
int P_Lum = 1500;
const int P_Lum_min = 500;
const int P_Lum_max = 10000;

int ValorSensorAnalogico;
float ValorSensorLum;

//------------------------- para a publish --------------------------------

EstadosModoDeFuncionamento estadoModoDeFuncionamentoAnterior;

//credenciais para acesso a rede e ao MQTT
const char* ssid = "Wokwi-GUEST";
const char* key = "";
const char* broker = "test.mosquitto.org";
int port = 1883;

byte estadoModoDeFuncionamentoAlterado_Publish;
bool publishRecolhidoManual = false;
bool publishEstendendoManual = false;
bool publishEstendidoManual = false;
bool publishRecolhendoManual = false;
bool publishMonitorandoAutomatico = false;
bool publishRecolhendoAutomatico = false;
bool publishEstendendoAutomatico = false;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void conexaoWiFi() {
  //Conexão ao Wi-Fi
  Serial.print("Conectando-se ao Wi-Fi ");  
  Serial.print(ssid);
  Serial.print(" ");
  WiFi.begin(ssid, key, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
}

void conexaoBroker() {
  //Conexão ao Broker
  mqttClient.setServer(broker, port);
  while (!mqttClient.connected()) {
    Serial.print("Conectando-se ao broker ");    
    //if (mqttClient.connect(WiFi.macAddress().c_str())) {
    if (mqttClient.connect("87987ji0938j1289KJSAUE3")) {
      Serial.println(" Conectado!");            
    } else {
      Serial.print(".");      
      delay(100);
    }
  }
}
//----------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  //Display
  LCD.init();
  LCD.backlight();

  //Entradas e saídas da simulação
  pinMode(BTE, INPUT);
  pinMode(BTM, INPUT);
  pinMode(BTR, INPUT);
  pinMode(BTA, INPUT);
  pinMode(Chu, INPUT);
  pinMode(Lum, INPUT);
  pinMode(FC_Dentro, INPUT);
  pinMode(FC_Fora, INPUT);
  pinMode(MT0, OUTPUT);
  pinMode(MT1, OUTPUT);

  //inicialização do estado inicial das máquinas
  estadoModoDeFuncionamento = Manual;
  estadoModoManual = Recolhido;
  estadoModoAutomatico = Monitorando;

  //para a parte da publish
  estadoModoDeFuncionamentoAlterado_Publish = 1;
  estadoModoDeFuncionamentoAnterior = Manual;

  conexaoWiFi();
  conexaoBroker();
}


// INÍCIO DAS FUNÇÕES DA MÁQUINA DO MODO DE FUNCIONAMENTO (MÁQUINA PRINCIPAL)
void processamentoModoManual(){
  LCD.setCursor(5, 0);
  LCD.print("MANUAL ");

  if(digitalRead(BTM)){
    estadoModoDeFuncionamento = Edicao;
  }
}

void processamentoModoAutomatico(){
    LCD.setCursor(1, 0);
    LCD.print("AUTOMATICO LUM");

    ValorSensorAnalogico = analogRead(Lum); //é o valor do sensor analógico
    ValorSensorLum = (float)ValorSensorAnalogico / 4095.0 * (100000.0 - 1) + 1; // faz a conversão para lux

    LCD.setCursor(2, 1);
    LCD.print(ValorSensorLum);
    LCD.print(" lux");

  if(digitalRead(BTM)){
    LCD.clear(); //limpa a tela do display
    estadoModoDeFuncionamento = Manual;
  }
}

void processamentoModoEdicao(){

  //apaga os leds ao sair do modo manual nos estados estendendo ou recolhendo, em que, eles estão acesos
  digitalWrite(MT0, LOW);
  digitalWrite(MT1, LOW);

  LCD.setCursor(2, 0);
  LCD.print("EDICAO P_LUM ");

  LCD.setCursor(0, 1);
  LCD.print("VALOR: ");
  LCD.print(P_Lum);
  LCD.print(" lux");

  Serial.print("MODO DE EDIÇÃO: P_LUM = ");
  Serial.println(String(P_Lum) + " lux");

  if(digitalRead(BTM)){
    LCD.clear();
    estadoModoDeFuncionamento = Automatico;
  }

  if(digitalRead(BTR)){
    estadoModoDeFuncionamento = Diminui;
  }

  if(digitalRead(BTA)){
    estadoModoDeFuncionamento = Aumenta;
  }
}

void processamentoAumentaLuminosidade(){
  if(P_Lum + 100 <= P_Lum_max){
    P_Lum = P_Lum + 100; //realiza o incremento
    Serial.println("P_Lum foi aumentado para: "+ String(P_Lum) + " lux");
  }else{
    Serial.println("Atingiu-se o limite máximo do parâmetro");
  }
  //delay(500);
  estadoModoDeFuncionamento = Edicao;
}

void processamentoDiminuiLuminosidade(){
  if(P_Lum - 100 >= P_Lum_min){
    P_Lum = P_Lum - 100; // realiza o decremento
    Serial.println("P_Lum foi diminuído para: "+ String(P_Lum) + " lux");
  }else{
    Serial.println("Atingiu-se o limite mínimo do parâmetro");
  }
  //delay(500);
  estadoModoDeFuncionamento = Edicao;
}
//------- FIM DAS FUNÇÕES DA MÁQUINA DO MODO DE FUNCIONAMENTO ------------



//-------------- INÍCIO DAS FUNÇÕES DO MODO MANUAL -----------------------
void processamentoRecolhido(){

  if(!publishRecolhidoManual){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Varal Recolhido");
    publishRecolhidoManual = true;
  } 
  Serial.println("Estado Recolhido -> motor desligado");
  digitalWrite(MT0, LOW);
  digitalWrite(MT1, LOW);

  if(digitalRead(BTE)){    
    estadoModoManual = Estendendo;

    //Reset das flags para continuar o looping
    publishRecolhidoManual = false;
    publishEstendendoManual = false;
    publishEstendidoManual = false;
    publishRecolhendoManual = false;
  }
  
}

void processamentoEstendendo(){

  if(!publishEstendendoManual){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Varal Estendendo");
    publishEstendendoManual = true;
  } 
  Serial.println("Estendendo -> motor acionado");
  digitalWrite(MT0, LOW);
  digitalWrite(MT1, HIGH);

  if(digitalRead(FC_Fora)){
    estadoModoManual = Estendido;

    //Reset das flags para continuar o looping
    publishRecolhidoManual = false;
    publishEstendendoManual = false;
    publishEstendidoManual = false;
    publishRecolhendoManual = false;
  }
 
}

void processamentoEstendido(){

  if(!publishEstendidoManual){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Varal Estendido");
    publishEstendidoManual = true;
  } 
  Serial.println("Estendido -> motor desligado");
  digitalWrite(MT0, LOW);
  digitalWrite(MT1, LOW);

  if(digitalRead(BTE)){
    estadoModoManual = Recolhendo;

    //Reset das flags para continuar o looping
    publishRecolhidoManual = false;
    publishEstendendoManual = false;
    publishEstendidoManual = false;
    publishRecolhendoManual = false;
  }

}

void processamentoRecolhendo(){

  if(!publishRecolhendoManual){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Varal Recolhendo");
    publishRecolhendoManual = true;
  } 
  Serial.println("Recolhendo -> motor ligado");
  digitalWrite(MT0, HIGH);
  digitalWrite(MT1, LOW);

  if(digitalRead(FC_Dentro)){
    estadoModoManual = Recolhido;

    //Reset das flags para continuar o looping
    publishRecolhidoManual = false;
    publishEstendendoManual = false;
    publishEstendidoManual = false;
    publishRecolhendoManual = false;
  }

}
// -------------- FIM DAS FUNÇÕES DO MODO MANUAL -------------------------


//--------------- INÍCIO DAS FUNÇÕES DO MODO AUTOMÁTICO ------------------
void processamentoMonitorando_Automatico(){
  
  if(!publishMonitorandoAutomatico){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Monitorando");
    publishMonitorandoAutomatico = true;
  }

  digitalWrite(MT0, LOW);
  digitalWrite(MT1, LOW);

  //int ValorSensorAnalogico = analogRead(Lum); //é o valor do sensor analógico
  //float ValorSensorLum = (float)ValorSensorAnalogico / 4095.0 * (100000.0 - 1) + 1; // faz a conversão para lux


  //parâmetros bool retornam true ou false
  bool ChuvaDetectada = digitalRead(Chu);
  bool FimDeCurso_Dentro = digitalRead(FC_Dentro);
  bool FimDeCurso_Fora = digitalRead(FC_Fora);

  
  Serial.print("Luminosidade: ");
  Serial.print(ValorSensorLum);

  Serial.print(" lux\tChuva: ");
  Serial.println(ChuvaDetectada);

  if((ChuvaDetectada || (ValorSensorLum < P_Lum)) && !FimDeCurso_Dentro){
    estadoModoAutomatico = Recolhendo_AT;

    //Reset das flags para continuar o looping
    publishMonitorandoAutomatico = false;
    publishRecolhendoAutomatico = false;
    publishEstendendoAutomatico = false;
  }

  if((!ChuvaDetectada && (ValorSensorLum > P_Lum)) && !FimDeCurso_Fora){
    estadoModoAutomatico = Estendendo_AT;

    //Reset das flags para continuar o looping
    publishMonitorandoAutomatico = false;
    publishRecolhendoAutomatico = false;
    publishEstendendoAutomatico = false;
  }

}


void processamentoRecolhendo_Automatico(){

  if(!publishRecolhendoAutomatico){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Varal Recolhendo");
    publishRecolhendoAutomatico = true;
  }

  Serial.println("Modo Automático - RECOLHENDO");

  digitalWrite(MT0, HIGH);
  digitalWrite(MT1, LOW);

  if(digitalRead(FC_Dentro)){
    estadoModoAutomatico = Monitorando;

    //Reset das flags para continuar o looping
    publishMonitorandoAutomatico = false;
    publishRecolhendoAutomatico = false;
    publishEstendendoAutomatico = false;
  }

}

void processamentoEstendendo_Automatico(){

  if(!publishEstendendoAutomatico){ //publica uma única vez dentro do estado
    mqttClient.publish("estadoModoDeFuncionamento", "Estendendo");
    publishEstendendoAutomatico = true;
  }
  
  Serial.println("Modo Automático - ESTENDENDO");

  digitalWrite(MT0, LOW);
  digitalWrite(MT1, HIGH);

  if(digitalRead(FC_Fora)){
    estadoModoAutomatico = Monitorando;

    //Reset das flags para continuar o looping
    publishMonitorandoAutomatico = false;
    publishRecolhendoAutomatico = false;
    publishEstendendoAutomatico = false;
  }
}
//--------------- FIM DAS FUNÇÕES DO MODO AUTOMÁTICO ----------------------



void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconexão ao Wi-Fi...");
    conexaoWiFi();
  }
  if (!mqttClient.connected()) {
    Serial.println("Reconexão ao broker...");
    conexaoBroker();
  }


  // verificação da mudança de estado para a publicação, de forma a publicar uma única vez a mensagem em cada estado
  if ((estadoModoDeFuncionamento != estadoModoDeFuncionamentoAnterior)) {
    estadoModoDeFuncionamentoAlterado_Publish = 1;
    estadoModoDeFuncionamentoAnterior = estadoModoDeFuncionamento;
  }

  switch(estadoModoDeFuncionamento){ //processamento da máquina principal
    case Manual:
      if(estadoModoDeFuncionamentoAlterado_Publish){
        estadoModoDeFuncionamentoAlterado_Publish = 0;
        mqttClient.publish("estadoModoDeFuncionamento", "Modo Manual");
      }
      processamentoModoManual();
      break;
    case Automatico:
      if(estadoModoDeFuncionamentoAlterado_Publish){
          estadoModoDeFuncionamentoAlterado_Publish = 0;
          mqttClient.publish("estadoModoDeFuncionamento", "Modo Automatico");
      }
      processamentoModoAutomatico();
      break;
    case Edicao:
      if(estadoModoDeFuncionamentoAlterado_Publish){
          estadoModoDeFuncionamentoAlterado_Publish = 0;
          mqttClient.publish("estadoModoDeFuncionamento", "Modo de Edicao");
      }
      processamentoModoEdicao();
      break;
    case Aumenta:
      if(estadoModoDeFuncionamentoAlterado_Publish){
          estadoModoDeFuncionamentoAlterado_Publish = 0;
          mqttClient.publish("estadoModoDeFuncionamento", "Modo de Edicao - Aumenta Luminosidade");
      }
      processamentoAumentaLuminosidade();
      break;
    case Diminui:
      if(estadoModoDeFuncionamentoAlterado_Publish){
          estadoModoDeFuncionamentoAlterado_Publish = 0;
          mqttClient.publish("estadoModoDeFuncionamento", "Modo de Edicao - Diminui Luminosidade");
      }
      processamentoDiminuiLuminosidade();
      break;
  }


  if(estadoModoDeFuncionamento == Manual){ //processamento do modo manual
    switch(estadoModoManual){
      case Recolhido:
        processamentoRecolhido();
        break;
      case Estendendo:
        processamentoEstendendo();
        break;
      case Estendido:
        processamentoEstendido();
        break;
      case Recolhendo:
        processamentoRecolhendo();
        break;
    }
  }

  
  if(estadoModoDeFuncionamento == Automatico){ //processamento do modo automático
    switch(estadoModoAutomatico){
      case Monitorando:
        processamentoMonitorando_Automatico();
        break;
      case Recolhendo_AT:
        processamentoRecolhendo_Automatico();
        break;
      case Estendendo_AT:
        processamentoEstendendo_Automatico();
        break;
    }
  }
  

  delay(100); // this speeds up the simulation
}
