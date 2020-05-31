#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include "MapInfo.h"
#include "resource.h"

HKEY chave;

HANDLE hThreadAtualizaMapa;
DADOS dados;
INFO info;
int MODIFICOU = 0;
TCHAR str[256] = TEXT(" ");
TCHAR str_taxiLivre[256] = TEXT(" ");
TCHAR str_taxiOcupado[256] = TEXT(" ");
TCHAR str_pessoaSemTaxi[256] = TEXT(" ");
TCHAR str_pessoaComTaxi[256] = TEXT(" ");

void(*ptr_register)(TCHAR*, int);

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TrataConfTaxiL(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TrataConfTaxiO(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TrataConfPessoaS(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TrataConfPessoaC(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("MapInfo");

//Icones
HBITMAP hTaxiLivre, hTaxiOcupado, hPessoaSemTaxi, hPessoaComTaxi;
HDC hdcTaxiLivre, hdcTaxiOcupado, hdcPessoaSemTaxi, hdcPessoaComTaxi;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	dados.terminar = 0;

	HINSTANCE hLib;

	hLib = LoadLibrary(PATH_DLL);
	if (hLib == NULL)
		return 0;

	ptr_register = (void(*)(TCHAR*, int))GetProcAddress(hLib, "dll_register");

	hMutex = CreateMutex(NULL, FALSE, NOME_MUTEXMAPA);
	if (hMutex == NULL) {
		_tprintf(TEXT("\n[ERRO] Erro ao criar Mutex!\n"));
		return -1;
	}
	ptr_register((TCHAR*)NOME_MUTEXMAPA, 1);
	WaitForSingleObject(hMutex, INFINITE);
	ReleaseMutex(hMutex);

	EspMapa = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(dados.mapa), SHM_NAME);
	if (EspMapa == NULL)
	{
		_tprintf(TEXT("\n[ERRO] Erro ao criar FileMapping!\n"));
		return -1;
	}
	ptr_register((TCHAR*)SHM_NAME, 6);

	shared = (MAPA*)MapViewOfFile(EspMapa, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(dados.mapa));
	if (shared == NULL)
	{
		_tprintf(TEXT("\n[ERRO] Erro em MapViewOfFile!\n"));
		CloseHandle(EspMapa);
		return -1;
	}
	ptr_register((TCHAR*)SHM_NAME, 7);

	recebeMapa(&dados);
	inicializaVariaveis();

	hThreadAtualizaMapa = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadAtualizaMapa, (LPVOID)&dados, 0, NULL);
	if (hThreadAtualizaMapa == NULL) {
		_tprintf(TEXT("\n[ERRO] Erro ao lan�ar Thread!\n"));
		return 0;
	}


	HWND hWnd;
	MSG lpMsg;
	WNDCLASSEX wcApp;

	wcApp.cbSize = sizeof(WNDCLASSEX);
	wcApp.hInstance = hInst;
	wcApp.lpszClassName = szProgName;
	wcApp.lpfnWndProc = TrataEventos;
	wcApp.style = CS_HREDRAW | CS_VREDRAW;
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);

	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION);

	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);

	wcApp.lpszMenuName = MAKEINTRESOURCE(IDC_MAPINFO);

	wcApp.cbClsExtra = 0;
	wcApp.cbWndExtra = 0;
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	if (!RegisterClassEx(&wcApp))
		return(0);

	hWnd = CreateWindow(
		szProgName,
		TEXT("MapInfo"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(HWND)HWND_DESKTOP,
		(HMENU)NULL,
		(HINSTANCE)hInst,
		0);

	ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg);
	}

	HANDLE ghEvents[2];
	ghEvents[0] = hThreadAtualizaMapa;
	WaitForMultipleObjects(1, ghEvents, TRUE, INFINITE);



	_tprintf(TEXT("\nPrima uma tecla...\n"));
	_gettch();

	CloseHandle(EspMapa);
	CloseHandle(atualizaMap);
	FreeLibrary(hLib);
	RegCloseKey(chave);

	return((int)lpMsg.wParam);
}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	RECT rect;
	PAINTSTRUCT ps;
	HFONT hFont;
	TCHAR caract;
	int xPos = 0, yPos = 0;

	hFont = CreateFont(12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Impact"));

	switch (messg) {
	case WM_CREATE:
		hdc = GetDC(hWnd);

		hdcTaxiLivre = CreateCompatibleDC(hdc);
		hTaxiLivre = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_taxiLivre, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		SelectObject(hdcTaxiLivre, hTaxiLivre);

		hdcTaxiOcupado = CreateCompatibleDC(hdc);
		hTaxiOcupado = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_taxiOcupado, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		SelectObject(hdcTaxiOcupado, hTaxiOcupado);

		hdcPessoaSemTaxi = CreateCompatibleDC(hdc);
		hPessoaSemTaxi = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_pessoaSemTaxi, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		SelectObject(hdcPessoaSemTaxi, hPessoaSemTaxi);

		hdcPessoaComTaxi = CreateCompatibleDC(hdc);
		hPessoaComTaxi = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_pessoaComTaxi, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		SelectObject(hdcPessoaComTaxi, hPessoaComTaxi);

		ReleaseDC(hWnd, hdc);
		break;
		//SE FOR PASSAGEIRO MOSTRA DESTINO E (TAXI QUE O FOR BUSCAR)
	case WM_LBUTTONDOWN: {
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		int x = (xPos - 80) / 8;
		int y = (yPos - 15) / 9;
		for (int i = 0; i < info.npassageiros; i++) {
			if (info.passageiros[i].X == x && info.passageiros[i].Y == y) {
				//MOSTRA DESTINO E (TAXI QUE O FOR BUSCAR)
				TCHAR aux[TAM];
				if (info.passageiros[i].tempoEspera != -1) 	//ESTA A ESPERA DO TAXI
					_stprintf_s(aux, TAM, TEXT("Passageiro a espera de '%s' para ir para (%d,%d)!"), info.passageiros[i].matriculaTaxi, info.passageiros[i].Xfinal, info.passageiros[i].Yfinal);
				else
					_stprintf_s(aux, TAM, TEXT("Passageiro quer ir para (%d,%d)!"), info.passageiros[i].Xfinal, info.passageiros[i].Yfinal);
				MessageBox(hWnd, aux, TEXT("PASSAGEIRO"), MB_ICONINFORMATION | MB_OK);
			}
		}
		break;
	}
					   //SE FOR TAXI MOSTRA MATRICULA E (DESTINO)
	case WM_MOUSEMOVE: {
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		int x = (xPos - 80) / 8;
		int y = (yPos - 15) / 9;
		for (int i = 0; i < info.ntaxis; i++) {
			if (info.taxis[i].X == x && info.taxis[i].Y == y) {
				//MOSTRA MATRICULA E (DESTINO)
				TCHAR aux[TAM];
				if (!info.taxis[i].disponivel) 	//SE TIVER UM PASSAGEIRO ATRIBUIDO
					_stprintf_s(aux, TAM, TEXT("T�xi '%s' vai para (%d,%d)!"), info.taxis[i].matricula, info.taxis[i].Xfinal, info.taxis[i].Yfinal);
				else
					_stprintf_s(aux, TAM, TEXT("T�xi '%s'!"), info.taxis[i].matricula);
				MessageBox(hWnd, aux, TEXT("T�XI"), MB_ICONINFORMATION | MB_OK);
			}
		}

		break;
	}
	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);
		SetTextColor(hdc, RGB(0, 0, 0));
		SelectObject(hdc, hFont);
		SetBkMode(hdc, TRANSPARENT);
		xPos = 0;
		yPos = 0;
		for (int i = 0; i < tamanhoMapa * tamanhoMapa; i++) {
			caract = shared[i].caracter;
			rect.left = 80 + (8 * xPos);
			rect.top = 15 + (9 * yPos);
			if (caract == '_' || caract == 'X')
				DrawText(hdc, &caract, 1, &rect, DT_SINGLELINE | DT_NOCLIP);
			else {
				int x = (xPos - 80) / 8;
				int y = (yPos - 15) / 9;
				if (isdigit(caract)) {
					for (int i = 0; i < info.ntaxis; i++) {
						if (info.taxis[i].X == x && info.taxis[i].Y == y && info.taxis[i].disponivel == 1)
							BitBlt(hdc, rect.left, rect.top, 100, 100, hdcTaxiLivre, 0, 0, SRCCOPY);
						else if (info.taxis[i].X == x && info.taxis[i].Y == y)
							BitBlt(hdc, rect.left, rect.top, 100, 100, hdcTaxiOcupado, 0, 0, SRCCOPY);
					}
				}
				else {
					for (int i = 0; i < info.npassageiros; i++) {
						if (info.passageiros[i].X == x && info.passageiros[i].Y == y && info.passageiros[i].tempoEspera == -1)
							BitBlt(hdc, rect.left, rect.top, 100, 100, hdcPessoaSemTaxi, 0, 0, SRCCOPY);
						else if (info.passageiros[i].X == x && info.passageiros[i].Y == y)
							BitBlt(hdc, rect.left, rect.top, 100, 100, hdcPessoaComTaxi, 0, 0, SRCCOPY);
					}
				}
			}
			xPos++;
			if (xPos == (tamanhoMapa + 1)) {
				yPos++;
				xPos = 0;
			}
		}
		EndPaint(hWnd, &ps);
		break;
	}
	case  WM_COMMAND: {
		hdc = GetDC(hWnd);
		switch (LOWORD(wParam)) {
		case ID_CONFIGURAR_TAXI_L: {
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONF_TAXI_L), hWnd, (DLGPROC)TrataConfTaxiL);
			if (MODIFICOU) {
				hdcTaxiLivre = CreateCompatibleDC(hdc);
				hTaxiLivre = (HBITMAP)LoadImage(GetModuleHandle(NULL), str, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				//SE CONSEGUIR ABRIR TEM DE SER GUARDADO NO REGISTRY
				if (hTaxiLivre == NULL)
					hTaxiLivre = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_taxiLivre, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				else {
					RegSetValueEx(chave, TEXT("TaxiLivre"), 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));
					wcscpy_s(str_taxiLivre, str);
				}
				SelectObject(hdcTaxiLivre, hTaxiLivre);
			}
			break;
		}
		case ID_CONFIGURAR_TAXI_O: {
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONF_TAXI_O), hWnd, (DLGPROC)TrataConfTaxiO);
			if (MODIFICOU) {
				hdcTaxiOcupado = CreateCompatibleDC(hdc);
				hTaxiOcupado = (HBITMAP)LoadImage(GetModuleHandle(NULL), str, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				//SE CONSEGUIR ABRIR TEM DE SER GUARDADO NO REGISTRY
				if (hTaxiOcupado == NULL)
					hTaxiOcupado = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_taxiOcupado, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				else{
					RegSetValueEx(chave, TEXT("TaxiLivre"), 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));
					wcscpy_s(str_taxiOcupado, str);
				}
				SelectObject(hdcTaxiOcupado, hTaxiOcupado);
			}
			break;
		}
		case ID_CONFIGURAR_PESSOA_S: {
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONF_PESS_S), hWnd, (DLGPROC)TrataConfPessoaS);
			if (MODIFICOU) {
				hdcPessoaSemTaxi = CreateCompatibleDC(hdc);
				hPessoaSemTaxi = (HBITMAP)LoadImage(GetModuleHandle(NULL), str, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				//SE CONSEGUIR ABRIR TEM DE SER GUARDADO NO REGISTRY
				if (hPessoaSemTaxi == NULL)
					hPessoaSemTaxi = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_pessoaSemTaxi, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				else{
					RegSetValueEx(chave, TEXT("TaxiLivre"), 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));
					wcscpy_s(str_pessoaSemTaxi, str);
				}
				SelectObject(hdcPessoaSemTaxi, hPessoaSemTaxi);
			}
			break;
		}
		case ID_CONFIGURAR_PESSOA_C: {
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONF_PESS_C), hWnd, (DLGPROC)TrataConfPessoaC);
			if (MODIFICOU) {
				hdcPessoaComTaxi = CreateCompatibleDC(hdc);
				hPessoaComTaxi = (HBITMAP)LoadImage(GetModuleHandle(NULL), str, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				//SE CONSEGUIR ABRIR TEM DE SER GUARDADO NO REGISTRY
				if (hPessoaComTaxi == NULL)
					hPessoaComTaxi = (HBITMAP)LoadImage(GetModuleHandle(NULL), str_pessoaComTaxi, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				else{
					RegSetValueEx(chave, TEXT("TaxiLivre"), 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));
					wcscpy_s(str_pessoaComTaxi, str);
				}
				SelectObject(hdcPessoaComTaxi, hPessoaComTaxi);
			}
			break;
		}
		case IDM_ABOUT: {
			MessageBox(hWnd, TEXT("Este trabalho � realizado por Cl�udia Tavares - 2017009310\nNo �mbito de Sistemas Operativos II - 2019/2020"), TEXT("About"), MB_ICONINFORMATION | MB_OK);
			break;
		}
		case IDM_EXIT: {
			int value = MessageBox(hWnd, TEXT("Tem a certeza que deseja sair?"), TEXT("Confirma��o"), MB_ICONQUESTION | MB_YESNO);
			if (value == IDYES)
			{
				DestroyWindow(hWnd);
			}
			break;
		}
		}
		MODIFICOU = 0;
		ReleaseDC(hWnd, hdc);
		break;
	}
	case WM_CLOSE: {
		int value = MessageBox(hWnd, TEXT("Tem a certeza que deseja sair?"), TEXT("Confirma��o"), MB_ICONQUESTION | MB_YESNO);
		if (value == IDYES)
		{
			DestroyWindow(hWnd);
		}
		break;
	}
	case WM_DESTROY: // Destruir a janela e terminar o programa
		TerminateThread(hThreadAtualizaMapa, 0);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, messg, wParam, lParam);
		break;
	}
	return(0);
}

LRESULT CALLBACK TrataConfTaxiL(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	switch (messg) {
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CANCEL:
			EndDialog(hWnd, 0);
			return true;
		case IDC_OK:
			GetDlgItemText(hWnd, IDC_NOME_TAXI_L, str, 256);
			MessageBox(hWnd, str, TEXT("Conteudo da Caixa de Texto "), MB_OK);
			MODIFICOU = 1;
			EndDialog(hWnd, 0);
			return true;
		}
	}
	return false;
}

LRESULT CALLBACK TrataConfTaxiO(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	switch (messg) {
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CANCEL:
			EndDialog(hWnd, 0);
			return true;
		case IDC_OK:
			GetDlgItemText(hWnd, IDC_NOME_TAXI_O, str, 256);
			MessageBox(hWnd, str, TEXT("Conteudo da Caixa de Texto "), MB_OK);
			MODIFICOU = 1;
			EndDialog(hWnd, 0);
			return true;
		}
	}
	return false;
}

LRESULT CALLBACK TrataConfPessoaS(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	switch (messg) {
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CANCEL:
			EndDialog(hWnd, 0);
			return true;
		case IDC_OK:
			GetDlgItemText(hWnd, IDC_NOME_PESS_S, str, 256);
			MessageBox(hWnd, str, TEXT("Conteudo da Caixa de Texto "), MB_OK);
			MODIFICOU = 1;
			EndDialog(hWnd, 0);
			return true;
		}
	}
	return false;
}

LRESULT CALLBACK TrataConfPessoaC(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	switch (messg) {
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CANCEL:
			EndDialog(hWnd, 0);
			return true;
		case IDC_OK:
			GetDlgItemText(hWnd, IDC_NOME_PESS_C, str, 256);
			MessageBox(hWnd, str, TEXT("Conteudo da Caixa de Texto "), MB_OK);
			MODIFICOU = 1;
			EndDialog(hWnd, 0);
			return true;
		}
	}
	return false;
}

void recebeMapa(DADOS* dados) {
	EspInfo = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(INFO), SHM_INFO);
	if (EspInfo == NULL)
	{
		_tprintf(TEXT("\n[ERRO] Erro ao criar FileMapping!\n"));
		return;
	}
	ptr_register((TCHAR*)SHM_INFO, 6);

	sharedInfo = (INFO*)MapViewOfFile(EspInfo, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(INFO));
	if (sharedInfo == NULL)
	{
		_tprintf(TEXT("\n[ERRO] Erro em MapViewOfFile!\n"));
		return;
	}
	ptr_register((TCHAR*)SHM_INFO, 7);

	MAPA* aux = NULL;
	CopyMemory(&aux, shared, sizeof(shared));
	for (int i = 0; tamanhoMapa == -1; i++)
		if (shared[i].caracter == '\n')
			tamanhoMapa = i;
	dados->mapa = (MAPA*)malloc(sizeof(MAPA) * tamanhoMapa * tamanhoMapa);
	CopyMemory(dados->mapa, &aux, sizeof(dados->mapa));

	CopyMemory(&info, sharedInfo, sizeof(INFO));
	return;
}

void inicializaVariaveis() {
	DWORD queAconteceu;
	DWORD tamanho;

	//Criar/abrir uma chave em HKEY_CURRENT_USER\Software\Mapa
	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Mapa"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &chave, &queAconteceu) != ERROR_SUCCESS) {
		_tprintf(TEXT("[ERRO] Erro ao criar/abrir chave (%d)\n"), GetLastError());
		return;
	}
	else {
		//Se a chave foi criada, inicializar os valores
		if (queAconteceu == REG_CREATED_NEW_KEY) {
			_tprintf(TEXT("[DETALHES] Chave: HKEY_CURRENT_USER\\Software\\Mapa\n"));
			RegSetValueEx(chave, TEXT("TaxiLivre"), 0, REG_SZ, (LPBYTE)TEXT("taxi_livre.bmp"), _tcslen(TEXT("taxi_livre.bmp")) * sizeof(TCHAR));
			RegSetValueEx(chave, TEXT("TaxiOcupado"), 0, REG_SZ, (LPBYTE)TEXT("taxi_ocupado.bmp"), _tcslen(TEXT("taxi_ocupado.bmp")) * sizeof(TCHAR));
			RegSetValueEx(chave, TEXT("PessoaSemTaxi"), 0, REG_SZ, (LPBYTE)TEXT("pessoa_semTaxi.bmp"), _tcslen(TEXT("pessoa_semTaxi.bmp")) * sizeof(TCHAR));
			RegSetValueEx(chave, TEXT("PessoaComTaxi"), 0, REG_SZ, (LPBYTE)TEXT("pessoa_comTaxi.bmp"), _tcslen(TEXT("pessoa_comTaxi.bmp")) * sizeof(TCHAR));
		}
		//Se a chave foi aberta, ler os valores l� guardados
		else if (queAconteceu == REG_OPENED_EXISTING_KEY) {
			_tprintf(TEXT("Chave: HKEY_CURRENT_USER\\Software\\Mapa\n"));
			tamanho = 256;
			RegQueryValueEx(chave, TEXT("TaxiLivre"), NULL, NULL, (LPBYTE)str_taxiLivre, &tamanho);
			str_taxiLivre[tamanho / sizeof(TCHAR)] = '\0';
			tamanho = 256;
			RegQueryValueEx(chave, TEXT("TaxiOcupado"), NULL, NULL, (LPBYTE)str_taxiOcupado, &tamanho);
			str_taxiOcupado[tamanho / sizeof(TCHAR)] = '\0';
			tamanho = 256;
			RegQueryValueEx(chave, TEXT("PessoaSemTaxi"), NULL, NULL, (LPBYTE)str_pessoaSemTaxi, &tamanho);
			str_pessoaSemTaxi[tamanho / sizeof(TCHAR)] = '\0';
			tamanho = 256;
			RegQueryValueEx(chave, TEXT("PessoaComTaxi"), NULL, NULL, (LPBYTE)str_pessoaComTaxi, &tamanho);
			str_pessoaComTaxi[tamanho / sizeof(TCHAR)] = '\0';
		}
	}
}

DWORD WINAPI ThreadAtualizaMapa(LPVOID param) {
	DADOS* dados = ((DADOS*)param);

	atualizaMap = CreateEvent(NULL, TRUE, FALSE, EVENT_ATUALIZAMAP);
	if (atualizaMap == NULL) {
		return 0;
	}
	SetEvent(atualizaMap);
	Sleep(500);
	ResetEvent(atualizaMap);

	while (1) {
		WaitForSingleObject(atualizaMap, INFINITE);

		if (dados->terminar)
			return 0;

		CopyMemory(dados->mapa, shared, sizeof(dados->mapa));

		CopyMemory(&info, sharedInfo, sizeof(INFO));

		Sleep(3000);
	}
	Sleep(500);

	ExitThread(0);
}