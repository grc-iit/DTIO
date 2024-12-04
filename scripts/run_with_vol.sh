
mpic++ test-hdf5.c -ldtio_vol_connector -lhdf5
LD_PRELOAD=/home/kbateman/DTIO/build/libdtio_vol_connector.so ./a.out
