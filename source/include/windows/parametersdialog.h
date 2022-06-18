#ifndef PARAMETERSDIALOG_H
#define PARAMETERSDIALOG_H

#include <QDialog>
#include <viewport.h>

namespace Ui {
class ParametersDialog;
}

class ParametersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParametersDialog(QWidget*, Viewport const*);
    ~ParametersDialog();

    mandelbrot::RendererSettings settings;
    bool cursorDependentZoom;
    bool lowResolution;

public slots:
    void iterations_slider_update(int);
    void threads_slider_update(int);
    void zoom_toggled(int);
    void low_res_toggled(int);
    void threads_auto_toggled(int);
    void iterations_auto_toggled(int);

private:
    Ui::ParametersDialog *ui;
};

#endif // PARAMETERSDIALOG_H
