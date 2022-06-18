#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "parametersdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    void keyPressEvent(QKeyEvent*) override;
    void move(int, int);
    void zoom(bool);

    ~MainWindow();

private slots: // these slots were automatically added in designer
    void on_offline_clicked();
    void on_reset_clicked();
    void on_screenshot_clicked();
    void on_parameters_clicked();
    void on_about_clicked();

    void parameters_closed(int);

private:
    Ui::MainWindow *ui;

    ParametersDialog* dialog;
    Viewport* viewport;
    QWidget* statusbar;
};
#endif // MAINWINDOW_H
