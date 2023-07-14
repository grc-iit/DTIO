# DTIO

DTIO, a new, distributed, scalable, and adaptive I/O System.
DTIO is a task-based I/O System, it is fully decoupled,
and is intended to grow in the intersection of HPC and BigData.

# Configuration

You should be able to intercept using LD_PRELOAD, make sure to set the
environment variable DTIO\_CONF\_PATH to the path of your DTIO
configuration file (e.g., /home/user/DTIO/conf/default.yaml),
otherwise we won't be able to find it.

# TODO LIST

- [ ] Task dependencies (don't care)
- [ ] Metadata persistent store (flush at the end)
- [ ] Automated server bootstrapping
- [ ] Investigate read simulation
- [ ] Handle MDM for outstanding operations (data in transit)
    - [ ] Discuss how important is this issue of decoupled components
    - [ ] Possible fixes:
        - [ ] Invalidation lists + timer expiration to clean up
        - [ ] Intermediate state of data in MDM

## Notes

*   Timeout task scheduling. line#66
*   check the `usleep` in task scheduler → infinite looping
*   Aggregating logs
    `cat ts_* >> ts.csv`
*   Printing correctly
    `std::stringstream stream; // #include <sstream> for this
    stream << 1 << 2 << 3;
    std::cout << stream.str();`
