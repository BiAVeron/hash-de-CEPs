# Análise de Performance de Tabela Hash em C

Projeto desenvolvido para a disciplina de **Estrutura de Dados**, com foco na implementação e análise de uma Tabela Hash com endereçamento aberto, suporte a redimensionamento dinâmico e diferentes estratégias de tratamento de colisão.

---

## Descrição do Projeto

Este projeto consiste na implementação de uma **biblioteca de Tabela Hash em linguagem C**. A estrutura foi projetada para ser flexível, suportando tanto **Sondagem Linear (Hash Simples)** quanto **Hash Duplo** como estratégias de tratamento de colisão.

O principal objetivo do trabalho foi realizar uma **análise de performance comparativa** entre essas duas estratégias, além de quantificar o custo (_overhead_) de utilizar uma estrutura de dados dinâmica que se redimensiona automaticamente com base em uma taxa de ocupação.  
Os dados utilizados para os testes foram um conjunto de **CEPs do Brasil**, obtidos através do **Kaggle**.

---

## Funcionalidades

- **Tabela Hash com Endereçamento Aberto**  
  Implementação de uma tabela hash que armazena os dados no próprio vetor.

- **Suporte a Duas Estratégias de Colisão**  
  O usuário pode escolher entre `HASH_SIMPLES` (sondagem linear) e `HASH_DUPLO` no momento da criação da tabela.

- **Redimensionamento Dinâmico**  
  A tabela dobra de tamanho automaticamente quando sua taxa de ocupação ultrapassa um limite pré-definido (70% por padrão), garantindo a eficiência das inserções futuras.

- **Busca por Chave Específica**  
  O sistema foi adaptado para buscar localidades utilizando os 5 primeiros dígitos de um CEP como chave.

- **Análise de Performance com `gprof`**  
  O código foi estruturado com funções específicas para permitir a medição de tempo e análise de performance utilizando o profiler `gprof`.

---

## Estrutura do Projeto

- `hash.c` – Arquivo principal contendo a implementação da biblioteca da Tabela Hash e as funções para os experimentos de performance.  
- `ceps.csv` – Arquivo de dados necessário para os testes. Deve estar na mesma pasta do executável.

---

## 🚀 Como Compilar e Executar

### Pré-requisitos

- Compilador `gcc`
- Ambiente compatível com `gprof` (Linux ou macOS)

### 🔧 Passos

1. **Coloque o arquivo `ceps.csv`** no mesmo diretório do código-fonte.

2. **Compile o programa com suporte a profiling**, utilizando a flag `-pg`:

   ```bash
   gcc -Wall -pg -o trab_hash hash.c
   ```

3. **Execute o programa** para gerar o arquivo `gmon.out`:

   ```bash
   ./trab_hash
   ```

4. **Gere o relatório de performance** com o `gprof`:

   ```bash
   gprof trab_hash gmon.out > analise.txt
   ```

   O arquivo `analise.txt` conterá os resultados detalhados da performance de cada função.
