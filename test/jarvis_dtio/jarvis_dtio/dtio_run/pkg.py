"""
This module provides classes and methods to launch the DtioRun application.
DtioRun is ....
"""

from jarvis_cd.basic.pkg import Application
from jarvis_util import *
import os


class DtioRun(Application):
    """
    This class provides methods to launch the DtioRun application.
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
                'name': 'worker_path',
                'msg': 'Path to the DTIO worker directory',
                'type': str,
                'default': '${HOME}/DTIO/run-test/test',
                'class': 'paths',
                'rank': 1,
            },
            {
                'name': 'pfs_path',
                'msg': 'Path to the parallel filesystem directory',
                'type': str,
                'default': '${HOME}/DTIO/run-test/pfs',
                'class': 'paths',
                'rank': 1,
            },
            {
                'name': 'hcl_server_list_path',
                'msg': 'Path to the HCL server list configuration',
                'type': str,
                'default': '${HOME}/DTIO/conf/hcl_servers',
                'class': 'paths',
                'rank': 1,
            },
            {
                'name': 'nats_url_client',
                'msg': 'NATS client URL',
                'type': str,
                'default': 'nats://localhost:4222/',
                'class': 'networking',
                'rank': 1,
            },
            {
                'name': 'nats_url_server',
                'msg': 'NATS server URL',
                'type': str,
                'default': 'nats://localhost:4223/',
                'class': 'networking',
                'rank': 1,
            },
            {
                'name': 'memcached_url_client',
                'msg': 'Memcached client URL',
                'type': str,
                'default': '--SERVER=localhost:11211',
                'class': 'networking',
                'rank': 1,
            },
            {
                'name': 'memcached_url_server',
                'msg': 'Memcached server URL',
                'type': str,
                'default': '--SERVER=localhost:11212',
                'class': 'networking',
                'rank': 1,
            },
            {
                'name': 'assignment_policy',
                'msg': 'Policy for assigning tasks',
                'type': str,
                'default': 'RANDOM',
                'class': 'scheduling',
                'rank': 1,
            },
            {
                'name': 'ts_num_worker_threads',
                'msg': 'Number of worker threads per task scheduler',
                'type': int,
                'default': 8,
                'class': 'scheduling',
                'rank': 1,
            },
            {
                'name': 'num_workers',
                'msg': 'Number of worker processes',
                'type': int,
                'default': 4,
                'class': 'scheduling',
                'rank': 1,
            },
            {
                'name': 'num_schedulers',
                'msg': 'Number of scheduler processes',
                'type': int,
                'default': 1,
                'class': 'scheduling',
                'rank': 1,
            },
            {
                'name': 'check_fs',
                'msg': 'Enable filesystem checking',
                'type': bool,
                'default': False,
                'class': 'features',
                'rank': 1,
            },
            {
                'name': 'never_trace',
                'msg': 'Disable tracing functionality',
                'type': bool,
                'default': True,
                'class': 'features',
                'rank': 1,
            },
            {
                'name': 'async_mode',
                'msg': 'Enable asynchronous mode',
                'type': bool,
                'default': False,
                'class': 'features',
                'rank': 1,
            },
            {
                'name': 'use_uring',
                'msg': 'Enable io_uring support',
                'type': bool,
                'default': False,
                'class': 'features',
                'rank': 1,
            },
            {
                'name': 'use_cache',
                'msg': 'Enable caching functionality',
                'type': bool,
                'default': False,
                'class': 'features',
                'rank': 1,
            },
            {
                'name': 'worker_staging_size',
                'msg': 'Size of worker staging area',
                'type': int,
                'default': 0,
                'class': 'features',
                'rank': 1,
            },
        ]

    def _configure(self, **kwargs):
        """
        Converts the Jarvis configuration to application-specific configuration.
        E.g., OrangeFS produces an orangefs.xml file.

        :param kwargs: Configuration parameters for this pkg.
        :return: None
        """
        # Create the DTIO configuration dictionary
        dtio_config = {
            'WORKER_PATH': os.path.expandvars(self.config['worker_path']),
            'PFS_PATH': os.path.expandvars(self.config['pfs_path']),
            'HCL_SERVER_LIST_PATH': os.path.expandvars(self.config['hcl_server_list_path']),
            'NATS_URL_CLIENT': self.config['nats_url_client'],
            'NATS_URL_SERVER': self.config['nats_url_server'],
            'MEMCACHED_URL_CLIENT': self.config['memcached_url_client'],
            'MEMCACHED_URL_SERVER': self.config['memcached_url_server'],
            'ASSIGNMENT_POLICY': self.config['assignment_policy'],
            'TS_NUM_WORKER_THREADS': self.config['ts_num_worker_threads'],
            'NUM_WORKERS': self.config['num_workers'],
            'NUM_SCHEDULERS': self.config['num_schedulers'],
            'CHECK_FS': str(self.config['check_fs']).lower(),
            'NEVER_TRACE': str(self.config['never_trace']).lower(),
            'ASYNC_MODE': str(self.config['async_mode']).lower(),
            'USE_URING': str(self.config['use_uring']).lower(),
            'USE_CACHE': str(self.config['use_cache']).lower(),
            'WORKER_STAGING_SIZE': self.config['worker_staging_size']
        }

        # Save DTIO configuration
        dtio_config_yaml = f'{self.shared_dir}/dtio_config.yaml'
        YamlFile(dtio_config_yaml).save(dtio_config)
        self.env['DTIO_CONF_PATH'] = dtio_config_yaml

    def start(self):
        """
        Launch an application. E.g., OrangeFS will launch the servers, clients,
        and metadata services on all necessary pkgs.

        :return: None
        """
        pass

    def stop(self):
        """
        Stop a running application. E.g., OrangeFS will terminate the servers,
        clients, and metadata services.

        :return: None
        """
        pass

    def kill(self):
        """
        Forcibly a running application. E.g., OrangeFS will terminate the
        servers, clients, and metadata services.

        :return: None
        """
        pass

    def clean(self):
        """
        Destroy all data for an application. E.g., OrangeFS will delete all
        metadata and data directories in addition to the orangefs.xml file.

        :return: None
        """
        pass

    def _get_stat(self, stat_dict):
        """
        Get statistics from the application.

        :param stat_dict: A dictionary of statistics.
        :return: None
        """
        stat_dict[f'{self.pkg_id}.runtime'] = self.start_time
        