#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Secur32.lib")

extern "C" void api_init(int port);
extern "C" void api_init_thread(unsigned short int port);
extern "C" bool is_free(unsigned short int port);
extern "C" unsigned short int api_init_thread_find();