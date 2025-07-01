#ifndef RTC_H_
#define RTC_H_

// Definir um novo tipo de função chamado data_t, que agrupa todos os valores relevantes
// em uma variável só.
typedef struct{
    int segundo;
    int minuto;
    int hora;
    int dia;
    int mes;
    int ano;
} data_t;

extern struct rtc_time tm;
extern const data_t calibracao;
extern const double latitude_local_graus; // São Paulo (negativo para Sul)
extern const double longitude_local_graus; // São Paulo (negativo para Oeste)
extern const double longitude_meridiano_padrao_graus; // Para UTC-3 (BRT)

double angulo_horario_solar(void);
double angulo_zenital(void);

#endif /* RTC_H_ */