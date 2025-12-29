#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include "chatwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_addFriendButton_clicked();
    void on_removeFriendButton_clicked();
    void on_friendsList_itemDoubleClicked(QListWidgetItem *item);
    void on_startServerButton_clicked();
    void newConnection();
    void readyRead();
    void clientDisconnected();

private:
    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QMap<QString, ChatWindow*> chatWindows;
    QMap<QTcpSocket*, QString> socketToFriend;
    quint16 serverPort;
    
    void saveFriendsList();
    void loadFriendsList();
};
#endif // MAINWINDOW_H
