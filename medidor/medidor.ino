#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>

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

void setup() {
  Serial.begin(9600);
  sensor1.pinoTensao = A2;
  sensor1.pinoCorrente = A0;
  sensor1.rangeCorrente = 20;
  sensor1.fatorCorrente[0] = 1.2674128714;
  sensor1.fatorCorrente[1] = 0.0621257196 + 0.02;
  sensor1.rangeTensao = 23;
  sensor1.fatorTensao[0] = 0.9426304149;
  sensor1.fatorTensao[1] = 0.053002733;

  sensor2.pinoTensao = A3;
  sensor2.pinoCorrente = A1;
  sensor2.rangeCorrente = 20;
  sensor2.fatorCorrente[0] = 1.2562496844;
  sensor2.fatorCorrente[1] = 0.0314567446 + 0.02;
  sensor2.rangeTensao = 23;
  sensor2.fatorTensao[0] = 0.9373513716;
  sensor2.fatorTensao[1] = 0.0769268843;
  
  saida = String("data hora, Sensor 1 -> Tensão, Sensor 1 -> Corrente, Sensor 2 -> Tensão, Sensor 2 -> Corrente");
  Serial.println(saida);
  // put your setup code here, to run once:
}

void loop() {
  tmElements_t tm;
  saida = String();

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
  } else {
    saida += millis();
  }
  saida += " , ";
  lerSensor(&sensor1);
  saida += sensor1.tensao;
  saida += " , ";
  saida += sensor1.corrente;
  saida += " , ";
  lerSensor(&sensor2);
  saida += sensor2.tensao;
  saida += " , ";
  saida += sensor2.corrente;
  saida += " , ";
  Serial.println(saida);
  delay(1000);

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

/*
   Funcao de regressao linear com com valores em ponto flutuante
*/
float converte(float x, float in_min, float in_max, float out_min,
               float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
