# given a regular ASCII string, converts it into a hex string that can be used as the second argument to the `cansend` program

can_id = 123
s = "hello world"
can_flags = 0

ord_values = map(ord, s)

# strip the 0x
hex_values = map(lambda o: format(o, '#04x')[2:], ord_values)

print(f"{can_id}##{can_flags}{''.join(hex_values)}")
