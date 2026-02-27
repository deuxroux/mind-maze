# mind-maze
ESP/Arduino IoT project for visuospatial learning via an adaptive maze task, with caregiver/educator monitoring; Final Coursework for CM3040

## Libraries
Libraries Used: 

| Library Name | Version | Used In Module | Purpose |
| :--- | :--- | :--- | :--- |
| **Adafruit GFX Library** | `1.12.4` | Maze | Core graphics primitives for the Sharp Memory Display. |
| **Adafruit Sharp Memory Display** | `1.1.4` | Maze | Hardware driver for the Sharp Memory LCD |
| **SPI** | `Built-in` | Maze | SPI bus used by the Sharp Memory Display driver. |
| **ArduinoJson** | `7.4.2` | Both | Serialization of JSON data for backend communication. |
| **LiquidCrystal I2C** | `1.1.2` | Monitoring | Driver for the 2x16 Character LCD. |
| **WiFiS3** | `Built-in` | Both | Network connectivity for the Arduino Uno R4 WiFi. |
| **aWOT** | `3.5.0` | Both | Arduino Web of Things for Web Portal Creation. |

## Endpoints
A small REST API is exposed by each node (port 80). All API routes return JSON responses.

**Common (Both Maze + Monitoring)**
- `GET /`  HTML dashboard page.
- `POST /api/push`  Store a received P2P JSON message into the node’s inbox.
- `GET /api/pull`  Return the current inbox message (or `empty=true`).
- `GET /api/send`  Send a lightweight test message to the peer’s `/api/push`. For debugging purposes. 

**Maze Module only**
- `GET /api/status`  Report maze runtime state (e.g., `mazeActive`).
- `GET /api/send_result`  Trigger sending a maze result payload to the peer (testing / demo support).

**Monitoring Module only**
- `GET /api/ack` — Acknowledge/clear the stored result (clears unread state).
- `POST /api/request_maze` — Request a new maze / complexity change on the Maze Module.

## Dataflow and Schemas
This project uses JSON in **two layers**:

1) **HTTP API responses**: what each node returns to a browser/caller when you hit an endpoint (e.g., `/api/pull`, `/api/push`).  
2) **Peer-to-peer (P2P) message payloads**: the inter-module messages stored in each node’s `inbox` and exchanged between nodes.

### 1) HTTP API response schemas

#### A) `/api/pull` response (specialized wrapper)
- **Purpose:** `/api/pull` is the only endpoint that returns a stored inbox message.
- **Where the P2P payload lives:** when `empty=false`, the inter-module message is embedded under the `message` field.

    {
      "title": "PullResponse",
      "type": "object",
      "required": ["ok", "empty", "message"],
      "properties": {
        "ok": { "type": "boolean" },
        "empty": { "type": "boolean" },
        "unread": { "type": "boolean" },
        "message": {
          "description": "If empty=false, this is the stored P2P inbox message payload",
          "oneOf": [
            { "type": "null" },
            { "$ref": "mindmaze.schema.json" }
          ]
        }
      }
    }

#### B) Generic API response envelope (common fields across endpoints)
- **Purpose:** most endpoints return `{ ok: ... }` plus optional metadata (error detail, peer info, bytes sent, etc.).
- **Note:** this describes the **HTTP response wrapper**, not the P2P payload itself.

    {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.local/mindmaze.api.responses.schema.json",
      "title": "MindMaze API Response Schemas",
      "type": "object",
      "required": ["ok"],
      "properties": {
        "ok": { "type": "boolean" },

        "error": {
          "type": "string",
          "description": "Present when ok=false (e.g., BAD_JSON)"
        },
        "detail": {
          "type": "string",
          "description": "Extra error information (e.g., ArduinoJson parse error string)"
        },

        "storedBytes": { "type": "integer", "minimum": 0 },
        "sentBytes": { "type": "integer", "minimum": 0 },
        "peer": { "type": "string" },

        "empty": { "type": "boolean" },
        "unread": { "type": "boolean" },

        "message": {
          "description": "Returned by /api/pull; null if empty=true",
          "type": ["object", "null"]
        },

        "mazeActive": { "type": "boolean" }
      },
      "additionalProperties": true
    }

### 2) P2P payload schema (`mindmaze.schema.json`)
- **Purpose:** defines the **inter-module inbox message objects** exchanged via `/api/push` and retrieved via `/api/pull`.
- **Used by:** `PullResponse.message` when `empty=false`.

    {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.local/mindmaze.schema.json",
      "title": "MindMaze P2P Message Schemas",
      "oneOf": [
        { "$ref": "#/$defs/TestMessage" },
        { "$ref": "#/$defs/RequestMazeMessage" },
        { "$ref": "#/$defs/MazeResultMessage" }
      ],
      "$defs": {
        "Ipv4String": {
          "type": "string",
          "description": "IPv4 string (usually WiFi.localIP().toString())",
          "pattern": "^(\\d{1,3}\\.){3}\\d{1,3}$"
        },

        "TestMessage": {
          "type": "object",
          "additionalProperties": true,
          "required": ["type", "from", "millis", "note"],
          "properties": {
            "type": { "const": "test" },
            "from": { "$ref": "#/$defs/Ipv4String" },
            "millis": { "type": "integer", "minimum": 0 },
            "note": { "type": "string" }
          }
        },

        "RequestMazeMessage": {
          "type": "object",
          "additionalProperties": true,
          "required": ["type", "from", "millis", "maze_seed"],
          "properties": {
            "type": {
              "type": "string",
              "enum": ["request_maze", "new_maze", "send_maze"]
            },
            "from": { "$ref": "#/$defs/Ipv4String" },
            "millis": { "type": "integer", "minimum": 0 },

            "maze_seed": {
              "type": "integer",
              "minimum": 1,
              "description": "Preferred seed key used by MonitoringModule"
            },

            "seed": {
              "type": "integer",
              "minimum": 1,
              "description": "Alternate seed key accepted by MazeModule"
            }
          }
        },

        "MazeResultMessage": {
          "type": "object",
          "additionalProperties": true,
          "required": ["type", "status", "duration_ms"],
          "properties": {
            "type": { "const": "maze_result" },

            "status": {
              "type": "string",
              "enum": ["success", "failure", "incomplete"],
              "description": "Maze completion outcome"
            },

            "duration_ms": {
              "type": "integer",
              "minimum": 0,
              "description": "0 may be used to indicate incomplete"
            },

            "success": {
              "type": "boolean",
              "description": "Optional legacy/alternate success flag (Monitoring supports it)"
            },

            "maze_id": {
              "type": "string",
              "description": "Optional identifier of maze/level (Monitoring supports it)"
            }
          }
        }
      }
    }
