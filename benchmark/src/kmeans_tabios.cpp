//
// Created by lukemartinlogan on 7/22/22.
//

#include "util.h"

int main(int argc, char **argv) {
  dtio::MPI_Init(&argc, &argv);
  if (argc != 5) {
    printf(
        "USAGE: ./kmean_tabios [dtio_conf] [file_path] [iter] [pfs_path]\n");
    exit(1);
  }

#ifdef COLLECT
  system("sh /home/cc/nfs/aetrio/scripts/log_reset.sh");
#endif
  std::stringstream stream;
  int rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  stream << "average_kmeans_tabios," << std::fixed << std::setprecision(10);
  std::string file_path = argv[2];
  int iteration = atoi(argv[3]);
  std::string pfs_path = argv[4];
  std::string filename = file_path + "test_" + std::to_string(rank) + ".dat";
  size_t io_per_teration = 32 * 1024 * 1024;
  std::vector<std::array<size_t, 2>> workload =
      std::vector<std::array<size_t, 2>>();
  workload.push_back({32 * 1024, 1024});

  size_t current_offset = 0;
  Timer global_timer = Timer();
  char *write_buf = new char[io_per_teration];
  gen_random(write_buf, io_per_teration);
  global_timer.resumeTime();
#ifdef TIMERBASE
  Timer map = Timer();
  map.resumeTime();
#endif
  FILE *fh = dtio::fopen(filename.c_str(), "w+");
#ifdef TIMERBASE
  map.pauseTime();
#endif
  global_timer.pauseTime();
  dtio::fwrite(write_buf, sizeof(char), io_per_teration, fh);
  delete (write_buf);
  size_t count = 0;
  std::vector<std::pair<size_t, std::vector<task>>> operations =
      std::vector<std::pair<size_t, std::vector<task>>>();
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0)
    std::cerr << "Data created Done\n";
  for (int i = 0; i < iteration; ++i) {
    for (auto item : workload) {
      for (int j = 0; j < item[1]; ++j) {
        if (count % 32 == 0) {
          std::random_device rd;
          std::mt19937 generator(rd());
          std::uniform_int_distribution<int> dist(0, 31);
          auto rand_offset = (dist(generator) * 1 * 1024 * 1024);
          global_timer.resumeTime();
#ifdef TIMERBASE
          map.resumeTime();
#endif
          dtio::fseek(fh, rand_offset, SEEK_SET);
#ifdef TIMERBASE
          map.pauseTime();
#endif
          global_timer.pauseTime();
          current_offset = static_cast<size_t>(rand_offset);
        }
        global_timer.resumeTime();
#ifdef TIMERBASE
        map.resumeTime();
#endif
#ifndef COLLECT
        operations.emplace_back(std::make_pair(
            item[0], dtio::fread_async(sizeof(char), item[0], fh)));

#endif
#ifdef TIMERBASE
        map.pauseTime();
#endif
        global_timer.pauseTime();
        current_offset += item[0];
        count++;
      }
    }
  }
  global_timer.resumeTime();
#ifdef TIMERBASE
  map.resumeTime();
#endif
  for (auto operation : operations) {
    wait_for_read(operation.first, operation.second, filename);
  }
  dtio::fclose(fh);
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0)
    std::cerr << "Read Done\n";
#ifdef TIMERBASE
  map.pauseTime();
#endif
  global_timer.pauseTime();
#ifdef TIMERBASE
  auto map_time = map.getElapsedTime();
  double map_sum;
  MPI_Allreduce(&map_time, &map_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  double map_mean = map_sum / comm_size;
  if (rank == 0)
    stream << map_mean << ",";
#endif
  std::string finalname = pfs_path + "final_" + std::to_string(rank) + ".dat";
  FILE *outfile;
  global_timer.resumeTime();
#ifdef TIMERBASE
  Timer reduce = Timer();
  reduce.resumeTime();
#endif
  outfile = dtio::fopen(finalname.c_str(), "w+");
#ifdef TIMERBASE
  reduce.pauseTime();
#endif
  global_timer.pauseTime();

  char out_buff[1024 * 1024];
  gen_random(out_buff, 1024 * 1024);
  global_timer.resumeTime();
#ifdef TIMERBASE
  reduce.resumeTime();
#endif
  dtio::fwrite(out_buff, sizeof(char), 1024 * 1024, outfile);
  dtio::fclose(outfile);
#ifdef TIMERBASE
  reduce.pauseTime();
#endif
  global_timer.pauseTime();
#ifdef TIMERBASE
  auto red_time = reduce.getElapsedTime();
  double red_sum, max, min;
  MPI_Allreduce(&red_time, &red_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(&red_time, &max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&red_time, &min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  double red_mean = red_sum / comm_size;
  if (rank == 0)
    stream << red_mean << "," << max << "," << min;
#endif
  auto time = global_timer.getElapsedTime();
  double sum;
  MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  double mean = sum / comm_size;
  if (rank == 0) {
#ifdef COLLECT
    double ts = get_average_ts();
    double worker = get_average_worker();
    stream << ts << "," << worker << ",";
#endif
    stream << "average," << mean << "\n";
    std::cerr << stream.str();
  }
  dtio::MPI_Finalize();
}
