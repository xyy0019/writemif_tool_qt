#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QByteArray>
#include <QImage>
#include <QCheckBox>
#include <QComboBox>
#include <QRegExpValidator>
#include <QRegExp>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QApplication>
#include <QTimer>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include "mainwindow.h"
#include "parse_hdmi.h"
QString fileName;
void MainWindow::onAboutClicked()
{
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("关于");

    QVBoxLayout *layout = new QVBoxLayout(&aboutDialog);

    // 添加 logo
    QLabel *logoLabel = new QLabel(&aboutDialog);
    QPixmap logoPixmap(":/res/OIP-C.jfif");
    logoPixmap = logoPixmap.scaled(200, 200, Qt::KeepAspectRatio);
    logoLabel->setPixmap(logoPixmap);
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    // 添加信息
    QLabel *infoLabel = new QLabel("这是一个图像数据解析工具，作者：yaoyu.xu 联系方式（vx）18800207492", &aboutDialog);
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    aboutDialog.setLayout(layout);
    aboutDialog.exec();
}

void MainWindow::onOpenFileClicked()
{
    fileName = QFileDialog::getOpenFileName(this, "Open File", "", "All Files (*)");
    if (fileName.isEmpty())
        return;
    filePathLineEdit->setText(fileName);
}

void MainWindow::displayScaledImage(const QImage image)
{
    // 确保label使用当前背景色填充
    imageLabel->setAutoFillBackground(true);
    QPalette palette = imageLabel->palette();
    palette.setColor(imageLabel->backgroundRole(), Qt::white);
    imageLabel->setPalette(palette);

    // 获取当前label大小
    QSize labelSize = imageLabel->size();

    // 直接缩放并显示图像，跳过创建空白图像的步骤
    QImage scaledImage = image.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 优化：使用QPixmap::scaled instead of QImage::scaled (可能更快)
    // QPixmap pixmap = QPixmap::fromImage(image).scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // imageLabel->setPixmap(pixmap);

    imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
    imageLabel->setAlignment(Qt::AlignCenter);

    // 确保布局更新
    imageLabel->update();
}

//*********************************************transfer start***********************************************
inline uint8_t clip(int value) {
    return static_cast<uint8_t>(std::min(std::max(value, 0), 255));
}

int64_t get_file_size(std::ifstream &f) {
    f.seekg(0, std::ios::end);
    int64_t s = (int64_t)f.tellg();
    f.seekg(0, std::ios::beg);
    return s;
}

void reverse_line_blocks(std::vector<char> &line) {
    const size_t block = 8;
    size_t full_blocks = line.size() / block;
    for (size_t b = 0; b < full_blocks; ++b) {
        size_t off = b * block;
        std::reverse(line.begin() + off, line.begin() + off + block);
    }
}

QByteArray convertLittleEndian10bit(const QByteArray& input) {
    QByteArray output;
    output.reserve(input.size()); // 输出大小与输入相同

    const uint8_t* inData = reinterpret_cast<const uint8_t*>(input.constData());
    size_t dataSize = input.size();

    for (size_t i = 0; i + 4 < dataSize; i += 5) {
        uint8_t buf[5];
        memcpy(buf, inData + i, 5);

        uint32_t y1 = buf[0] | ((buf[1] & 0x03) << 8);
        uint32_t y2 = ((buf[1] >> 2) & 0x3F) | ((buf[2] & 0x0F) << 6);
        uint32_t y3 = ((buf[2] >> 4) & 0x0F) | ((buf[3] & 0x3F) << 4);
        uint32_t y4 = ((buf[3] >> 6) & 0x03) | ((uint32_t)buf[4] << 2);

        uint64_t packed = ((uint64_t)y1) | ((uint64_t)y2 << 10)
                        | ((uint64_t)y3 << 20) | ((uint64_t)y4 << 30);

        uint8_t out[5];
        out[0] = (packed >> 32) & 0xFF;
        out[1] = (packed >> 24) & 0xFF;
        out[2] = (packed >> 16) & 0xFF;
        out[3] = (packed >> 8)  & 0xFF;
        out[4] = (packed >> 0)  & 0xFF;

        output.append(reinterpret_cast<char*>(out), 5);
    }

    // 处理尾部不足5字节的数据（直接复制）
    size_t remaining = dataSize % 5;
    if (remaining > 0) {
        output.append(reinterpret_cast<const char*>(inData + dataSize - remaining), remaining);
        qDebug() << "警告: 输入大小不是5的倍数，保留尾部" << remaining << "字节";
    }

    return output;
}

void reverse_data(QByteArray& fileData, int width, int height, float bytes_per_pixel) {
    const int bytes_per_block = 8;
    const int total_blocks = (width * height * bytes_per_pixel) / bytes_per_block;

    if (fileData.size() % bytes_per_block != 0) {
        throw std::runtime_error("Error: Data size is not a multiple of 8 bytes.");
    }

    for (int i = 0; i < total_blocks; ++i) {
        for (int j = 0; j < bytes_per_block / 2; ++j) {
            char temp = static_cast<char>(fileData[i * bytes_per_block + j] & 0xFF);
            fileData[i * bytes_per_block + j] = static_cast<char>(fileData[i * bytes_per_block + (bytes_per_block - 1 - j)] & 0xFF);
            fileData[i * bytes_per_block + (bytes_per_block - 1 - j)] = temp;
        }
    }
}

QByteArray convertRgb10ToRgb8(const QByteArray& inputData, int width, int height) {
    QImage image(width, height, QImage::Format_RGB888);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixelIndex = (y * width + x) * 4;

            uint8_t byte1 = static_cast<uint8_t>(inputData[pixelIndex]);
            uint8_t byte2 = static_cast<uint8_t>(inputData[pixelIndex + 1]);
            uint8_t byte3 = static_cast<uint8_t>(inputData[pixelIndex + 2]);
            uint8_t byte4 = static_cast<uint8_t>(inputData[pixelIndex + 3]);
            uint16_t r = ((byte1 << 4) | (byte2 >> 4)) >> 2;
            uint16_t g = (((byte2 & 0x0F) << 6) | (byte3 >> 2)) >> 2;
            uint16_t b = (((byte3 & 0x03) << 8) | byte4) >> 2;
            uint8_t r8 = r & 0xff;
            uint8_t g8 = g & 0xff;
            uint8_t b8 = b & 0xff;

            image.setPixel(x, y, qRgb(r8, g8, b8));
        }
    }

    QByteArray outputData;
    const uchar* bits = image.bits();
    int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < height; ++y) {
        outputData.append(reinterpret_cast<const char*>(bits + y * bytesPerLine), bytesPerLine);
    }

    return outputData;
}

QByteArray convertyuv444_10_To_8(const QByteArray& inputData, int width, int height) {
    QImage image(width, height, QImage::Format_RGB888);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixelIndex = (y * width + x) * 4;

            uint8_t byte1 = static_cast<uint8_t>(inputData[pixelIndex]);
            uint8_t byte2 = static_cast<uint8_t>(inputData[pixelIndex + 1]);
            uint8_t byte3 = static_cast<uint8_t>(inputData[pixelIndex + 2]);
            uint8_t byte4 = static_cast<uint8_t>(inputData[pixelIndex + 3]);
            uint16_t Y = ((byte1 << 4) | (byte2 >> 4)) >> 2;
            uint16_t U = (((byte2 & 0x0F) << 6) | (byte3 >> 2)) >> 2;
            uint16_t V = (((byte3 & 0x03) << 8) | byte4) >> 2;
            uint8_t y8 = Y & 0xff;
            uint8_t u8 = U & 0xff;
            uint8_t v8 = V & 0xff;

            image.setPixel(x, y, qRgb(y8, u8, v8));
        }
    }

    QByteArray outputData;
    const uchar* bits = image.bits();
    int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < height; ++y) {
        outputData.append(reinterpret_cast<const char*>(bits + y * bytesPerLine), bytesPerLine);
    }

    return outputData;
}

QByteArray convertyuv444_8_ToRgb(const QByteArray& inputData, int width, int height) {
    QImage image(width, height, QImage::Format_RGB888);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int yuv_index = y * width * 3 + x * 3;

            uint8_t Y = static_cast<uint8_t>(inputData[yuv_index + 0]);
            uint8_t U = static_cast<uint8_t>(inputData[yuv_index + 1]);
            uint8_t V = static_cast<uint8_t>(inputData[yuv_index + 2]);

            //601
            int R = clip(1.164 * (Y - 16) + 1.596 * (V - 128));
            int G = clip(1.164 * (Y - 16) - 0.813 * (V - 128) - 0.391 * (U - 128));
            int B = clip(1.164 * (Y - 16) + 2.018 * (U - 128));

            /*709
            r = 1.164 * (y - black) + 1.793 * (v - half);
            g = 1.164 * (y - black) - 0.534 * (v - half) - 0.213 * (u - half);
            b = 1.164 * (y - black) + 2.115 * (u - half);
            */
            uint8_t r8 = clip(R);
            uint8_t g8 = clip(G);
            uint8_t b8 = clip(B);

            image.setPixel(x, y, qRgb(r8, g8, b8));
        }
    }

    QByteArray outputData;
    const uchar* bits = image.bits();
    int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < height; ++y) {
        outputData.append(reinterpret_cast<const char*>(bits + y * bytesPerLine), bytesPerLine);
    }

    return outputData;
}

QByteArray converty_8_ToRgb(const QByteArray& inputData, int width, int height) {
    QImage image(width, height, QImage::Format_RGB888);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int yuv_index = y * width + x;
            qDebug() << yuv_index;
            uint8_t Y = static_cast<uint8_t>(inputData[yuv_index]);
            uint8_t U = 128;
            uint8_t V = 128;

            //601
            int R = clip(1.164 * (Y - 16) + 1.596 * (V - 128));
            int G = clip(1.164 * (Y - 16) - 0.813 * (V - 128) - 0.391 * (U - 128));
            int B = clip(1.164 * (Y - 16) + 2.018 * (U - 128));

            /*709
            r = 1.164 * (y - black) + 1.793 * (v - half);
            g = 1.164 * (y - black) - 0.534 * (v - half) - 0.213 * (u - half);
            b = 1.164 * (y - black) + 2.115 * (u - half);
            */
            uint8_t r8 = clip(R);
            uint8_t g8 = clip(G);
            uint8_t b8 = clip(B);

            image.setPixel(x, y, qRgb(r8, g8, b8));
        }
    }

    QByteArray outputData;
    const uchar* bits = image.bits();
    int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < height; ++y) {
        outputData.append(reinterpret_cast<const char*>(bits + y * bytesPerLine), bytesPerLine);
    }

    return outputData;
}

QByteArray convertyuv422_8_To_444(const QByteArray& inputData, int width, int height) {
    QByteArray outputData(width * height * 3, '\0');
    uchar* outputPtr = reinterpret_cast<uchar*>(outputData.data());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 2) {
            int yuv_index = y * width * 2 + x * 2;

            uint8_t Y1_422 = inputData[yuv_index + 0];
            uint8_t U = inputData[yuv_index + 1];
            uint8_t Y2_422 = inputData[yuv_index + 2];
            uint8_t V = inputData[yuv_index + 3];

            //uint8_t V = inputData[yuv_index + 0];
            //uint8_t Y1_422 = inputData[yuv_index + 1];
            //uint8_t U = inputData[yuv_index + 2];
            //uint8_t Y2_422 = inputData[yuv_index + 3];

            *outputPtr++ = Y1_422;
            *outputPtr++ = U;
            *outputPtr++ = V;

            *outputPtr++ = Y2_422;
            *outputPtr++ = U;
            *outputPtr++ = V;
        }
    }

    return outputData;
}

QByteArray convertyuv422_10_To_444(const QByteArray& inputData, int width, int height) {
    QByteArray outputData(width * height * 3, '\0');
    uchar* outputPtr = reinterpret_cast<uchar*>(outputData.data());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 2) {
            int yuv_index = y * width * 5 / 2 + x * 5 / 2;

            uint8_t byte1 = inputData[yuv_index + 0];
            uint8_t byte2 = inputData[yuv_index + 1];
            uint8_t byte3 = inputData[yuv_index + 2];
            uint8_t byte4 = inputData[yuv_index + 3];
            uint8_t byte5 = inputData[yuv_index + 4];

            uint16_t y1 = byte1;
            uint16_t u = (((byte2 & 0x3f) << 4) | (byte3 >> 4)) >> 2;
            uint16_t y2 = (((byte3 & 0x0f) << 6) | (byte4 >> 2)) >> 2;
            uint16_t v = (((byte4 & 0x03) << 8) | byte5) >> 2;
            *outputPtr++ = y1 & 0xff;
            *outputPtr++ = u & 0xff;
            *outputPtr++ = v & 0xff;

            *outputPtr++ = y2 & 0xff;
            *outputPtr++ = u & 0xff;
            *outputPtr++ = v & 0xff;
        }
    }

    return outputData;
}

QByteArray convertYuv422sp_10_To_YUYV(const QByteArray& inputData, int width, int height) {
    // 检查输入数据长度是否符合要求
    int expectedSize = width * height * 2.5;
    if (inputData.size() != expectedSize) {
        return QByteArray(); // 输入数据长度不符合要求，返回空字节数组
    }

    // 计算输出数据的大小，YUYV 每个像素对需要 4 个字节
    QByteArray outputData(width * height * 2, '\0');
    uchar* outputPtr = reinterpret_cast<uchar*>(outputData.data());

    // 计算输入数据中 Y 分量的字节数
    int ySize = width * height * 10 / 8;
    // 计算 UV 分量的字节数
    int uvSize = width * height * 10 / 8;

    // 存储 Y 分量的数组
    uchar* yData = new uchar[width * height];
    int yIndex = 0;

    // 处理 Y 分量
    for (int yuv_index = 0; yuv_index < ySize; yuv_index += 5) {
        uint8_t byte1 = inputData[yuv_index + 0];
        uint8_t byte2 = inputData[yuv_index + 1];
        uint8_t byte3 = inputData[yuv_index + 2];
        uint8_t byte4 = inputData[yuv_index + 3];
        uint8_t byte5 = inputData[yuv_index + 4];

        // 拼成 40 位的数
        uint64_t combined = static_cast<uint64_t>(byte1) << 32 |
                            static_cast<uint64_t>(byte2) << 24 |
                            static_cast<uint64_t>(byte3) << 16 |
                            static_cast<uint64_t>(byte4) << 8 |
                            static_cast<uint64_t>(byte5);

        // 按照 10 位去分成 4 个 y
        int y1 = (combined >> 30) & 0x3FF;
        int y2 = (combined >> 20) & 0x3FF;
        int y3 = (combined >> 10) & 0x3FF;
        int y4 = combined & 0x3FF;

        // y 再去右移两位转成 8 位
        yData[yIndex++] = static_cast<uchar>(y1 >> 2);
        yData[yIndex++] = static_cast<uchar>(y2 >> 2);
        yData[yIndex++] = static_cast<uchar>(y3 >> 2);
        yData[yIndex++] = static_cast<uchar>(y4 >> 2);
    }

    // 存储 UV 分量的数组
    uchar* uvData = new uchar[width * height];
    int uvDataIndex = 0;

    // 处理 UV 分量
    int uvIndex = ySize;
    for (int i = 0; i < uvSize; i += 5) {
        uint8_t byte1 = inputData[uvIndex + 0];
        uint8_t byte2 = inputData[uvIndex + 1];
        uint8_t byte3 = inputData[uvIndex + 2];
        uint8_t byte4 = inputData[uvIndex + 3];
        uint8_t byte5 = inputData[uvIndex + 4];

        // 拼成 40 位的数
        uint64_t combined = static_cast<uint64_t>(byte1) << 32 |
                            static_cast<uint64_t>(byte2) << 24 |
                            static_cast<uint64_t>(byte3) << 16 |
                            static_cast<uint64_t>(byte4) << 8 |
                            static_cast<uint64_t>(byte5);

        // 按照 10 位去分成 4 个分量（2 个 U 和 2 个 V 交替）
        for (int j = 0; j < 4; j += 2) {
            int u = (combined >> ((3 - j) * 10)) & 0x3FF;
            int v = (combined >> ((2 - j) * 10)) & 0x3FF;
            uvData[uvDataIndex++] = static_cast<uchar>(u >> 2);
            uvData[uvDataIndex++] = static_cast<uchar>(v >> 2);
        }
        uvIndex += 5;
    }

    // 将 Y、U、V 按照 YUYV 格式存储到输出数据中
    for (int i = 0; i < width * height / 2; ++i) {
        *outputPtr++ = yData[i * 2];
        *outputPtr++ = uvData[i * 2 + 1];
        *outputPtr++ = yData[i * 2 + 1];
        *outputPtr++ = uvData[i * 2];
    }

    // 释放动态分配的内存
    delete[] yData;
    delete[] uvData;

    return outputData;
}

QByteArray convert_yu12_8_To_rgb(const QByteArray& inputData, int width, int height) {
    QByteArray outputData(width * height * 3, '\0');
    uchar* rgbData = reinterpret_cast<uchar*>(outputData.data());

    size_t rgbIndex = 0;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int index_y = i * width + j;
            int index_u = width * height + (i / 2) * (width / 2) + (j / 2);
            int index_v = index_u + (width * height / 4);
            unsigned char y = inputData[index_y];
            unsigned char u = inputData[index_u];
            unsigned char v = inputData[index_v];

            int r = static_cast<int>(y + 1.402 * (v - 128));
            int g = static_cast<int>(y - 0.344136 * (u - 128) - 0.714136 * (v - 128));
            int b = static_cast<int>(y + 1.772 * (u - 128));

            r = clip(r);
            g = clip(g);
            b = clip(b);

            rgbData[rgbIndex++] = static_cast<unsigned char>(r);
            rgbData[rgbIndex++] = static_cast<unsigned char>(g);
            rgbData[rgbIndex++] = static_cast<unsigned char>(b);
       }
    }

    return outputData;
}

QByteArray convert_yv12_8_To_rgb(const QByteArray& inputData, int width, int height) {
    QByteArray outputData(width * height * 3, '\0');
    uchar* rgbData = reinterpret_cast<uchar*>(outputData.data());

    size_t rgbIndex = 0;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int index_y = i * width + j;
            int index_v = width * height + (i / 2) * (width / 2) + (j / 2);
            int index_u = index_v + (width * height / 4);
            unsigned char y = inputData[index_y];
            unsigned char u = inputData[index_u];
            unsigned char v = inputData[index_v];

            int r = static_cast<int>(y + 1.402 * (v - 128));
            int g = static_cast<int>(y - 0.344136 * (u - 128) - 0.714136 * (v - 128));
            int b = static_cast<int>(y + 1.772 * (u - 128));

            r = clip(r);
            g = clip(g);
            b = clip(b);

            rgbData[rgbIndex++] = static_cast<unsigned char>(r);
            rgbData[rgbIndex++] = static_cast<unsigned char>(g);
            rgbData[rgbIndex++] = static_cast<unsigned char>(b);
       }
    }

    return outputData;
}

QByteArray convert_nv21_8_To_rgb(const QByteArray& inputData, int width, int height) {
    QByteArray outputData(width * height * 3, '\0');
    uchar* rgbData = reinterpret_cast<uchar*>(outputData.data());

    size_t rgbIndex = 0;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int index_y = i * width + j;
            int index_v = width * height + (i / 2) * width + (j / 2) * 2;
            int index_u = width * height + (i / 2) * width + (j / 2) * 2 + 1;
            unsigned char y = inputData[index_y];
            unsigned char u = inputData[index_u];
            unsigned char v = inputData[index_v];

            int r = static_cast<int>(y + 1.402 * (v - 128));
            int g = static_cast<int>(y - 0.344136 * (u - 128) - 0.714136 * (v - 128));
            int b = static_cast<int>(y + 1.772 * (u - 128));

            r = clip(r);
            g = clip(g);
            b = clip(b);

            rgbData[rgbIndex++] = static_cast<unsigned char>(r);
            rgbData[rgbIndex++] = static_cast<unsigned char>(g);
            rgbData[rgbIndex++] = static_cast<unsigned char>(b);
       }
    }

    return outputData;
}

QByteArray convert_nv12_8_To_rgb(const QByteArray& inputData, int width, int height) {
    QByteArray outputData(width * height * 3, '\0');
    uchar* rgbData = reinterpret_cast<uchar*>(outputData.data());

    size_t rgbIndex = 0;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int index_y = i * width + j;
            int index_u = width * height + (i / 2) * width + (j / 2) * 2;
            int index_v = width * height + (i / 2) * width + (j / 2) * 2 + 1;
            unsigned char y = inputData[index_y];
            unsigned char u = inputData[index_u];
            unsigned char v = inputData[index_v];

            int r = static_cast<int>(y + 1.402 * (v - 128));
            int g = static_cast<int>(y - 0.344136 * (u - 128) - 0.714136 * (v - 128));
            int b = static_cast<int>(y + 1.772 * (u - 128));

            r = clip(r);
            g = clip(g);
            b = clip(b);

            rgbData[rgbIndex++] = static_cast<unsigned char>(r);
            rgbData[rgbIndex++] = static_cast<unsigned char>(g);
            rgbData[rgbIndex++] = static_cast<unsigned char>(b);
       }
    }

    return outputData;
}

QByteArray convert_nv_10_To_8(const QByteArray& inputData) {
    if (inputData.size() % 5 != 0) {
        std::cerr << "输入数据大小不是5的倍数！" << std::endl;
        return QByteArray();
    }

    std::vector<unsigned char> outputData;
    outputData.reserve(inputData.size() * 4 / 5);

    for (int i = 0; i < inputData.size(); i += 5) {
        unsigned char byte1 = inputData[i];
        unsigned char byte2 = inputData[i + 1];
        unsigned char byte3 = inputData[i + 2];
        unsigned char byte4 = inputData[i + 3];
        unsigned char byte5 = inputData[i + 4];

        uint16_t y1 = byte1;
        uint16_t y2 = (((byte2 & 0x3f) << 4) | (byte3 >> 4)) >> 2;
        uint16_t y3 = (((byte3 & 0x0f) << 6) | (byte4 >> 2)) >> 2;
        uint16_t y4 = (((byte4 & 0x03) << 8) | byte5) >> 2;

        unsigned char outputByte1 = y1 & 0xFF;
        unsigned char outputByte2 = y2 & 0xFF;
        unsigned char outputByte3 = y3 & 0xFF;
        unsigned char outputByte4 = y4 & 0xFF;
        outputData.push_back(outputByte1);
        outputData.push_back(outputByte2);
        outputData.push_back(outputByte3);
        outputData.push_back(outputByte4);
    }

    QByteArray outputArray(reinterpret_cast<const char*>(outputData.data()), outputData.size());
    return outputArray;
}

//*********************************************transfer end***********************************************


void MainWindow::transfer_dsc()
{
    // 禁用按钮防止重复点击
    transferButton->setEnabled(false);
    qApp->processEvents(); // 立即更新UI

    // 创建线程（如果不存在）
    if (!m_workerThread) {
        m_workerThread = new QThread(this);
        connect(m_workerThread, &QThread::finished, [this](){
            m_workerThread->deleteLater();
            m_workerThread = nullptr;
            transferButton->setEnabled(true); // 线程结束后恢复按钮
        });
    }

    // 获取参数（必须在主线程获取）
    QString inputFile = filePathLineEdit->text();
    int bitsPerComponent = bpcComboBox->currentText().toInt();

    // 使用 lambda 在子线程执行任务
    QObject *worker = new QObject();
    worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, worker, [=](){
        QFileInfo inputFileInfo(inputFile);
        QString inputDir = inputFileInfo.absolutePath();  // 输入文件所在目录
        QString fileName = inputFileInfo.fileName();      // 输入文件名

        // 在输入文件目录创建配置文件
        QString listFilePath = inputDir + "/test_list.txt";
        QString cfgFilePath = inputDir + "/test_dsc_1_1.cfg";

        // 1. 创建测试列表文件
        QFile listFile(listFilePath);
        if (!listFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            emit decodeFailed("无法创建测试列表文件");
            return;
        }
        listFile.write(fileName.toUtf8() + "\n");  // 只写入文件名（相对路径）
        listFile.close();

        // 2. 创建配置文件
        QFile configFile(cfgFilePath);
        if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            emit decodeFailed("无法创建配置文件");
            return;
        }
        QTextStream(&configFile) << "DSC_VERSION_MINOR\t1\n"
                                << "FUNCTION  2\n"
                                << "SRC_LIST\t\ttest_list.txt\n"  // 相对路径
                                << "DPX_FILE_OUTPUT    0\n"
                                << "PPM_FILE_OUTPUT 1\n"
                                << "BITS_PER_COMPONENT " << bitsPerComponent << "\n";
        configFile.close();

        // 3. 启动进程
        QProcess process;
        process.setProgram("dsc.exe");  // 依赖系统PATH或工作目录
        process.setArguments({"-F", "test_dsc_1_1.cfg"});
        process.setWorkingDirectory(inputDir);  // 关键：设置工作目录

        // 4. 自动清理临时文件（可选）
        QObject::connect(
            &process,
            static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=]() { // 注意：lambda 可以忽略参数
                QFile::remove(listFilePath);
                QFile::remove(cfgFilePath);
            }
        );

        process.start();
        if (!process.waitForFinished(-1)) {
            emit decodeFailed("解码失败: " + process.errorString());
            return;
        }

        // 生成结果文件名
        QString resultFile = inputFile;
        resultFile.chop(4);
        resultFile.append(".out.ppm");

        emit decodeCompleted(resultFile); // 发送完成信号
        // ----------------- 子线程操作结束 -----------------

        worker->deleteLater(); // 清理临时对象
    });

    // 连接信号
    connect(worker, &QObject::destroyed, m_workerThread, &QThread::quit); // 自动退出线程

    // 处理结果信号（需要回到主线程）
    connect(this, &MainWindow::decodeCompleted, this, [this](const QString &path){
        QImage image(path);
        if (image.isNull()) {
            QMessageBox::warning(this, "错误", "无法加载生成的图像");
        } else {
            displayScaledImage(image);
        }
    });

    connect(this, &MainWindow::decodeFailed, this, [this](const QString &msg){
        QMessageBox::critical(this, "错误", msg);
    });

    // 启动线程
    m_workerThread->start();
}

//****************************************************hdmi start*************************************************
void MainWindow::playVideoWithFFmpeg(const QString &videoPath) {
    // 确保视频文件存在
    if (!QFile::exists(videoPath)) {
        QMessageBox::warning(this, "播放失败", "视频文件不存在！");
        return;
    }

    // 创建一个新的QProcess用于播放视频
    QProcess *playerProcess = new QProcess(this);


    // 构建FFmpeg播放命令
    QStringList args;

    // 检查系统类型，选择合适的播放方式
    #ifdef Q_OS_WIN
    // Windows系统: 直接使用ffplay
    args << "-i" << videoPath;
    playerProcess->start("ffplay", args);
    #else
    // Linux/macOS系统: 使用ffplay
    args << "-i" << videoPath << "-autoexit";
    playerProcess->start("ffplay", args);
    #endif

    qDebug() << "执行播放命令:" << "ffplay" << args.join(" ");

    // 启动FFmpeg播放进程
    if (!playerProcess->waitForStarted()) {
        QMessageBox::critical(this, "播放失败", "无法启动FFmpeg播放器！");
        delete playerProcess;
    } else {
        qDebug() << "视频播放已启动...";
    }
}

void MainWindow::transfer_hdmi()
{
    // 获取输入文件路径
    QString inputPath = filePathLineEdit->text().trimmed();
    if (inputPath.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入有效的文件路径！");
        return;
    }

    imageLabel->clear();

    // 检查文件是否存在
    QFile inputFile(inputPath);
    if (!inputFile.exists()) {
        QMessageBox::warning(this, "文件不存在", "指定的输入文件不存在，请检查路径！");
        return;
    }

    tmdsclk = tmdsclkLineEdit->text().toInt();
    int frameCount;
    if (tmdsclk == 0) {
        frameCount = 1;
    } else {
        frameCount = inputFile.size() / tmdsclk;
        if (frameCount == 0) {
            QMessageBox::warning(this, "输入错误", "tmdsclk err!");
            return;
        }
    }

    qint64 fileSize = inputFile.size();
    hdmifilesize = fileSize;
    qint64 bytesPerFrame = fileSize / frameCount;

    // 校验帧数和文件大小
    if (frameCount <= 0 || bytesPerFrame <= 0) {
        QMessageBox::warning(this, "参数错误", "帧数必须大于0，且文件大小需能被帧数整除！");
        return;
    }

    // 确保输出目录存在
    QDir outputDir(QDir::currentPath() + "/hdmi");
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            QMessageBox::critical(this, "目录创建失败", "无法创建输出目录！");
            return;
        }
    }

    // 打开输入文件
    if (!inputFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "文件打开失败", "无法读取输入文件！");
        return;
    }

    // 循环拆分并处理每一帧
    for (int i = 0; i < frameCount; i++) {
        // 计算当前帧的起始位置和大小
        qint64 startPos = i * bytesPerFrame;
        qint64 frameSize = (i == frameCount - 1) ?
                          fileSize - startPos :  // 最后一帧处理剩余数据
                          bytesPerFrame;

        // 定位到当前帧起始位置
        if (inputFile.seek(startPos) != true) {
            QMessageBox::critical(this, QString("帧定位失败"),
                                QString("无法定位到第 %1 帧的起始位置！").arg(i+1));
            inputFile.close();
            return;
        }

        // 读取当前帧数据
        QByteArray frameData = inputFile.read(frameSize);
        if (frameData.size() != frameSize) {
            QMessageBox::critical(this, QString("帧读取失败"),
                                QString("无法完整读取第 %1 帧的数据！").arg(i+1));
            inputFile.close();
            return;
        }

        // 创建临时文件存储当前帧数据
        QString tempFilePath = QDir::tempPath() + QString("/frame_%1.tmp").arg(i);
        QFile tempFile(tempFilePath);
        if (!tempFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, QString("临时文件创建失败"),
                                QString("无法创建第 %1 帧的临时文件！").arg(i+1));
            inputFile.close();
            tempFile.remove(); // 清理已创建的临时文件
            return;
        }
        if (tempFile.write(frameData) != frameData.size()) {
            QMessageBox::critical(this, QString("临时文件写入失败"),
                                QString("无法写入第 %1 帧的临时文件！").arg(i+1));
            inputFile.close();
            tempFile.remove();
            return;
        }
        tempFile.close();

        // 构建输出路径（例如：output_frame_0.bmp, output_frame_1.bmp）
        QString outputPath = QDir::currentPath() + QString("/hdmi/output_frame_%1.bmp").arg(i);
        QByteArray inputBa = tempFilePath.toLocal8Bit();
        QByteArray outputBa = outputPath.toLocal8Bit();

        // 调用hdmi_parse处理当前帧
        int result = hdmi_parse(
            const_cast<char*>(inputBa.constData()),
            const_cast<char*>(outputBa.constData())
        );

        // 关闭输入文件
        tempFile.remove(); // 清理临时文件

        if (result != 0) {
            QMessageBox::critical(this, QString("第 %1 帧处理失败").arg(i+1),
                                QString("错误代码：%1").arg(result));
            return; // 这里选择失败时终止整个流程，如需继续处理可改为标记错误并继续
        }

        // 验证输出文件是否存在和可读
        QFile outputFile(outputPath);
        if (!outputFile.exists()) {
            QMessageBox::critical(this, QString("文件不存在"),
                                QString("第 %1 帧的BMP文件未生成！").arg(i+1));
            qDebug() << "生成的BMP文件不存在:" << outputPath;
            continue;
        }

        if (!outputFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, QString("文件无法打开"),
                                QString("第 %1 帧的BMP文件无法打开！").arg(i+1));
            qDebug() << "无法打开生成的BMP文件:" << outputPath;
            continue;
        }

        // 验证文件大小是否合理（BMP文件通常至少有54字节的文件头）
        qint64 bmpSize = outputFile.size();
        outputFile.close();

        if (bmpSize < 54) {
            QMessageBox::critical(this, QString("文件格式错误"),
                                QString("第 %1 帧的BMP文件大小异常！").arg(i+1));
            qDebug() << "生成的BMP文件大小异常:" << outputPath << "大小:" << bmpSize;
            continue;
        }

        qDebug() << "第" << i+1 << "帧处理完成";
    }

    inputFile.close();

    QString firstFramePath = QDir::currentPath() + QString("/hdmi/output_frame_0.bmp");
    QImage firstFrameImage(firstFramePath);

    if (!firstFrameImage.isNull()) {
        originalImage = firstFrameImage;
        displayScaledImage(originalImage); // 显示第一帧

        // 启用帧导航（如果存在多帧）
        if (frameCount > 1) {
            hdmi_enableFrameNavigation();
        }
    } else {
        QMessageBox::warning(this, "图像加载失败",
                            QString("无法加载第一帧图像：%1").arg(firstFramePath));
        imageLabel->clear();
    }

    htotalLineEdit->setText(QString("%1").arg(q_htotal / q_colordepth * 4));
    vtotalLineEdit->setText(QString("%1").arg(q_vtotal));
    hactiveLineEdit->setText(QString("%1").arg(q_hactive / q_colordepth * 4));
    vactiveLineEdit->setText(QString("%1").arg(q_vactive));
    hdmiColorDepthEdit->setText(QString("%1").arg(q_colordepth * 2));
    hdmiColorSpaceEdit->setText(QString("%1").arg(q_colorspace));

    // 验证所有BMP文件是否存在
    QStringList bmpFiles;
    for (int i = 0; i < frameCount; i++) {
        QString bmpPath = QDir::currentPath() + QString("/hdmi/output_frame_%1.bmp").arg(i);
        if (!QFile::exists(bmpPath)) {
            QMessageBox::critical(this, "文件缺失", QString("第 %1 帧BMP文件不存在！").arg(i+1));
            return;
        }
        bmpFiles.append(bmpPath);
    }

    // 生成FFmpeg输入文件列表
    QString ffmpegListPath = QDir::currentPath() + "/hdmi/ffmpeg_input_list.txt";
    QFile ffmpegListFile(ffmpegListPath);
    if (!ffmpegListFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "文件创建失败", "无法创建FFmpeg输入列表文件！");
        return;
    }

    QTextStream out(&ffmpegListFile);
    for (const QString &bmpPath : bmpFiles) {
        // 正确转义路径中的引号和特殊字符
        QString mutableBmpPath = bmpPath; // 创建可变副本
        mutableBmpPath.replace("'", "'\\''"); // 调用非const版本
        QString escapedPath = mutableBmpPath;
        out << "file '" << escapedPath << "'\n";
        out << "duration 0.04\n"; // 假设25fps，每帧持续0.04秒
    }
    ffmpegListFile.close();

    // 构建正确的FFmpeg命令
    /*QString mp4Path = QDir::currentPath() + "/output.mp4";
    QStringList args;
    args << "-y"                         // 覆盖已存在文件
         << "-f" << "concat"             // 使用concat demuxer
         << "-safe" << "0"               // 允许包含绝对路径
         << "-i" << ffmpegListPath       // 输入文件列表
         << "-c:v" << "libx264"          // 视频编码
         << "-pix_fmt" << "yuv420p"      // 像素格式
         << "-vf" << "scale=trunc(iw/2)*2:trunc(ih/2)*2" // 适配分辨率
         << mp4Path;

    // 检查ffmpeg可执行文件是否存在
    QFileInfo ffmpegInfo("./ffmpeg/bin");
    if (!ffmpegInfo.exists() || !ffmpegInfo.isExecutable()) {
        QMessageBox::critical(this, "FFmpeg缺失", "找不到或无法执行FFmpeg可执行文件！");
        return;
    }*/

    // 执行FFmpeg合并
    //QProcess ffmpeg;
    //qDebug() << "执行命令:" << "./ffmpeg/bin/ffmpeg" << args.join(" ");
    //ffmpeg.start("./ffmpeg/bin/ffmpeg", args);

    /*if (!ffmpeg.waitForStarted()) {
        QMessageBox::critical(this, "FFmpeg启动失败", "无法启动FFmpeg进程！");
        return;
    }

    if (!ffmpeg.waitForFinished(-1)) {
        QMessageBox::critical(this, "FFmpeg执行超时", "FFmpeg处理时间过长！");
        return;
    }

    // 检查退出状态
    if (ffmpeg.exitCode() != 0) {
        QString errorOutput = ffmpeg.readAllStandardError();
        qDebug() << "FFmpeg错误输出:" << errorOutput;
        QMessageBox::critical(this, "FFmpeg处理失败",
                            QString("FFmpeg处理失败，错误代码: %1\n\n%2")
                            .arg(ffmpeg.exitCode())
                            .arg(errorOutput));
        return;
    }*/

    // 验证输出文件是否生成
    /*if (QFile::exists(mp4Path)) {
        QMessageBox::information(this, "处理完成", "视频已成功生成！");
    } else {
        QMessageBox::warning(this, "生成失败", "视频生成失败，未找到输出文件！");
    }
    playVideoWithFFmpeg(mp4Path);
    // 清理临时文件
    QFile::remove(ffmpegListPath);*/
}

// 启用HDMI帧导航（在类中声明为private函数）
void MainWindow::hdmi_enableFrameNavigation()
{
    // 更新帧信息
    hdmi_updateFrameInfo();
}

void MainWindow::hdmi_updateFrameDisplay()
{
    int frameIndex = hdmi_currentFrameIndex;
    // 校验索引范围
    if (frameIndex < 0) {
        qDebug() << "错误：帧索引不能为负数";
        return;
    }

    // 构建文件路径
    QString framePath = QDir::currentPath() + QString("/hdmi/output_frame_%1.bmp").arg(frameIndex);
    QFile file(framePath);

    // 检查文件是否存在
    if (!file.exists()) {
        qDebug() << "错误：帧文件不存在 - " << framePath;
        imageLabel->clear();
        return;
    }

    // 直接从文件加载图像
    QImage frameImage(framePath);
    if (frameImage.isNull()) {
        qDebug() << "错误：无法加载帧图像 - " << framePath;
        imageLabel->clear();
        return;
    }

    // 显示图像并更新信息
    displayScaledImage(frameImage); // 保持原有显示逻辑
    hdmi_updateFrameInfo(); // 建议传递索引以便更新信息（如果需要）
    qDebug() << "成功加载并显示第" << frameIndex + 1 << "帧";
}

// 更新HDMI帧信息（在类中声明为private函数）
void MainWindow::hdmi_updateFrameInfo()
{
    int totalFrames = hdmifilesize / tmdsclk;
    hdmi_frameInfoLabel->setText(
        QString("HDMI帧: %1/%2")
            .arg(hdmi_currentFrameIndex + 1)
            .arg(totalFrames)
    );
}

//****************************************************hdmi end*************************************************

//****************************************************bin start*************************************************
void MainWindow::onSaveImageClicked()
{
    int currentIndex = frameSlider->value();
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image as BMP", "", "Bitmap Files (*.bmp)");
    originalImage = frameImages[currentIndex];
    if (!fileName.isEmpty()) {
        if (!originalImage.save(fileName, "bmp")) {
            QMessageBox::warning(this, "Error", "Failed to save image.");
        }
    }
}

void MainWindow::onColorSpaceChanged() {
    QString selectedColorSpace = colorSpaceComboBox->currentText();
    if (selectedColorSpace == "rgba" ||
        selectedColorSpace == "yuv420p(YV12)" ||
        selectedColorSpace == "yuv420p(YU12)") {
        colorDepthComboBox->clear();
        colorDepthComboBox->addItem("8");
    } else {
        colorDepthComboBox->clear();
        QStringList colorDepthOptions = {"8", "10"};
        colorDepthComboBox->addItems(colorDepthOptions);
        colorDepthComboBox->setEnabled(true);
    }
}

bool saveFramesToRgbBin(const QList<QImage> &frames, const QString &fileName) {
    if (frames.isEmpty()) {
        qDebug() << "Error: No frames to save.";
        return false;
    }

    int width = frames.first().width();
    int height = frames.first().height();

    for (const QImage &frame : frames) {
        if (frame.width() != width || frame.height() != height) {
            qDebug() << "Error: All frames must have the same dimensions.";
            return false;
        }

        // 确保所有帧都是RGB888格式
        if (frame.format() != QImage::Format_RGB888) {
            qDebug() << "Error: Frame is not in RGB888 format";
            return false;
        }
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error: Cannot open file for writing:" << fileName;
        return false;
    }

    // 计算每行有效数据大小（无填充）
    const int lineSize = width * 3;

    for (const QImage &frame : frames) {
        // 直接处理RGB888数据，无需转换
        const uchar *pixelData = frame.bits();
        int bytesPerLine = frame.bytesPerLine();

        // 逐行写入，跳过可能的行填充
        for (int y = 0; y < height; ++y) {
            const uchar *line = pixelData + y * bytesPerLine;
            file.write(reinterpret_cast<const char*>(line), lineSize);
        }
    }

    file.close();
    qDebug() << "Successfully saved" << frames.size()
             << "frames to" << fileName
             << "Size:" << file.size() << "bytes";
    return true;
}

void convertRgbToMp4(const QString &rgbFilePath, const QString &mp4FilePath, int width, int height, int fps = 25) {
    // 构建FFmpeg命令
    QStringList args;
    args << "-y"  // 覆盖已存在的文件
         << "-f" << "rawvideo"
         << "-pix_fmt" << "rgb24"
         << "-s:v" << QString("%1x%2").arg(width).arg(height)
         << "-r" << QString::number(fps)
         << "-i" << rgbFilePath
         // 添加时间基设置，确保输出视频使用指定的帧率
         << "-video_track_timescale" << QString::number(fps)
         // 使用更明确的帧率设置选项
         << "-r" << QString::number(fps)
         << "-c:v" << "libx264"
         << "-preset" << "medium"  // 设置编码预设
         << "-tune" << "animation"  // 针对动画优化
         << "-pix_fmt" << "yuv420p"
         << mp4FilePath;

    // 执行FFmpeg进程
    QProcess ffmpegProcess;
    ffmpegProcess.start("./ffmpeg/bin/ffmpeg", args);

    if (!ffmpegProcess.waitForStarted()) {
        qDebug() << "Error: FFmpeg启动失败，请确保已安装并添加到PATH";
        return;
    }

    if (!ffmpegProcess.waitForFinished(-1)) {
        qDebug() << "Error: FFmpeg处理超时";
        return;
    }

    if (ffmpegProcess.exitCode() != 0) {
        qDebug() << "Error: FFmpeg转换失败:" << ffmpegProcess.readAllStandardError();
        return;
    }

    qDebug() << "Success: 视频转换完成:" << mp4FilePath;
}

void MainWindow::transfer_bin()
{
    bool widthOk, heightOk, padOk,frameOk;
    int width = widthLineEdit->text().toInt(&widthOk);
    if (width % 2 != 0) {
        QMessageBox::warning(this, "Error", "不支持width奇数");
        return;
    }
    int height = heightLineEdit->text().toInt(&heightOk);
    if (height % 2 != 0) {
        QMessageBox::warning(this, "Error", "不支持height奇数");
    }
    int padded_line_bytes = padLineEdit->text().toInt(&padOk);
    int frameCount = frameLineEdit->text().toInt(&frameOk); // 新增：帧数输入
    bool need_crop = cropCheckBox->isChecked();
    bool little = littleCheckBox->isChecked();
    QString rgbFilePath = QDir::currentPath() + "/output.rgb";
    QString mp4FilePath = QDir::currentPath() + "/output.mp4";

    if (!widthOk || !heightOk || !frameOk || frameCount <= 0) {
        QMessageBox::warning(this, "Invalid Input", "请输入有效的分辨率和帧数。");
        return;
    }

    QString colorspace = colorSpaceComboBox->currentText();
    QString colorDepthStr = colorDepthComboBox->currentText();
    int colordepth = colorDepthStr.toInt();
    float bytes_per_pixel = 0;
    int reverse = reverseCheckBox->isChecked();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Cannot open file: " + file.errorString());
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    int frameSize = calculateFrameSize(width, height, padded_line_bytes, colorspace, colordepth);
    if (frameSize < 0) {
        QMessageBox::warning(this, "Error", "不支持的颜色空间或位深度");
        return;
    }
    qDebug () << frameSize;
    int totalSize = frameSize * frameCount;
    if (totalSize > fileData.size()) {
        QMessageBox::warning(this, "Warning", "文件数据不足，无法提取指定帧数");
        return;
    }

    frameImages.clear();
    imageLabel->setPixmap(QPixmap());
    scaledImages.clear();
    for (int i = 0; i < frameCount; i++) {
        QByteArray frameData = fileData.mid(i * frameSize, frameSize);

        if (colorspace == "rgb") {
            if (colordepth == 8) {
                bytes_per_pixel = 3;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            } else if (colordepth == 10) {
                bytes_per_pixel = 4;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convertRgb10ToRgb8(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            }
        } else if (colorspace == "rgba") {
            bytes_per_pixel = 4;
            if (width * height * bytes_per_pixel > frameData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(frameData, width, height, bytes_per_pixel);
            frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_ARGB32).copy();
        } else if (colorspace == "yuv444") {
            if (colordepth == 8) {
                bytes_per_pixel = 3;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convertyuv444_8_ToRgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            } else if (colordepth == 10) {
                bytes_per_pixel = 4;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convertyuv444_10_To_8(frameData, width, height);
                frameData = convertyuv444_8_ToRgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            }
        } else if (colorspace == "yuv422") {
            if (colordepth == 8) {
                bytes_per_pixel = 2;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convertyuv422_8_To_444(frameData, width, height);
                frameData = convertyuv444_8_ToRgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            } else if (colordepth == 10) {
                bytes_per_pixel = 2.5;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convertyuv422_10_To_444(frameData, width, height);
                frameData = convertyuv444_8_ToRgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            }
        } else if (colorspace == "yuv420p(YU12)") {
            bytes_per_pixel = 1.5;
            if (width * height * bytes_per_pixel > frameData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(frameData, width, height, bytes_per_pixel);
            frameData = convert_yu12_8_To_rgb(frameData, width, height);
            frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colorspace == "yuv420p(YV12)") {
            bytes_per_pixel = 1.5;
            if (width * height * bytes_per_pixel > frameData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(frameData, width, height, bytes_per_pixel);
            frameData = convert_yv12_8_To_rgb(frameData, width, height);
            frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colorspace == "yuv420sp(NV21)") {
            if (colordepth == 8) {
                bytes_per_pixel = 1.5;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convert_nv21_8_To_rgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            } else if (colordepth == 10) {
                bytes_per_pixel = 1.875;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convert_nv_10_To_8(frameData);
                frameData = convert_nv12_8_To_rgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            }
        } else if (colorspace == "yuv420sp(NV12)") {
            if (colordepth == 8) {
                bytes_per_pixel = 1.5;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convert_nv12_8_To_rgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            } else if (colordepth == 10) {
                bytes_per_pixel = 1.875;
                if (width * height * bytes_per_pixel > frameData.size()) {
                    QMessageBox::warning(this, "Warning", "No enough data.");
                    return;
                }
                if (reverse)
                    reverse_data(frameData, width, height, bytes_per_pixel);
                frameData = convert_nv_10_To_8(frameData);
                frameData = convert_nv21_8_To_rgb(frameData, width, height);
                frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
            }
        } else if (colorspace == "yuv422sp") {
            bytes_per_pixel = 2.5;
            if (width * height * bytes_per_pixel > frameData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(frameData, width, height, bytes_per_pixel);
            frameData = convertYuv422sp_10_To_YUYV(frameData, width, height);
            frameData = convertyuv422_8_To_444(frameData, width, height);
            frameData = convertyuv444_8_ToRgb(frameData, width, height);
            frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colorspace == "dpss_yuv422sp") {
            // 计算有效行字节数（原始数据，无填充）和需要裁剪的字节数
               int raw_line_bytes = width * 10 / 8;  // 10bit数据的有效行字节数
               int lineSize = padded_line_bytes;  // 带填充的原始行字节数（正确的行大小）
               int total_lines = 0;
               // 计算总行数：根据数据大小判断平面数
               if (padded_line_bytes)
                   total_lines = frameData.size() / padded_line_bytes;
               if (total_lines && total_lines != height && total_lines != height * 2) {
                   QMessageBox::warning(this, "Warning", "Data size does not match expected lines.");
                   return;
               }

               // 统一使用frameBuffer处理，确保整个作用域可访问
               std::vector<char> frameBuffer(frameData.begin(), frameData.end());
               bool bufferModified = false;

               // 处理反转
               if (reverse) {
                   // 逐行处理反转（基于带填充的原始行大小），处理所有行
                   for (int y = 0; y < total_lines; y++) {
                       size_t lineStart = y * lineSize;  // 正确的行起始位置（基于带填充的行大小）
                       // 提取完整的一行（带填充）
                       std::vector<char> line(frameBuffer.begin() + lineStart,
                                             frameBuffer.begin() + lineStart + lineSize);

                       reverse_line_blocks(line);  // 反转处理
                       std::copy(line.begin(), line.end(), frameBuffer.begin() + lineStart);
                   }
                   bufferModified = true;
               }

               // 处理裁剪（固定执行，按你的逻辑1表示需要裁剪）
               if (need_crop) {
                   std::vector<char> croppedBuffer;
                   croppedBuffer.reserve(int64_t(raw_line_bytes) * total_lines);  // 预分配裁剪后的总大小

                   for (int y = 0; y < total_lines; y++) {
                       size_t lineStart = y * lineSize;  // 基于带填充的行大小计算起始位置
                       // 提取带填充的完整行（可能已反转）
                       std::vector<char> line(frameBuffer.begin() + lineStart,
                                             frameBuffer.begin() + lineStart + lineSize);

                       // 关键：裁剪掉填充部分，只保留有效数据（前raw_line_bytes字节）
                       line.resize(raw_line_bytes);  // 保留有效数据，去掉后面的crop_bytes

                       // 将裁剪后的行添加到新缓冲区
                       croppedBuffer.insert(croppedBuffer.end(), line.begin(), line.end());
                   }

                   frameBuffer = std::move(croppedBuffer);  // 替换为裁剪后的数据
                   bufferModified = true;
               }

               // 更新frameData（仅当数据被修改时）
               if (bufferModified)
               frameData = QByteArray(frameBuffer.data(), static_cast<int>(frameBuffer.size()));
               // 小端转换
               if (little)
                   frameData = convertLittleEndian10bit(frameData);
               // 后续格式转换（确保裁剪后的尺寸与转换逻辑匹配）
               bytes_per_pixel = 2.5;
               if (width * height * bytes_per_pixel > frameData.size()) {
                   QMessageBox::warning(this, "Warning", "No enough data.");
                   return;
               }
               frameData = convertYuv422sp_10_To_YUYV(frameData, width, height);
               frameData = convertyuv422_8_To_444(frameData, width, height);
               frameData = convertyuv444_8_ToRgb(frameData, width, height);
               frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()),
                                  width, height, QImage::Format_RGB888).copy();
        } else if (colorspace == "y") {
            bytes_per_pixel = 1;
            if (width * height * bytes_per_pixel > frameData.size()) {
                qDebug() << frameData.size();
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(frameData, width, height, bytes_per_pixel);
            frameData = converty_8_ToRgb(frameData, width, height);
            frameImage = QImage(reinterpret_cast<const uchar*>(frameData.constData()), width, height, QImage::Format_RGB888).copy();
        }

        if (!frameImage.isNull())
            frameImages.append(frameImage);
    }
    bool success = saveFramesToRgbBin(frameImages, rgbFilePath);
    if (success) {
        convertRgbToMp4(rgbFilePath, mp4FilePath, width, height, fpsLineEdit->text().toInt());
        qDebug() << "RGB binary file saved successfully!";
    } else {
        qDebug() << "Failed to save RGB binary file!";
    }

    // 显示处理结果
    if (frameCount == 1 && !frameImages.isEmpty()) {
        // 单帧直接显示
        originalImage = frameImages.first();
        displayScaledImage(originalImage);
    } else if (frameCount > 1) {
        // 多帧显示第一帧，并启用帧浏览功能
        if (!frameImages.isEmpty()) {
            originalImage = frameImages.first();
            displayScaledImage(originalImage);
            enableFrameNavigation(frameCount); // 新增：启用帧导航功能
        }
    }

}

// 新增：计算帧大小的辅助函数
int MainWindow::calculateFrameSize(int width, int height, int padded_line_bytes, QString& colorspace, int colordepth)
{
    float bytesPerPixel = 0;

    if (colorspace == "rgb") {
        bytesPerPixel = (colordepth == 8) ? 3 : 4;
    } else if (colorspace == "rgba") {
        bytesPerPixel = 4;
    } else if (colorspace == "yuv444") {
        bytesPerPixel = (colordepth == 8) ? 3 : 4;
    } else if (colorspace == "yuv422") {
        bytesPerPixel = (colordepth == 8) ? 2 : 2.5;
    } else if (colorspace == "yuv420p(YU12)" || colorspace == "yuv420p(YV12)" ||
               colorspace == "yuv420sp(NV21)" || colorspace == "yuv420sp(NV12)") {
        bytesPerPixel = (colordepth == 8) ? 1.5 : 1.875;
    } else if (colorspace == "yuv422sp" || colorspace == "dpss_yuv422sp") {
        bytesPerPixel = 2.5;
    } else if (colorspace == "y"){
        bytesPerPixel = 1;
    } else {
        return -1; // 不支持的格式
    }
    if (colorspace == "dpss_yuv422sp" && padded_line_bytes)
        return padded_line_bytes * height * 8 / 12 * 3;
    else
        return width * height * bytesPerPixel;
}

// 新增：帧导航功能
void MainWindow::enableFrameNavigation(int frameCount)
{
    // 显示帧导航控件
    frameSlider->setRange(0, frameCount - 1);
    frameSlider->setValue(0);
    frameSlider->show();

    frameLabel->setText(QString("当前帧: 1/%1").arg(frameCount));
    frameLabel->show();

    prevFrameButton->show();
    nextFrameButton->show();

    disconnect(prevFrameButton, &QPushButton::clicked, this, &MainWindow::showPreviousFrame);
    disconnect(nextFrameButton, &QPushButton::clicked, this, &MainWindow::showNextFrame);
    // 连接信号槽
    //connect(frameSlider, &QSlider::valueChanged, this, &MainWindow::displayFrame);
    connect(prevFrameButton, &QPushButton::clicked, this, &MainWindow::showPreviousFrame);
    connect(nextFrameButton, &QPushButton::clicked, this, &MainWindow::showNextFrame);
}

// 新增：显示指定帧
void MainWindow::displayFrame(int frameIndex)
{
    if (frameIndex >= 0 && frameIndex < frameImages.size()) {
        originalImage = frameImages[frameIndex];
        displayScaledImage(originalImage);
        qDebug() << frameIndex;
        frameLabel->setText(QString("当前帧: %1/%2").arg(frameIndex + 1).arg(frameImages.size()));
        frameSlider->setValue(frameIndex);
    }
}

// 新增：显示上一帧
void MainWindow::showPreviousFrame()
{
    int currentIndex = frameSlider->value();
    if (currentIndex > 0) {
        displayFrame(currentIndex - 1);
    }
}

// 新增：显示下一帧
void MainWindow::showNextFrame()
{
    int currentIndex = frameSlider->value();
    if (currentIndex < frameImages.size() - 1) {
        displayFrame(currentIndex + 1);
    }
}

void MainWindow::togglePlay() {
    if (frameTimer->isActive()) {
        frameTimer->stop();
        playButton->setText("播放");
    } else {
        if (frameImages.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先解析帧数据");
            return;
        }

        // 初始化缩放图像缓存
        scaledImages.resize(frameImages.size());

        int fps = fpsLineEdit->text().toInt();
        if (fps <= 0) fps = 25;
        frameTimer->setInterval(1000 / fps);
        currentPlayingFrame = 0;
        frameTimer->start();
        playButton->setText("暂停");
        updateCurrentFrame(currentPlayingFrame);
    }
}

void MainWindow::playNextFrame() {
    currentPlayingFrame++;
    if (currentPlayingFrame >= frameImages.size()) {
        currentPlayingFrame = 0; // 循环播放
    }
    updateCurrentFrame(currentPlayingFrame);
    frameSlider->setValue(currentPlayingFrame); // 更新滑块位置
}

// 确保 updateCurrentFrame 函数已实现（更新图像显示）
void MainWindow::updateCurrentFrame(int index) {
    if (index < 0 || index >= frameImages.size()) return;
    scaledImages.resize(frameImages.size());

    scaledImages[index] = frameImages[index].scaled(
        imageLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    // 关闭自动缩放，使用预缩放的图像
    imageLabel->setScaledContents(false);

    // 在一个单独的绘制操作中更新图像
    QPixmap pixmap = QPixmap::fromImage(scaledImages[index]);
    imageLabel->setPixmap(pixmap);

    // 更新帧标签
    frameLabel->setText(QString("当前帧: %1/%2").arg(index+1).arg(frameImages.size()));

    // 强制立即更新
    imageLabel->update();
}
