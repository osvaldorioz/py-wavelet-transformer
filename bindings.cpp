#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>  // Para soporte NumPy
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

namespace py = pybind11;
using namespace cv;
using namespace std;

// Función para aplicar 1D Haar DWT (forward)
void haar1D(vector<double>& data) {
    int n = data.size();
    vector<double> temp(n);
    for (int i = 0; i < n / 2; ++i) {
        temp[i] = (data[2 * i] + data[2 * i + 1]) / sqrt(2.0);
        temp[i + n / 2] = (data[2 * i] - data[2 * i + 1]) / sqrt(2.0);
    }
    data = temp;
}

// Función para aplicar 1D Haar inverse DWT
void ihaar1D(vector<double>& data) {
    int n = data.size();
    vector<double> temp(n);
    for (int i = 0; i < n / 2; ++i) {
        temp[2 * i] = (data[i] + data[i + n / 2]) / sqrt(2.0);
        temp[2 * i + 1] = (data[i] - data[i + n / 2]) / sqrt(2.0);
    }
    data = temp;
}

// Función para aplicar 2D Haar DWT (forward)
void haar2D(vector<vector<double>>& image) {
    int rows = image.size();
    int cols = image[0].size();
    for (int i = 0; i < rows; ++i) {
        haar1D(image[i]);
    }
    vector<vector<double>> temp(rows, vector<double>(cols));
    for (int j = 0; j < cols; ++j) {
        vector<double> col_data(rows);
        for (int i = 0; i < rows; ++i) {
            col_data[i] = image[i][j];
        }
        haar1D(col_data);
        for (int i = 0; i < rows; ++i) {
            temp[i][j] = col_data[i];
        }
    }
    image = temp;
}

// Función para aplicar 2D Haar inverse DWT
void ihaar2D(vector<vector<double>>& image) {
    int rows = image.size();
    int cols = image[0].size();
    vector<vector<double>> temp(rows, vector<double>(cols));
    for (int j = 0; j < cols; ++j) {
        vector<double> col_data(rows);
        for (int i = 0; i < rows; ++i) {
            col_data[i] = image[i][j];
        }
        ihaar1D(col_data);
        for (int i = 0; i < rows; ++i) {
            temp[i][j] = col_data[i];
        }
    }
    image = temp;
    for (int i = 0; i < rows; ++i) {
        ihaar1D(image[i]);
    }
}

// Función de umbralización
void threshold(vector<vector<double>>& coeffs, double thresh) {
    int rows = coeffs.size();
    int cols = coeffs[0].size();
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (abs(coeffs[i][j]) < thresh) {
                coeffs[i][j] = 0.0;
            }
        }
    }
}

// Conversión de np.ndarray a cv::Mat
Mat numpy_to_mat(py::array_t<uint8_t> input) {
    py::buffer_info buf = input.request();
    if (buf.ndim != 2) {
        throw std::runtime_error("La imagen debe ser 2D (escala de grises)");
    }
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    return Mat(rows, cols, CV_8UC1, buf.ptr);
}

// Conversión de cv::Mat a np.ndarray
py::array_t<uint8_t> mat_to_numpy(const Mat& mat) {
    if (mat.type() != CV_8UC1) {
        throw std::runtime_error("La imagen debe ser CV_8UC1");
    }
    py::array_t<uint8_t> result({mat.rows, mat.cols});
    py::buffer_info buf = result.request();
    memcpy(buf.ptr, mat.data, mat.rows * mat.cols * sizeof(uint8_t));
    return result;
}

// Función principal expuesta: Procesa la imagen (np.ndarray en gris) y devuelve np.ndarray
py::array_t<uint8_t> process_image(py::array_t<uint8_t> input) {
    // Convertir np.ndarray a cv::Mat
    Mat image = numpy_to_mat(input);
    if (image.empty()) {
        throw std::runtime_error("Imagen vacía");
    }

    int rows = image.rows;
    int cols = image.cols;

    // Asegurar potencias de 2
    if ((rows & (rows - 1)) != 0 || (cols & (cols - 1)) != 0) {
        int newRows = 1 << (int)ceil(log2(rows));
        int newCols = 1 << (int)ceil(log2(cols));
        resize(image, image, Size(newCols, newRows), 0, 0, INTER_CUBIC);
        rows = newRows;
        cols = newCols;
    }

    // Convertir a vector<vector<double>> normalizado [0,1]
    vector<vector<double>> pixels(rows, vector<double>(cols));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            pixels[i][j] = image.at<uchar>(i, j) / 255.0;
        }
    }

    // Aplicar DWT
    haar2D(pixels);

    // Umbralización
    double thresh = 0.08;
    threshold(pixels, thresh);

    // Reconstruir
    ihaar2D(pixels);

    // Convertir de vuelta a Mat
    Mat output(rows, cols, CV_8UC1);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double val = pixels[i][j] * 255.0;
            output.at<uchar>(i, j) = saturate_cast<uchar>(round(val));
        }
    }

    // Convertir a np.ndarray
    return mat_to_numpy(output);
}

PYBIND11_MODULE(wavelet, m) {
    m.doc() = "Módulo pybind11 para compresión wavelet Haar";
    m.def("process_image", &process_image, "Procesa una imagen con wavelet Haar",
          py::arg("input"));
}