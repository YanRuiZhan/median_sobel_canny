#ifndef ICM_MAIN_WINDOW_HPP
#define ICM_MAIN_WINDOW_HPP

#include "icm/image_io.hpp"

#include <QFutureWatcher>
#include <QMainWindow>
#include <QString>

class QLabel;
class QComboBox;
class QSpinBox;

class QPushButton;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openImage();
    void saveResult();
    void runProcessing();
    void updateControls();
    void processingFinished();

private:
    void setPreview(const icm::Image& image, QLabel* target);
    void updateButtons();
    void resizeEvent(QResizeEvent* event) override;

    QLabel* inputPreview_ = nullptr;
    QLabel* outputPreview_ = nullptr;
    QLabel* inputInfo_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* elapsed_ = nullptr;
    QComboBox* method_ = nullptr;
    QWidget* medianControls_ = nullptr;
    QWidget* cannyControls_ = nullptr;

    QSpinBox* medianWindow_ = nullptr;
    QSpinBox* lowThreshold_ = nullptr;
    QSpinBox* highThreshold_ = nullptr;
    QPushButton* processButton_ = nullptr;
    QPushButton* saveButton_ = nullptr;

    QString inputPath_;
    icm::Image inputImage_;
    icm::Image outputImage_;
    struct ProcessingResult {
        icm::Image image;
        double elapsedMs = 0;
    };
    QFutureWatcher<ProcessingResult> watcher_;
};

#endif
