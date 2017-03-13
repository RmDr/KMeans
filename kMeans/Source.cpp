#include"client_server.h"

void CalculateClustersCenters() {
	Client client(2, 3, 4, "my_test.txt");
	client.ProcessStreamPool();

	std::ofstream ofstream;
	ofstream.open("my_test_answers.txt");
	for (const auto & cluster : client.GetClusters()) {
		cluster.Print(ofstream);
	}
	ofstream.close();
}

int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(nullptr);

	CalculateClustersCenters();

    system("pause");
    return 0;
}
