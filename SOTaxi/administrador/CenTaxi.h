#pragma once
#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void(*ptr_register)(TCHAR*, int);
void(*ptr_log)(TCHAR*);

#define TAM 50
#define MAXTAXIS 10
#define MAXPASS 10
#define TempoManifestacoes 5
#define WAITTIMEOUT 1000
#define PATH TEXT("..\\mapa.txt")
#define PATH_DLL TEXT("..\\SO2_TP_DLL_32.dll")

int MaxPass = MAXPASS;
int MaxTaxi = MAXTAXIS;

char id_mapa_pass = 'A';

int tamanhoMapa = -1;

#define EVENT_TRANSPORTE TEXT("Transporte")
HANDLE transporte;

//1 INSTANCIA APENAS
#define SEMAPHORE_NAME TEXT("SEMAPHORE_ADMIN")
HANDLE Semaphore;

#define NOME_MUTEX_MAPA TEXT("MutexMapa")
typedef struct {
	char caracter;
} MAPA;

#define TAM_ID 10
#define PIPE_NAME TEXT("\\\\.\\pipe\\comunica")
HANDLE hPipe;

typedef struct {
	unsigned int X, Y, Xfinal, Yfinal;
	int movimento;
	int terminar;
	char id_mapa;
	TCHAR matriculaTaxi[7];
	int tempoEspera;
	TCHAR id[TAM_ID];
} PASSAGEIRO;

#define NOME_MUTEX_TAXI TEXT("MutexTaxi")
typedef struct {
	TCHAR matricula[7];
	unsigned int X, Y, Xfinal, Yfinal;
	int disponivel;
	int autoResposta;
	int interessado;
	int terminar;
	int id_mapa;
	float velocidade;
} TAXI;

HANDLE pipeT[50];
int numPipes = 0;

#define MAX_PASS 5
#define BUFFER_CIRCULAR TEXT("BufferCircular")
#define SEMAPHORE_MUTEX TEXT("SEM_MUTEX")
#define SEMAPHORE_ITENS TEXT("SEM_ITENS")
#define SEMAPHORE_VAZIOS TEXT("SEM_VAZIOS")
HANDLE sem_mutex, sem_itens, sem_vazios;

HANDLE hTimer;
int acabouTempo = 0;

typedef struct {
	PASSAGEIRO Passageiros[MAX_PASS];
	int NextIn = 0, NextOut = 0;
} BUFFER;

HANDLE hMemoria;
BUFFER* BufferMemoria;

typedef struct {
	TAXI taxis[MAXTAXIS];
	int nTaxis;
	PASSAGEIRO passageiros[MAXPASS];
	int nPassageiros;
} INFO;

#define SHM_INFO TEXT("EspacoInfo")
HANDLE EspInfo;	//FileMapping
INFO* sharedInfo = NULL;

//SEMAFOROS
#define EVENT_NOVOT TEXT("NovoTaxi")
#define EVENT_SAIUT TEXT("SaiuTaxi")
#define EVENT_MOVIMENTO TEXT("MovimentoTaxi")
#define EVENT_RESPOSTA TEXT("RespostaDoAdmin")
#define EVENT_INFOA TEXT("InfoAdmin")
#define EVENT_SAIUA TEXT("SaiuAdmin")


#define EVENT_NOVOP TEXT("NovoPass")
#define EVENT_RESPOSTAP TEXT("RespostaPass")
#define EVENT_MOVIMENTOP TEXT("MovimentoPass")

#define EVENT_ATUALIZAMAP TEXT("AtualizaMapa")

#define SHM_TAXI TEXT("EspacoTaxis")
#define SHM_MAPA TEXT("EspacoMapa")
#define SHM_MAPA_INICIAL TEXT("EspacoMapaInicial")

#define NOME_MUTEX_DADOS TEXT("MutexDados")
typedef struct {
	INFO *info;

	HANDLE EspTaxis;	//FileMapping
	HANDLE hMutexDados;
	TAXI* sharedTaxi;
	HANDLE novoTaxi;
	HANDLE saiuTaxi;
	HANDLE movimentoTaxi;
	HANDLE respostaAdmin;
	int aceitacaoT;
	int esperaManifestacoes;

	HANDLE novoPassageiro;
	HANDLE respostaPass;
	HANDLE respostaMov;

	MAPA* mapa;
	HANDLE hFile;
	HANDLE EspMapaAtual;	//FileMapping
	HANDLE EspMapa;	//FileMapping
	MAPA* sharedMapa = NULL;
	MAPA* sharedMapaInicial = NULL;
	HANDLE atualizaMap;

	int terminar;
	HANDLE infoAdmin;
	HANDLE saiuAdmin;
} DADOS;

void inicializaBuffer();
void ajuda();
void listarTaxis(DADOS* dados);
void listarPassageiros(DADOS* dados);
void verMapa(DADOS* dados);
void leMapa(DADOS* dados);
void novoP(DADOS* dados);
void transportePassageiro(DADOS* dados, int indice);
boolean adicionaTaxi(DADOS* dados, TAXI novo);
boolean removeTaxi(DADOS* dados, TAXI novo);
boolean adicionaPassageiro(DADOS* dados, PASSAGEIRO novo);
boolean removePassageiro(DADOS* dados, PASSAGEIRO novo);
void eliminaIdMapa(DADOS* dados, char id);
void expulsarTaxi(DADOS* dados, TCHAR* matr);
void transporteAceite(DADOS* dados);
void enviaTaxi(DADOS* dados, TAXI* taxi);
void deslocaPassageiroParaPorta(DADOS* dados);
int calculaDistancia(int inicioX, int inicioY, int fimX, int fimY);
DWORD WINAPI ThreadTempoTransporte(LPVOID param);
DWORD WINAPI ThreadComandos(LPVOID param);
DWORD WINAPI ThreadNovoTaxi(LPVOID param);
DWORD WINAPI ThreadSaiuTaxi(LPVOID param);
DWORD WINAPI ThreadMovimento(LPVOID param);
DWORD WINAPI ThreadNovoPassageiro(LPVOID param);