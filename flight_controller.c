#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

// ==========================================
//           DEFINIÇÃO DOS ESTADOS
// ==========================================
typedef enum {
    STATE_STANDBY,          // Na rampa
    STATE_POWERED_FLIGHT,   // Motor queimando
    STATE_COASTING,         // Subindo por inércia (motor apagou)
    STATE_APOGEE,           // Ponto mais alto (acionar paraquedas)
    STATE_RECOVERY          // Caindo com paraquedas
} FlightState;

// ==========================================
//           STRUCT DOS SENSORES
// ==========================================
typedef struct {
    float accel_x, accel_y, accel_z; // Acelerômetro
    float pressure;                  // Barômetro
    float altitude;                  // Calculada a partir da pressão
    float temperature;               // Opcional, pensarei depois se será útil
} SensorData;

// Definir funções aqui
// init_sensors();
// read_sensors();
// start_engine();
// start_recovery();

// Variáveis Globais de Controle
FlightState current_state = STATE_STANDBY;

void main(void){
    printf("Iniciando aviônica...");
    init_sensors(); // Essa função tealvez também poderia retornar o status de todos os sensores
                    // E sinalizar um erro caso houvesse um
    
    // Preciso de um modo para armazenar as medidas anteriores para montar as condições corretas de mudança de estado.
    // Por enquanto vou utilizar simples structs.
    float memory_accel_z[3], memory_bar[3];
    int i = 0;
    while(1) { // Loop principal do controlador
        SensorData data = read_sensors(); // Struct recebe as leituras
                                          // Estou consierando que essa leitura ocorre a cada 0,1s
        memory_accel_z[i%3] = data.accel_z;
        switch (current_state){
        
            case STATE_STANDBY:
                int flag_powered_flight = 0;
                for(int j = 0; j < 3; j++){
                    if(memory_accel_z[j] > 2) flag_powered_flight =1;
                    else flag_powered_flight = 0;
                }
                if(flag_powered_flight == 1) current_state = STATE_POWERED_FLIGHT;
                break;
            
            case STATE_POWERED_FLIGHT:
                int flag_coasting = 0;
                for(int j = 0; j < 3; j++){
                    // Acho que não preciso me preocupar com medidas (ruídos) positivos aqui
                    // Caso eu precise, preciso trocar para um método de comparação de médias.
                    if(memory_accel_z[j] < 0) flag_coasting = 1;
                    else flag_coasting = 0;
                }
                if(flag_coasting == 1) current_state = STATE_COASTING;
                break;

            case STATE_COASTING:

                break;
        }
    }
}