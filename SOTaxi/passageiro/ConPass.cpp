#include "ConPass.h"

void(*ptr_register)(TCHAR*, int);

int _tmain() {
	HANDLE hThreadComandos, hThreadMovimentaPassageiro, hThreadRespostaTransporte;
	DADOS dados;
	dados.nPassageiros = 0;
	dados.terminar = 0;

	HINSTANCE hLib;

	hLib = LoadLibrary(PATH_DLL);
	if (hLib == NULL)
		return 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	ptr_register = (void(*)(TCHAR*, int))GetProcAddress(hLib, "dll_register");

	Semaphore = CreateSemaphore(NULL, 1, 1, SEMAPHORE_NAME);
	if (Semaphore == NULL) {
		_tprintf(TEXT("CreateSemaphore failed.\n"));
		return FALSE;
	}
	ptr_register((TCHAR*)SEMAPHORE_NAME, 3);

	_tprintf(TEXT("\nAguardando autoriza��o para entrar...\n"));
	WaitForSingleObject(Semaphore, INFINITE);
	_tprintf(TEXT("\nEntrei!\n"));

	hThreadComandos = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadComandos, (LPVOID)&dados, 0, NULL);
	if (hThreadComandos == NULL) {
		_tprintf(TEXT("\n[ERRO] Erro ao lan�ar Thread!\n"));
		return 0;
	}
	//hThreadMovimentaPassageiro = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadMovimentaPassageiro, (LPVOID)&dados, 0, NULL);
	//if (hThreadMovimentaPassageiro == NULL) {
	//	_tprintf(TEXT("\nErro ao lan�ar Thread!\n"));
	//	return 0;
	//}
	//hThreadRespostaTransporte = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadRespostaTransporte, (LPVOID)&dados, 0, NULL);
	//if (hThreadRespostaTransporte == NULL) {
	//	_tprintf(TEXT("\nErro ao lan�ar Thread!\n"));
	//	return 0;
	//}

	HANDLE ghEvents[1];
	ghEvents[0] = hThreadComandos;
	/*ghEvents[1] = hThreadMovimentaPassageiro;
	ghEvents[2] = hThreadRespostaTransporte;*/
	WaitForMultipleObjects(1, ghEvents, TRUE, INFINITE);

	_tprintf(TEXT("Passageiros v�o sair!\n"));
	_tprintf(TEXT("Prima uma tecla...\n"));
	_gettch();

	ReleaseSemaphore(Semaphore, 1, NULL);

	CloseHandle(Semaphore);
	FreeLibrary(hLib);

	return 0;
}

void novoPassageiro(DADOS* dados) {

	_tprintf(_T("\n[NOVO] Id do Passageiro: "));
	_fgetts(dados->passageiros[dados->nPassageiros].id, TAM, stdin);
	dados->passageiros[dados->nPassageiros].id[_tcslen(dados->passageiros[dados->nPassageiros].id) - 1] = '\0';

	_tprintf(_T("\n[NOVO]  Localizacao do Passageiro (X Y) : "));
	_tscanf_s(_T("%d"), &dados->passageiros[dados->nPassageiros].detalhes.X);
	_tscanf_s(_T("%d"), &dados->passageiros[dados->nPassageiros].detalhes.Y);

	_tprintf(_T("\n[NOVO]  Local de destino do Passageiro (X Y) : "));
	_tscanf_s(_T("%d"), &dados->passageiros[dados->nPassageiros].detalhes.Xfinal);
	_tscanf_s(_T("%d"), &dados->passageiros[dados->nPassageiros].detalhes.Yfinal);
	dados->passageiros[dados->nPassageiros].detalhes.movimento = 0;
	dados->passageiros[dados->nPassageiros].detalhes.terminar = 0;
	dados->passageiros[dados->nPassageiros].detalhes.id_mapa = TEXT('.');

	//VAI AO ADMIN VER SE PODE CRIAR

	return;
}

DWORD WINAPI ThreadComandos(LPVOID param) {
	TCHAR op[TAM], i;
	DADOS* dados = ((DADOS*)param);

	do {
		_tprintf(_T("\n\n"));
		i = _gettch();
		_tprintf(_T("%c"), i);
		op[0] = i;
		_fgetts(&op[1], sizeof(op), stdin);
		op[_tcslen(op) - 1] = '\0';
		if (_tcscmp(op, TEXT("novo"))) {		//NOVO PASSAGEIRO
			novoPassageiro(dados);
			dados->nPassageiros++;
		}
		_tprintf(_T("\n\n"));
	} while (_tcscmp(op, TEXT("fim")));

	for (int i = 0; i < MAX_PASS; i++)
		dados->passageiros[i].detalhes.terminar = 1;

	dados->terminar = 1;

	ExitThread(0);
}

DWORD WINAPI ThreadMovimentoPassageiro(LPVOID param) {	//ADMIN MANDA PASSAGEIRO
	DADOS* dados = ((DADOS*)param);

	ExitThread(0);
}

DWORD WINAPI ThreadRespostaTransporte(LPVOID param) {	//ADMIN MANDA PASSAGEIRO
	DADOS* dados = ((DADOS*)param);

	ExitThread(0);
}