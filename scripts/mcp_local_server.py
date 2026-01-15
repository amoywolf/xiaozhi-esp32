import argparse
import asyncio
import json
import time

from aiohttp import web


def build_ota_response(ws_url):
    now_ms = int(time.time() * 1000)
    return {
        "websocket": {
            "url": ws_url,
            "version": 3,
        },
        "server_time": {
            "timestamp": now_ms,
            "timezone_offset": 0,
        },
    }


async def ota_handler(request):
    ws_url = request.app["ws_url"]
    print(f"HTTP: OTA request from {request.remote}")
    payload = build_ota_response(ws_url)
    return web.json_response(payload)


async def ws_handler(request):
    print(f"WS: client connect from {request.remote}")
    ws = web.WebSocketResponse()
    await ws.prepare(request)

    scene = request.app["scene"]
    brightness = request.app["brightness"]
    speed = request.app["speed"]
    sent = False
    log_binary = request.app["log_binary"]
    do_get = request.app["do_get"]

    async for msg in ws:
        if msg.type == web.WSMsgType.TEXT:
            try:
                data = json.loads(msg.data)
            except json.JSONDecodeError:
                print("WS: non-JSON text received")
                continue

            if data.get("type") == "hello":
                print("WS: hello received")
                hello = {
                    "type": "hello",
                    "transport": "websocket",
                    "session_id": "local-test",
                    "audio_params": {
                        "sample_rate": 16000,
                        "frame_duration": 60,
                    },
                }
                await ws.send_str(json.dumps(hello))

                if not sent:
                    sent = True
                    print(f"WS: sending MCP scene={scene}")
                    mcp_payload = {
                        "type": "mcp",
                        "payload": {
                            "jsonrpc": "2.0",
                            "id": 1,
                            "method": "tools/call",
                            "params": {
                                "name": "self.led_scene.set",
                                "arguments": {},
                            },
                        },
                    }
                    args_obj = mcp_payload["payload"]["params"]["arguments"]
                    args_obj["scene"] = scene
                    if brightness >= 0:
                        args_obj["brightness"] = brightness
                    if speed >= 1:
                        args_obj["speed"] = speed
                    await asyncio.sleep(0.5)
                    await ws.send_str(json.dumps(mcp_payload))
                    if do_get:
                        get_payload = {
                            "type": "mcp",
                            "payload": {
                                "jsonrpc": "2.0",
                                "id": 2,
                                "method": "tools/call",
                                "params": {
                                    "name": "self.led_scene.get",
                                    "arguments": {},
                                },
                            },
                        }
                        await asyncio.sleep(0.5)
                        await ws.send_str(json.dumps(get_payload))
            elif data.get("type") == "mcp":
                payload = data.get("payload", {})
                print(f"WS: mcp reply {json.dumps(payload, ensure_ascii=True)}")

        elif msg.type == web.WSMsgType.BINARY:
            if log_binary:
                print(f"WS: binary message {len(msg.data)} bytes")
        elif msg.type == web.WSMsgType.ERROR:
            print(f"WS: error {ws.exception()}")
            break

    return ws


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--ws-host", required=True, help="LAN IP for device to connect")
    parser.add_argument("--scene", default="party")
    parser.add_argument("--brightness", type=int, default=-1, help="0-8, -1 keep")
    parser.add_argument("--speed", type=int, default=-1, help="1-10, -1 keep")
    parser.add_argument("--log-binary", action="store_true", help="Log binary audio frames")
    parser.add_argument("--do-get", action="store_true", help="Send self.led_scene.get after set")
    args = parser.parse_args()

    ws_url = f"ws://{args.ws_host}:{args.port}/ws"

    app = web.Application()
    app["ws_url"] = ws_url
    app["scene"] = args.scene
    app["brightness"] = args.brightness
    app["speed"] = args.speed
    app["log_binary"] = args.log_binary
    app["do_get"] = args.do_get
    app.router.add_route("GET", "/", ota_handler)
    app.router.add_route("POST", "/", ota_handler)
    app.router.add_route("GET", "/ws", ws_handler)
    app.router.add_route("GET", "/ws/", ws_handler)

    async def start_servers():
        runner = web.AppRunner(app)
        await runner.setup()

        site = web.TCPSite(runner, "0.0.0.0", args.port)
        await site.start()

        print(f"OTA server: http://0.0.0.0:{args.port}/")
        print(f"WebSocket server: {ws_url}")

        while True:
            await asyncio.sleep(3600)

    asyncio.run(start_servers())


if __name__ == "__main__":
    main()
