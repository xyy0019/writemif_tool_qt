#include "mainwindow.h"
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
#include <iostream>

QString fileName;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , openFileButton(new QPushButton("Open File", this))
    , saveImageButton(new QPushButton("Save as BMP", this))
    , transferButton(new QPushButton("transfer", this))
    , imageLabel(new QLabel(this))
    , originalImage()
    , filePathLineEdit(new QLineEdit(this))
    , filePathQlabel(new QLabel("bin文件路径:", this))
    , widthLineEdit(new QLineEdit(this))
    , heightLineEdit(new QLineEdit(this))
    , reverseCheckBox(new QCheckBox("Reverse", this))
    , colorSpaceComboBox(new QComboBox(this))
    , colorDepthComboBox(new QComboBox(this))
{
    this->setWindowTitle("图像数据解析 Author by yaoyu.xu");
    this->setWindowIcon(QIcon(":/res/logo.png"));
    // 设置窗口初始大小
    this->setFixedSize(600, 400);

    // 设置 color space 选项
    QStringList colorSpaceOptions = {"yuv422", "yuv444", "rgb", "rgba", "yuv420p(YV12)",
                                     "yuv420p(YU12)", "yuv420sp(NV12)", "yuv420sp(NV21)"};
    colorSpaceComboBox->addItems(colorSpaceOptions);

    // 设置 color depth 选项
    QStringList colorDepthOptions = {"8", "10"};
    colorDepthComboBox->addItems(colorDepthOptions);

    // 创建一个水平布局用于文件路径输入框、打开文件按钮
    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(filePathQlabel);
    horizontalLayout->addWidget(filePathLineEdit);
    horizontalLayout->addWidget(openFileButton);
    horizontalLayout->addWidget(transferButton);
    // 创建一个水平布局用于分辨率、颜色空间和颜色深度设置以及反转复选框
    QHBoxLayout *horizontalLayout1 = new QHBoxLayout;
    QLabel *resolutionLabel = new QLabel("分辨率:", this);
    horizontalLayout1->addWidget(resolutionLabel);
    horizontalLayout1->addWidget(widthLineEdit);
    horizontalLayout1->addWidget(heightLineEdit);

    QLabel *colorSpaceLabel = new QLabel("Color Space:", this);
    QLabel *colorDepthLabel = new QLabel("Color Depth:", this);
    horizontalLayout1->addWidget(colorSpaceLabel);
    horizontalLayout1->addWidget(colorSpaceComboBox);
    horizontalLayout1->addWidget(colorDepthLabel);
    horizontalLayout1->addWidget(colorDepthComboBox);
    horizontalLayout1->addWidget(reverseCheckBox);
    horizontalLayout1->addWidget(saveImageButton);  // 添加保存按钮
    // 创建一个垂直布局用于包含两个水平布局、保存按钮和图像标签
    QVBoxLayout *verticalLayout = new QVBoxLayout;
    verticalLayout->addLayout(horizontalLayout); // 添加第一个水平布局
    verticalLayout->addLayout(horizontalLayout1); // 添加第二个水平布局
    verticalLayout->addWidget(imageLabel);       // 添加图像标签

    // 创建一个中心widget并设置垂直布局
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(verticalLayout);
    setCentralWidget(centralWidget);

    // 设置只允许数字的验证器
    QRegExp regExp("^\\d+$");
    QRegExpValidator *validator = new QRegExpValidator(regExp, this);
    widthLineEdit->setValidator(validator);
    heightLineEdit->setValidator(validator);

    // 连接按钮的点击信号到槽函数
    connect(openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(transferButton, &QPushButton::clicked, this, &MainWindow::transfer);
    connect(saveImageButton, &QPushButton::clicked, this, &MainWindow::onSaveImageClicked);
    connect(colorSpaceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onColorSpaceChanged);
}

MainWindow::~MainWindow()
{

}

inline uint8_t clip(int value) {
    return static_cast<uint8_t>(std::min(std::max(value, 0), 255));
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

            int R = clip(1.164 * (Y - 16) + 1.793 * (V - 128));
            int G = clip(1.164 * (Y - 16) - 0.534 * (V - 128) - 0.213 * (U - 128));
            int B = clip(1.164 * (Y - 16) + 2.115 * (U - 128));

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

void MainWindow::onOpenFileClicked()
{
    fileName = QFileDialog::getOpenFileName(this, "Open File", "", "All Files (*)");
    if (fileName.isEmpty())
        return;
    filePathLineEdit->setText(fileName);
}

void MainWindow::displayScaledImage(const QImage image)
{
    QSize labelSize = imageLabel->size();
    QImage scaledImage = image.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
    imageLabel->setAlignment(Qt::AlignCenter);
}

void MainWindow::transfer()
{
    bool widthOk, heightOk;
    int width = widthLineEdit->text().toInt(&widthOk);
    int height = heightLineEdit->text().toInt(&heightOk);
    if (!widthOk || !heightOk) {
        QMessageBox::warning(this, "Invalid Input", "Please enter valid resolution values.");
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

    if (colorspace == "rgb") {
        if (colordepth == 8) {
            bytes_per_pixel = 3;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colordepth == 10) {
            bytes_per_pixel = 4;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
               reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convertRgb10ToRgb8(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        }
    } else if(colorspace == "rgba") {
        bytes_per_pixel = 4;
        if (width * height * bytes_per_pixel > fileData.size()) {
            QMessageBox::warning(this, "Warning", "No enough data.");
            return;
        }
        if (reverse)
           reverse_data(fileData, width, height, bytes_per_pixel);
        originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_ARGB32).copy();
    } else if (colorspace == "yuv444") {
        if (colordepth == 8) {
            bytes_per_pixel = 3;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convertyuv444_8_ToRgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colordepth == 10) {
            bytes_per_pixel = 4;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convertyuv444_10_To_8(fileData, width, height);
            fileData = convertyuv444_8_ToRgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        }
    } else if (colorspace == "yuv422") {
        if (colordepth == 8) {
            bytes_per_pixel = 2;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convertyuv422_8_To_444(fileData, width, height);
            fileData = convertyuv444_8_ToRgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colordepth == 10) {
            bytes_per_pixel = 2.5;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convertyuv422_10_To_444(fileData, width, height);
            fileData = convertyuv444_8_ToRgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        }
    } else if (colorspace == "yuv420p(YU12)") {
        bytes_per_pixel = 1.5;
        if (width * height * bytes_per_pixel > fileData.size()) {
            QMessageBox::warning(this, "Warning", "No enough data.");
            return;
        }
        if (reverse)
            reverse_data(fileData, width, height, bytes_per_pixel);
        fileData = convert_yu12_8_To_rgb(fileData, width, height);
        originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
    } else if (colorspace == "yuv420p(YV12)") {
        bytes_per_pixel = 1.5;
        if (width * height * bytes_per_pixel > fileData.size()) {
            QMessageBox::warning(this, "Warning", "No enough data.");
            return;
        }
        if (reverse)
            reverse_data(fileData, width, height, bytes_per_pixel);
        fileData = convert_yv12_8_To_rgb(fileData, width, height);
        originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
    } else if (colorspace == "yuv420sp(NV21)") {
        if (colordepth == 8) {
            bytes_per_pixel = 1.5;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convert_nv21_8_To_rgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colordepth == 10) {
            bytes_per_pixel = 1.875;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convert_nv_10_To_8(fileData);
            fileData = convert_nv12_8_To_rgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        }
    } else if (colorspace == "yuv420sp(NV12)") {
        if (colordepth == 8) {
            bytes_per_pixel = 1.5;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convert_nv12_8_To_rgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        } else if (colordepth == 10) {
            bytes_per_pixel = 1.875;
            if (width * height * bytes_per_pixel > fileData.size()) {
                QMessageBox::warning(this, "Warning", "No enough data.");
                return;
            }
            if (reverse)
                reverse_data(fileData, width, height, bytes_per_pixel);
            fileData = convert_nv_10_To_8(fileData);
            fileData = convert_nv21_8_To_rgb(fileData, width, height);
            originalImage = QImage(reinterpret_cast<const uchar*>(fileData.constData()), width, height, QImage::Format_RGB888).copy();
        }
    }
    displayScaledImage(originalImage);
}

void MainWindow::onSaveImageClicked()
{
    if (originalImage.isNull()) {
        QMessageBox::warning(this, "Warning", "No image to save.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Image as BMP", "", "Bitmap Files (*.bmp)");
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
