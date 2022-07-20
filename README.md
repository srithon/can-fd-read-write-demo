To setup, create a named pipe using:
```sh
mkfifo "pseudo-device"
```

Next, run the writer service in one terminal and the reader service in the
other. The writer service will take line input from STDIN and write it to the
pipe using the SocketCAN protocol, where the reader service will read it.
