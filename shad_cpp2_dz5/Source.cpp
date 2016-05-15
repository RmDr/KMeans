#include"client_server.h"

int main() {
	Client client;
	client.ProcessStreamPool();
	client.PrintClusters();

    system("pause");
    return 0;
}
