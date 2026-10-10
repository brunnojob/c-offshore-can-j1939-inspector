# c-offshore-can-j1939-inspector

Decode 29-bit CAN identifiers and identify sequence gaps in recorded offshore equipment traffic.

```sh
make test
./app < examples/input.txt
```

Offline diagnostic tool. Inputs and limits must be validated for each installation; no equipment commands are sent.

MIT. Built by [brunnodev](https://brunnodev.store).
