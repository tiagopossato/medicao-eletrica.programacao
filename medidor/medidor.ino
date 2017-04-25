#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>
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

/*
   quantidade de amostras na porta analogica
*/
#define AMOSTRAS  100

Sensor sensor1;
Sensor sensor2;
String saida;

String inputString = "";         // a string to hold incoming data

/*
   Funcao de regressao linear com com valores em ponto flutuante
*/
float converte(float x, float in_min, float in_max, float out_min,
               float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void setup () {
  inputString.reserve(3);
  serialPainel.begin(9600);
  Serial.begin(9600);
  
  sensor1.pinoTensao = A3;
  sensor1.pinoCorrente = A2;
  sensor1.rangeCorrente = 20;
  sensor1.fatorCorrente[0] = 1.2671894881;
  sensor1.fatorCorrente[1] = 0.0625259983;
  sensor1.rangeTensao = 30;
  //sensor1.fatorTensao[0] = 0.9431656581;
  //sensor1.fatorTensao[1] = 0.0438440746;

  sensor2.pinoTensao = A1;
  sensor2.pinoCorrente = A0;
  sensor2.rangeCorrente = 20;
  sensor2.fatorCorrente[0] = 1.2549100775;
  sensor2.fatorCorrente[1] = 0.033910469;
  sensor2.rangeTensao = 30;
  //sensor2.fatorTensao[0] = 0.9398017364;
  //sensor2.fatorTensao[1] = 0.0487064435;


  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    salvar = false;
  } else {
    salvar = true;
  }


}

void loop() {
  tmElements_t tm;
  saida = String();
  String nomeArquivo = String("");
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;

  if (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    if (inChar != '\n')inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      if (inputString.length() >= 1) {
        serialPainel.println(inputString.toInt());
      }
      inputString = "";
    }
  }

  if (serialPainel.available()) {
    Serial.write(serialPainel.read());
  }

  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    /*--------LE DATA E HORA---------*/
    if (RTC.read(tm)) {
      saida += tm.Day;
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

      calculaPosicao(&tm);
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

/**
   Calcula posicao do painel pela hora do dia
*/
uint32_t calculaPosicao(tmElements_t *tm) {
  uint8_t tmp = 1;
  uint16_t dias = tm->Day;
  for (; tmp < tm->Month; tmp++) {
    switch (tmp) {
      case 1: dias += 31; break;
      case 2: dias += 28; break;
      case 3: dias += 31; break;
      case 4: dias += 30; break;
      case 5: dias += 31; break;
      case 6: dias += 30; break;
      case 7: dias += 31; break;
      case 8: dias += 31; break;
      case 9: dias += 30; break;
      case 10: dias += 31; break;
      case 11: dias += 30; break;
    }
  }

  uint32_t segundosPorDia = porDoSol(dias) - nascerDoSol(dias);
  uint32_t segundosAtual = ((uint32_t)tm->Hour * 60 * 60 ) + ((uint32_t)tm->Minute * 60 ) + (uint32_t)tm->Second;
  uint8_t segundosRelativos = (segundosAtual - nascerDoSol(dias)) * 100 / segundosPorDia;

  if (segundosRelativos >= 0 && segundosRelativos <= 100) {
    serialPainel.println(segundosRelativos);
    Serial.print(segundosRelativos);
    Serial.println("% do dia.");
  }

}

/**
   Calcula o segundo em que o sol nasce no dia
*/
uint32_t nascerDoSol(uint16_t dia) {
  return (0.0000000302070774172166 * pow(dia, 5))
         - (0.0000198076304492236 * pow(dia, 4))
         + (0.0035950755 * pow(dia, 3))
         - (0.2694919508 * pow(dia, 2))
         + (48.27020716989 * dia)
         + 19901.4743825752;
}

uint32_t porDoSol(uint16_t dia) {
  return (0.0000000415881756309789 * pow(dia, 5))
         - (0.000042747 * pow(dia, 4))
         + (0.0155753852 * pow(dia, 3))
         - (2.1677644896 * pow(dia, 2))
         + (58.67243756 * dia)
         + 69346.2344757363;
}

