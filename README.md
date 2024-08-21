<!--
EN:
-->

*EN:*
1. Added support for compiling for x64 architecture
2. Added proxy class\
   2.1. `#include "RakNet/SOCKS5.hpp"`\
   2.2. `SOCKS5::`
3. Added explicit randomization of the client port in the range of 45000-64000
4. Added a fix for connecting to servers on React hosting
5. Added a unified handler for incoming RPC\
   5.1. `RakClientInterface::RegisterRPCHandle()`\
   5.2. `void RPCHandler(int botID, int RPC_ID, BitStream bs, RakPeerInterface* pRakPeer);`

<!--
RU:
-->

*RU:*
1. Добавлена поддержка компиляции под x64 архитектуру
2. Добавлен прокси класс\
   2.1. `#include "RakNet/SOCKS5.hpp"`\
   2.2. `SOCKS5::`
3. Добавлена явная рандомизация порта клиента при подключении в пределах 45000-64000
4. Добавлен фикс для подключения к серверам на хостинге React
5. Добавлен единый обработчик входящих RPC\
   5.1. `RakClientInterface::RegisterRPCHandle()`\
   5.2. `void RPCHandler(int botID, int RPC_ID, BitStream bs, RakPeerInterface);`