"""
This module provides classes and methods to inject the Dtio interceptor.
Dtio is ....
"""
from jarvis_cd.basic.pkg import Interceptor
from jarvis_util import *


class Dtio(Interceptor):
    """
    This class provides methods to inject the Dtio interceptor.
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
                'name': 'interface_type',  # The name of the parameter
                'msg': '',  # Describe this parameter
                'type': str,  # What is the parameter type?
                'default': 'posix',  # What is the default value if not required?
                # Does this parameter have specific valid inputs?
                'choices': ['posix', 'stdio', 'hdf5'],
                # When type is list, what do the entries of the list mean?
                # A list of dicts just like this one.
                'args': []
            },
        ]

    def _configure(self, **kwargs):
        """
        Converts the Jarvis configuration to application-specific configuration.
        E.g., OrangeFS produces an orangefs.xml file.

        :param kwargs: Configuration parameters for this pkg.
        :return: None
        """
        pass

    def modify_env(self):
        """
        Modify the jarvis environment.

        :return: None
        """
        pass
