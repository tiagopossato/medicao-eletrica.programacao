#include <Time.h>

#ifdef  _WIN32
#include <TimeLib.h>
#endif

//Função para resetar o programa
void(* resetFunc) (void) = 0;


int extraiCodigo(char *entrada) {
  char i = 0;
  String tmp;
  tmp.reserve(6);
  for (i = 0;; i++) {
    //aceita valores até 32,767
    if (i > 5) break;
    if (entrada[i] == '/') break;
    tmp += entrada[i];
  }

  sprintf(entrada, entrada + i + 1);
  Serial.print("Numero: ");
  Serial.println(tmp.toInt());
  return tmp.toInt();
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

/**
   Calcula o segundo em que o sol se põe no dia
*/
uint32_t porDoSol(uint16_t dia) {
  return (0.0000000415881756309789 * pow(dia, 5))
         - (0.000042747 * pow(dia, 4))
         + (0.0155753852 * pow(dia, 3))
         - (2.1677644896 * pow(dia, 2))
         + (58.67243756 * dia)
         + 69346.2344757363;
}

/**
   Retorna o dia do ano a partir de 1º de Janeiro
*/
uint16_t diaDoAno(tmElements_t *tm) {
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
  return dias;
}

/**
   Calcula o segundo atual a partir da zero horas
*/
uint32_t segundoAtual(tmElements_t *tm) {
  return ((uint32_t)tm->Hour * 60 * 60 ) + ((uint32_t)tm->Minute * 60 ) + (uint32_t)tm->Second;
}

/**
   Calcula posicao do painel pela hora do dia
*/
/*
uint8_t calculaPosicao(tmElements_t *tm) {

  uint16_t dias = diaDoAno(tm);

  uint32_t segundosPorDia = porDoSol(dias) - nascerDoSol(dias);
  uint32_t segundosAtual = segundoAtual(tm);
  uint8_t segundosRelativos = (segundosAtual - nascerDoSol(dias)) * 100 / segundosPorDia;

  if (segundosRelativos >= 0 && segundosRelativos <= 100) {
    Serial.print(segundosRelativos);
    Serial.println("% do dia.");
    return segundosRelativos;
  }

  return 0;
}

*/

