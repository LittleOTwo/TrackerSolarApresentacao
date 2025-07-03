#include "rtc_e_sol.h"
#include <stdio.h>
#include <math.h>
#include <zephyr/drivers/rtc.h>

#ifndef M_PI
#define M_PI 3.1415926535f
#endif

int cor[12]={0,1,-1,0,0,1,1,2,3,3,4,4};

int eh_bissexto(int ano){
    if ((ano % 4 == 0 && ano % 100 != 0) || (ano % 400 == 0)){
        return 1;
    }
    else return 0;
}

int calcular_n(int dia, int mes, int ano){
    return dia + (mes - 1) * 30 + cor[mes - 1] + eh_bissexto(ano);
}

float para_radianos(float graus){
    return graus * (M_PI / 180.0f);
}

float para_graus(float radianos){
    return radianos * (180.0f / M_PI);
}

float angulo_horario_solar(){
    int n = calcular_n(tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

    float B_rad = (2.0f * M_PI * (n - 81.0f)) / 364.0f;  // B em radianos

    float EoT_minutos = 9.87f * sinf(2.0f * B_rad) - 7.53f * cosf(B_rad) - 1.5f * sinf(B_rad);  

    float hcl_decimal = tm.tm_hour + (tm.tm_min / 60.0f) + (tm.tm_sec / 3600.0f);

    float delta_t_longitude_minutos = 4.0f * (longitude_meridiano_padrao_graus - longitude_local_graus);

    float tsl_horas = hcl_decimal + (EoT_minutos + delta_t_longitude_minutos) / 60.0f;

    float omega_ahs_graus = 15.0f * (tsl_horas - 12.0f);  // Ângulo horário solar
    return omega_ahs_graus;
}

float angulo_zenital(){
    int n = calcular_n(tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    float omega_ahs_graus = angulo_horario_solar();
    float argumento_declinacao_graus = (360.0f / 365.0f) * (284.0f + n);
    float argumento_declinacao_rad = para_radianos(argumento_declinacao_graus);
    float delta_ds_graus = 23.45f * sinf(argumento_declinacao_rad);  // Declinação solar

    float latitude_rad = para_radianos(latitude_local_graus);
    float delta_ds_rad = para_radianos(delta_ds_graus);
    float omega_ah_rad = para_radianos(omega_ahs_graus);

    float cos_theta_z = sinf(latitude_rad) * sinf(delta_ds_rad) + cosf(latitude_rad) * cosf(delta_ds_rad) * cosf(omega_ah_rad);

    // Garantir que o argumento de acos está no intervalo [-1.0, 1.0]
    if (cos_theta_z > 1.0f) cos_theta_z = 1.0f;
    if (cos_theta_z < -1.0f) cos_theta_z = -1.0f;

    float angulo_zenital_rad = acosf(cos_theta_z);
    float angulo_zenital_graus = para_graus(angulo_zenital_rad);
    return angulo_zenital_graus;
}