import dlib
import cv2
import os
import numpy as np
import requests
from supabase_client import create_supabase_client
from datetime import datetime, timedelta
import threading
from queue import Queue

# Initialize dlib's face detector, shape predictor, and face recognition model
face_detector = dlib.get_frontal_face_detector()
shape_predictor = dlib.shape_predictor("/home/sharjeel/Desktop/New folder/shape_predictor_68_face_landmarks.dat")
face_recognition_model = dlib.face_recognition_model_v1("/home/sharjeel/Desktop/New folder/dlib_face_recognition_resnet_model_v1.dat")

# Initialize Supabase client
supabase = create_supabase_client()

# Fetch data from Supabase
response = supabase.table("student").select("*").execute()
students = response.data

# Prepare known face encodings and names
known_face_encodings = []
known_face_names = []
status_changed_today = {}  # To track if status changed to "home" today

for student in students:
    name = student['name']
    picture_url = student['picture']

    try:
        # Download image from the URL
        response = requests.get(picture_url, stream=True)
        if response.status_code == 200:
            image = np.frombuffer(response.content, np.uint8)
            image = cv2.imdecode(image, cv2.IMREAD_COLOR)
            rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

            detections = face_detector(rgb_image, 1)

            for detection in detections:
                shape = shape_predictor(rgb_image, detection)
                face_encoding = np.array(face_recognition_model.compute_face_descriptor(rgb_image, shape))
                known_face_encodings.append(face_encoding)
                known_face_names.append(name)

    except Exception as e:
        print(f"Error processing image for {name}: {e}")

# Dictionary to track last recognition time
last_recognition_time = {}

# Queue for recognized face data
recognition_queue = Queue()

def process_recognition():
    while True:
        # Wait for data in the queue
        data = recognition_queue.get()
        if data is None:  # Stop signal
            break

        name, student_id, current_time = data

        # Fetch student's current status
        student_record = supabase.table("student").select("status, last_time").eq("id", student_id).execute().data[0]
        current_status = student_record['status']
        last_status_change = datetime.fromisoformat(student_record['last_time'])

        # Toggle status logic
        if current_status == "onboard":
            if (current_time - last_status_change).total_seconds() > 60:
                supabase.table("student").update({
                    "status": "home",
                    "last_time": current_time.isoformat()
                }).eq("id", student_id).execute()
                status_changed_today[name] = current_time.date()
        elif current_status == "home":
            if name not in status_changed_today or status_changed_today[name] != current_time.date():
                supabase.table("student").update({
                    "status": "onboard",
                    "last_time": current_time.isoformat()
                }).eq("id", student_id).execute()

# Start the processing thread
processing_thread = threading.Thread(target=process_recognition, daemon=True)
processing_thread.start()

# Start video capture
video_capture = cv2.VideoCapture("http://192.168.175.90/mjpeg/1")

while True:
    result, video_frame = video_capture.read()
    if not result:
        break

    # Resize frame for faster processing
    small_frame = cv2.resize(video_frame, (0, 0), fx=0.25, fy=0.25)
    rgb_small_frame = cv2.cvtColor(small_frame, cv2.COLOR_BGR2RGB)

    # Detect faces and recognize them
    detections = face_detector(rgb_small_frame, 1)
    for detection in detections:
        shape = shape_predictor(rgb_small_frame, detection)
        face_encoding = np.array(face_recognition_model.compute_face_descriptor(rgb_small_frame, shape))

        # Compare with known faces
        face_distances = np.linalg.norm(known_face_encodings - face_encoding, axis=1)
        if len(face_distances) > 0:
            best_match_index = np.argmin(face_distances)
            if face_distances[best_match_index] < 0.45:  # Threshold for a match
                name = known_face_names[best_match_index]

                # Check last recognition time
                current_time = datetime.now()
                student_id = [s['id'] for s in students if s['name'] == name][0]

                if name not in last_recognition_time or (current_time - last_recognition_time[name]).total_seconds() > 5:
                    last_recognition_time[name] = current_time
                    recognition_queue.put((name, student_id, current_time))
            else:
                name = "Unknown"
        else:
            name = "Unknown"

        # Scale back face location to original size
        top, right, bottom, left = (
            detection.top() * 4,
            detection.right() * 4,
            detection.bottom() * 4,
            detection.left() * 4,
        )

        # Draw rectangle and label around the face
        cv2.rectangle(video_frame, (left, top), (right, bottom), (0, 255, 0), 2)
        cv2.putText(video_frame, name, (left, top - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)

    # Display the frame
    cv2.imshow("Face Recognition", video_frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

# Stop the processing thread
recognition_queue.put(None)
processing_thread.join()

video_capture.release()
cv2.destroyAllWindows()
