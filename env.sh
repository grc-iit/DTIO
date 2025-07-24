module load DTIO
spack load cityhash liburing fmt spdlog hdf5
scspkg build profile m=cmake path=.env.cmake
scspkg build profile m=dotenv path=.env
