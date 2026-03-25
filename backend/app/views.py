"""
Flask Documentation:     https://flask.palletsprojects.com/
Jinja2 Documentation:    https://jinja.palletsprojects.com/
Werkzeug Documentation:  https://werkzeug.palletsprojects.com/
This file creates your application.
"""

# from crypt import methods
import site 

from app import app, Config,  mongo, Mqtt
from flask import escape, render_template, request, jsonify, send_file, redirect, make_response, send_from_directory 
from json import dumps, loads 
from werkzeug.utils import secure_filename
from datetime import datetime,timedelta, timezone
from os import getcwd
from os.path import join, exists
from time import time, ctime
from math import floor
 



#####################################
#   Routing for your application    #
#####################################


@app.route('/api/range/get/<start>/<end>', methods=['GET'])
def get_range(start, end):
    '''Returns readings between two timestamps
    '''
    if request.method == "GET":
        try:
            START = escape(start)
            END = escape(end)
            data = mongo.getRange(START, END)
            if data:
                return jsonify({"status": "found", "data":data})
            
        except Exception as e:
            print(f"get_range error: f{str(e)}")

    # FILE DATA NOT EXIST
    return jsonify({"status":"not found","data":[]})

@app.route('/api/mmar/field', methods=['GET']) 
def get_field_mmar():   
    '''RETURNS MIN, MAX, AVG AND RANGE FOR EACH FIELD. THAT FALLS WITHIN THE START AND END DATE RANGE'''
   
    field = request.args.get('field')
    start = request.args.get('start')
    end   = request.args.get('end')

    if not all([field, start, end]):
        return jsonify({"error": "field, start and end are required"}), 400

    result = mongo.getFieldMMARByTimestamp(field, start, end)
    if result is None:
        return jsonify({"error": "No data found for that range"}), 404
    return jsonify(result), 200

@app.route('/api/stats/distribution', methods=['GET'])
def get_distribution():
    '''Returns frequency distribution for a field between two timestamps.
    '''
    valid_fields = [
        "temperature_c", "pressure_hpa", "altitude_m",
        "dht_temp_c", "humidity_pct", "heat_index_c", "soil_moisture_pct"
    ]

    field = request.args.get('field')
    start = request.args.get('start')
    end   = request.args.get('end')

    if not all([field, start, end]):
        return jsonify({"error": "field, start and end are required"}), 400

    if field not in valid_fields:
        return jsonify({"error": f"Invalid field. Choose from: {valid_fields}"}), 400

    result = mongo.getFrequencyDistro(field, start, end)
    if result is None:
        return jsonify({"error": "No data found"}), 404
    return jsonify(result), 200
   


@app.route('/api/file/get/<filename>', methods=['GET']) 
def get_images(filename):   
    '''Returns requested file from uploads folder'''
   
    if request.method == "GET":
        directory   = join( getcwd(), Config.UPLOADS_FOLDER) 
        filePath    = join( getcwd(), Config.UPLOADS_FOLDER, filename) 

        # RETURN FILE IF IT EXISTS IN FOLDER
        if exists(filePath):        
            return send_from_directory(directory, filename)
        
        # FILE DOES NOT EXIST
        return jsonify({"status":"file not found"}), 404


@app.route('/api/file/upload',methods=["POST"])  
def upload():
    '''Saves a file to the uploads folder'''
    
    if request.method == "POST": 
        file     = request.files['file']
        filename = secure_filename(file.filename)
        file.save(join(getcwd(),Config.UPLOADS_FOLDER , filename))
        return jsonify({"status":"File upload successful", "filename":f"{filename}" })

 


###############################################################
# The functions below should be applicable to all Flask apps. #
###############################################################


@app.after_request
def add_header(response):
    """
    Add headers to both force latest IE rendering engine or Chrome Frame,
    and also tell the browser not to cache the rendered page. If we wanted
    to we could change max-age to 600 seconds which would be 10 minutes.
    """
    response.headers['X-UA-Compatible'] = 'IE=Edge,chrome=1'
    response.headers['Cache-Control'] = 'public, max-age=0'
    return response

@app.errorhandler(405)
def page_not_found(error):
    """Custom 404 page."""    
    return jsonify({"status": 404}), 404