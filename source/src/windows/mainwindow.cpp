#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    viewport = ui->mandelbrot;
    statusbar = ui->info;

    connect(
        viewport, SIGNAL(widgetInfoDelivery(mandelbrot::ViewportInfo)),
        statusbar, SLOT(updateInfo(mandelbrot::ViewportInfo))
    );
    setFocus();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_O:
        on_offline_clicked();
        break;
    case Qt::Key_R:
        on_reset_clicked();
        break;
    case Qt::Key_S:
        on_screenshot_clicked();
        break;
    case Qt::Key_P:
        on_parameters_clicked();
        break;
    case Qt::Key_A:
        on_about_clicked();
        break;
    case Qt::Key_Up:
        move(0, 1);
        break;
    case Qt::Key_Down:
        move(0, -1);
        break;
    case Qt::Key_Left:
        move(1, 0);
        break;
    case Qt::Key_Right:
        move(-1, 0);
        break;
    case Qt::Key_Plus:
        zoom(true);
        break;
    case Qt::Key_Minus:
        zoom(false);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MainWindow::move(int dx, int dy) {
    QSizeF step = viewport->size() / 15.;
    viewport->move(QPointF(step.width() * dx, step.height() * dy));
}

void MainWindow::zoom(bool zoomIn) {
    QSizeF center = viewport->size() / 2.;
    viewport->zoom(QPointF(center.width(), center.height()), zoomIn ? 1 : -1);
}

void MainWindow::on_offline_clicked() {
    bool offline = viewport->getOffline();
    viewport->setOffline(!offline);

    const static QString toggle_offline = "(O) Offline";
    const static QString toggle_online = "(O) Online";
    ui->offline->setText(!offline ? toggle_online : toggle_offline);
}

void MainWindow::on_reset_clicked() {
    viewport->reset();
}

void MainWindow::on_screenshot_clicked() {
    if (!viewport->screenshot()) {
        QMessageBox::information(
                    this,
                    "Invalid action",
                    "Screenshot capture is impossible during initial rendering"
                    );
    }
}

void MainWindow::on_parameters_clicked() {
    dialog = new ParametersDialog(this, viewport);
    dialog->show();
    connect(dialog, SIGNAL(finished(int)), this, SLOT(parameters_closed(int)));
}

void MainWindow::parameters_closed(int saved) {
    using namespace mandelbrot;
    if (saved) {
        viewport->setRendererSettings(dialog->settings);
        viewport->setCursorDependentZoom(dialog->cursorDependentZoom);
        viewport->setLowResolution(dialog->lowResolution);
    }
    dialog->deleteLater();
}

void MainWindow::on_about_clicked() {
    QString info = "Interactive visualizer of the Mandelbrot set\nPerformed by letstatt\n\nAVX2: %1";
    QMessageBox::about(
                this,
                "About",
#ifdef AVX
                info.arg("on")
#else
                info.arg("off")
#endif
                );
}

MainWindow::~MainWindow() {
    delete ui;
}
