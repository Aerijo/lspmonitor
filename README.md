# lspmonitor
Cross platform tool to monitor and interact with LSP servers and clients

## About

This tool acts as a man-in-the-middle between LSP clients and servers, providing full analysis and control over the protocol with minimum effort. As this tool does not require any support from the client or server, it will work on nearly all of them. Inserting it is as simple as prepending `lspmonitor -- ` to the original call to spawn the language server. The `lspmonitor` process will then launch the server itself, functioning as a proxy between the server and client.

If you can't modify the server and arguments used by the client directly, you can replace the server executable itself with the following (on Linux, presumably something similar exists on Windows)
```
#! /usr/bin/env sh
exec lspmonitor -- renamedServerExe
```

It's a work in progress, currently only logs client-server LSP interactions over stdio.

### Planned
- Support connecting over Unix domain sockets and TCP as well
- Run GUI in separate process so client can't kill it
- Schema validation of all communications
- Manual / automatic control over message order and timing.
- Scripting stub servers / clients for testing
- Defining a set of `$lspmonitor` notifications to probe supporting server/client state.
