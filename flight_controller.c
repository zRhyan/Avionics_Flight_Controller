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
    
    while(1) { // Loop principal do controlador
        SensorData data = read_sensors(); // Struct recebe as leituras

        switch (current_state){
        case STATE_STANDBY:
            /* code */
            break;
        
        default:
            break;
        }
    }
}