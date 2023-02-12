#!/bin/python3

import sys

def main(argv) -> int:
    print("Sandor Laboratories Seismometer Monitor")
    print("Version 0.0.1-dev")
    print("Edward Sandor, February 2023")
    print("https://git.sandorlaboratories.com/edward/seismometer")
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
