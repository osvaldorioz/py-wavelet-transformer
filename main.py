from fastapi import FastAPI, File, UploadFile, Response
import cv2
import numpy as np
from wavelet import process_image
import os
from datetime import datetime

app = FastAPI(title="Microservicio de Compresión Wavelet")

# Directorio para guardar imágenes
IMAGES_DIR = "/home/hadoop/Documentos/cpp_programs/pybind/py-transform-wavelet/images"

@app.post("/process-image", response_class=Response)
async def process_image_endpoint(file: UploadFile = File(...)):
    # Crear directorio 'images' si no existe
    os.makedirs(IMAGES_DIR, exist_ok=True)
    
    # Leer el archivo subido
    contents = await file.read()
    nparr = np.frombuffer(contents, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_GRAYSCALE)
    
    if img is None:
        return Response(content="Error: No se pudo cargar la imagen", status_code=400)
    
    # Procesar con el binding C++
    processed_img = process_image(img)
    
    # Codificar como PNG para la respuesta
    _, encoded_img = cv2.imencode('.png', processed_img)
    
    # Guardar la imagen procesada en el directorio 'images'
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    output_path = os.path.join(IMAGES_DIR, f"output_{timestamp}.png")
    cv2.imwrite(output_path, processed_img)
    
    # Retornar como respuesta binaria
    return Response(content=encoded_img.tobytes(), media_type="image/png")