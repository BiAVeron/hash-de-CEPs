Análise de Performance de Tabela Hash em C
Projeto desenvolvido para a disciplina de Estrutura de Dados, com foco na implementação e análise de uma Tabela Hash com endereçamento aberto, suporte a redimensionamento dinâmico e diferentes estratégias de tratamento de colisão.

📜 Descrição do Projeto
Este projeto consiste na implementação de uma biblioteca de Tabela Hash em linguagem C. A estrutura foi projetada para ser flexível, suportando tanto Sondagem Linear (Hash Simples) quanto Hash Duplo como estratégias de tratamento de colisão.

O principal objetivo do trabalho foi realizar uma análise de performance comparativa entre essas duas estratégias, além de quantificar o custo (overhead) de utilizar uma estrutura de dados dinâmica que se redimensiona automaticamente com base em uma taxa de ocupação. Os dados utilizados para os testes foram um conjunto de CEPs do Brasil, obtidos através do Kaggle.

✨ Funcionalidades
Tabela Hash com Endereçamento Aberto: Implementação de uma tabela hash que armazena os dados no próprio vetor.

Suporte a Duas Estratégias de Colisão: O usuário pode escolher entre HASH_SIMPLES (sondagem linear) e HASH_DUPLO no momento da criação da tabela.

Redimensionamento Dinâmico: A tabela dobra de tamanho automaticamente quando sua taxa de ocupação ultrapassa um limite pré-definido (70% por padrão), garantindo a eficiência das inserções futuras.

Busca por Chave Específica: O sistema foi adaptado para buscar localidades utilizando os 5 primeiros dígitos de um CEP como chave.

Análise de Performance com gprof: O código foi estruturado com funções específicas para permitir a medição de tempo e a análise de performance com o profiler gprof.

📂 Estrutura do Projeto
hash.c: Arquivo principal contendo a implementação completa da biblioteca da Tabela Hash e as funções para os experimentos de performance.

ceps.csv: O arquivo de dados necessário para a execução dos testes. Deve ser colocado na mesma pasta do executável.

🚀 Como Compilar e Executar
Pré-requisitos
Compilador gcc

Ambiente compatível com gprof (padrão em sistemas Linux e macOS)

Passos
Dataset: Certifique-se de que o arquivo ceps.csv está no mesmo diretório do seu código-fonte.

Compilação para Profiling: Compile o programa utilizando a flag -pg, que é essencial para gerar as informações para o gprof.

Bash

gcc -Wall -pg -o trab_hash hash.c
Execução: Rode o programa para executar os testes. Isso irá gerar um arquivo gmon.out.

Bash

./trab_hash
Análise com gprof: Gere o relatório de análise a partir do gmon.out.

Bash

gprof meu_programa gmon.out > analise.txt
O arquivo relatorio.txt conterá os resultados detalhados da performance de cada função.
