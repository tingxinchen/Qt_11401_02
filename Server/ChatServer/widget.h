#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QMap>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void startServer();       // 按下啟動按鈕
    void onNewConnection();   // 處理新連線
    void onReadyRead();       // 讀取並分發 JSON 訊息
    void onDisconnected();    // 處理斷線

private:
    // UI 元件
    QLineEdit *ipEdit;
    QLineEdit *portEdit;
    QPushButton *startBtn;
    QTextEdit *logEdit;       // 顯示 Server 狀態 Log

    // 網路元件
    QTcpServer *tcpServer;
    QMap<QString, QTcpSocket*> clientList; // 暱稱 -> Socket

    void updateUserList();    // 廣播名單
};
#endif
