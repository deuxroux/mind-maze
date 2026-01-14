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
def save_result(data):
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, 'r') as f:
            records = json.load(f)
    else:
        records = [] #init new array if file doesn't exist
    
    records.append(data)
    
    # save back to file
    with open(DATA_FILE, 'w') as f:
        json.dump(records, f, indent=4)

def load_db():
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
    content = request.json 
    print(f"Received from ESP: {content}")
    
    #expected format : {"patient_id": 1, "date": "2026-01-14", "time_taken": 45, "status": "success", "notification_status": "awaiting physician alert"}
    save_result(content)
    
    # respond with JSON acknowledgement
    return jsonify({"status": "received", "message": "Data saved successfully!"}), 200

# Requirement: Broker new mazes [cite: 183]
@app.route('/api/get_maze', methods=['GET'])
def get_maze():
    return

if __name__ == '__main__':
    # Host='0.0.0.0' allows the ESP microcontroller access on local networks
    app.run(host='0.0.0.0', port=5050, debug=True)