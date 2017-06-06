#include "util.h"
#include <Wire.h>
#include <Time.h>
#include <avr/wdt.h>

#ifdef  _WIN32
#include <TimeLib.h>
#endif

#include <DS1307RTC.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>

SoftwareSerial serialPainel(2, 3); // RX, TX

const int chipSelect = 10;
bool salvar;

//estrutura com os dados de um sensor
struct Sensor {
  char pinoTensao;
  char pinoCorrente;
  float corrente;
  float tensao;
  float fatorTensao[2] = {1, 0};
  float fatorCorrente[2] = {1, 0};
  char rangeCorrente;
  char rangeTensao = 5;
};

struct Painel {
  uint8_t posicao;
};

/*
   quantidade de amostras na porta analogica
*/
#define AMOSTRAS  100

Sensor sensor1;
Sensor sensor2;
Painel painelLesteOeste;

String saida = "";

String inputString = "";         // a string to hold incoming data
char entradaSeguidor[64];

/*
   Funcao de regressao linear com com valores em ponto flutuante
*/
float converte(float x, float in_min, float in_max, float out_min,
               float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup () {
  inputString.reserve(15);
  saida.reserve(64);
  serialPainel.begin(9600);
  Serial.begin(9600);

  sensor1.pinoTensao = A3;
  sensor1.pinoCorrente = A2;
  sensor1.rangeCorrente = 20;
  sensor1.fatorCorrente[0] = 1.2549100775;
  sensor1.fatorCorrente[1] = 0.083910469;
  sensor1.rangeTensao = 30;
  //sensor1.fatorTensao[0] = 0.9431656581;
  //sensor1.fatorTensao[1] = 0.0438440746;

  sensor2.pinoTensao = A1;
  sensor2.pinoCorrente = A0;
  sensor2.rangeCorrente = 20;
  sensor2.fatorCorrente[0] = 1.2671894881;
  sensor2.fatorCorrente[1] = 0.0695259983;
  sensor2.rangeTensao = 30;
  //sensor2.fatorTensao[0] = 0.9398017364;
  //sensor2.fatorTensao[1] = 0.0487064435;

  painelLesteOeste.posicao = 0;

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    salvar = false;
  } else {
    salvar = true;
  }

  //Inicializa o Watchdog
  //wdt_enable(WDTO_500MS);

}
unsigned long previousMillis = 0;
void loop() {
  tmElements_t tm;
  String nomeArquivo = String("");

  //wdt_reset();  //  reseta o watchdog

  if (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    if (inChar != '\n')inputString += inChar;
    if (inChar == '\n') {
      if (inputString.length() >= 1) {
        serialPainel.println(inputString);
      }
      inputString = "";
    }
  }

  if (serialPainel.available()) {
    // get the new byte:
    char inSeg = (char)serialPainel.read();
    if (inSeg != '\n') {
      sprintf(entradaSeguidor, "%s%c", entradaSeguidor, inSeg);
    }
    if (inSeg == '\n' || strlen(entradaSeguidor) == 63) {
      if (strlen(entradaSeguidor) >= 1) {
        if (sanitizaEntrada(entradaSeguidor)) {
          trataMensagem(entradaSeguidor);
        }
      }
      for (int i = 0; i < 64; i++) {
        entradaSeguidor[i] = '\0';
      }
    }
  }

  if (millis() - previousMillis >= 1000) {
    previousMillis = millis();
    /*--------LE DATA E HORA---------*/
    if (RTC.read(tm)) {
      saida = tm.Day;
      saida += '/';
      saida += tm.Month;
      saida += '/';
      saida += tmYearToCalendar(tm.Year);
      saida += ' ';
      saida += tm.Hour;
      saida += ':';
      saida += tm.Minute;
      saida += ':';
      saida += tm.Second;
      nomeArquivo += tm.Day;
      //nomeArquivo += " ";
      nomeArquivo += tm.Month;
      //nomeArquivo += " ";
      nomeArquivo += tmYearToCalendar(tm.Year);
      nomeArquivo += ".csv";

      String posicao = "[2/";
      posicao += diaDoAno(&tm);
      posicao += "/";
      posicao += segundoAtual(&tm);
      posicao += "]";
      //Serial.println(posicao);

      serialPainel.println(posicao);

    } else {
      saida += millis();
    }
    saida += ",";
    /*--------LE SENSORES---------*/
    lerSensor(&sensor1);
    saida += sensor1.tensao;
    saida += ",";
    saida += sensor1.corrente;
    saida += ",";
    lerSensor(&sensor2);
    saida += sensor2.tensao;
    saida += ",";
    saida += sensor2.corrente;
    saida += ",";
    saida += painelLesteOeste.posicao;
    /*------SALVA DADOS NO CARTAO DE MEMORIA--*/
    if (salvar) {
      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      if (nomeArquivo.length() < 3) nomeArquivo = "datalog.csv";
      File dataFile = SD.open(nomeArquivo, FILE_WRITE);
      // if the file is available, write to it:
      if (dataFile) {
        dataFile.println(saida);
        dataFile.close();
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.print("error opening ");
        Serial.println(nomeArquivo);
      }
    }
    /*--------MOSTRA NA SERIAL---------*/
    Serial.println(saida);
    /*--------Aguarda 1 segundo---------*/
  }

}


void lerSensor(Sensor *sensor) {
  long lido = 0;
  //tensao
  for (int i = 0; i < AMOSTRAS; i++) lido += analogRead(sensor->pinoTensao);
  //converte e aplica curva de calibração no valor lido
  sensor->tensao  = sensor->fatorTensao[0] * converte((float) lido / AMOSTRAS, 0.0, 1023.0, 0.0, sensor->rangeTensao) + sensor->fatorTensao[1];
  //corrente
  lido = 0;
  for (int i = 0; i < AMOSTRAS; i++) lido += analogRead(sensor->pinoCorrente);
  //converte e aplica curva de calibração
  sensor->corrente = sensor->fatorCorrente[0] * converte((float) lido / AMOSTRAS, 0.0, 1023.0, -sensor->rangeCorrente, sensor->rangeCorrente) + sensor->fatorCorrente[1];
}


void trataMensagem(char *mensagem) {
  //Serial.println(mensagem);
  uint8_t codigo = (uint8_t)extraiCodigo(mensagem);
  switch (codigo) {
    case POSICAOLESTEOESTE:
      painelLesteOeste.posicao = (uint8_t)extraiCodigo(mensagem);
      break;
  }
}


