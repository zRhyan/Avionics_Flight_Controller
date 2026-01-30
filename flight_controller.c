#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

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
    float memory_accel_z[3] = {0}, memory_bar[2] = {0}, last_delta_H = 0;
    int cycles_1 = 0, K = 1;
    float const_1, const_2, const_3, const_4; // Algum valor 
    while(1) { // Loop principal do controlador
        SensorData data = read_sensors(); // Struct recebe as leituras
                                          // Estou considerando que essa leitura ocorre a cada 0,1s
        memory_accel_z[cycles_1%3] = data.accel_z;
        memory_bar[1] = data.pressure;
        

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
                
                // 1. ATUALIZAÇÃO DO last_delta_H
                float delta_H_meas, delta_H_phy, delta_H_update, delta_H_min_1;

                // 1.1 Delta medido pelo barômetro
                delta_H_meas = fabs(memory_bar[1] - memory_bar[0]);

                // 1.2 Limite físico baseado na aceleração
                //    ΔH ≈ 0.5 * a * dt²  (dt = 0.01s)
                delta_H_phy = 0.5f * fabs(data.accel_z) * 0.0001f; // Ainda preciso aprimorar essa fórmulas

                // 1.3 Impor mínimo físico
                if(delta_H_phy < delta_H_min_1)
                    delta_H_phy = delta_H_min_1; // Verificar se será necessário um limite mínimo

                // 1.4 Atualização controlada
                if(delta_H_meas <= delta_H_phy)
                    delta_H_update = delta_H_meas;
                else
                    delta_H_update = delta_H_phy;

                // 1.5 Atualizar memória do barômetro
                memory_bar[0] = memory_bar[1];

                // 2. Lógica para mudar de estado
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
                
                // 1. Registro das alturas (pressões) com filtro
                // Adicionar lógica para obter o primeiro valor válido, memory_bar[0];
                float delta_H_adapt, delta_H_min_2, delta_H_phy, delta_H_limit;
                
                // 2. Filtro físico
                delta_H_min_2; // Algum valor 
                delta_H_phy = (const_4)*exp(-K/const_1); // Decaimento exponencial da velocidade
                if(delta_H_phy >= delta_H_min_2) delta_H_limit = delta_H_phy;
                else delta_H_limit = delta_H_min_2;

                // 3. Filtro adaptativo
                delta_H_adapt =  const_2*last_delta_H + const_3;

                // 4. Híbrido
                if(delta_H_phy > delta_H_adapt) delta_H_limit = delta_H_adapt;

                if(abs(memory_bar[0] - memory_bar[1]) <= delta_H_limit) {
                    // memory_bar[0] - valor validado
                    // memory_bar[1] - valor em análise
                    last_delta_H = abs(memory_bar[0] - memory_bar[1]);
                    memory_bar[0] = memory_bar[1];
                }
                else {
                    last_delta_H = delta_H_limit;
                    memory_bar[0] += delta_H_limit;
                }
                
                K++;
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