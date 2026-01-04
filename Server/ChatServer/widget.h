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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    // Server
    void startServer();       // 啟動
    void stopServer();        // 斷開所有連線並關閉 Server

    // 事件
    void onNewConnection();   // 處理新 Client 連入
    void onReadyRead();       // 解析與轉發 JSON 訊息 (含文字、檔案)
    void onDisconnected();    // 處理成員離開並更新名單

private:
    // UI
    QLineEdit *ipEdit, *portEdit;
    QPushButton *startBtn, *stopBtn;
    QTextEdit *logEdit;       // 伺服器日誌

    // 核心物件
    QTcpServer *tcpServer;
    QMap<QString, QTcpSocket*> clientList; // 在線名單映射 [暱稱 -> Socket] [cite: 39, 46]

    // 輔助
    void updateUserList();    // 廣播最新在線名單 [cite: 11]
    void setupUI();           // 初始化介面佈局 [cite: 19]
};

#endif
