#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QLineEdit>
#include <QComboBox>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenFileClicked();
    void onSaveImageClicked();
    void transfer();
    void onColorSpaceChanged();
    void onAboutClicked();
    void onFileTypeChanged(int index);
    int calculateFrameSize(int width, int height, int padded_line_bytes, QString& colorspace, int colordepth);
    void enableFrameNavigation(int frameCount);
    void displayFrame(int frameIndex);
    void showPreviousFrame();
    void showNextFrame();
    void togglePlay();
    void playNextFrame();
    void updateCurrentFrame(int index);
private:
    QPushButton *openFileButton;
    QPushButton *saveImageButton;
    QPushButton *transferButton;
    QPushButton *aboutButton;
    QLabel *imageLabel;
    QImage originalImage;
    QImage frameImage;
    QLineEdit *widthLineEdit;  // 新增：用于输入图像宽度
    QLineEdit *heightLineEdit; // 新增：用于输入图像高度
    QLineEdit *padLineEdit; // 新增：用于输入图像高度
    QCheckBox *reverseCheckBox; // 新增：用于选择是否反转图像
    QCheckBox *cropCheckBox;
    QCheckBox *littleCheckBox;
    QComboBox *colorSpaceComboBox;
    QComboBox *colorDepthComboBox;
    QComboBox *fileTypeComboBox;
    QWidget *originalSecondRowWidget;  // 图像模式布局容器
    QLineEdit* frameLineEdit;        // 帧数输入框
    QSlider* frameSlider;            // 帧滑块
    QLabel* frameLabel;              // 当前帧标签
    QPushButton* prevFrameButton;    // 上一帧按钮
    QPushButton* nextFrameButton;    // 下一帧按钮
    QList<QImage> frameImages;       // 存储所有帧图像
    QLabel* frameCountLabel;      // 总帧数标签
    QLineEdit* fpsLineEdit;       // 帧率输入框
    QPushButton* playButton;      // 播放按钮
    QTimer* frameTimer;           // 播放定时器
    int currentPlayingFrame;      // 当前播放帧索引
    QVector<QImage> scaledImages;
    // HDMI模式专用控件
    QLineEdit *tmdsclkLineEdit;
    QLineEdit *htotalLineEdit;         // HDMI htotal输入框
    QLineEdit *hactiveLineEdit;        // HDMI hactive输入框
    QLineEdit *vtotalLineEdit;         // HDMI vtotal输入框
    QLineEdit *vactiveLineEdit;        // HDMI vactive输入框
    QLineEdit *hdmiColorSpaceEdit; // 颜色空间选择框（HDMI模式）
    QLineEdit *hdmiColorDepthEdit; // 颜色深度选择框（HDMI模式）
    //QPushButton *hdmiSaveImageButton;  // 保存按钮（HDMI模式）
    QWidget *hdmiWidget;               // HDMI布局容器
    int hdmi_currentFrameIndex;
    QLabel *hdmi_frameInfoLabel;
    QPushButton *hdmi_prevFrameBtn;  // 上一帧按钮
    QPushButton *hdmi_nextFrameBtn;  // 下一帧按钮
    int tmdsclk;
    qint64 hdmifilesize;
    QLineEdit *filePathLineEdit;
    QLabel *filePathQlabel;
    QComboBox *bpcComboBox; // 新增bpc选择框
    QLabel *bpcLabel;       // 新增bpc标签
    QThread *m_workerThread = nullptr;
    void displayScaledImage(const QImage image);
    void playVideoWithFFmpeg(const QString &videoPath);
    void transfer_hdmi();
    void hdmi_enableFrameNavigation();
    void hdmi_updateFrameDisplay();
    void hdmi_updateFrameInfo();
    void transfer_bin();
    void transfer_dsc();
signals:
    void decodeCompleted(const QString &path);
    void decodeFailed(const QString &msg);
};

#endif // MAINWINDOW_H
