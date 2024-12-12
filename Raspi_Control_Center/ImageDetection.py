import datetime, time, os, subprocess 

# This is the Machine Learning algorithm that will detect if a 3D print has errors 
def capture_image(filename: str = "3D_print_image.jpg") -> None:
    try: 
        # No other library work, so just do this manually! 
        # Use libcamera-still to take a picture and save it 
        subprocess.run(["libcamera-still", "-o", f"3d_print_pic/{filename}", "--nopreview"], check=True, stdout=subprocess.DEVNULL )
    except subprocess.CalledProcessError as e:
        print(f"Error capturing image: {e}")
    
def handle_3D_fail_detection(): 
    """
    Function that captures an image from camera and 
    pass that image to the C++ ML algo to determine if that image has a defect or not 
    """ 

    date_now = datetime.datetime.now( datetime.timezone.utc ).timestamp()   
    img_name: str = f"{date_now}_3D_print.jpg" 
    capture_image(filename=img_name)
    abs_img_path = "/home/jon/HomeAutomation/3d_print_pic/" + img_name 
    # Now, we will run the ML algorithm on this image 
    time.sleep(2) # some delay to let image save
    # We will Popen instead of run() to be able to grab the output of the ML model 
    run_ML_command = [os.path.abspath("ML_Model2/build/app")] + [f"{abs_img_path}"]
    p = subprocess.Popen( run_ML_command,  shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    try: 
        stdout, stderr = p.communicate() 
        #print("sending command: ", run_ML_command)
        stdout = stdout.decode('utf-8')
        stderr = stderr.decode('utf-8') 
        # Try to process output 
        output = stdout.split(":")
        if len(stderr) > 0: 
            raise ValueError(f"Error: {stderr}")

        prediction = output[0] 
        
        prediction_confidence = output[1] 
    
        if prediction == "good": 
            print(f"Prediction: Good, confidence: {prediction_confidence}") 
        else:
            print(f"Prediction: Bad, confidence: {prediction_confidence}") 
        
    except Exception as e: 
        print(f"ERROR: unable to process output from ML algo output of {output}!", e) 
        raise 
