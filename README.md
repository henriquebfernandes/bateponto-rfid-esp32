# Projeto — Bate-ponto RFID com ESP32

## 1. Visão Geral
O ESP32 registra o ponto a partir de cartões RFID. O gerenciamento do sistema acontece em uma página web local.

## 2. Componentes
- ESP32
- Leitor RFID RC522
- LED verde
- LED vermelho
- Buzzer

## 3. Inicialização
1. O ESP32 se conecta à rede Wi-Fi.
2. Sincroniza a data e a hora pela internet via NTP.
3. Disponibiliza a página web por um domínio local mDNS, como `bateponto.local`.
4. Inicia a leitura de cartões RFID.

## 4. Autenticação Web
- A senha é a única credencial de acesso.
- Todas as páginas de gerenciamento exigem autenticação.
- A sessão expira após um tempo configurável. Um novo login restabelece o acesso.

## 5. Gerenciamento de Pessoas
- Cadastrar pessoas.
- Editar nomes.
- Excluir pessoas.
- Visualizar e gerenciar cartões associados.

Uma pessoa pode ter zero, um ou vários cartões. Cada cartão pertence a no máximo uma pessoa. A exclusão de uma pessoa também remove todos os cartões e registros de ponto associados a ela.

## 6. Cadastro de Cartões
1. O operador seleciona uma pessoa na página web.
2. Clica no botão de adicionar cartão.
3. O RC522 entra em modo de cadastro pelo tempo configurado.
4. Se estiver livre, o primeiro cartão lido é associado à pessoa selecionada.
5. O modo de cadastro termina após a primeira leitura ou quando o prazo expira.

## 7. Registro Automático de Ponto
1. O RC522 identifica o cartão apresentado.
2. O sistema verifica se o cartão está associado a uma pessoa e se o cooldown permite um novo registro.
3. Quando aceita a leitura, alterna entre entrada e saída conforme o último registro da pessoa.
4. Salva a pessoa, o tipo, a data, o horário e a fonte `Leitor RFID`.

O cooldown define o intervalo mínimo entre registros consecutivos da mesma pessoa.

## 8. Gerenciamento de Registros
- Visualizar todos os registros ordenados por data e horário.
- Adicionar registros manualmente, informando pessoa, entrada ou saída, data e horário.
- Editar pessoa, tipo, data e horário dos registros existentes.
- Excluir registros.

A fonte é definida na criação e fica disponível apenas para leitura. O valor é `Leitor RFID` nos registros automáticos e `Manual` nos registros criados pela página web.

## 9. Sinalização
- Em caso de sucesso, o ESP32 acende o LED verde e emite um som de confirmação.
- Em caso de falha, acende o LED vermelho e emite um som distinto.
- Os LEDs e o buzzer ficam ativos por um breve intervalo.

## 10. Armazenamento
O sistema armazena pessoas, cartões e registros no LittleFS, na memória flash do ESP32. Os dados persistem após reinicializações.

## 11. Configurações
Todas as configurações ficam exclusivamente no código.

- SSID e senha do Wi-Fi.
- Domínio local mDNS.
- Senha de acesso à página web.
- Tempo limite da sessão de login.
- Tempo limite de leitura para cadastro.
- Cooldown entre registros da mesma pessoa.
- Fuso horário para data e hora.
- Pinos do RC522, LEDs e buzzer.
