# DTIO

DTIO, a new, distributed, scalable, and adaptive I/O System.
DTIO is a task-based I/O System, it is fully decoupled,
and is intended to grow in the intersection of HPC and BigData.



# Installation

Clone grc-repo
```
git clone git@github.com:grc-iit/grc-repo.git
spack repo add grc-repo
```

Clone iowarp-install
```
git clone https://github.com/iowarp/iowarp-install.git
spack repo add iowarp-install/iowarp-spack
```

## For general users
```
spack install dtio
```

## For developers
```
spack install dtio +nocompile
```

```
spack load dtio +nocompile
bash env.sh
cmake ../ -D DTIO_ENABLE_CMAKE_DOTENV=ON
make -j32 install
```
