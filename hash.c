#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#define SEED    0x12345678
#define ARQUIVO_CEP "ceps.csv" 
#define TOTAL_BUCKETS_EXP_BUSCA 6100
#define REPETICOES_PARA_MEDICAO 1000
#define NUM_BUSCAS_POR_EXPERIMENTO 1000
#define TOTAL_CEPS_A_CARREGAR 7000 

typedef enum { HASH_SIMPLES, HASH_DUPLO } tipoHash;

typedef struct {
    uintptr_t * table;
    int size;
    int max;
    uintptr_t deleted;
    char * (*get_key)(void *);
    tipoHash tipo; //vamos definir se vai ser simples ou duplo
    float taxa_max_ocupacao; //para definir o limite de ocupação
}thash;

//criei essa estrutura pro prefixo do cep e apaguei taluno
typedef struct {
    char cep[9];     
    char cidade[50];   
    char estado[3];    
} tcep;

//hash simples
uint32_t hashf(const char* str, uint32_t h){
    /* One-byte-at-a-time Murmur hash 
    Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

// criei a segunda função da hash dupla
uint32_t hashf2(const char *str){
    uint32_t h = 0;
    for (; *str; ++str){
        h = (h * 33) ^ (unsigned char)(*str);
    }
    return (h % 97) + 1;
}

int hash_insere(thash * h, void * bucket);

int hash_constroi(thash * h,int nbuckets, char * (*get_key)(void *), tipoHash tipo){
    h->table =calloc(sizeof(void *),nbuckets+1);
    if (h->table == NULL){
        return EXIT_FAILURE;
    }
    h->max = nbuckets+1;
    h->size = 0;
    h->deleted = (uintptr_t)&(h->size);
    h->get_key = get_key;
    h->tipo = tipo; //adicionamos pra aceitar o tipo da hash
    h->taxa_max_ocupacao = 0.7; //vou deixar esse valor fixo
    return EXIT_SUCCESS;

}

//criei a função de redimensionamento
int hash_redimensiona(thash * h){
    int novo_max = h->max * 2; 
    uintptr_t* tabela_antiga = h->table;
    int max_antigo = h->max;
    
    // tipo da hash antiga para a nova
    hash_constroi(h, novo_max, h->get_key, h->tipo); 

    for(int i = 0; i < max_antigo; i++){
        if (tabela_antiga[i] != 0 && tabela_antiga[i] != h->deleted){
            hash_insere(h, (void *)tabela_antiga[i]);
        }
    }

    free(tabela_antiga); 
    return EXIT_SUCCESS;
}

//alterei essa função tbm pra receber os dois tipos
int hash_insere(thash * h, void * bucket){
    // verificar se precisa redimensionar
    float taxa_atual = (float)h->size / h->max;
    if (taxa_atual > h->taxa_max_ocupacao) {
        hash_redimensiona(h); // redimensiona a tabela
    }

    //tabela no tamanho correto, calculando os hashes
    const char * key = h->get_key(bucket);
    uint32_t hash_val = hashf(key, SEED); // Passando a SEEd
    uint32_t step = 1;

    if (h->tipo == HASH_DUPLO){
        //vamos garantir que todos os slots sejam visitados
        step = (hashf2(key) % (h->max -1)) + 1;
    }

    for (int i = 0; i < h->max; i++) {
        int pos = (hash_val + i * step) % h->max;
        if (h->table[pos] == 0 || h->table[pos] == h->deleted) {
            h->table[pos] = (uintptr_t)bucket;
            h->size++;
            return EXIT_SUCCESS;
        }
    }
   
    //algo deu errado (tabela cheia)
    free(bucket);
    return EXIT_FAILURE; 
}


void * hash_busca(thash h, const char * key){
    uint32_t hash_val = hashf(key,SEED);
    uint32_t step = 1;

     if (h.tipo == HASH_DUPLO) {
        step = (hashf2(key) % (h.max - 1)) + 1;
    }
    
    for (int i = 0; i < h.max; i++) {
        int pos = (hash_val + i * step) % h.max;
        if (h.table[pos] == 0) {
            return NULL; //encontrou um espaço vazio -chave não existe
        }
        if (h.table[pos] != h.deleted) {
            if (strcmp(h.get_key((void *)h.table[pos]), key) == 0) {
                return (void *)h.table[pos];
            }
        }
    }
        
    return NULL;
}

void hash_apaga(thash *h){
    for(int i = 0; i < h->max; i++) {
        if (h->table[i] != 0 && h->table[i] != h->deleted) {
            free((void *)h->table[i]);
        }
    }
    free(h->table);
}

//usando apenas os 5 digitos do cep como chave
char * get_cep_prefixo(void * reg){
    static char prefixo[6]; // 5 + '\0'
    strncpy(prefixo, ((tcep *)reg)->cep, 5);
    prefixo[5] = '\0';
    return prefixo;
}

//funcao de alocação de item novo
void * aloca_cep(const char * cep, const char * cidade, const char * estado){
    tcep * registro = malloc(sizeof(tcep));
    if (registro){
        strcpy(registro->cep, cep);
        strcpy(registro->cidade, cidade);
        strcpy(registro->estado, estado);
    }
    return registro;
}

tcep** carrega_ceps_do_arquivo(const char *nome_arquivo, int* n_registros) {
    FILE *f = fopen(nome_arquivo, "r");
    
    if (!f) {
        perror("Erro ao abrir o arquivo de CEPs");
        exit(EXIT_FAILURE);
    }

    tcep** registros = malloc(sizeof(tcep*) * TOTAL_CEPS_A_CARREGAR);
    if (!registros) {
        printf("Erro ao alocar memoria para registros\n");
        exit(EXIT_FAILURE);
    }

    char linha[256];
    fgets(linha, sizeof(linha), f); // Ignora cabeçalho

    int count = 0;
    while (fgets(linha, sizeof(linha), f) && count < TOTAL_CEPS_A_CARREGAR) {
        char cep[9], cidade[50], estado[3];
        //formato csv
        if (sscanf(linha, "%8[^,],%49[^,],%2s", cep, cidade, estado) == 3) {
            registros[count] = aloca_cep(cep, cidade, estado);
            count++;
        }
    }

    fclose(f);
    *n_registros = count;
    return registros;
}

void libera_ceps_carregados(tcep** registros, int n_registros) {
    for (int i = 0; i < n_registros; i++) {
        free(registros[i]);
    }
    free(registros);
}

// parte 4.1 do trabalho: experimento busca vs ocupaçao
void run_busca_experimento(tipoHash tipo, float ocupacao, tcep** ceps_carregados, int total_ceps) {
    thash h;
    hash_constroi(&h, TOTAL_BUCKETS_EXP_BUSCA, get_cep_prefixo, tipo);
    h.taxa_max_ocupacao = 1.01; // desabilita redimensionamento para o teste

    int n_inserir = (int)(TOTAL_BUCKETS_EXP_BUSCA * ocupacao);
    for (int i = 0; i < n_inserir && i < total_ceps; i++) {
        // apenas apontando para a memoria já existente
        void* bucket_sem_alocar = ceps_carregados[i];
        
        // insere manual para evitar redimensionamento e free
        const char* key = h.get_key(bucket_sem_alocar);
        uint32_t hash_val = hashf(key, SEED);
        uint32_t step = (tipo == HASH_DUPLO) ? (hashf2(key) % (h.max - 1)) + 1 : 1;
        for (int j = 0; j < h.max; j++) {
            int pos = (hash_val + j * step) % h.max;
            if (h.table[pos] == 0) {
                h.table[pos] = (uintptr_t)bucket_sem_alocar;
                h.size++;
                break;
            }
        }
    }

    // faz as buscas
    for (int k = 0; k < REPETICOES_PARA_MEDICAO; k++) {
        for (int i = 0; i < NUM_BUSCAS_POR_EXPERIMENTO && i < n_inserir; i++) {
            hash_busca(h, get_cep_prefixo(ceps_carregados[i]));
        }
    }
    free(h.table);
}

// taxa de ocupação (HASH SIMPLES)
void busca_10_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.10, ceps, n); }
void busca_20_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.20, ceps, n); }
void busca_30_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.30, ceps, n); }
void busca_40_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.40, ceps, n); }
void busca_50_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.50, ceps, n); }
void busca_60_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.60, ceps, n); }
void busca_70_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.70, ceps, n); }
void busca_80_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.80, ceps, n); }
void busca_90_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.90, ceps, n); }
void busca_99_simples(tcep** ceps, int n) { run_busca_experimento(HASH_SIMPLES, 0.99, ceps, n); }

//taxa de ocupação (HASH DUPLA)
void busca_10_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.10, ceps, n); }
void busca_20_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.20, ceps, n); }
void busca_30_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.30, ceps, n); }
void busca_40_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.40, ceps, n); }
void busca_50_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.50, ceps, n); }
void busca_60_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.60, ceps, n); }
void busca_70_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.70, ceps, n); }
void busca_80_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.80, ceps, n); }
void busca_90_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.90, ceps, n); }
void busca_99_duplo(tcep** ceps, int n) { run_busca_experimento(HASH_DUPLO, 0.99, ceps, n); }


//parte2 do trabalho: Overhead de inserção
void insere_6100_inicial() {
    for (int i = 0; i < REPETICOES_PARA_MEDICAO; i++) {
        thash h;
        hash_constroi(&h, 6100, get_cep_prefixo, HASH_SIMPLES);
        h.taxa_max_ocupacao = 1.01;

        int n_registros = 0;
        tcep** registros = carrega_ceps_do_arquivo(ARQUIVO_CEP, &n_registros);

        for (int i = 0; i < n_registros; i++) {
            //realocar para a hash poder tomar posse da memória
            hash_insere(&h, aloca_cep(registros[i]->cep, registros[i]->cidade, registros[i]->estado));
        }

        libera_ceps_carregados(registros, n_registros);
        hash_apaga(&h);
    }
}

//insere todos os CEPs em uma tabela pequena + redimensionamento
void insere_1000_inicial() {
    for (int i = 0; i < REPETICOES_PARA_MEDICAO; i++) {
        thash h;
        hash_constroi(&h, 1000, get_cep_prefixo, HASH_SIMPLES);
        h.taxa_max_ocupacao = 0.7; //ativa o redimensionamento

        int n_registros = 0;
        tcep** registros = carrega_ceps_do_arquivo(ARQUIVO_CEP, &n_registros);

        for (int i = 0; i < n_registros; i++) {
            hash_insere(&h, aloca_cep(registros[i]->cep, registros[i]->cidade, registros[i]->estado));
        }
        
        libera_ceps_carregados(registros, n_registros);
        hash_apaga(&h);
    }
}


void test_ceps(){
    thash h;
    hash_constroi(&h, 10, get_cep_prefixo, HASH_SIMPLES);
    h.taxa_max_ocupacao = 0.7;

    hash_insere(&h, aloca_cep("01310-200", "Sao Paulo", "SP"));
    hash_insere(&h, aloca_cep("70040-010", "Brasilia", "DF"));
    hash_insere(&h, aloca_cep("30130-000", "Belo Horizonte", "MG"));
    hash_insere(&h, aloca_cep("40026-010", "Salvador", "BA"));
    hash_insere(&h, aloca_cep("69005-040", "Manaus", "AM"));
    hash_insere(&h, aloca_cep("80020-320", "Curitiba", "PR"));
    printf("Tabela antes do redimensionamento: size=%d, max=%d\n", h.size, h.max);
    hash_insere(&h, aloca_cep("90010-110", "Porto Alegre", "RS"));
    printf("Tabela apos o redimensionamento: size=%d, max=%d\n", h.size, h.max);
    hash_insere(&h, aloca_cep("50030-000", "Recife", "PE"));

    tcep * r = hash_busca(h, "01310");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // São Paulo - SP

    r = hash_busca(h, "99999");
    if (!r) printf("Prefixo nao encontrado\n");

    printf("\n");
    hash_apaga(&h);
}

int main(int argc, char* argv[]){
    test_ceps();

    //carregar todos os ceps
    printf("Carregando base de dados de CEPs na memoria...\n");
    int n_ceps;
    tcep** ceps_para_experimento = carrega_ceps_do_arquivo(ARQUIVO_CEP, &n_ceps);
    printf("%d registros de CEPs carregados.\n\n", n_ceps);
    
    //experimento de busca e ocupação
    printf("Executando Experimento 4.1: Tempo de Busca vs. Ocupacao...\n");
    // Hash Simples
    busca_10_simples(ceps_para_experimento, n_ceps);
    busca_20_simples(ceps_para_experimento, n_ceps);
    busca_30_simples(ceps_para_experimento, n_ceps);
    busca_40_simples(ceps_para_experimento, n_ceps);
    busca_50_simples(ceps_para_experimento, n_ceps);
    busca_60_simples(ceps_para_experimento, n_ceps);
    busca_70_simples(ceps_para_experimento, n_ceps);
    busca_80_simples(ceps_para_experimento, n_ceps);
    busca_90_simples(ceps_para_experimento, n_ceps);
    busca_99_simples(ceps_para_experimento, n_ceps);
    // Hash Dupla
    busca_10_duplo(ceps_para_experimento, n_ceps);
    busca_20_duplo(ceps_para_experimento, n_ceps);
    busca_30_duplo(ceps_para_experimento, n_ceps);
    busca_40_duplo(ceps_para_experimento, n_ceps);
    busca_50_duplo(ceps_para_experimento, n_ceps);
    busca_60_duplo(ceps_para_experimento, n_ceps);
    busca_70_duplo(ceps_para_experimento, n_ceps);
    busca_80_duplo(ceps_para_experimento, n_ceps);
    busca_90_duplo(ceps_para_experimento, n_ceps);
    busca_99_duplo(ceps_para_experimento, n_ceps);
    printf("Experimento 4.1 concluido.\n\n");

    //libera a memória usada pelo experimento de busca
    libera_ceps_carregados(ceps_para_experimento, n_ceps);
    
    // experimentot Overhead de Inserção
    // gprof irá medir o tempo gasto em cada uma destas funções
    printf("Executando Experimento 4.2: Overhead de Insercao Dinamica...\n");
    insere_6100_inicial(); // Inserção sem redimensionamento
    insere_1000_inicial(); // Inserção com redimensionamento
    printf("Experimento 4.2 concluido.\n\n");

    printf("Todos os experimentos foram concluidos.\n");

    return 0;
}