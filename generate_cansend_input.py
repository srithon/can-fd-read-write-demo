# given a regular ASCII string, converts it into a hex string that can be used as the second argument to the `cansend` program

import argparse


def create_arg_parser():
    arg_parser = argparse.ArgumentParser(
        description=
        'Given CAN-FD parameters, converts it into a hex string that can be used as the second argument to the `cansend` program',
        add_help=True,
        exit_on_error=True)

    arg_parser.add_argument('can_id',
                            help='CAN ID as a 0-3 digit decimal number',
                            type=int)
    arg_parser.add_argument('--flags',
                            help='CAN flags as a single hex character',
                            type=str,
                            default='0')
    arg_parser.add_argument('data',
                            help='ASCII string to send as data in the frame',
                            type=str)

    return arg_parser


arg_parser = create_arg_parser()
params = arg_parser.parse_args()

_can_id = params.can_id
assert _can_id <= 999 and _can_id >= 0
# want it to be padded to 3 characters
can_id = format(_can_id, '#03d')

# TODO: make sure that it's not longer than max length
data = params.data

# normalize to lowercase
can_flags = params.flags.lower()
assert can_flags in '0123456789abcdef'

ord_values = map(ord, data)

# strip the 0x
hex_values = map(lambda o: format(o, '#04x')[2:], ord_values)

print(f"{can_id}##{can_flags}{''.join(hex_values)}")
