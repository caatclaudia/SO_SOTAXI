#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#define TAM 200
#define MAXTAXIS 10
#define MAXPASS 10
#define TempoManifestacoes 5
#define WAITTIMEOUT 2000

#define SHM_NAME TEXT("EspacoTaxis")
#define NOME_MUTEX TEXT("MutexTaxi")
#define EVENT_NOVOT TEXT("NovoTaxi")
#define EVENT_RESPOSTA TEXT("RespostaDoAdmin")

//CenTaxi
//1 instancia
//le o mapa e gere-o
//sabe dos taxis, posicoes e estado

typedef struct {
	TCHAR id[TAM];
	unsigned int X, Y, Xfinal, Yfinal;
	int movimento;
	int terminar;
} PASSAGEIRO;

typedef struct {
	TCHAR matricula[6];
	unsigned int X, Y, Xfinal, Yfinal;
	int disponivel;
	TCHAR idPassageiro[TAM];
	float velocidade;
	int autoResposta;
	int interessado;
	int terminar;
	HANDLE hMutex;
} TAXI;


typedef struct {
	int nTaxis;
	TAXI taxis[MAXTAXIS];
	int nPassageiros;
	PASSAGEIRO passageiros[MAXPASS];
	int respostaAuto;
	int esperaManifestacoes;
	int terminar;
	HANDLE EspTaxis;	//FileMapping
	HANDLE hMutexDados;
	TAXI* shared;
	HANDLE novoTaxi;
} DADOS;


void ajuda();
void listarTaxis(DADOS* dados);
void listarPassageiros(DADOS* dados);
boolean adicionaTaxi(DADOS* dados, TAXI novo);
boolean removeTaxi(DADOS* dados, TAXI novo);
boolean adicionaPassageiro(DADOS* dados, PASSAGEIRO novo);
boolean removePassageiro(DADOS* dados, PASSAGEIRO novo);
DWORD WINAPI ThreadComandos(LPVOID param);
DWORD WINAPI ThreadNovoTaxi(LPVOID param);
DWORD WINAPI ThreadNovoPassageiro(LPVOID param);


int _tmain(int argc, LPTSTR argv[]) {
	HANDLE hThreadComandos, hThreadNovoTaxi, hThreadNovoPassageiro;
	DADOS dados;
	dados.nTaxis = 0;
	dados.nPassageiros = 0;
	dados.terminar = 0;
	dados.respostaAuto = 1;
	dados.esperaManifestacoes = TempoManifestacoes;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	dados.hMutexDados = CreateMutex(NULL, FALSE, TEXT("MutexDados"));
	if (dados.hMutexDados == NULL) {
		_tprintf(TEXT("\nErro ao criar Mutex!\n"));
		return 0;
	}
	WaitForSingleObject(dados.hMutexDados, INFINITE);
	ReleaseMutex(dados.hMutexDados);

	dados.novoTaxi = CreateEvent(NULL, TRUE, FALSE, EVENT_NOVOT);
	if (dados.novoTaxi == NULL) {
		_tprintf(TEXT("CreateEvent failed.\n"));
		return 0;
	}
	SetEvent(dados.novoTaxi);
	ResetEvent(dados.novoTaxi);

	dados.EspTaxis = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TAXI), SHM_NAME);
	if (dados.EspTaxis == NULL)
	{
		_tprintf(TEXT("CreateFileMapping failed.\n"));
		return 0;
	}

	dados.shared = (TAXI*)MapViewOfFile(dados.EspTaxis, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TAXI));
	if (dados.shared == NULL)
	{
		_tprintf(TEXT("Terminal failure: MapViewOfFile.\n"));
		CloseHandle(dados.EspTaxis);
		return 0;
	}

	SetEvent(dados.novoTaxi);
	Sleep(500);
	ResetEvent(dados.novoTaxi);

	hThreadComandos = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadComandos, (LPVOID)&dados, 0, NULL); //CREATE_SUSPENDED para nao comecar logo
	if (hThreadComandos == NULL) {
		_tprintf(TEXT("\nErro ao lan�ar Thread!\n"));
		return 0;
	}
	hThreadNovoTaxi = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadNovoTaxi, (LPVOID)&dados, 0, NULL); //CREATE_SUSPENDED para nao comecar logo
	if (hThreadNovoTaxi == NULL) {
		_tprintf(TEXT("\nErro ao lan�ar Thread!\n"));
		return 0;
	}
	//hThreadNovoPassageiro = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadNovoPassageiro, (LPVOID)&dados, 0, NULL); //CREATE_SUSPENDED para nao comecar logo
	//if (hThreadNovoPassageiro == NULL) {
	//	_tprintf(TEXT("\nErro ao lan�ar Thread!\n"));
	//	return 0;
	//}

	HANDLE ghEvents[2];
	ghEvents[0] = hThreadComandos;
	ghEvents[1] = hThreadNovoTaxi;
	//ghEvents[2] = hThreadNovoPassageiro;
	DWORD dwResultEspera;
	do {
		dwResultEspera = WaitForMultipleObjects(2, ghEvents, TRUE, WAITTIMEOUT);
		if (dwResultEspera == WAITTIMEOUT) {
			dados.terminar = 1;
			_tprintf(TEXT("As Threads vao parar!\n"));
			break;
		}
	} while (1);

	_tprintf(_T("\nAdministrador vai encerrar!"));
	_gettch();

	UnmapViewOfFile(dados.shared);
	CloseHandle(dados.EspTaxis);
	CloseHandle(dados.novoTaxi);
	return 0;
}

void ajuda() {
	_tprintf(_T("\n\n expulsar - EXPULSAR TAXI"));
	_tprintf(_T("\n listar - LISTAR T�XIS"));
	_tprintf(_T("\n aceita��oT - PAUSAR/RECOMECAR ACEITA��O DE TAXIS"));
	_tprintf(_T("\n manifesta��es - DEFINIR INTERVALO DE TEMPO DURANTE O QUAL AGUARDA MANIFESTA�OES DOS TAXIS"));
	_tprintf(_T("\n fim - ENCERRAR TODO O SISTEMA"));
	return;
}

void listarTaxis(DADOS* dados) {
	for (int i = 0; i < dados->nTaxis; i++) {
		_tprintf(_T("\nTaxi %d : "), i);
		_tprintf(_T("\n (%d, %d) "), dados->taxis[i].X, dados->taxis[i].Y);
		if (dados->taxis[i].disponivel)
			_tprintf(_T("sem passageiro!\n"));
		else
			_tprintf(_T("com passageiro!\n"));
	}
	return;
}

void listarPassageiros(DADOS* dados) {
	for (int i = 0; i < dados->nPassageiros; i++) {
		_tprintf(_T("\nPassageiro %d : "), i);
		_tprintf(_T("\n (%d, %d) -> (%d, %d)\n"), dados->passageiros[i].X, dados->passageiros[i].Y, dados->passageiros[i].Xfinal, dados->passageiros[i].Yfinal);
	}
	return;
}

DWORD WINAPI ThreadComandos(LPVOID param) {
	TCHAR op[TAM];
	DADOS* dados = ((DADOS*)param);

	do {
		_tprintf(_T("\n>>"));
		_fgetts(op, TAM, stdin);
		WaitForSingleObject(dados->hMutexDados, INFINITE);
		op[_tcslen(op) - 1] = '\0';
		if (_tcscmp(op, TEXT("expulsar"))) {		//EXPULSAR TAXI

		}
		else if (_tcscmp(op, TEXT("listar"))) {		//LISTAR TAXIS
			listarTaxis(dados);
		}
		else if (_tcscmp(op, TEXT("aceitacaoT"))) {		//PAUSAR/RECOMECAR ACEITA��O DE TAXIS
			if (dados->respostaAuto)
				dados->respostaAuto = 0;
			else
				dados->respostaAuto = 1;
			//ENVIAR INFORMA��O AOS TAXIS
		}
		else if (_tcscmp(op, TEXT("manifestacoes"))) {		//DEFINIR INTERVALO DE TEMPO DURANTE O QUAL AGUARDA MANIFESTA�OES DOS TAXIS
			_tprintf(_T("\nIntervalo de tempo durante o qual aguarda manifesta��es (em segundos): "));
			_tscanf_s(_T("%d"), &dados->esperaManifestacoes);
			if (dados->esperaManifestacoes <= 0)
				dados->esperaManifestacoes = TempoManifestacoes;
		}
		else if (_tcscmp(op, TEXT("ajuda"))) {		//AJUDA NOS COMANDOS
			ajuda();
		}
		ReleaseMutex(dados->hMutexDados);
	} while (_tcscmp(op, TEXT("fim")));

	ExitThread(0);
}

DWORD WINAPI ThreadNovoTaxi(LPVOID param) {		//VERIFICA SE HA NOVOS TAXIS
	DADOS* dados = ((DADOS*)param);
	TAXI novo;

	while (1) {
		WaitForSingleObject(dados->novoTaxi, INFINITE);

		if (dados->terminar)
			return 0;
		WaitForSingleObject(dados->hMutexDados, INFINITE);

		CopyMemory(&novo, dados->shared, sizeof(TAXI));
		/*if (adicionaTaxi(dados, novo)) {
			_tprintf(TEXT("Novo Taxi: %s\n"), dados->taxis[dados->nTaxis - 1].matricula);
			CopyMemory(dados->shared, &dados->taxis[dados->nTaxis - 1], sizeof(TAXI));
		}
		else {
			novo.terminar = 1;
			CopyMemory(dados->shared, &dados->taxis[dados->nTaxis - 1], sizeof(TAXI));
		}*/
		_tprintf(TEXT("Novo Taxi: %s\n"), novo.matricula);
		ReleaseMutex(dados->hMutexDados);
		
		Sleep(1000);
	}

	ExitThread(0);
}


DWORD WINAPI ThreadNovoPassageiro(LPVOID param) {		//VERIFICA SE HA NOVOS PASSAGEIROS
	DADOS* dados = ((DADOS*)param);

	ExitThread(0);
}

boolean adicionaTaxi(DADOS* dados, TAXI novo) {
	if (dados->nTaxis >= MAXTAXIS)
		return FALSE;
	for (int i = 0; i < dados->nTaxis; i++)
		if (_tcscmp(novo.matricula, dados->taxis[i].matricula))
			return FALSE;

	dados->taxis[dados->nTaxis] = novo;
	dados->nTaxis++;
	return TRUE;
}

boolean removeTaxi(DADOS* dados, TAXI novo) {
	for (int i = 0; i < dados->nTaxis; i++) {
		if (_tcscmp(novo.matricula, dados->taxis[i].matricula)) {
			for (int k = i; k < dados->nTaxis - 1; k++) {
				dados->taxis[k] = dados->taxis[k + 1];
			}
			dados->nTaxis--;
			return TRUE;
		}
	}
	return FALSE;
}

boolean adicionaPassageiro(DADOS* dados, PASSAGEIRO novo) {
	if (dados->nPassageiros >= MAXPASS)
		return FALSE;

	dados->passageiros[dados->nPassageiros] = novo;
	dados->nPassageiros++;
	return TRUE;
}

boolean removePassageiro(DADOS* dados, PASSAGEIRO novo) {
	for (int i = 0; i < dados->nPassageiros; i++) {
		if (_tcscmp(novo.id, dados->passageiros[i].id)) {
			for (int k = i; k < dados->nPassageiros - 1; k++) {
				dados->passageiros[k] = dados->passageiros[k + 1];
			}
			dados->nPassageiros--;
			return TRUE;
		}
	}
	return FALSE;
}