#include "rtc_e_sol.h"

// Código feito por:
// Philip William -- Código solarimétrico e do RTC
// Anita Cunha, Yara Rodrigues -- Código dos servomotores

// PASSOS LÓGICOS PARA O TRACKER FUNCIONAR
// 1. Inicializar os hardwares relevantes
// 2. Puxar o valor do RTC a partir do rtc.c
// 3. Calcular o ângulo horário solar e ângulo zenital a partir do solarimetria.c
// 4. Girar o motor no ângulo adequado a partir do servo.c
// 5. Esperar 5 minutos para repetir o prodecimento do passo 2-4
// 6. Quando o ângulo zenital passar do horizonte (ou do ângulo de ataque da placa),
// antecipar a posição do dia seguinte e testar em intervalos de 30 minutos até que o sol nasça novamente.

// Valores de entrada
const data_t calibracao = {0, 0, 15, 28, 6, 2025};  // 28/06/2025 15:00:00
const double latitude_local_graus = -23.55;  // São Paulo (negativo para Sul)
const double longitude_local_graus = -46.63;  // São Paulo (negativo para Oeste)
const double longitude_meridiano_padrao_graus = -45.0;  // Para UTC-3 (BRT)
struct rtc_time tm;

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <pwm_z401.h>
#include "rtc_e_sol.h"
//#define CONFIG_SET_RTC_TIME  // comente o define para não configurar o RTC


//ADC
#define ADC_RESOLUTION      12
#define ADC_GAIN            ADC_GAIN_1
#define ADC_REFERENCE       ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID      0  //Canal do ADC, veja a pinagem e nomes em \.platformio\packages\framework-zephyr\_pio\modules\hal\nxp\dts\nxp\kinetis\MKL25Z128VLK4-pinctrl.h
#define ADC_VREF_MV         3300

static int16_t sample_buffer;

//GPIO
#define LED_NODE DT_ALIAS(led1)

#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);


//PWM
#define TPM_MODULE 1000         // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS))
uint16_t duty  = 950;  

//RTC


#define DS1307_NODE DT_ALIAS(ds1307)

#if !DT_NODE_HAS_STATUS(DS1307_NODE, okay)
#error "DS1307 RTC device not found in device tree"
#endif

const struct device *rtc = DEVICE_DT_GET(DS1307_NODE);

int main(void){
    // 1.
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 19);
    pwm_tpm_CnV(TPM2, 1, duty);
    printk("Sistema do tracker solar inicializado.\n");

    while(1){
        // 2.
        if (rtc_get_time(rtc, &tm) == 0) {
            printk("Hora: %04d-%02d-%02d %02d:%02d:%02d\n",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min, tm.tm_sec);
        } else {
            printk("Erro ao ler a hora do RTC\n");
        }

        // 3.
        double omega_atual_graus = angulo_horario_solar();
        double zenital_atual_graus = angulo_zenital();
        if(omega_atual_graus < -900.0 || zenital_atual_graus < -900.0){
            printk("Erro ao calcular o ângulo omega e zenital.\n");
            k_msleep(60000);
            continue;
        }
        printk("Calculado: Omega = %.2f graus, Zenital = %.2f graus.\n", omega_atual_graus, zenital_atual_graus);

        // 4.
        double angulo_servo_alvo = 90.0;
        if(zenital_atual_graus < 90.0){
            angulo_servo_alvo = omega_atual_graus + 90.0;
            if(angulo_servo_alvo < 0.0) angulo_servo_alvo = 0;
            if(angulo_servo_alvo > 180.0) angulo_servo_alvo = 180.0;

            //servo_definir_angulo(angulo_servo_alvo);
            printk("Servo: %.1f graus (Sol acima do horizonte)\n", angulo_servo_alvo);

            // 5.
            k_msleep(5*60*1000);
        }
        else{
            angulo_servo_alvo = 0.0;
            //servo_definir_angulo(angulo_servo_alvo);
            printk("Servo: %.1f graus (Sol abaixo do horizonte, posicao de espera)\n", angulo_servo_alvo);

            // 6.
            while(1){
                printk("Modo noturno. Aguardando 30 minutos para nova verificação.\n");
                k_msleep(300);
                printk("Cheguei aqui.");

                if (rtc_get_time(rtc, &tm) == 0) {
                    printk("Verificação noturna RTC: %02d/%02d %02d:%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min);
                } else {
                    printk("Erro ao ler a hora do RTC\n");
                }

                if(tm.tm_hour>=4 && tm.tm_hour<=8){
                    double omega_check = angulo_horario_solar();
                    double zenital_check = angulo_zenital();
                    printk("Check matinal: Omega=%.2f, Zenital=%.2f\n", omega_check, zenital_check);

                    if(zenital_check < 90.0 && zenital_check >= 0.0){
                        printk("Sol detectado. Retornando operações.\n");
                        break;
                    }
                }
            }
        }
    }
    return 0;
}
