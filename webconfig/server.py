import json
import os
import logging
import time
from flask import Flask, request, jsonify, send_from_directory

# Configure logging
logging.basicConfig(
    filename="server.log",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)

app = Flask(__name__)

CONFIG_FILE = "config.json"

@app.route("/config.json", methods=["GET"])
def get_config():
    """
    Serve config.json contents as JSON array of objects: [ {name, value}, ... ].
    """
    time.sleep(1)
    return send_from_directory(".", "config.json")

@app.route("/config.json", methods=["PUT"])
def put_config():
    """
    Save the JSON body to config.json (replaces previous contents).
    Expects an array of objects [ { "name": "...", "value": "..." }, ... ].
    """
    time.sleep(1)
    try:
        with open(CONFIG_FILE, "w") as f:
            f.write(request.data.decode("utf-8"))
        logging.info("Updated config.json via PUT")
        return jsonify({"status": "success", "message": "Configuration saved!"}), 200
    except Exception as e:
        logging.error(f"Error saving config.json: {str(e)}")
        return jsonify({"error": str(e)}), 500

#@app.route("/css/all.min.css", methods=["GET"])
@app.route("/css/font-awesome-6.4.2-all.min.css", methods=["GET"])
def css():
    return send_from_directory(".", "css/all.min.css")

@app.route("/webfonts/fa-solid-900.woff2", methods=["GET"])
def font1():
    return send_from_directory(".", "webfonts/fa-solid-900.woff2")

@app.route("/webfonts/fa-solid-900.tff", methods=["GET"])
def font2():
    return send_from_directory(".", "webfonts/fa-solid-900.tff")

@app.route("/tailwind_output.css", methods=["GET"])
def tailwind_css():
    return send_from_directory(".", "tailwind_output.css")

@app.route("/")
def serve_index():
    """
    Serve 'index.html' from the same directory (no separate local server needed).
    """
    return send_from_directory(".", "index.html")

@app.route("/index_inlined.html")
def serve_index_inlined():
    return send_from_directory(".", "index_inlined.html")



if __name__ == "__main__":
    logging.info("Starting Flask server on http://127.0.0.1:5000")
    app.run(host="127.0.0.1", port=5000, debug=True)
