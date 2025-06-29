import serial
import time
import keyboard
import pyaudio
import whisper
import numpy as np
import queue
import threading

# Initialize serial connection to Arduino
ser = serial.Serial("COM6", 9600, timeout=1)

# Load Whisper model
model = whisper.load_model("base.en")

# Audio parameters
CHUNK = 1024
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000
SILENCE_THRESHOLD = 500   
MIN_AUDIO_LENGTH = 1.0

# Global control variables
audio_queue = queue.Queue()
recording_active = False
processing_active = False

def send_command(command):
    """Send the command to the Arduino via serial."""
    ser.write(command.encode())

def process_command(text):
    """Process the text command and format it for the robot."""
    print(f"Received command: {text}")
    
    text = text.lower()
    
    if "forward" in text:
        distance = extract_number(text)
        command = f"F{distance}"
    elif "backward" in text:
        distance = extract_number(text)
        command = f"B{distance}"
    elif "trace" in text:
        cars = extract_number(text)
        command = f"T{cars}"
    elif "left" in text:
        angle = extract_number(text)
        command = f"L{angle}"
    elif "right" in text:
        angle = extract_number(text)
        command = f"R{angle}"
    elif "stop" in text:
        command = "S"
    else:
        print("Command not recognized")
        return
    
    send_command(command)
    print('Sending:', command)

def extract_number(text):
    """Extracts the first number from the text."""
    words = text.split()
    for word in words:
        if word.isdigit():
            return int(word)
    
    number_words = {
        'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5,
        'six': 6, 'seven': 7, 'eight': 8, 'nine': 9, 'ten': 10,
        'twenty': 20, 'thirty': 30, 'forty': 40, 'fifty': 50
    }
    
    for word, num in number_words.items():
        if word in text:
            return num
    
    return 0  # Default if no number found

def audio_callback(in_data, frame_count, time_info, status):
    """Callback function for audio stream."""
    if recording_active:
        audio_queue.put(np.frombuffer(in_data, dtype=np.int16))
    return (in_data, pyaudio.paContinue)

def record_audio():
    """Record audio while recording_active is True."""
    p = pyaudio.PyAudio()
    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK,
                    stream_callback=audio_callback)
    
    stream.start_stream()
    print("Recording started... (Press space to stop)")
    
    while recording_active:
        time.sleep(0.1)
    
    stream.stop_stream()
    stream.close()
    p.terminate()
    print("Recording stopped")

def process_audio():
    """Process audio data from queue and transcribe."""
    global processing_active
    
    audio_data = []
    while not audio_queue.empty() or recording_active:
        try:
            chunk = audio_queue.get_nowait()
            audio_data.append(chunk)
        except queue.Empty:
            time.sleep(0.05)
            continue
    
    if audio_data:
        audio_array = np.concatenate(audio_data)
        audio_duration = len(audio_array) / RATE
        
        if audio_duration >= MIN_AUDIO_LENGTH:
            print(f"Processing {audio_duration:.2f} seconds of audio...")
            audio_float = audio_array.astype(np.float32) / 32768.0
            result = model.transcribe(audio_float)
            return result["text"].strip()
    
    return ""

def listen_for_commands():
    """Main loop for listening to commands."""
    global recording_active, processing_active
    
    print("Press space to start recording...")
    
    while True:
        if keyboard.is_pressed('space'):
            time.sleep(0.3)  # Debounce
            
            if not recording_active:
                # Start new recording
                recording_active = True
                processing_active = True
                audio_queue.queue.clear()
                
                # Start recording in separate thread
                record_thread = threading.Thread(target=record_audio)
                record_thread.start()
                
                print("Recording... Press space to stop")
            else:
                # Stop recording and process
                recording_active = False
                record_thread.join()
                
                # Process recorded audio
                command_text = process_audio()
                
                if command_text:
                    print(f"Recognized: {command_text}")
                    process_command(command_text)
                else:
                    print("No speech detected")
                
                processing_active = False
                print("Press space to start recording...")
            
            time.sleep(0.3)  # Additional debounce

if __name__ == "__main__":
    try:
        listen_for_commands()
    except KeyboardInterrupt:
        print("Program stopped by user")
    finally:
        recording_active = False
        processing_active = False
        ser.close()