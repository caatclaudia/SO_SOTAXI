#include "Header.h"

void comunicacaoParaCentral(DADOS *dados) {
	CopyMemory(dados->shared, dados->taxi, sizeof(TAXI));
	return;
}

void avisaNovoTaxi(DADOS *dados) {
	comunicacaoParaCentral(dados);

	SetEvent(dados->novoTaxi);
	Sleep(500);
	ResetEvent(dados->novoTaxi);

	_tprintf(TEXT("\nAguardando resposta da Central...\n"));
	WaitForSingleObject(dados->respostaAdmin, INFINITE);

	CopyMemory(dados->taxi, dados->shared, sizeof(TAXI));
	if (!dados->taxi->terminar)
		_tprintf(TEXT("\nBem Vindo!\n"));

	return;
}

void avisaTaxiSaiu(DADOS *dados) {
	SetEvent(dados->saiuTaxi);
	ResetEvent(dados->saiuTaxi);

	comunicacaoParaCentral(dados);

	SetEvent(dados->saiuTaxi);
	Sleep(500);
	ResetEvent(dados->saiuTaxi);

	return;
}

void avisaMovimentoTaxi(DADOS *dados) {
	//MANDA PARA ADMIN
	comunicacaoParaCentral(dados);

	SetEvent(dados->movimentoTaxi);
	Sleep(500);
	ResetEvent(dados->movimentoTaxi);
	return;
}
