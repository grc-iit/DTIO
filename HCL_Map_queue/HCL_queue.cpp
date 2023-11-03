#include <execinfo.h>
#include <hcl/common/data_structures.h>
#include <hcl/queue/queue.h>
#include <mpi.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <queue>
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
}

int main(int argc, char *argv[]) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE) {
    printf("Didn't receive appropriate MPI threading specification\n");
    exit(EXIT_FAILURE);
  }
  int comm_size, my_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  int ranks_per_server = comm_size, num_request = 100;
  long size_of_request = 1024;
  bool debug = false;
  bool server_on_node = false;
  if (argc > 1) ranks_per_server = atoi(argv[1]);
  if (argc > 2) num_request = atoi(argv[2]);
  if (argc > 3) size_of_request = (long)atol(argv[3]);
  if (argc > 4) server_on_node = (bool)atoi(argv[4]);
  if (argc > 5) debug = (bool)atoi(argv[5]);

  /* if(comm_size/ranks_per_server < 2){
       perror("comm_size/ranks_per_server should be atleast 2 for this test\n");
       exit(-1);
   }*/
  int len;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(processor_name, &len);
  if (debug) {
    printf("%s/%d: %d\n", processor_name, my_rank, getpid());
  }

  if (debug && my_rank == 0) {
    printf("%d ready for attach\n", comm_size);
    fflush(stdout);
    getchar();
  }
  MPI_Barrier(MPI_COMM_WORLD);
  bool is_server = (my_rank + 1) % ranks_per_server == 0;
  int my_server = my_rank / ranks_per_server;
  int num_servers = comm_size / ranks_per_server;

  // The following is used to switch to 40g network on Ares.
  // This is necessary when we use RoCE on Ares.
  std::string proc_name = std::string(processor_name);
  /*int split_loc = proc_name.find('.');
  std::string node_name = proc_name.substr(0, split_loc);
  std::string extra_info = proc_name.substr(split_loc+1, string::npos);
  proc_name = node_name + "-40g." + extra_info;*/

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
  char *server_list_path = std::getenv("SERVER_LIST_PATH");

  HCL_CONF->IS_SERVER = is_server;
  HCL_CONF->MY_SERVER = my_server;
  HCL_CONF->NUM_SERVERS = num_servers;
  HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
  HCL_CONF->SERVER_LIST_PATH = std::string(server_list_path) + "server_list";


  hcl::queue<KeyType> *queue = new hcl::queue<KeyType>();

  // Create a KeyType instance
  KeyType key(42);

  // Create a std::array instance
  uint16_t vals = 1;

  // Push the key-value pair into the queue
  queue->Push(key, vals);

  // Use std::pair to handle the return value of Pop
  std::pair<bool, KeyType> popResult = queue->Pop(vals);

  // Check the key in the queue
  if (popResult.first) {
      printf("Popped value for key (%zu): %u\n", key.a, vals);
  } else {
      printf("Key not found in the queue.\n");
  }
}

