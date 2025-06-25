#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define SEED    0x12345678

typedef struct {
    uintptr_t * table;
    int size;
    int max;
    uintptr_t deleted;
    char * (*get_key)(void *);
    tipoHash tipo; //vamos definir se vai ser simples ou duplo
    float limite_ocupação //para definir o limite de ocupação
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
    uint32_t hash = hashf(h->get_key(bucket));
    uint32_t hash2 = (h->tipo == HASH_DUPLO) ? hashf2(key) : 1;
    int i = 0;
    int pos;

    if (h->max == (h->size+1)){
        free(bucket);
        return EXIT_FAILURE; /* hash full*/
    }

    //vamos verificar a taxa de ocupação antes de inserir
    float taxa = (float)(h->size + 1) / (h->max - 1);
    if (taxa > h->ocupacao_max) {
        hash_redimensiona(h);
    }

    do{
        pos = (hash + i * hash2) % h->max;
        if (h->table[pos] == 0 || h->table[pos] == h->deleted){
            h->table[pos] = (uintptr_t) bucket;
            h->size++;
            return EXIT_SUCCESS;
        }
        i++;
    } while(i<h->max);

    free(bucket);
    return EXIT_SUCCESS;
}



int hash_constroi(thash * h,int nbuckets, char * (*get_key)(void *) ){
    h->table =calloc(sizeof(void *),nbuckets+1);
    if (h->table == NULL){
        return EXIT_FAILURE;
    }
    h->max = nbuckets+1;
    h->size = 0;
    h->deleted = (uintptr_t)&(h->size);
    h->get_key = get_key;
    h->tipo = tipo; //adicionamos pra aceitar o tipo da hash
    h->limite_ocupacao = 0.6; //vou deixar esse valor fixo
    return EXIT_SUCCESS;

}


void * hash_busca(thash h, const char * key){
    uint32_t hash = hashf(key,SEED);
    uint32_t hash2 = (h.tipo == HASH_DUPLO) ? hashf2(key) : 1;
    int i = 0;
    int pos;

    do{
        pos = (hash + i * hash2)%h.max;
        
        if(h.table[pos] = 0)
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

        if(h.table[pos] = 0)
            return NULL;
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
    int novo_max = (h->max - 1) * 2;
    thash nova;
    hash_constroi(&nova, novo_max, h->get_key, h->tipo);
    nova.ocupacao_max = h->ocupacao_max;

    for(int i = 0; i < h->max; i++){
        if (h->table[i] != 0 && h->table[i] != h->deleted){
            void * item = (void *)h->table[i];
            hash_insere(&nova, item);
        }
    }

    //para liberar a tabela antigaa
    free(h->table);

    //h recebe a nova estrutura
    *h = nova;

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

void test_ceps(){
    thash h;
    hash_constroi(&h, 10, get_cep_prefixo, HASH_SIMPLES);
    h.ocupacao_max = 0.7;

    hash_insere(&h, aloca_cep("01310-200", "São Paulo", "SP"));
    hash_insere(&h, aloca_cep("70040-010", "Brasília", "DF"));
    hash_insere(&h, aloca_cep("30130-000", "Belo Horizonte", "MG"));

    tcep * r = hash_busca(h, "01310");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // São Paulo - SP

    r = hash_busca(h, "70040");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // Brasília - DF

    r = hash_busca(h, "30130");
    if (r) printf("%s - %s\n", r->cidade, r->estado);void test_ceps(){
    thash h;
    hash_constroi(&h, 10, get_cep_prefixo, HASH_SIMPLES);
    h.ocupacao_max = 0.7;

    hash_insere(&h, aloca_cep("01310-200", "São Paulo", "SP"));
    hash_insere(&h, aloca_cep("70040-010", "Brasília", "DF"));
    hash_insere(&h, aloca_cep("30130-000", "Belo Horizonte", "MG"));

    tcep * r = hash_busca(h, "01310");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // sao paulo sp

    r = hash_busca(h, "70040");
    if (r) printf("%s - %s\n", r->cidade, r->estado); // brasilia df

    r = hash_busca(h, "30130");
    if (r) printf("%s - %s\n", r->cidade, r->estado); //belo horizonte mg

    r = hash_busca(h, "99999");
    if (!r) printf("Prefixo não encontrado\n");

    hash_apaga(&h);
}


    r = hash_busca(h, "99999");
    if (!r) printf("Prefixo não encontrado\n");

    hash_apaga(&h);
}


void test_hash(){
    thash h;
    int nbuckets = 10;
    hash_constroi(&h,nbuckets,get_key);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_FAILURE);
    hash_apaga(&h);
}
void test_search(){
    thash h;
    int nbuckets = 10;
    taluno * aluno;
    hash_constroi(&h,nbuckets,get_key);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("takashi","1123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("matsubara","2123456789"))==EXIT_SUCCESS);

    aluno = hash_busca(h,"edson");
    assert(aluno->cpf[0]=='0');
    aluno = hash_busca(h,"takashi");
    assert(aluno->cpf[0]=='1');
    aluno = hash_busca(h,"matsubara");
    assert(aluno->cpf[0]=='2');
    aluno = hash_busca(h,"patricia");
    assert(aluno == NULL);

    hash_apaga(&h);
}

void test_remove(){
    thash h;
    int nbuckets = 10;
    taluno * aluno;
    hash_constroi(&h,nbuckets,get_key);
    assert(hash_insere(&h,aloca_aluno("edson","0123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("takashi","1123456789"))==EXIT_SUCCESS);
    assert(hash_insere(&h,aloca_aluno("matsubara","2123456789"))==EXIT_SUCCESS);

    aluno = hash_busca(h,"edson");
    assert(aluno->cpf[0]=='0');
    aluno = hash_busca(h,"takashi");
    assert(aluno->cpf[0]=='1');
    aluno = hash_busca(h,"matsubara");
    assert(aluno->cpf[0]=='2');
    aluno = hash_busca(h,"patricia");
    assert(aluno == NULL);

    assert(h.size == 3);
    assert(hash_remove(&h,"edson")==EXIT_SUCCESS);
    aluno = hash_busca(h,"edson");
    assert(aluno == NULL);
    assert(h.size == 2);

    assert(hash_remove(&h,"edson")==EXIT_FAILURE);

    aluno = hash_busca(h,"matsubara");
    assert(aluno->cpf[0]=='2');


    hash_apaga(&h);

}


int main(int argc, char* argv[]){
    test_hash();
    test_search();
    test_remove();
    test_ceps();
    return 0;
}