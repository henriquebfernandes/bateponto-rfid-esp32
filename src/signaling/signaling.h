#pragma once

// inicializa as saídas e permite acionar e encerrar a sinalização
bool beginSignaling();
void signalResult(bool success);
void updateSignaling();
