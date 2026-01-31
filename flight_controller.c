#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

// ==========================================
//           CONSTANTES E CONFIGURAÇÕES
// ==========================================
#define DT 0.02f // Intervalo de tempo entre leituras (20ms)

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

// ==========================================
//           PROTÓTIPOS E AUXILIARES
// ==========================================
// Função simples para converter Pressão em Altitude (Modelo Padrão)
// h = 44330 * (1 - (P / P0)^(1/5.255))
float pressure_to_altitude(float pressure) {
    float P0 = 101325.0f; // Pressão ao nível do mar (pode ser calibrada no start)
    return 44330.0f * (1.0f - powf(pressure / P0, 0.1903f));
}

// Stubs para compilação
void init_sensors() {}
SensorData read_sensors() { SensorData d = {0}; return d; } // Stub
void max_height() {}
void start_recovery() {}

// Variáveis Globais de Controle
FlightState current_state = STATE_STANDBY;

void main(void){
    printf("Iniciando aviônica...");
    init_sensors(); 
    
    // Variáveis de Memória e Controle
    // memory_alt[0] = Altitude Anterior (Validada)
    // memory_alt[1] = Altitude Atual (Lida do sensor/Estimada)
    float memory_accel_z[3] = {0}, memory_alt[2] = {0}, last_delta_H = 0, signed_delta_H = 0;
    
    // Variáveis de Física
    float current_velocity = 0.0f; // Velocidade vertical estimada
    float initial_coasting_velocity = 0.0f; // Velocidade no momento do corte do motor (antiga const_4)
    
    int cycles_1 = 0, K = 1;
    
    // Constantes do Filtro (Placeholder values)
    float const_1 = 5.0f;  // Tau (constante de tempo do arrasto)
    float const_2 = 0.8f;  // Peso do histórico no filtro adaptativo
    float const_3 = 0.2f;  // Bias/Offset do filtro adaptativo
    float delta_H_min_1 = 0.5f; // Mínimo deslocamento físico considerado no voo motorizado
    float delta_H_min_2 = 0.1f; // Mínimo deslocamento físico no coasting

    // Flags
    int count_descent = 0;

    while(1) { // Loop principal do controlador
        SensorData data = read_sensors(); 
        
        // Conversão Imediata: Trabalhamos apenas com METROS agora
        data.altitude = pressure_to_altitude(data.pressure);

        memory_accel_z[cycles_1%3] = data.accel_z;
        memory_alt[1] = data.altitude; 

        switch (current_state){
        
            case STATE_STANDBY:
                // Inicializa a memória de altitude com o valor atual para evitar saltos no início
                memory_alt[0] = memory_alt[1]; 
                current_velocity = 0.0f; // Garante velocidade zero na rampa

                int flag_powered_flight = 0;
                for(int j = 0; j < 3; j++){
                    // Verifica se há aceleração sustentada > 2g
                    if(memory_accel_z[j] > 2.0f) flag_powered_flight++;
                    else flag_powered_flight = 0;
                }
                if(flag_powered_flight == 3) current_state = STATE_POWERED_FLIGHT;
                break;
            
            case STATE_POWERED_FLIGHT:
                
                // 0. ATUALIZAÇÃO DA VELOCIDADE (Integração de Euler)
                // v = v0 + a*t
                current_velocity += data.accel_z * DT;

                // 1. ATUALIZAÇÃO DO last_delta_H
                float delta_H_meas, delta_H_phy;

                // 1.1 Delta medido pelo barômetro (Agora em Metros)
                // Como está subindo, Alt[1] > Alt[0], então o delta é positivo
                delta_H_meas = fabs(memory_alt[1] - memory_alt[0]);

                // 1.2 Limite físico baseado na Cinemática Completa
                // ΔH ≈ v*dt + 0.5*a*dt²
                // Usamos fabs para garantir magnitude positiva do passo máximo
                delta_H_phy = fabs((current_velocity * DT) + (0.5f * data.accel_z * DT * DT));

                // 1.3 Impor mínimo físico (Para evitar travar se velocidade momentaneamente for 0)
                if(delta_H_phy < delta_H_min_1)
                    delta_H_phy = delta_H_min_1;

                // 1.4 Atualização controlada (Gating)
                if(delta_H_meas <= delta_H_phy)
                    last_delta_H = delta_H_meas;
                else
                    last_delta_H = delta_H_phy; // Clamp: limita ao máximo físico possível

                // 1.5 Atualizar memória de altitude (validada)
                // Se o medido foi aceito, usa o medido. Se foi rejeitado, projeta o físico.
                if (delta_H_meas <= delta_H_phy)
                     memory_alt[0] = memory_alt[1];
                else 
                     memory_alt[0] += delta_H_phy; // Reconstrói a trajetória suave

                // 2. Lógica para mudar de estado (Detecção de Burnout)
                int flag_coasting = 0;
                for(int j = 0; j < 3; j++){
                    // Aceleração negativa indica arrasto (motor apagou)
                    if(memory_accel_z[j] < 0.0f) flag_coasting++;
                    else flag_coasting = 0;
                }
                
                if(flag_coasting == 3) {
                    current_state = STATE_COASTING;
                    // Captura a velocidade final do motor para usar no decaimento do coasting
                    initial_coasting_velocity = current_velocity;
                }
                break;

            case STATE_COASTING:
                
                // 1. Definição das variáveis locais
                float delta_H_adapt, delta_H_limit;
                
                // 2. Filtro físico (Decaimento Exponencial)
                // Usa a velocidade capturada na transição (initial_coasting_velocity)
                // ΔH_phy = V_inicial * exp(-t/tau) * dt
                // t = K * DT
                float velocity_decay = initial_coasting_velocity * exp(-(K * DT) / const_1);
                delta_H_phy = velocity_decay * DT; 

                // Garante um mínimo para não zerar totalmente
                if(delta_H_phy < delta_H_min_2) delta_H_limit = delta_H_min_2;
                else delta_H_limit = delta_H_phy;

                // 3. Filtro adaptativo (Baseado na última variação válida)
                delta_H_adapt = const_2 * last_delta_H + const_3;

                // 4. Híbrido: sistema se adpta até onde a física permite
                if(delta_H_phy > delta_H_adapt) delta_H_limit = delta_H_adapt;

                // 5. Validação da Leitura (Gating)
                // memory_alt[1] é o dado novo (sujo), memory_alt[0] é o validado
                if(fabs(memory_alt[1] - memory_alt[0]) <= delta_H_limit) {
                    // Dado Válido: Aceita a leitura do sensor
                    last_delta_H = fabs(memory_alt[1] - memory_alt[0]);
                    signed_delta_H = memory_alt[1] - memory_alt[0];
                    memory_alt[0] = memory_alt[1];
                }
                else {
                    // Dado Inválido (Ruído): Projeta a altitude usando o limite calculado
                    // Como estamos subindo, somamos o limite à altitude anterior
                    last_delta_H = delta_H_limit;
                    signed_delta_H = delta_H_limit;
                    memory_alt[0] += delta_H_limit; 
                }
                
                // Atualização da velocidade estimada para o próximo ciclo 
                current_velocity -= 9.81f * DT;

                // Detecção do apogeu
                if(signed_delta_H < 0) count_descent++;
                if(count_descent >= 2) current_state = STATE_APOGEE;

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