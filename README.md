El algoritmo implementado es la **Transformada Discreta de Wavelet (DWT)** con la wavelet **Haar** para procesar imágenes de documentos (por ejemplo, escaneos en PNG o JPG) con el objetivo de comprimirlas mientras se preserva la legibilidad del texto. A continuación, se describe el proceso en detalle:

1. **Carga de la imagen**:
   - La imagen se carga en escala de grises usando OpenCV (`cv2.imdecode` en Python, `imread` en C++).
   - Si la imagen no tiene dimensiones que sean potencias de 2, se redimensiona a la potencia de 2 más cercana (por ejemplo, 512x512 o 1024x1024) para compatibilidad con la DWT Haar simple.

2. **Transformada Wavelet Haar 2D**:
   - **Paso 1: DWT horizontal**: Cada fila de la imagen se descompone en coeficientes de aproximación (promedios) y detalle (diferencias) usando los filtros Haar (baja frecuencia: `(a+b)/√2`, alta frecuencia: `(a-b)/√2`). Esto produce subbandas LL (aproximación) y LH (detalles horizontales).
   - **Paso 2: DWT vertical**: Se aplica la misma transformada a las columnas de las subbandas, generando cuatro subbandas: LL (baja-baja, imagen aproximada), LH (detalles horizontales), HL (detalles verticales), HH (detalles diagonales).
   - La wavelet Haar es simple y rápida, ya que usa promedios y diferencias sin cálculos complejos.

3. **Umbralización para compresión**:
   - Los coeficientes de detalle (LH, HL, HH) que están por debajo de un umbral (0.08 en escala [0,1], equivalente a ~20 en [0,255]) se establecen a cero. Esto elimina detalles finos y ruido, reduciendo la cantidad de datos sin afectar significativamente los bordes nítidos del texto.
   - El umbral está ajustado para documentos, preservando la legibilidad para OCR o visualización.

4. **Reconstrucción (Inverse DWT)**:
   - Se aplica la transformada inversa Haar 2D (primero vertical, luego horizontal) para reconstruir la imagen a partir de los coeficientes umbralizados.
   - Los valores se escalan de vuelta a [0,255] y se convierten a una imagen en escala de grises (`CV_8UC1`).

5. **Guardado y respuesta**:
   - En el microservicio FastAPI, la imagen procesada se codifica como PNG (`cv2.imencode`) y se retorna como respuesta binaria (`media_type="image/png"`).
   - También se guarda en el directorio `images` con un nombre único basado en la marca de tiempo (por ejemplo, `output_20250909_203305_123456.png`).

### Conveniencia del algoritmo

#### Ventajas
1. **Eficiente para documentos**:
   - La wavelet Haar es ideal para imágenes de documentos (texto, líneas, bordes nítidos) porque captura cambios abruptos (bordes) en la subbanda de detalles, preservando la legibilidad incluso tras la umbralización.
   - La compresión elimina ruido y detalles finos irrelevantes, reduciendo el tamaño del archivo (típicamente 20-50% en PNG optimizado) sin afectar la calidad del texto para OCR o lectura humana.

2. **Simplicidad y velocidad**:
   - Haar es la wavelet más simple (solo promedios y diferencias), lo que resulta en un algoritmo rápido y de baja complejidad computacional, adecuado para microservicios donde la latencia es crítica.

3. **Flexibilidad**:
   - El umbral (0.08) es ajustable para equilibrar calidad y compresión. Un umbral menor (por ejemplo, 0.05) mejora la calidad; uno mayor (por ejemplo, 0.1) aumenta la compresión.
   - El código soporta imágenes de entrada arbitrarias (convertidas a gris), y el redimensionamiento a potencias de 2 asegura compatibilidad con Haar.

#### Limitaciones
1. **Compresión moderada**:
   - La umbralización sola no logra tasas de compresión tan altas como algoritmos especializados (por ejemplo, JPEG 2000, que combina DWT con codificación entrópica). La reducción de tamaño depende de la optimización del formato de salida (PNG en este caso).
   - Para documentos con mucho texto, la compresión es efectiva, pero para imágenes con más detalles (por ejemplo, fotos), podría requerir múltiples niveles de DWT.

2. **Requisito de potencias de 2**:
   - El redimensionamiento a potencias de 2 puede introducir artefactos mínimos o cambiar las dimensiones de la imagen. Alternativas como padding (relleno) podrían evitarse con una implementación más compleja.

3. **Un solo nivel de DWT**:
   - La implementación actual usa un solo nivel de descomposición wavelet. Múltiples niveles (aplicando DWT iterativamente en la subbanda LL) podrían mejorar la compresión, pero aumentarían la complejidad y el tiempo de procesamiento.

4. **Dependencia de OpenCV**:
   - Aunque OpenCV es robusto, agrega una dependencia externa. Sin embargo, es ampliamente soportado y compatible con Python/C++.

#### Conveniencia para el caso de uso
El algoritmo **Wavelet Haar** es ideal por la compresión de imágenes de documentos con preservación de legibilidad:
- **Preservación de texto**: La wavelet Haar mantiene bordes nítidos, esenciales para texto en documentos escaneados, asegurando que OCR (como Tesseract) funcione bien.
- **Eficiencia**: La implementación en C++ con pybind11 es rápida y adecuada para un microservicio que procesa imágenes en tiempo real.
- **Facilidad de integración**: El endpoint FastAPI permite subir imágenes desde clientes web o scripts (por ejemplo, via `curl`), y el guardado local en `images` es útil para auditorías o almacenamiento.
- **Ajustabilidad**: El umbral permite personalizar la compresión según las necesidades (calidad vs. tamaño).


### Conclusión
El algoritmo DWT Haar implementado es **apropiado y eficiente** para comprimir imágenes de documentos, especialmente por su simplicidad, velocidad y capacidad para preservar bordes de texto. La integración en un microservicio FastAPI con guardado local lo hace práctico para aplicaciones web o pipelines automatizados. 
