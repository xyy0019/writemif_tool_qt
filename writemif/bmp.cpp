#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include "bmp.h"

uint8_t reverse_byte(uint8_t byte) {
    uint8_t reversed = 0;
    for (int i = 0; i < 8; i++) {
        reversed <<= 1;
        reversed |= (byte & 1);
        byte >>= 1;
    }
    return reversed;
}

uint64_t byte_64(uint8_t byte) {
    return byte;
}

template<typename T>
constexpr T clamp(T value, T low, T high) {
    return (value < low) ? low : (value > high) ? high : value;
}

// 转换20字节缓冲区为4个像素的函数
std::vector<uint8_t> process_buffer_10(const uint8_t* buffer, int colorspace) {
    std::vector<uint8_t> pixels(12); // 每个像素3个字节（R, G, B），共4个像素

    uint64_t r = byte_64(buffer[1]) << 0 |
    byte_64(buffer[5]) << 8 |
    byte_64(buffer[9]) << 16 |
    byte_64(buffer[13]) << 24 |
    byte_64(buffer[17]) << 32;
    uint16_t r1 = r >> 30 >> 2;
    uint16_t r2 = ((r >> 20) & 0x3ff) >> 2;
    uint16_t r3 = ((r >> 10) & 0x3ff) >> 2;
    uint16_t r4 = (r & 0x3ff) >> 2;

    uint64_t g = byte_64(buffer[2]) << 0 |
    byte_64(buffer[6]) << 8 |
    byte_64(buffer[10]) << 16 |
    byte_64(buffer[14]) << 24 |
    byte_64(buffer[18]) << 32;
    uint16_t g1 = g >> 30 >> 2;
    uint16_t g2 = ((g >> 20) & 0x3ff) >> 2;
    uint16_t g3 = ((g >> 10) & 0x3ff) >> 2;
    uint16_t g4 = (g & 0x3ff) >> 2;

    uint64_t b = byte_64(buffer[3]) << 0 |
    byte_64(buffer[7]) << 8 |
    byte_64(buffer[11]) << 16 |
    byte_64(buffer[15]) << 24 |
    byte_64(buffer[19]) << 32;
    uint16_t b1 = b >> 30 >> 2;
    uint16_t b2 = ((b >> 20) & 0x3ff) >> 2;
    uint16_t b3 = ((b >> 10) & 0x3ff) >> 2;
    uint16_t b4 = (b & 0x3ff) >> 2;
    // 将R、G、B值放入像素数组中
    if (colorspace == 0) {
        pixels[0] = b1;
        pixels[1] = g1;
        pixels[2] = r1;

        pixels[3] = b2;
        pixels[4] = g2;
        pixels[5] = r2;

        pixels[6] = b3;
        pixels[7] = g3;
        pixels[8] = r3;

        pixels[9] = b4;
        pixels[10] = g4;
        pixels[11] = r4;
    } else {
        int y1 = g1;
        int u1 = b1;
        int v1 = r1;
        int y2 = g2;
        int u2 = b2;
        int v2 = r2;
        int y3 = g3;
        int u3 = b3;
        int v3 = r3;
        int y4 = g4;
        int u4 = b4;
        int v4 = r4;

        int R1 = static_cast<int>(y1 + 1.402 * (v1- 128));
        int G1 = static_cast<int>(y1 - 0.344136 * (u1 - 128) - 0.714136 * (v1 - 128));
        int B1 = static_cast<int>(y1 + 1.772 * (u1 - 128));

        // 将结果限制在0-255范围内
        R1 = clamp(R1, 0, 255);
        G1 = clamp(G1, 0, 255);
        B1 = clamp(B1, 0, 255);

        int R2 = static_cast<int>(y2 + 1.402 * (v2- 128));
        int G2 = static_cast<int>(y2 - 0.344136 * (u2 - 128) - 0.714136 * (v2 - 128));
        int B2 = static_cast<int>(y2 + 1.772 * (u2 - 128));

        // 将结果限制在0-255范围内
        R2 = clamp(R2, 0, 255);
        G2 = clamp(G2, 0, 255);
        B2 = clamp(B2, 0, 255);

        int R3 = static_cast<int>(y3 + 1.402 * (v3- 128));
        int G3 = static_cast<int>(y3 - 0.344136 * (u3 - 128) - 0.714136 * (v3 - 128));
        int B3 = static_cast<int>(y3 + 1.772 * (u3 - 128));

        // 将结果限制在0-255范围内
        R3 = clamp(R3, 0, 255);
        G3 = clamp(G3, 0, 255);
        B3 = clamp(B3, 0, 255);

        int R4 = static_cast<int>(y4 + 1.402 * (v4- 128));
        int G4 = static_cast<int>(y4 - 0.344136 * (u4 - 128) - 0.714136 * (v4 - 128));
        int B4 = static_cast<int>(y4 + 1.772 * (u4 - 128));

        // 将结果限制在0-255范围内
        R4 = clamp(R3, 0, 255);
        G4 = clamp(G3, 0, 255);
        B4 = clamp(B3, 0, 255);

        pixels[0] = B1;
        pixels[1] = G1;
        pixels[2] = R1;

        pixels[3] = B2;
        pixels[4] = G2;
        pixels[5] = R2;

        pixels[6] = B3;
        pixels[7] = G3;
        pixels[8] = R3;

        pixels[9] = B4;
        pixels[10] = G4;
        pixels[11] = R4;
    }
    return pixels;
}

// 转换20字节缓冲区为4个像素的函数
std::vector<uint8_t> process_buffer_12(const uint8_t* buffer, int colorspace) {
    std::vector<uint8_t> pixels(6);
    // 提取R、G、B分量
        // 注意：这里我们实际上只使用了每个颜色通道的前8位，忽略了低2位
    uint8_t byte1 = reverse_byte(buffer[1]);
    uint8_t byte2 = reverse_byte(buffer[5]);
    uint8_t byte3 = reverse_byte(buffer[9]);
    uint16_t r1 = (byte1 << 4) | (byte2 >> 4);
    r1 = uint8_t(r1 & 0xff);
    r1 = reverse_byte(r1);
    uint16_t r2 = ((byte2 & 0xff) << 4) | byte3;
    r2 = uint8_t(r2 & 0xff);
    r2 = reverse_byte(r2);

    byte1 = reverse_byte(buffer[2]);
    byte2 = reverse_byte(buffer[6]);
    byte3 = reverse_byte(buffer[10]);
    uint16_t g1 = (byte1 << 4) | (byte2 >> 4);
    g1 = uint8_t(g1 & 0xff);
    g1 = reverse_byte(g1);
    uint16_t g2 = ((byte2 & 0xff) << 4) | byte3;
    g2 = uint8_t(g2 & 0xff);
    g2 = reverse_byte(g2);

    byte1 = reverse_byte(buffer[3]);
    byte2 = reverse_byte(buffer[7]);
    byte3 = reverse_byte(buffer[11]);
    uint16_t b1 = (byte1 << 4) | (byte2 >> 4);
    b1 = uint8_t(b1 & 0xff);
    b1 = reverse_byte(b1);
    uint16_t b2 = ((byte2 & 0xff) << 4) | byte3;
    b2 = uint8_t(b2 & 0xff);
    b2 = reverse_byte(b2);
    // 将R、G、B值放入像素数组中
    if (colorspace == 0) {
        pixels[0] = b1;
        pixels[1] = g1;
        pixels[2] = r1;

        pixels[3] = b2;
        pixels[4] = g2;
        pixels[5] = r2;
    } else {
        int y1 = g1;
        int u1 = b1;
        int v1 = r1;
        int y2 = g2;
        int u2 = b2;
        int v2 = r2;

        int R1 = static_cast<int>(y1 + 1.402 * (v1- 128));
        int G1 = static_cast<int>(y1 - 0.344136 * (u1 - 128) - 0.714136 * (v1 - 128));
        int B1 = static_cast<int>(y1 + 1.772 * (u1 - 128));

        // 将结果限制在0-255范围内
        R1 = clamp(R1, 0, 255);
        G1 = clamp(G1, 0, 255);
        B1 = clamp(B1, 0, 255);

        int R2 = static_cast<int>(y2 + 1.402 * (v2- 128));
        int G2 = static_cast<int>(y2 - 0.344136 * (u2 - 128) - 0.714136 * (v2 - 128));
        int B2 = static_cast<int>(y2 + 1.772 * (u2 - 128));

        // 将结果限制在0-255范围内
        R2 = clamp(R2, 0, 255);
        G2 = clamp(G2, 0, 255);
        B2 = clamp(B2, 0, 255);

        pixels[0] = B1;
        pixels[1] = G1;
        pixels[2] = R1;

        pixels[3] = B2;
        pixels[4] = G2;
        pixels[5] = R2;
    }

    return pixels;
}

// 创建BMP文件的函数
void hdmi_create_bmp_from_12bit_data(const std::string& inputBinFile, const std::string& outputBmpFile, int hactive, int vactive, int colorspace) {
    std::ifstream infile(inputBinFile, std::ios::binary);
    if (!infile) {
        std::cerr << "Error opening input file!" << std::endl;
        return;
    }

    std::vector<uint8_t> pixel_data;
    uint8_t buffer[12];

    while (infile.read(reinterpret_cast<char*>(buffer), sizeof(buffer))) {
        std::vector<uint8_t> pixels = process_buffer_12(buffer, colorspace);
        pixel_data.insert(pixel_data.end(), pixels.begin(), pixels.end());
    }

    infile.close();

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    memset(&fileHeader, 0, sizeof(fileHeader));
    memset(&infoHeader, 0, sizeof(infoHeader));

    fileHeader.bfType = 0x4D42; // "BM"
    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = hactive;
    infoHeader.biHeight = vactive;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biSizeImage = pixel_data.size();

    fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + infoHeader.biSizeImage;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);

    std::ofstream outfile(outputBmpFile, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening output file!" << std::endl;
        return;
    }

    int row_size = (infoHeader.biWidth * 3 + 3) & ~3;

    std::vector<uint8_t> flipped_pixel_data(pixel_data.size());

    for (int y = 0; y < infoHeader.biHeight; ++y) {
        int source_index = (infoHeader.biHeight - 1 - y) * row_size;
        int dest_index = y * row_size;
        memcpy(flipped_pixel_data.data() + dest_index, pixel_data.data() + source_index, row_size);
    }
    outfile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    outfile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    outfile.write(reinterpret_cast<const char*>(flipped_pixel_data.data()), pixel_data.size());

    outfile.close();
}

void hdmi_create_bmp_from_10bit_data(const std::string& inputBinFile, const std::string& outputBmpFile, int hactive, int vactive, int colorspace) {
    std::ifstream infile(inputBinFile, std::ios::binary);
    if (!infile) {
        std::cerr << "Error opening input file!" << std::endl;
        return;
    }

    std::vector<uint8_t> pixel_data;
    uint8_t buffer[20];

    while (infile.read(reinterpret_cast<char*>(buffer), sizeof(buffer))) {
        std::vector<uint8_t> pixels = process_buffer_10(buffer, colorspace);
        pixel_data.insert(pixel_data.end(), pixels.begin(), pixels.end());
    }

    infile.close();

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    memset(&fileHeader, 0, sizeof(fileHeader));
    memset(&infoHeader, 0, sizeof(infoHeader));

    fileHeader.bfType = 0x4D42; // "BM"
    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = hactive;
    infoHeader.biHeight = vactive;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biSizeImage = pixel_data.size();

    fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + infoHeader.biSizeImage;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);

    std::ofstream outfile(outputBmpFile, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening output file!" << std::endl;
        return;
    }

    int row_size = (infoHeader.biWidth * 3 + 3) & ~3;

    // 创建一个新的数组来存储翻转后的像素数据
    std::vector<uint8_t> flipped_pixel_data(pixel_data.size());

    // 复制并翻转像素数据
    for (int y = 0; y < infoHeader.biHeight; ++y) {
        int source_index = (infoHeader.biHeight - 1 - y) * row_size;
        int dest_index = y * row_size;
        memcpy(flipped_pixel_data.data() + dest_index, pixel_data.data() + source_index, row_size);
    }
    outfile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    outfile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    outfile.write(reinterpret_cast<const char*>(flipped_pixel_data.data()), pixel_data.size());

    // 确保文件已正确关闭
    outfile.close();
}


void hdmi_create_bmp_from_8bit_data(const std::string& inputBinFile, const std::string& outputBmpFile, int width, int height, int colorspace) {
    std::ifstream binFile(inputBinFile, std::ios::binary);
    if (!binFile.is_open()) {
        std::cerr << "Error: The file '" << inputBinFile << "' could not be opened." << std::endl;
        return;
    }
    int x;
    std::vector<std::vector<uint8_t>> allRows(height);
    for (int y = height - 1; y >= 0; --y) {
        allRows[y].resize(width * 4);
        if (!binFile.read(reinterpret_cast<char*>(allRows[y].data()), width * 4)) {
            std::cerr << "Error: Not enough data in '" << inputBinFile << "' to fill the image 1." << std::endl;
            return;
        }
    }
    binFile.clear();
    binFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> bgrData;
    if (colorspace == 3)
       bgrData.resize(width * 2 * height * 3);
    else
       bgrData.resize(width * height * 3);
    for (int y = height - 1; y >= 0; --y) {
        std::vector<uint8_t> rowBuffer(width * 4);
        if (!binFile.read(reinterpret_cast<char*>(rowBuffer.data()), rowBuffer.size())) {
            std::cerr << "Error: Not enough data in '" << inputBinFile << "' to fill the image." << std::endl;
            return;
        }
        for (x = 0; x < width;) {
            if (colorspace == 0) {
                bgrData[(y * width + x) * 3 + 0] = rowBuffer[(x * 4 + 3)]; // B
                bgrData[(y * width + x) * 3 + 1] = rowBuffer[(x * 4 + 2)]; // G
                bgrData[(y * width + x) * 3 + 2] = rowBuffer[(x * 4 + 1)]; // R
                ++x;
            } else if (colorspace == 2) {
                uint8_t Y = rowBuffer[x * 4 + 2];
                uint8_t U = rowBuffer[x * 4 + 3];
                uint8_t V = rowBuffer[x * 4 + 1];

                // YUV到RGB的转换公式
                int R = static_cast<int>(Y + 1.402 * (V - 128));
                int G = static_cast<int>(Y - 0.344136 * (U - 128) - 0.714136 * (V - 128));
                int B = static_cast<int>(Y + 1.772 * (U - 128));

                // 将结果限制在0-255范围内
                R = clamp(R, 0, 255);
                G = clamp(G, 0, 255);
                B = clamp(B, 0, 255);

                // 将RGB值存储到rgbData中
                int rgbIndex = (y * width + x) * 3;
                bgrData[rgbIndex] = static_cast<uint8_t>(B); // B
                bgrData[rgbIndex + 1] = static_cast<uint8_t>(G); // G
                bgrData[rgbIndex + 2] = static_cast<uint8_t>(R); //
                ++x;
            }else if (colorspace == 1) {
                uint16_t Y1 = (rowBuffer[x * 4 + 2] << 4) | (rowBuffer[x * 4 + 3] & 0xf);
                uint16_t Cb = (rowBuffer[x * 4 + 1] << 4) | (rowBuffer[x * 4 + 3] >> 4);
                uint16_t Y2 = (rowBuffer[x * 4 + 6] << 4) | (rowBuffer[x * 4 + 7] & 0xf);
                uint16_t Cr = (rowBuffer[x * 4 + 5] << 4) | (rowBuffer[x * 4 + 7] >> 4);
                int y1 = Y1 >> 4;
                int u1 = Cb >> 4;
                int v1 = Cr >> 4;
                int y2 = Y2 >> 4;
                int u2 = Cb >> 4;
                int v2 = Cr >> 4;

                int R1 = static_cast<int>(y1 + 1.402 * (v1 - 128));
                int G1 = static_cast<int>(y1 - 0.344136 * (u1 - 128) - 0.714136 * (v1 - 128));
                int B1 = static_cast<int>(y1 + 1.772 * (u1 - 128));;
                R1 = clamp(R1, 0, 255);
                G1 = clamp(G1, 0, 255);
                B1 = clamp(B1, 0, 255);
                int R2 = static_cast<int>(y2 + 1.402 * (v2 - 128));
                int G2 = static_cast<int>(y2 - 0.344136 * (u2 - 128) - 0.714136 * (v2 - 128));
                int B2 = static_cast<int>(y2 + 1.772 * (u2 - 128));
                R2 = clamp(R2, 0, 255);
                G2 = clamp(G2, 0, 255);
                B2 = clamp(B2, 0, 255);
                int rgbIndex = (y * width + x) * 3;
                bgrData[rgbIndex] = static_cast<uint8_t>(B1);
                bgrData[rgbIndex + 1] = static_cast<uint8_t>(G1);
                bgrData[rgbIndex + 2] = static_cast<uint8_t>(R1);
                bgrData[rgbIndex + 3] = static_cast<uint8_t>(B2);
                bgrData[rgbIndex + 4] = static_cast<uint8_t>(G2);
                bgrData[rgbIndex + 5] = static_cast<uint8_t>(R2);
                x = x + 2;
            }else if (colorspace == 3) {
				 // 获取当前像素的数据
                uint8_t Y1 = rowBuffer[x * 4 + 2]; // 第一个Y分量
                uint8_t Y2 = rowBuffer[x * 4 + 1]; // 第二个Y分量
                
                // 根据行号确定UV分量的来源
                uint8_t U, V;
                int currentRowIndex = y; // 转换为正向行索引
                
                if (currentRowIndex % 2 == 0) { // 偶数行
                    // 当前行存储U分量，下一行存储V分量
                    V = allRows[y][x * 4 + 3]; // 当前行的U
                    // 确保不越界访问
                    if (currentRowIndex + 1 < height) {
                        U = allRows[currentRowIndex + 1][x * 4 + 3]; // 下一行的V
                    } else {
                        U = 128; // 如果下一行不存在，使用默认值
                    }
                } else { // 奇数行
                    // 当前行存储V分量，上一行存储U分量
                    U = allRows[y][x * 4 + 3]; // 当前行的V
                    // 确保不越界访问
                    if (currentRowIndex - 1 >= 0) {
                        V = allRows[currentRowIndex - 1][x * 4 + 3]; // 上一行的U
                    } else {
                        V = 128; // 如果上一行不存在，使用默认值
                    }
                }
                
                // YUV到RGB转换
                int R1 = static_cast<int>(Y1 + 1.402 * (V - 128));
                int G1 = static_cast<int>(Y1 - 0.344136 * (U - 128) - 0.714136 * (V - 128));
                int B1 = static_cast<int>(Y1 + 1.772 * (U - 128));
                
                int R2 = static_cast<int>(Y2 + 1.402 * (V - 128));
                int G2 = static_cast<int>(Y2 - 0.344136 * (U - 128) - 0.714136 * (V - 128));
                int B2 = static_cast<int>(Y2 + 1.772 * (U - 128));
                
                // 钳制值到0-255范围
                R1 = std::max(0, std::min(255, R1));
                G1 = std::max(0, std::min(255, G1));
                B1 = std::max(0, std::min(255, B1));
                R2 = std::max(0, std::min(255, R2));
                G2 = std::max(0, std::min(255, G2));
                B2 = std::max(0, std::min(255, B2));
                
                // 计算像素位置
                int pixelIndexBase = y * (width * 2) + (x * 2);
                int rgbIndex1 = pixelIndexBase * 3;
                int rgbIndex2 = (pixelIndexBase + 1) * 3;
                
                // 存储RGB值
                bgrData[rgbIndex1] = static_cast<uint8_t>(B1);
                bgrData[rgbIndex1 + 1] = static_cast<uint8_t>(G1);
                bgrData[rgbIndex1 + 2] = static_cast<uint8_t>(R1);
                
                bgrData[rgbIndex2] = static_cast<uint8_t>(B2);
                bgrData[rgbIndex2 + 1] = static_cast<uint8_t>(G2);
                bgrData[rgbIndex2 + 2] = static_cast<uint8_t>(R2);
                
                x = x + 1;
            }
        }
    }
    binFile.close();

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    fileHeader.bfType = 0x4D42;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    infoHeader.biSize = sizeof(BMPInfoHeader);
    if (colorspace == 3)
        infoHeader.biWidth = width * 2;
    else
        infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = infoHeader.biWidth * height * 3;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    fileHeader.bfSize = fileHeader.bfOffBits + infoHeader.biSizeImage;
    std::ofstream bmpFile(outputBmpFile, std::ios::binary);
    if (!bmpFile.is_open()) {
        std::cerr << "Error: The file '" << outputBmpFile << "' could not be created." << std::endl;
        return;
    } else {
        std::cout<<"bmp file open" << std::endl;
    }

    bmpFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(BMPFileHeader));
    bmpFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(BMPInfoHeader));
    bmpFile.write(reinterpret_cast<const char*>(bgrData.data()), bgrData.size());

    bmpFile.close();
    std::cout << "BMP file created successfully 8bit: " << outputBmpFile << std::endl;
}
