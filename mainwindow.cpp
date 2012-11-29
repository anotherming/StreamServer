#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "client.h"
#include "zlog.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); 
    zlog_init("../zlog.conf");
	category = zlog_get_category("default");
    init_client (&client, 1);  

    this->setAttribute (Qt::WA_PaintOutsidePaintEvent, true);

    client.main_window = this;
    client.run (&client); 
}

MainWindow::~MainWindow()
{
    delete ui;
    destroy_client (&client);
    zlog_fini ();
}

void MainWindow::on_btPlay_clicked()
{
    client.start (&client, (char *)"sw", true);
}

void MainWindow::on_btStop_clicked()
{
    client.stop (&client, (char *)"sw");

}

void MainWindow::on_btSeek_clicked()
{

}

void MainWindow::draw (void *data, int len) {
    QPixmap pixmap;
    pixmap.loadFromData((const uchar *)data,len,"PPM");

    this->pixmap = pixmap;
    usleep (50);
    update ();
}

void MainWindow::paintEvent (QPaintEvent *event) {
	QPainter painter (this);
	QRect rect = this->ui->frMovie->contentsRect();
    painter.drawPixmap (rect, this->pixmap);
}