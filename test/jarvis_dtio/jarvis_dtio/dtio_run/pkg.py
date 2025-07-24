"""
This module provides classes and methods to launch the DtioRun application.
DtioRun is a lightweight I/O interception system that allows selective monitoring
and redirection of file I/O operations based on configurable path patterns.
"""

from jarvis_cd.basic.pkg import Application
from jarvis_util import *


class DtioRun(Application):
    """
    This class provides methods to configure and launch the DtioRun application.
    DtioRun handles path-based I/O interception configuration through include/exclude
    path patterns defined in YAML format.
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
                'name': 'include_paths',
                'msg': 'Paths to include for DTIO interception',
                'type': list,
                'default': ['/tmp', '/scratch', '/mnt/shared', '/data'],
                'class': 'paths',
                'rank': 1,
                'args': [
                    {
                        'name': 'path',
                        'msg': 'A string path to include',
                        'type': str,
                    }
                ]
            },
            {
                'name': 'exclude_paths',
                'msg': 'Paths to exclude from DTIO interception',
                'type': list,
                'default': ['/tmp/system', '/tmp/.X11-unix', '/scratch/logs'],
                'class': 'paths',
                'rank': 2,
                'args': [
                    {
                        'name': 'path',
                        'msg': 'A string path to exclude',
                        'type': str,
                    }
                ]
            },
        ]

    def _configure(self, **kwargs):
        """
        Converts the Jarvis configuration to application-specific configuration.
        E.g., OrangeFS produces an orangefs.xml file.

        :param kwargs: Configuration parameters for this pkg.
        :return: None
        """
        # Use lists directly from config
        include_paths = self.config['include_paths']
        exclude_paths = self.config['exclude_paths']

        # Create the DTIO configuration dictionary
        dtio_config = {
            'include': include_paths,
            'exclude': exclude_paths
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
        