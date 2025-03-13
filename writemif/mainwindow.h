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
private:
    QPushButton *openFileButton;
    QPushButton *saveImageButton;
    QPushButton *transferButton;
    QLabel *imageLabel;
    QImage originalImage;
    QLineEdit *filePathLineEdit;
    QLabel *filePathQlabel;
    QLineEdit *widthLineEdit;  // 新增：用于输入图像宽度
    QLineEdit *heightLineEdit; // 新增：用于输入图像高度
    QCheckBox *reverseCheckBox; // 新增：用于选择是否反转图像
    QComboBox *colorSpaceComboBox;
    QComboBox *colorDepthComboBox;
    void displayScaledImage(const QImage image);
};

#endif // MAINWINDOW_H
