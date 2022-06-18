#include "parametersdialog.h"
#include "ui_parametersdialog.h"

ParametersDialog::ParametersDialog(QWidget *parent, Viewport const* viewport) :
    QDialog(parent),
    ui(new Ui::ParametersDialog)
{
    using namespace mandelbrot;

    ui->setupUi(this);
    settings = viewport->getRendererSettings();
    cursorDependentZoom = viewport->getCursorDependentZoom();
    lowResolution = viewport->getLowResolution();

    // zoom checkbox
    ui->zoom->setChecked(cursorDependentZoom);
    connect(ui->zoom, SIGNAL(stateChanged(int)), this, SLOT(zoom_toggled(int)));

    // low resolution checkbox
    ui->low_res->setChecked(lowResolution);
    connect(ui->low_res, SIGNAL(stateChanged(int)), this, SLOT(low_res_toggled(int)));

    // threads count auto checkbox
    ui->threads_auto->setChecked(settings.threadsCountAuto);
    connect(ui->threads_auto, SIGNAL(stateChanged(int)), this, SLOT(threads_auto_toggled(int)));

    // iterations count auto checkbox
    ui->iterations_auto->setChecked(settings.iterationsCountAuto);
    connect(ui->iterations_auto, SIGNAL(stateChanged(int)), this, SLOT(iterations_auto_toggled(int)));

    // threads count slider
    ui->threads_slider->setRange(1, MAX_THREADS_COUNT);
    ui->threads_slider->setValue(settings.threadsCount);
    threads_slider_update(settings.threadsCount);
    connect(ui->threads_slider, SIGNAL(valueChanged(int)), this, SLOT(threads_slider_update(int)));

    // iterations count slider
    ui->iterations_slider->setRange(MIN_ITERATIONS_BY_PIXEL, MAX_ITERATIONS_BY_PIXEL);
    ui->iterations_slider->setValue(settings.iterationsCount);
    iterations_slider_update(settings.iterationsCount);
    connect(ui->iterations_slider, SIGNAL(valueChanged(int)), this, SLOT(iterations_slider_update(int)));
}

void ParametersDialog::iterations_slider_update(int val) {
    ui->iterations_counter->setText(QString("Iterations per pixel: %1").arg(val));
    settings.iterationsCount = val;
}

void ParametersDialog::threads_slider_update(int val) {
    ui->threads_counter->setText(QString("Threads count: %1").arg(val));
    settings.threadsCount = val;
}

void ParametersDialog::zoom_toggled(int state) {
    cursorDependentZoom = (state > 0);
}

void ParametersDialog::low_res_toggled(int state) {
    lowResolution = (state > 0);
}

void ParametersDialog::threads_auto_toggled(int state) {
    settings.threadsCountAuto = (state > 0);
}

void ParametersDialog::iterations_auto_toggled(int state) {
    settings.iterationsCountAuto = (state > 0);
}

ParametersDialog::~ParametersDialog() {
    delete ui;
}
