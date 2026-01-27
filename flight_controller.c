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
// max_height();

// Variáveis Globais de Controle
FlightState current_state = STATE_STANDBY;

void main(void){
    printf("Iniciando aviônica...");
    init_sensors(); // Essa função tealvez também poderia retornar o status de todos os sensores
                    // E sinalizar um erro caso houvesse um
    
    // Preciso de um modo para armazenar as medidas anteriores para montar as condições corretas de mudança de estado.
    // Por enquanto vou utilizar simples structs.
    float memory_accel_z[3] = {0}, memory_bar[6] = {0}, average_pressure[2] = {0};
    int cycles_1 = 0, cycles_2 = 0;
    while(1) { // Loop principal do controlador
        SensorData data = read_sensors(); // Struct recebe as leituras
                                          // Estou consierando que essa leitura ocorre a cada 0,1s
        memory_accel_z[cycles_1%3] = data.accel_z;
        
        if(cycles_2 < 3) memory_bar[cycles_2%3] = data.pressure; // Apenas [0], [1] e [2]
        else memory_bar[cycles_2%3 + 3] = data.pressure;         // Apenas [3], [4] e [5]
        
        if(current_state == STATE_COASTING){
            if(cycles_2 == 0) {
                // average_pressure[0] sempre será a média mais antiga
                average_pressure[0] = (memory_bar[0] + memory_bar[1] + memory_bar[2])/3;
                average_pressure[1] = (memory_bar[3] + memory_bar[4] + memory_bar[5])/3;
            }
            else average_pressure[1] = (memory_bar[3] + memory_bar[4] + memory_bar[5])/3;
        }
        switch (current_state){
        
            case STATE_STANDBY:
                int flag_powered_flight = 0;
                for(int j = 0; j < 3; j++){
                    if(memory_accel_z[j] > 2) flag_powered_flight++;
                    else flag_powered_flight = 0;
                }
                if(flag_powered_flight == 3) current_state = STATE_POWERED_FLIGHT;
                break;
            
            case STATE_POWERED_FLIGHT:
                int flag_coasting = 0;
                for(int j = 0; j < 3; j++){
                    // Acho que não preciso me preocupar com medidas (ruídos) positivas aqui
                    // Caso eu precise, preciso trocar para um método de comparação de médias.
                    if(memory_accel_z[j] < 0) flag_coasting++;
                    else flag_coasting = 0;
                }
                if(flag_coasting == 3) current_state = STATE_COASTING;
                break;

            case STATE_COASTING:
                // O sinal para passar para o próximo estado (apogeu), é accel_z = 0 e a menor pressão.
                // O problema é que essas duas medidas são sensíveis e, portanto, são facilmente interferidas por ruídos.
                int flag_null_accel = 0;
                if(data.accel_z > -0.1 && data.accel_z < 0.1) flag_null_accel = 1;
                if(average_pressure[0] < average_pressure[1] && flag_null_accel == 1) {
                    current_state = STATE_APOGEE;
                    break; // Já sai do case e armazena as 6 últimas medidas de pressão para estimar a altura máxima
                }
                // Passar os memory_bar[3], [4] e [5], respectivamente para [0], [1] e [2], para manter average_pressure[0] como a mais antiga
                for(int j = 0; j < 3; j++) {memory_bar[j] = memory_bar[j+3];}
                cycles_2++;
                break;
            
            case STATE_APOGEE:
                max_height();
                current_state = STATE_RECOVERY;
                break;

            case STATE_RECOVERY:
                start_recovery();
        }
        cycles_1++;
    }
}