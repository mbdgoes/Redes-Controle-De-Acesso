# Sistema de Controle de Acesso aos Prédios Universitários 
Trabalho Prático da disciplina de Redes de Computadores
  
## Descrição:
Este trabalho tem como objetivo implementar um sistema de controle de acesso aos prédios universitários utilizando catracas inteligentes simuladas, estabelecendo a comunicação entre interfaces de controle (ICs) e servidores através do uso de sockets. Para alcançar esse objetivo, foram desenvolvidos dois tipos de executáveis: um servidor, que pode atuar como servidor de usuários (SU) ou servidor de localização (SL), e um cliente que representa as interfaces de controle. O sistema implementa uma arquitetura onde os servidores se comunicam entre si através de uma conexão peer-to-peer, enquanto mantêm conexões simultâneas com múltiplos clientes.
  
Os servidores são responsáveis por diferentes aspectos do sistema: o servidor de usuários (SU) gerencia o cadastro e autenticação das pessoas, enquanto o servidor de localização (SL) mantém o registro da posição atual de cada indivíduo dentro do campus. A comunicação entre estes componentes acontece através de um protocolo próprio implementado sobre TCP, garantindo a confiabilidade na troca de mensagens. As interfaces de controle, por sua vez, atuam como pontos de acesso onde os funcionários podem realizar operações como cadastro de usuários, consulta de localização e controle de entrada e saída dos prédios.

O desenvolvimento foi realizado em linguagem C utilizando a biblioteca padrão POSIX para comunicação via sockets, com suporte tanto para IPv4 quanto IPv6. A implementação foca em criar uma base robusta para o sistema de controle de acesso. As seções seguintes detalharão a arquitetura do sistema, o protocolo de comunicação utilizado, e as decisões de implementação adotadas para atender aos requisitos especificados.

### Instruções de utilização
1. Compilar os executáveis:
```bash
 make
```
2. Iniciar os servidores (Em terminais diferentes):
```bash
 ./server 40000 50000 #Servidor de localização 
```
```bash
 ./server 40000 60000 #Servidor de usuário
```
A porta 40000 é usada para a comunicação P2P, 50000 para o servidor de usuários e 60000 para o servidor de localização

3. Iniciar o cliente:
```bash
 ./client [IP] 50000 60000 [LocId]
```
O cliente é inicializado passando o IP do servidor, as portas onde estão escutando e um Id de localização (valor entre 1 e 10)

### Comandos da entrada padrão

### Servidor

* **kill**: Requisita o fechamento da comunicação peer-2-peer e encerramento do servidor

### Cliente

* **kill**: Requisita o fechamento das conexões com os servidores e encerramento do cliente

* **add UID IS_SPECIAL**: Requisita a criação de usuário no servidor de usuários
    * UID: string de exatamente 10 caracteres
    * IS_SPECIAL: 1 ou 0 (usuário tem ou não tem permissão especial, respectivamente)

* **find UID**: Requisita a localização de um usuário no servidor de localização
    * UID: string de exatamente 10 caracteres

* **in UID**: Requisita a entrada de um usuário em um prédio
    * UID: string de exatamente 10 caracteres

* **out UID**: Requisita a saída de um usuário em um prédio
    * UID: string de exatamente 10 caracteres

* **inspect UID LOC**: Requisa a lista de pessoas que estão em um prédio, com autenticação
    * UID: string de exatamente 10 caracteres a ser usada para autenticação
    * LOC: número indicando o prédio desejado