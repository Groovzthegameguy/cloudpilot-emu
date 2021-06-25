
import asyncio

import websockets

from logger import logger
from proxy_context import ProxyContext


class Server:
    def __init__(self):
        self.connections = set()
        self.terminating = False
        self.server = None

    async def start(self, host, port):
        self.server = await websockets.serve(self._handle, host=host, port=port, close_timeout=1)
        print(f'server listening on {host}:{port}\n')

        await self.server.wait_closed()

        print("\nserver stopped")

    def stop(self):
        if self.server == None:
            return

        if not self.terminating:
            self.server.close()

            for connection in self.connections:
                asyncio.create_task(connection.close())

            self.terminating = True

        logger.info(
            f'waiting for {len(self.connections)} connections to close')

    async def _handle(self, socket, path):
        self.connections.add(socket)

        context = ProxyContext()
        await context.start(socket)

        self.connections.remove(socket)
