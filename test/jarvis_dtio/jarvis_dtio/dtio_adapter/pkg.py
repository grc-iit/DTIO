"""
This module provides classes and methods to inject the DTIO interceptors.
DTIO intercepts the I/O calls used by native programs and routes them to DTIO.
"""
from jarvis_cd.basic.pkg import Interceptor
from jarvis_util import *


class DtioAdapter(Interceptor):
    """
    This class provides methods to inject the DTIO interceptors.
    """
    def _init(self):
        """
        Initialize paths
        """
        pass

    def _configure_menu(self):
        """
        Create a CLI menu for the configurator method.
        For thorough documentation of these parameters, view:
        https://github.com/scs-lab/jarvis-util/wiki/3.-Argument-Parsing

        :return: List(dict)
        """
        return [
            {
                'name': 'posix',
                'msg': 'Intercept POSIX I/O',
                'type': bool,
                'default': False
            },
            {
                'name': 'stdio',
                'msg': 'Intercept STDIO I/O',
                'type': bool,
                'default': False
            },
            {
                'name': 'mpi',
                'msg': 'Intercept MPI-IO',
                'type': bool,
                'default': False
            },
            {
                'name': 'hdf5',
                'msg': 'Intercept HDF5 I/O via VOL connector',
                'type': bool,
                'default': False
            },
        ]

    def _configure(self, **kwargs):
        """
        Converts the Jarvis configuration to application-specific configuration.
        E.g., DTIO interceptor configuration.

        :param kwargs: Configuration parameters for this pkg.
        :return: None
        """
        has_one = False
        if self.config['posix']:
            self.env['DTIO_POSIX'] = self.find_library('dtio_posix_interception')
            if self.env['DTIO_POSIX'] is None:
                raise Exception('Could not find dtio_posix_interception')
            self.env['DTIO_ROOT'] = str(pathlib.Path(self.env['DTIO_POSIX']).parent.parent)
            print(f'Found libdtio_posix_interception.so at {self.env["DTIO_POSIX"]}')
            has_one = True
        if self.config['stdio']:
            self.env['DTIO_STDIO'] = self.find_library('dtio_stdio_interception')
            if self.env['DTIO_STDIO'] is None:
                raise Exception('Could not find dtio_stdio_interception')
            self.env['DTIO_ROOT'] = str(pathlib.Path(self.env['DTIO_STDIO']).parent.parent)
            print(f'Found libdtio_stdio_interception.so at {self.env["DTIO_STDIO"]}')
            has_one = True
        if self.config['mpi']:
            self.env['DTIO_MPI'] = self.find_library('dtio_mpi_interception')
            if self.env['DTIO_MPI'] is None:
                raise Exception('Could not find dtio_mpi_interception')
            self.env['DTIO_ROOT'] = str(pathlib.Path(self.env['DTIO_MPI']).parent.parent)
            print(f'Found libdtio_mpi_interception.so at {self.env["DTIO_MPI"]}')
            has_one = True
        if self.config['hdf5']:
            self.env['DTIO_VOL'] = self.find_library('dtio_vol_connector')
            if self.env['DTIO_VOL'] is None:
                raise Exception('Could not find dtio_vol_connector')
            self.env['DTIO_ROOT'] = str(pathlib.Path(self.env['DTIO_VOL']).parent.parent)
            print(f'Found libdtio_vol_connector.so at {self.env["DTIO_VOL"]}')
            has_one = True
        if not has_one:
            raise Exception('DTIO API interceptor not selected')

    def modify_env(self):
        """
        Modify the jarvis environment to enable DTIO interception.

        :return: None
        """
        if self.config['posix']:
            self.append_env('LD_PRELOAD', self.env['DTIO_POSIX'])
        if self.config['stdio']:
            self.append_env('LD_PRELOAD', self.env['DTIO_STDIO'])
        if self.config['mpi']:
            self.append_env('LD_PRELOAD', self.env['DTIO_MPI'])
        if self.config['hdf5']:
            plugin_path_parent = (
                str(pathlib.Path(self.env['DTIO_VOL']).parent))
            self.setenv('HDF5_PLUGIN_PATH', plugin_path_parent)
            self.setenv('HDF5_VOL_CONNECTOR', 'dtio under_vol=0;under_info={};')

    def start(self):
        """
        Start DTIO system if needed.
        """
        # Add any DTIO system startup logic here if needed
        pass

    def stop(self):
        """
        Stop DTIO system if needed.
        """
        # Add any DTIO system cleanup logic here if needed
        pass 