#!/usr/bin/env python3
"""
Simple WebRTC Signaling Relay Server
Pairs two clients in the same room and forwards SDP/ICE messages between them.

Usage:
    pip install websockets
    python webrtc_relay.py

Then in the 119_WebRTCSTUN sample:
    Host:  type room name -> click "Start Relay"
    Client: type room name -> click "Connect Relay"
    Or specify relay server: room@host:port

Both sides connect outbound - no port forwarding needed.
"""

import asyncio
import json
import websockets
from websockets.server import serve

PORT = 9876
rooms = {}   # room_id -> list of unpaired websocket connections
peers = {}   # websocket -> partner websocket (after pairing)

async def handler(websocket):
    room_id = None

    try:
        async for message in websocket:
            # After pairing, forward everything directly to partner
            if websocket in peers:
                partner = peers[websocket]
                if partner.open:
                    await partner.send(message)
                continue

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
                    if not rooms[room_id]:
                        del rooms[room_id]
                    # Link peers so each handler forwards to its partner
                    peers[paired[0]] = paired[1]
                    peers[paired[1]] = paired[0]
                    # Send to client first so it sets up SDP/ICE handler before host sends offer
                    await paired[1].send(json.dumps({"type": "paired"}).encode())
                    await paired[0].send(json.dumps({"type": "paired"}).encode())
                    print(f"[{room_id}] Paired 2 clients")

    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        # Clean up peer link
        if websocket in peers:
            partner = peers.pop(websocket)
            peers.pop(partner, None)
        if room_id and room_id in rooms:
            if websocket in rooms[room_id]:
                rooms[room_id].remove(websocket)
                if rooms[room_id]:
                    print(f"[{room_id}] Client left ({len(rooms[room_id])} in room)")
                else:
                    del rooms[room_id]
                    print(f"[{room_id}] Room closed")

async def main():
    print(f"WebRTC Signaling Relay listening on ws://0.0.0.0:{PORT}")
    print("Clients connect and get paired by room ID.")
    async with serve(handler, "0.0.0.0", PORT):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())