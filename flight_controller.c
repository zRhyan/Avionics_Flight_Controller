#include <stdio.h>
#include <stdbool.h>

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
    float temperature;               // Opcional
} SensorData;

// Variáveis Globais de Controle
FlightState current_state = STATE_STANDBY;

void main(void){
    printf("Iniciando aviônica...");
    init_sensors();
}