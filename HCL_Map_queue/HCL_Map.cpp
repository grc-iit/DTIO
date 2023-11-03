#include <execinfo.h>
#include <hcl/common/data_structures.h>
#include <hcl/unordered_map/unordered_map.h>
#include <mpi.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <utility>

struct KeyType {
  size_t a;
  KeyType() : a(0) {}
  KeyType(size_t a_) : a(a_) {}
#ifdef HCL_ENABLE_RPCLIB
  MSGPACK_DEFINE(a);
#endif
  /* equal operator for comparing two Matrix. */
  bool operator==(const KeyType &o) const { return a == o.a; }
  KeyType &operator=(const KeyType &other) {
    a = other.a;
    return *this;
  }
  bool operator<(const KeyType &o) const { return a < o.a; }
  bool operator>(const KeyType &o) const { return a > o.a; }
  bool Contains(const KeyType &o) const { return a == o.a; }
};
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
template <typename A>
void serialize(A &ar, KeyType &a) {
  ar &a.a;
}
#endif
namespace std {
template <>
struct hash<KeyType> {
  size_t operator()(const KeyType &k) const { return k.a; }
};
}  // namespace std


int main(int argc, char *argv[]) {
	MPI_Init(&argc, &argv);
	int comm_size = 0, my_rank = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	int ranks_per_server = comm_size, num_request = 100;
	long size_of_request = 1024;
	bool debug = false;
	bool server_on_node = false;
	char *server_list_path = std::getenv("SERVER_LIST_PATH");

	if (argc > 1) ranks_per_server = atoi(argv[1]);
	if (argc > 2) num_request = atoi(argv[2]);
	if (argc > 3) size_of_request = (long)atol(argv[3]);
	if (argc > 4) server_on_node = (bool)atoi(argv[4]);
	if (argc > 5) debug = (bool)atoi(argv[5]);
	int len;
	if (debug && my_rank == 0) {
		printf("%d ready for attach\n", comm_size);
		fflush(stdout);
		getchar();
	}
	MPI_Barrier(MPI_COMM_WORLD);
	bool is_server = (my_rank + 1) % ranks_per_server == 0;
	int my_server = my_rank / ranks_per_server;
	int num_servers = comm_size / ranks_per_server;
	size_t size_of_elem = sizeof(int);

	printf("rank %d, is_server %d, my_server %d, num_servers %d\n", my_rank,
	 is_server, my_server, num_servers);

	const int array_size = TEST_REQUEST_SIZE;
    
	if (size_of_request != array_size) {
	printf(
	"Please set TEST_REQUEST_SIZE in include/hcl/common/constants.h "
	"instead. Testing with %d\n",
	array_size);
	}
	HCL_CONF->IS_SERVER = is_server;
	HCL_CONF->MY_SERVER = my_server;
	HCL_CONF->NUM_SERVERS = num_servers;
	HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
	printf("HCL_CONF->SERVER_LIST_PATH: %s\n", server_list_path);
	HCL_CONF->SERVER_LIST_PATH = std::string(server_list_path) + "server_list"; 
	


	hcl::unordered_map<KeyType, std::array<int, array_size>> *map = new hcl::unordered_map<KeyType, std::array<int, array_size>>();

	// Create a KeyType instance
	KeyType key(42);

	// Create a std::array instance
	std::array<int, array_size> vals = {1, 2, 3};

	// Add the key-value pair to the map
	map->Put(key, vals);

	// Accessing the value using the key
	std::pair<bool, std::array<int, array_size>> result = map->Get(key);

	// Check the key in the map
	if (result.first) {
		std::array<int, array_size> foundValue = result.second;
		printf("Value for key (%zu): ", key.a);
		for (int value : foundValue) {
			printf("%d ", value);
		}
		printf("\n");
	} else {
		printf("Key not found.\n");
	}

	delete(map);
	MPI_Finalize();
	exit(EXIT_SUCCESS);
}

