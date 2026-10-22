#include "main_window.hpp"

#include "icm/canny.hpp"
#include "icm/median_blur.hpp"
#include "icm/sobel.hpp"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QResizeEvent>
#include <QSpinBox>
#include <QStatusBar>
#include <QVariant>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <utility>

namespace {

QImage toQImage(const icm::Image& image) {
    if (image.w <= 0 || image.h <= 0 || image.ch < 1 || image.ch > 4 || image.data.empty())
        return {};

    const int bytesPerPixel = image.ch == 2 ? 4 : (image.ch == 1 ? 1 : image.ch);
    const QImage::Format format = image.ch == 1
        ? QImage::Format_Grayscale8
        : (image.ch == 2 || image.ch == 4 ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    QImage result(image.w, image.h, format);
    if (result.isNull())
        return {};

    for (int y = 0; y < image.h; ++y) {
        if (static_cast<int>(image.data.size()) <= y ||
            static_cast<int>(image.data[y].size()) < image.w * image.ch)
            return {};
        uchar* row = result.scanLine(y);
        for (int x = 0; x < image.w; ++x) {
            const int offset = x * image.ch;
            if (image.ch == 1) {
                row[x] = static_cast<uchar>(icm::clamp255(image.data[y][offset]));
            } else if (image.ch == 2) {
                const uchar gray = static_cast<uchar>(icm::clamp255(image.data[y][offset]));
                const int target = x * bytesPerPixel;
                row[target] = gray;
                row[target + 1] = gray;
                row[target + 2] = gray;
                row[target + 3] = static_cast<uchar>(icm::clamp255(image.data[y][offset + 1]));
            } else {
                const int target = x * bytesPerPixel;
                for (int channel = 0; channel < image.ch; ++channel)
                    row[target + channel] = static_cast<uchar>(icm::clamp255(image.data[y][offset + channel]));
            }
        }
    }
    return result;
}

QLabel* createPreview(const QString& text) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(320, 240);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    label->setStyleSheet("QLabel { background: #20242b; color: #aab2bf; border: 1px solid #3b414b; }");
    label->setScaledContents(false);
    return label;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("工业摄影测量作业1"));
    resize(1000, 680);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    auto* toolbar = new QHBoxLayout;
    auto* openButton = new QPushButton(QStringLiteral("打开图片"), central);
    saveButton_ = new QPushButton(QStringLiteral("保存结果"), central);
    processButton_ = new QPushButton(QStringLiteral("开始处理"), central);
    method_ = new QComboBox(central);
    method_->addItem(QStringLiteral("中值滤波"), QStringLiteral("median"));
    method_->addItem(QStringLiteral("Sobel 边缘"), QStringLiteral("sobel"));
        method_->addItem(QStringLiteral("改进 Sobel 边缘"), QStringLiteral("sobel_improved"));
    method_->addItem(QStringLiteral("Canny 边缘"), QStringLiteral("canny"));
    method_->addItem(QStringLiteral("改进 Canny 边缘"), QStringLiteral("canny_improved"));
    toolbar->addWidget(openButton);
    toolbar->addWidget(method_);

    medianControls_ = new QWidget(central);
    auto* medianLayout = new QHBoxLayout(medianControls_);
    medianLayout->setContentsMargins(0, 0, 0, 0);
    medianLayout->addWidget(new QLabel(QStringLiteral("窗口大小"), medianControls_));
    medianWindow_ = new QSpinBox(medianControls_);
    medianWindow_->setRange(1, 99);
    medianWindow_->setSingleStep(2);
    medianWindow_->setValue(3);
    medianLayout->addWidget(medianWindow_);
    toolbar->addWidget(medianControls_);

    cannyControls_ = new QWidget(central);
    auto* cannyLayout = new QHBoxLayout(cannyControls_);
    cannyLayout->setContentsMargins(0, 0, 0, 0);
    cannyLayout->addWidget(new QLabel(QStringLiteral("低阈值"), cannyControls_));
    lowThreshold_ = new QSpinBox(cannyControls_);
    lowThreshold_->setRange(0, 255);
    lowThreshold_->setValue(20);
    cannyLayout->addWidget(lowThreshold_);
    cannyLayout->addWidget(new QLabel(QStringLiteral("高阈值"), cannyControls_));
    highThreshold_ = new QSpinBox(cannyControls_);
    highThreshold_->setRange(0, 255);
    highThreshold_->setValue(110);
    cannyLayout->addWidget(highThreshold_);
    toolbar->addWidget(cannyControls_);

    bilateralControls_ = new QWidget(central);
    auto* bilateralLayout = new QHBoxLayout(bilateralControls_);
    bilateralLayout->setContentsMargins(0, 0, 0, 0);
    bilateralLayout->addWidget(new QLabel(QStringLiteral("灰度尺度 σ_color"), bilateralControls_));
    sigmaColor_ = new QDoubleSpinBox(bilateralControls_);
    sigmaColor_->setDecimals(1);
    sigmaColor_->setRange(0.1, 255.0);
    sigmaColor_->setSingleStep(5.0);
    sigmaColor_->setValue(45.0);
    sigmaColor_->setToolTip(QStringLiteral("值越大，平滑越强，但可能损失弱边缘。"));
    bilateralLayout->addWidget(sigmaColor_);
    toolbar->addWidget(bilateralControls_);
    toolbar->addStretch();
    toolbar->addWidget(processButton_);
    toolbar->addWidget(saveButton_);
    root->addLayout(toolbar);

    auto* previews = new QGridLayout;
    inputPreview_ = createPreview(QStringLiteral("输入图像"));
    outputPreview_ = createPreview(QStringLiteral("处理结果"));
    previews->addWidget(new QLabel(QStringLiteral("输入"), central), 0, 0);
    previews->addWidget(new QLabel(QStringLiteral("输出"), central), 0, 1);
    previews->addWidget(inputPreview_, 1, 0);
    previews->addWidget(outputPreview_, 1, 1);
    previews->setColumnStretch(0, 1);
    previews->setColumnStretch(1, 1);
    previews->setRowStretch(1, 1);
    root->addLayout(previews, 1);

    inputInfo_ = new QLabel(QStringLiteral("尚未打开图片"), central);
    inputInfo_->setTextFormat(Qt::PlainText);
    inputInfo_->setWordWrap(true);
    inputInfo_->setMinimumWidth(0);
    inputInfo_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    root->addWidget(inputInfo_);
    setCentralWidget(central);
    status_ = new QLabel(QStringLiteral("请选择一张图片开始。"), this);
    statusBar()->addWidget(status_);
    elapsed_ = new QLabel(QStringLiteral("处理耗时：—"), this);
    statusBar()->addPermanentWidget(elapsed_);

    connect(openButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(saveButton_, &QPushButton::clicked, this, &MainWindow::saveResult);
    connect(processButton_, &QPushButton::clicked, this, &MainWindow::runProcessing);
    connect(method_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateControls);
    connect(&watcher_, &QFutureWatcher<ProcessingResult>::finished, this, &MainWindow::processingFinished);
    updateControls();
    updateButtons();
}

void MainWindow::openImage() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开图片"), QString(),
        QStringLiteral("图像文件 (*.png *.jpg *.jpeg *.bmp *.tga *.gif);;所有文件 (*)"));
    if (path.isEmpty())
        return;

    icm::Image image = icm::read_image(path.toStdString());
    if (image.data.empty()) {
        QMessageBox::warning(this, QStringLiteral("打开失败"), QStringLiteral("无法读取所选图片。"));
        return;
    }
    inputPath_ = path;
    inputImage_ = std::move(image);
    outputImage_ = {};
    setPreview(inputImage_, inputPreview_);
    outputPreview_->setPixmap(QPixmap());
    outputPreview_->setProperty("sourceImage", QVariant());
    outputPreview_->setText(QStringLiteral("处理结果"));
    inputInfo_->setText(QStringLiteral("%1 × %2 · %3 通道\n%4")
                        .arg(inputImage_.w).arg(inputImage_.h).arg(inputImage_.ch).arg(path));
    status_->setText(QStringLiteral("图片已载入。"));
    elapsed_->setText(QStringLiteral("处理耗时：—"));
    updateButtons();
}

void MainWindow::saveResult() {
    if (outputImage_.data.empty())
        return;
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存处理结果"), QStringLiteral("result.png"),
        QStringLiteral("PNG 图片 (*.png)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
        path += QStringLiteral(".png");
    if (!icm::save_image(path.toStdString(), outputImage_)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法保存图片，请检查目标路径。"));
        return;
    }
    status_->setText(QStringLiteral("结果已保存：%1").arg(path));
}

void MainWindow::runProcessing() {
    if (inputImage_.data.empty() || watcher_.isRunning())
        return;
    const QString method = method_->currentData().toString();
    const int window = medianWindow_->value();
    const int low = lowThreshold_->value();
    const int high = highThreshold_->value();
    const double sigmaColor = sigmaColor_->value();
    if (method == QStringLiteral("median") && window % 2 == 0) {
        QMessageBox::warning(this, QStringLiteral("参数无效"), QStringLiteral("中值滤波窗口大小必须为奇数。"));
        return;
    }
    if ((method == QStringLiteral("canny") || method == QStringLiteral("canny_improved")) && low > high) {
        QMessageBox::warning(this, QStringLiteral("参数无效"), QStringLiteral("低阈值不能大于高阈值。"));
        return;
    }

    const icm::Image source = inputImage_;
    processButton_->setEnabled(false);
    saveButton_->setEnabled(false);
    status_->setText(QStringLiteral("正在处理…"));
    elapsed_->setText(QStringLiteral("处理耗时：计算中…"));
    watcher_.setFuture(QtConcurrent::run([source, method, window, low, high, sigmaColor]() {
        ProcessingResult result;
        QElapsedTimer timer;
        timer.start();
        if (method == QStringLiteral("median"))
            result.image = icm::median_blur(source, window);
        else if (method == QStringLiteral("sobel"))
            result.image = icm::sobel_sharpen(source);
        else if (method == QStringLiteral("sobel_improved"))
            result.image = icm::sobel_sharpen_improved(source);
        else if (method == QStringLiteral("canny_improved"))
            result.image = icm::canny_edge_improved(source, low, high, 3, 1.5, sigmaColor);
        else
            result.image = icm::canny_edge(source, low, high);
        result.elapsedMs = timer.nsecsElapsed() / 1000000.0;
        return result;
    }));
}

void MainWindow::updateControls() {
    const QString method = method_->currentData().toString();
    medianControls_->setVisible(method == QStringLiteral("median"));
    bilateralControls_->setVisible(method == QStringLiteral("canny_improved"));
    cannyControls_->setVisible(method == QStringLiteral("canny") ||
                               method == QStringLiteral("canny_improved"));
}

void MainWindow::processingFinished() {
    auto result = watcher_.result();
    outputImage_ = std::move(result.image);
    elapsed_->setText(QStringLiteral("处理耗时：%1 ms").arg(result.elapsedMs, 0, 'f', 2));
    if (outputImage_.data.empty()) {
        outputPreview_->setPixmap(QPixmap());
        outputPreview_->setProperty("sourceImage", QVariant());
        outputPreview_->setText(QStringLiteral("处理结果"));
        status_->setText(QStringLiteral("处理失败：未生成有效图像。"));
    } else {
        setPreview(outputImage_, outputPreview_);
        status_->setText(QStringLiteral("处理完成：%1 × %2 · %3 通道")
                         .arg(outputImage_.w).arg(outputImage_.h).arg(outputImage_.ch));
    }
    updateButtons();
}

void MainWindow::setPreview(const icm::Image& image, QLabel* target) {
    const QImage qtImage = toQImage(image);
    if (qtImage.isNull()) {
        target->setPixmap(QPixmap());
        target->setText(QStringLiteral("无法显示此图像"));
        return;
    }
    target->setProperty("sourceImage", QVariant::fromValue(qtImage));
    target->setText(QString());
    target->setPixmap(QPixmap::fromImage(qtImage).scaled(
        target->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    for (QLabel* label : {inputPreview_, outputPreview_}) {
        const QVariant stored = label->property("sourceImage");
        if (stored.isValid()) {
            const QImage image = stored.value<QImage>();
            label->setPixmap(QPixmap::fromImage(image).scaled(
                label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void MainWindow::updateButtons() {
    const bool busy = watcher_.isRunning();
    processButton_->setEnabled(!busy && !inputImage_.data.empty());
    saveButton_->setEnabled(!busy && !outputImage_.data.empty());
}
