from flask import Flask, request, jsonify, render_template
import json, os, time
from datetime import datetime, timezone


app = Flask(__name__)

DATA_FILE = 'database.json' #use single file for this project

#HELPERS
def iso_timestamp():
    return datetime.now(timezone.utc).isoformat()

def get_new_id(prefix):
    return f"{prefix}-{int(time.time()*1000)}" #  example result-1705254321347

#functions to save and get data
def save_db(dataUpdate):
    with open(DATA_FILE, "w") as f:
        json.dump(dataUpdate, f)

def load_db():
    if not os.path.exists(DATA_FILE):
        return {"metadata": {}, "patients": {}, "devices": {}, "maze_state": {}, "results": [], "alerts": []}
    with open(DATA_FILE, "r") as f:
        return json.load(f)

# Create web interface to show historic patient data entries
#DASHBOARD ROUT
@app.route('/')
def dashboard():
    data = load_db()
    # Render the HTML template
    return render_template(
        "dashboard.html",
        patients=data.get("patients", {}),
        devices=data.get("devices", {}),
        maze_state=data.get("maze_state", {}),
        results=list(reversed(data.get("results", []))),  # newest first
        alerts=list(reversed(data.get("alerts", [])))
    )
# REST API ENDPOINTS
@app.route('/api/upload_result', methods=['POST'])
def upload_result():
    # Receive JSON and save
    payload = request.get_json(force= True) or {}
    patient_id = payload.get("patient_id") #assumes we have this field uploaded
    completed_result_id= get_new_id();
    active_maze_id = payload.get("active_maze_id")
    print(f"Received from ESP: {payload}")
    
    #expected format : {"patient_id": 1, "date": "2026-01-14", "time_taken": 45, "status": "success", "notification_status": "awaiting physician alert"}
    db = load_db()
    ms = db.setdefault("maze_state", {}).setdefault(patient_id, {
        "patient_id": patient_id,
        "status": "idle",
        "pending": False,
        "active_maze_id": None,
        "requested_at": None,
        "completed_result_id": None,
        "completed_at": None,
        "acked_result_id": None,
        "acked_at": None
    })

    ms["status"] = "completed"
    ms["pending"] = True
    ms["active_maze_id"] = active_maze_id
    ms["completed_result_id"] = completed_result_id
    ms["completed_at"] = iso_timestamp()

    #  append to results log
    db.setdefault("results", []).append({
        "result_id": completed_result_id,
        "patient_id": patient_id,
        "active_maze_id": active_maze_id,
        "completed_at": ms["completed_at"],
        "source": "web_dummy"
    })
    
    # respond with JSON acknowledgement
    return jsonify({"received": True, "maze_state": ms}), 200

# Requirement: Broker new mazes [cite: 183]
@app.route('/api/get_maze', methods=['GET'])
def get_maze():
    return

if __name__ == '__main__':
    # Host='0.0.0.0' allows the ESP microcontroller access on local networks
    app.run(host='0.0.0.0', port=5050, debug=True)


@app.route("/api/maze_state/<patient_id>", methods=["GET"])
def get_maze_state(patient_id):
    db = load_db()
    ms = db.get("maze_state", {}).get(patient_id)
    if not ms:
        return jsonify({"error": "unknown patient"}), 404
    return jsonify(ms), 200