#!/usr/bin/env python3
"""
Simple WebRTC Signaling Relay Server
Pairs two clients in the same room and forwards messages between them.

Usage:
    pip install websockets
    python webrtc_relay.py

Then in your game, use:
    server->ListenRelay(URL("ws://your-ip:9876"), "myroom");
    client->ConnectRelay(URL("ws://your-ip:9876"), "myroom");

Both sides connect outbound - no port forwarding needed anywhere.
"""

import asyncio
import json
import websockets
from websockets.server import serve

PORT = 9876
rooms = {}  # room_id -> list of websocket connections

async def handler(websocket):
    room_id = None
    paired_with = None

    try:
        async for message in websocket:
            try:
                data = json.loads(message)
            except json.JSONDecodeError:
                continue

            msg_type = data.get("type")

            if msg_type == "join":
                room_id = data.get("room", "default")
                if room_id not in rooms:
                    rooms[room_id] = []

                # Remove stale connections
                rooms[room_id] = [ws for ws in rooms[room_id] if ws.open]

                rooms[room_id].append(websocket)
                print(f"[{room_id}] Client joined ({len(rooms[room_id])} in room)")

                # Pair when we have 2 clients
                if len(rooms[room_id]) >= 2:
                    paired = rooms[room_id][:2]
                    rooms[room_id] = rooms[room_id][2:]
                    await paired[0].send(json.dumps({"type": "paired"}))
                    await paired[1].send(json.dumps({"type": "paired"}))
                    print(f"[{room_id}] Paired 2 clients")
                    # Forward between paired clients
                    asyncio.create_task(forward(paired[0], paired[1], room_id))

    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        if room_id and room_id in rooms:
            if websocket in rooms[room_id]:
                rooms[room_id].remove(websocket)
                print(f"[{room_id}] Client left ({len(rooms[room_id])} in room)")

async def forward(ws_a, ws_b, room_id):
    """Forward binary messages between two paired WebSocket connections."""
    async def relay(src, dst, label):
        try:
            async for message in src:
                if isinstance(message, bytes):
                    await dst.send(message)
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            print(f"[{room_id}] Relay {label} ended")

    await asyncio.gather(
        relay(ws_a, ws_b, "A->B"),
        relay(ws_b, ws_a, "B->A"),
    )

async def main():
    print(f"WebRTC Signaling Relay listening on ws://0.0.0.0:{PORT}")
    print("Clients connect and get paired by room ID.")
    async with serve(handler, "0.0.0.0", PORT):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())