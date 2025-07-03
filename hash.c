#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#define SEED    0x12345678

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

// criei a segunda função da hash dupla
uint32_t hashf2(const char *str){
    uint32_t h = 0;
    for (; *str; ++str){
        h = (h * 33) ^ (unsigned char)(*str);
    }
    return (h % 97) + 1;
}

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

//alterei essa função tbm pra receber os dois tipos, assim como nas funções de inserir e remover
int hash_insere(thash * h, void * bucket){
    // verificar se precisa redimensionar
    float taxa_atual = (float)h->size / h->max;
    if (taxa_atual > h->taxa_max_ocupacao) {
        hash_redimensiona(h); // redimensiona a tabela
    }

    //tabela no tamanho correto, calculando os hashes
    const char * key = h->get_key(bucket);
    uint32_t hash = hashf(key, SEED); // Passando a SEED
    uint32_t hash2 = (h->tipo == HASH_DUPLO) ? hashf2(key) : 1;
    int i = 0;
    int pos;

    // A tabela nunca vai encher 100% por causa do redimensionamento
    do {
        pos = (hash + i * hash2) % h->max;
        if (h->table[pos] == 0 || h->table[pos] == h->deleted) {
            h->table[pos] = (uintptr_t) bucket;
            h->size++;
            return EXIT_SUCCESS;
        }
        i++;
    } while(i < h->max); // loop de segurança

    //algo deu errado (tabela cheia mesmo após redimensionar?)
    free(bucket);
    return EXIT_FAILURE; 
}

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
    h->taxa_max_ocupacao = 0.6; //vou deixar esse valor fixo
    return EXIT_SUCCESS;

}

void * hash_busca(thash h, const char * key){
    uint32_t hash = hashf(key,SEED);
    uint32_t hash2 = (h.tipo == HASH_DUPLO) ? hashf2(key) : 1;
    int i = 0;
    int pos;

    do{
        pos = (hash + i * hash2)%h.max;
        
        if(h.table[pos] == 0)
            return NULL;
        if (strcmp(h.get_key((void *)h.table[pos]),key) == 0)
            return (void *)h.table[pos];
        i++;
    }
    while(i<h.max);
        
    return NULL;
}

int hash_remove(thash * h, const char * key){
    uint32_t hash = hashf(key,SEED);
    uint32_t hash2 = (h.tipo == HASH_DUPLO) ? hashf2(key) : 1;
    int i = 0;
    int pos;

    do{
        pos = (hash + i * hash2)%h.max;

        if(h.table[pos] == 0)
            return EXIT_FAILURE;

        if (strcmp(h.get_key((void *)h.table[pos]),key) == 0){
            free((void *)h->table[pos]);
            h->table[pos] = h->deleted;
            h->size--;
            return EXIT_SUCCESS;
        }
        i++;
    }
    while(i<h.max);
        
    return EXIT_FAILURE;
}

void hash_apaga(thash *h){
    int pos;
    for(pos =0;pos< h->max;pos++){
        if (h->table[pos] != 0){
            if (h->table[pos]!=h->deleted){
                free((void *)h->table[pos]);
            }
        }
    }
    free(h->table);
}

//criei a função de redimensionamento
int hash_redimensiona(thash * h){
    //estamos dobrando a capacidade total
    int novo_max = h->max * 2; 
    thash nova_hash;
    
    // tipo da hash antiga para a nova
    hash_constroi(&nova_hash, novo_max, h->get_key, h->tipo); 
    nova_hash.taxa_max_ocupacao = h->taxa_max_ocupacao;

    for(int i = 0; i < h->max; i++){
        if (h->table[i] != 0 && h->table[i] != h->deleted){
            hash_insere(&nova_hash, (void *)h->table[i]);
        }
    }

    free(h->table); 
    *h = nova_hash; 

    return EXIT_SUCCESS;
}

//criei essa estrutura pro prefixo do cep e apaguei taluno
typedef struct {
    char cep[9];     
    char cidade[50];   
    char estado[3];    
} tcep;

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
    strcpy(registro->cep, cep);
    strcpy(registro->cidade, cidade);
    strcpy(registro->estado, estado);
    return registro;
}

void carrega_ceps(thash *h, const char *nome_arquivo, int max_linhas) {
    FILE *f = fopen(nome_arquivo, "r");
    if (!f) {
        perror("Erro ao abrir o arquivo de CEPs");
        exit(EXIT_FAILURE);
    }

    char linha[256];
    int count = 0;
    while (fgets(linha, sizeof(linha), f) && count < max_linhas) {
        char cep[9], cidade[50], estado[3];
        if (sscanf(linha, "%8[^,],%49[^,],%2s", cep, cidade, estado) == 3) {
            hash_insere(h, aloca_cep(cep, cidade, estado));
            count++;
        }
    }

    fclose(f);
}

void experimento_busca_vs_ocupacao(tipoHash tipo) {
    const int total_buckets = 6100;
    const int total_ceps = 6100;  // assumindo pelo menos 6100 linhas no CSV
    const float ocupacoes[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.99};
    const int n_ocupacoes = sizeof(ocupacoes) / sizeof(float);
    const int n_buscas = 1000;

    FILE *saida = fopen(tipo == HASH_SIMPLES ? "resultados_hash_simples.csv" : "resultados_hash_duplo.csv", "w");
    fprintf(saida, "taxa_ocupacao,tempo_ms\n");

    for (int i = 0; i < n_ocupacoes; i++) {
        thash h;
        hash_constroi(&h, total_buckets, get_cep_prefixo, tipo);
        h.taxa_max_ocupacao = 1.0; 

        FILE *f = fopen("ceps.csv", "r");
        if (!f) {
            perror("Erro ao abrir ceps.csv");
            exit(EXIT_FAILURE);
        }

        char linha[256];
        fgets(linha, sizeof(linha), f);
        int inseridos = 0;

        while (fgets(linha, sizeof(linha), f) && inseridos < ocupacoes[i] * total_buckets) {
            char cep[10], cidade[50], estado[3];
            if (sscanf(linha, "%9[^;];%49[^;];%2s", cep, cidade, estado) == 3) {
                hash_insere(&h, aloca_cep(cep, cidade, estado));
                inseridos++;
            }
        }
        fclose(f);

        // fazer buscas
        f = fopen("ceps.csv", "r");
        fgets(linha, sizeof(linha), f);
        char ceps_para_buscar[n_buscas][6];
        int buscados = 0;

        while (fgets(linha, sizeof(linha), f) && buscados < n_buscas) {
            char cep[10], cidade[50], estado[3];
            if (sscanf(linha, "%9[^;];%49[^;];%2s", cep, cidade, estado) == 3) {
                strncpy(ceps_para_buscar[buscados], cep, 5);
                ceps_para_buscar[buscados][5] = '\0';
                buscados++;
            }
        }
        fclose(f);

        clock_t inicio = clock();
        for (int j = 0; j < n_buscas; j++) {
            hash_busca(h, ceps_para_buscar[j]);
        }
        clock_t fim = clock();

        double tempo_ms = ((double)(fim - inicio)) / CLOCKS_PER_SEC * 1000.0;
        fprintf(saida, "%.0f,%.3f\n", ocupacoes[i] * 100, tempo_ms);

        hash_apaga(&h);
    }

    fclose(saida);
}

void experimento_overhead_insercao() {
    FILE *f = fopen("ceps.csv", "r");
    if (!f) {
        perror("Erro ao abrir ceps.csv");
        exit(EXIT_FAILURE);
    }

    char linha[256];
    fgets(linha, sizeof(linha), f); 

    // lendo todos os registros do CSV para memória
    #define MAX_REGS 7000
    tcep* registros[MAX_REGS];
    int n = 0;

    while (fgets(linha, sizeof(linha), f) && n < MAX_REGS) {
        char cep[10], cidade[50], estado[3];
        if (sscanf(linha, "%9[^;];%49[^;];%2s", cep, cidade, estado) == 3) {
            registros[n++] = aloca_cep(cep, cidade, estado);
        }
    }
    fclose(f);

    // experimentoA - hash sem redimensionamento
    thash hA;
    hash_constroi(&hA, 6100, get_cep_prefixo, HASH_SIMPLES);
    hA.taxa_max_ocupacao = 1.0;

    clock_t inicioA = clock();
    for (int i = 0; i < n; i++) {
        hash_insere(&hA, registros[i]);
    }
    clock_t fimA = clock();
    double tempoA = ((double)(fimA - inicioA)) / CLOCKS_PER_SEC * 1000.0;

    hash_apaga(&hA);

    // experimentoB - hash com redimensionamento
    thash hB;
    hash_constroi(&hB, 1000, get_cep_prefixo, HASH_SIMPLES);
    hB.taxa_max_ocupacao = 0.6;

    clock_t inicioB = clock();
    for (int i = 0; i < n; i++) {
        hash_insere(&hB, registros[i]);
    }
    clock_t fimB = clock();
    double tempoB = ((double)(fimB - inicioB)) / CLOCKS_PER_SEC * 1000.0;

    hash_apaga(&hB); //liberando

    printf("Tempo inserção sem redimensionamento (6100 buckets): %.3f ms\n", tempoA);
    printf("Tempo inserção com redimensionamento (1000 buckets): %.3f ms\n", tempoB);
}

void test_ceps(){
    thash h;
    hash_constroi(&h, 10, get_cep_prefixo, HASH_SIMPLES);
    h.taxa_max_ocupacao = 0.7;

    hash_insere(&h, aloca_cep("01310-200", "São Paulo", "SP"));
    hash_insere(&h, aloca_cep("70040-010", "Brasília", "DF"));
    hash_insere(&h, aloca_cep("30130-000", "Belo Horizonte", "MG"));

    tcep * r = hash_busca(h, "01310");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // São Paulo - SP

    r = hash_busca(h, "70040");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // Brasília - DF

    r = hash_busca(h, "30130");
    if (r) printf("%s - %s\n", r->cidade, r->estado);

    r = hash_busca(h, "99999");
    if (!r) printf("Prefixo não encontrado\n");

    hash_apaga(&h);
}

int main(int argc, char* argv[]){
    test_ceps();

    printf("Experimento: Tempo de Busca vs Taxa de Ocupacao\n");
    printf("\n--- HASH SIMPLES ---\n");
    experimento_busca_vs_ocupacao(HASH_SIMPLES);
    
    printf("\n--- HASH DUPLO ---\n");
    experimento_busca_vs_ocupacao(HASH_DUPLO);

    printf("\nExperimento: Overhead de Insercao Dinamica\n");
    experimento_overhead_insercao();

    return 0;
}