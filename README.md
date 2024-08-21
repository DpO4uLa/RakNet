1. Добавлена поддержка компиляции под x64 архитектуру
2. Добавлен прокси класс
#include "RakNet/SOCKS5.hpp"
SOCKS5::
3. Добавлена явная рандомизация порта клиента при подключении в пределах 45000-64000
4. Добавлен фикс для подключения к серверам на хостинге React
5. Добавлен единый обработчик входящих RPC
RakClientInterface::RegisterRPCHandle()
void RPCHandler(int botID, int RPC_ID, BitStream bs, RakPeerInterface* pRakPeer);