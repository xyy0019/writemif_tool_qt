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
#include <QDebug>
#include <QTimer>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    openFileButton(new QPushButton("打开文件", this)),
    saveImageButton(new QPushButton("保存为BMP", this)),
    transferButton(new QPushButton("转换", this)),
    aboutButton(new QPushButton("关于", this)),
    imageLabel(new QLabel(this)),
    widthLineEdit(new QLineEdit(this)),
    heightLineEdit(new QLineEdit(this)),
    padLineEdit(new QLineEdit(this)),
    colorSpaceComboBox(new QComboBox(this)),
    colorDepthComboBox(new QComboBox(this)),
    fileTypeComboBox(new QComboBox(this)),
    originalSecondRowWidget(new QWidget(this)),
    reverseCheckBox(new QCheckBox(this)),
    cropCheckBox(new QCheckBox(this)),
    littleCheckBox(new QCheckBox(this)),
    // HDMI模式控件初始化
    htotalLineEdit(new QLineEdit(this)),
    hactiveLineEdit(new QLineEdit(this)),
    vtotalLineEdit(new QLineEdit(this)),
    vactiveLineEdit(new QLineEdit(this)),
    hdmiColorSpaceEdit(new QLineEdit(this)), // 正确初始化QComboBox
    hdmiColorDepthEdit(new QLineEdit(this)), // 正确初始化QComboBox
    hdmi_frameInfoLabel(new QLabel(this)),
    //hdmiSaveImageButton(new QPushButton("保存为BMP", this)),
    hdmiWidget(new QWidget(this)),
    filePathLineEdit(new QLineEdit(this)),
    bpcComboBox(new QComboBox(this)),
    bpcLabel(new QLabel("BPC:", this)),
    frameCountLabel(new QLabel("总帧数:", this)),
    fpsLineEdit(new QLineEdit(this)),
    playButton(new QPushButton("播放", this)),
    frameTimer(new QTimer(this)),
    currentPlayingFrame(0),
    hdmi_currentFrameIndex(0),
    hdmi_prevFrameBtn(new QPushButton("<< 上一帧", this)),
    hdmi_nextFrameBtn(new QPushButton("下一帧 >>", this))
{
    // 基础窗口设置
    setWindowTitle("图像数据解析 Author by yaoyu.xu");
    setFixedSize(1350, 720);

    // 文件类型选择框
    QStringList fileTypeOptions = {"二进制", "HDMI", "DSC"};
    fileTypeComboBox->addItems(fileTypeOptions);

    // 颜色空间选项
    QStringList colorSpaceOptions = {
        "yuv422", "yuv444", "rgb", "rgba",
        "yuv420p(YV12)", "yuv420p(YU12)",
        "yuv420sp(NV12)", "yuv420sp(NV21)", "yuv422sp", "dpss_yuv422sp", "y"
    };

    // bpc选项
    QStringList bpcOptions = {"8", "10", "12"};
    bpcComboBox->addItems(bpcOptions);
    bpcComboBox->setVisible(false);
    bpcLabel->setVisible(false);

    // 图像数据模式布局
    frameLineEdit = new QLineEdit(this);
    frameLineEdit->setObjectName("frameLineEdit");
    frameLineEdit->setValidator(new QIntValidator(1, 300, this)); // 设置帧数范围
    frameLineEdit->setText("1");
    frameSlider = new QSlider(Qt::Horizontal, this);
    frameSlider->setObjectName("frameSlider");
    // 初始化导航控件
    frameLabel = new QLabel("当前帧: 0/0", this);
    prevFrameButton = new QPushButton("<<", this);
    nextFrameButton = new QPushButton(">>", this);
    cropCheckBox->setChecked(false);
    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(new QLabel("分辨率:", this));
    imageLayout->addWidget(widthLineEdit);
    imageLayout->addWidget(heightLineEdit);
    imageLayout->addWidget(padLineEdit);
    imageLayout->addWidget(new QLabel("Color Space:", this));
    colorSpaceComboBox->addItems(colorSpaceOptions);
    imageLayout->addWidget(colorSpaceComboBox);
    imageLayout->addWidget(new QLabel("Color Depth:", this));
    colorDepthComboBox->addItems({"8", "10"});
    imageLayout->addWidget(colorDepthComboBox);
    imageLayout->addWidget(new QLabel("swap:", this));
    imageLayout->addWidget(reverseCheckBox);
    imageLayout->addWidget(new QLabel("crop:", this));
    imageLayout->addWidget(cropCheckBox);
    imageLayout->addWidget(new QLabel("little:", this));
    imageLayout->addWidget(littleCheckBox);
    imageLayout->addWidget(frameCountLabel);
    imageLayout->addWidget(frameLineEdit);
    imageLayout->addWidget(frameSlider);
    imageLayout->addWidget(frameLabel);
    imageLayout->addWidget(prevFrameButton);
    imageLayout->addWidget(nextFrameButton);
    fpsLineEdit->setValidator(new QIntValidator(1, 60, this));
    imageLayout->addWidget(new QLabel("帧率(fps):"));
    fpsLineEdit->setText("25"); // 默认帧率
    imageLayout->addWidget(fpsLineEdit);
    imageLayout->addWidget(playButton);
    imageLayout->addWidget(saveImageButton);
    imageLayout->insertStretch(-1, 1); // 右侧弹性空间

    originalSecondRowWidget = new QWidget(this);
    originalSecondRowWidget->setLayout(imageLayout);

    // HDMI模式布局
    QHBoxLayout *hdmiLayout = new QHBoxLayout;
    tmdsclkLineEdit = new QLineEdit(this);
    tmdsclkLineEdit->setText("0");
    hdmiLayout->addWidget(new QLabel("htotal:", this));
    hdmiLayout->addWidget(htotalLineEdit);
    hdmiLayout->addWidget(new QLabel("hactive:", this));
    hdmiLayout->addWidget(hactiveLineEdit);
    hdmiLayout->addWidget(new QLabel("vtotal:", this));
    hdmiLayout->addWidget(vtotalLineEdit);
    hdmiLayout->addWidget(new QLabel("vactive:", this));
    hdmiLayout->addWidget(vactiveLineEdit);
    hdmiLayout->addWidget(new QLabel("Color Space:", this));
    hdmiLayout->addWidget(hdmiColorSpaceEdit);
    hdmiLayout->addWidget(new QLabel("Color Depth:", this));
    hdmiLayout->addWidget(hdmiColorDepthEdit);
    hdmiLayout->addWidget(new QLabel("tmdsclk", this));
    hdmiLayout->addWidget(tmdsclkLineEdit);
    hdmiLayout->addWidget(hdmi_frameInfoLabel);
    hdmiLayout->addWidget(hdmi_prevFrameBtn);
    hdmiLayout->addWidget(hdmi_nextFrameBtn);
    hdmiLayout->insertStretch(-1, 1); // 右侧弹性空间

    hdmiWidget = new QWidget(this);
    hdmiWidget->setLayout(hdmiLayout);
    hdmiWidget->setVisible(false);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(12, 12, 12, 12); // 窗口边距
    mainLayout->setSpacing(0); // 垂直间距

    // 第一行布局
    QHBoxLayout *firstRow = new QHBoxLayout;
    firstRow->setSpacing(8);
    firstRow->addWidget(fileTypeComboBox);
    firstRow->addWidget(bpcLabel);
    firstRow->addWidget(bpcComboBox);
    firstRow->addWidget(new QLabel("文件路径:", this));
    firstRow->addWidget(filePathLineEdit, 2); // 占用更多空间
    firstRow->addWidget(openFileButton);
    firstRow->addWidget(transferButton);
    firstRow->addWidget(aboutButton);

    mainLayout->addLayout(firstRow);
    mainLayout->addSpacing(0); // 行间距
    mainLayout->addWidget(originalSecondRowWidget);
    mainLayout->addWidget(hdmiWidget);
    mainLayout->addSpacing(0);
    mainLayout->addWidget(imageLabel, 1); // 图像显示区域自动扩展

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // 设置输入验证
    QRegExpValidator *numValidator = new QRegExpValidator(QRegExp("^\\d+$"), this);
    widthLineEdit->setValidator(numValidator);
    heightLineEdit->setValidator(numValidator);
    htotalLineEdit->setValidator(numValidator);
    hactiveLineEdit->setValidator(numValidator);
    vtotalLineEdit->setValidator(numValidator);
    vactiveLineEdit->setValidator(numValidator);

    // 设置只允许数字的验证器
    QRegExp regExp("^\\d+$");
    QRegExpValidator *validator = new QRegExpValidator(regExp, this);
    widthLineEdit->setValidator(validator);
    heightLineEdit->setValidator(validator);
    htotalLineEdit->setValidator(validator);
    hactiveLineEdit->setValidator(validator);
    vtotalLineEdit->setValidator(validator);
    vactiveLineEdit->setValidator(validator);

    // 连接按钮的点击信号到槽函数
    connect(openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(transferButton, &QPushButton::clicked, this, &MainWindow::transfer);
    connect(saveImageButton, &QPushButton::clicked, this, &MainWindow::onSaveImageClicked);
    //connect(hdmiSaveImageButton, &QPushButton::clicked, this, &MainWindow::onSaveImageClicked);
    connect(colorSpaceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onColorSpaceChanged);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::onAboutClicked);
    connect(fileTypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFileTypeChanged);
    connect(frameTimer, &QTimer::timeout, this, &MainWindow::playNextFrame);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::togglePlay);

    // 连接上一帧按钮
    connect(hdmi_prevFrameBtn, &QPushButton::clicked, this, [this]() {
        if (hdmi_currentFrameIndex > 0) {
            hdmi_currentFrameIndex--;
            hdmi_updateFrameDisplay(); // 传递当前帧索引
        }
    });

    // 连接下一帧按钮
    connect(hdmi_nextFrameBtn, &QPushButton::clicked, this, [this]() {
        tmdsclk = tmdsclkLineEdit->text().toInt();
        QString inputPath = filePathLineEdit->text().trimmed();
        QFile inputFile(inputPath);
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
        if (hdmi_currentFrameIndex < frameCount - 1) {
            hdmi_currentFrameIndex++;
            hdmi_updateFrameDisplay(); // 传递当前帧索引
        }
    });
}

void MainWindow::transfer()
{
    int Mode =fileTypeComboBox->currentIndex();
    // 获取输入路径并验证
    if (Mode == 0) {
        transfer_bin();
    } else if (Mode == 1){
        transfer_hdmi();
    } else {
        transfer_dsc();
    }
}

void MainWindow::onFileTypeChanged(int index)
{
    if (index == 0) { // 图像数据二进制
        originalSecondRowWidget->setVisible(true);
        hdmiWidget->setVisible(false);
        bpcLabel->setVisible(false);
        bpcComboBox->setVisible(false);
    } else if (index == 1) { // HDMI
        originalSecondRowWidget->setVisible(false);
        hdmiWidget->setVisible(true);
        bpcLabel->setVisible(false);
        bpcComboBox->setVisible(false);
    } else if (index == 2) {
        originalSecondRowWidget->setVisible(false);
        hdmiWidget->setVisible(false);
        bpcLabel->setVisible(true);
        bpcComboBox->setVisible(true);
    }
}

MainWindow::~MainWindow()
{

}
