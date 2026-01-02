#include "widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHostAddress>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // --- 1. 手動建構 UI 介面 ---
    this->setWindowTitle("TCP 通訊伺服器 ");
    this->resize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *settingLayout = new QHBoxLayout();

    ipEdit = new QLineEdit("127.0.0.1");
    portEdit = new QLineEdit("8888");
    startBtn = new QPushButton("啟動伺服器");
    logEdit = new QTextEdit();
    logEdit->setReadOnly(true);

    settingLayout->addWidget(new QLabel("IP:"));
    settingLayout->addWidget(ipEdit);
    settingLayout->addWidget(new QLabel("Port:"));
    settingLayout->addWidget(portEdit);
    settingLayout->addWidget(startBtn);

    mainLayout->addLayout(settingLayout);
    mainLayout->addWidget(new QLabel("伺服器日誌:"));
    mainLayout->addWidget(logEdit);

    // --- 2. 初始化 Server ---
    tcpServer = new QTcpServer(this);

    // 連接啟動按鈕
    connect(startBtn, &QPushButton::clicked, this, &Widget::startServer);
    // 當有新連線時
    connect(tcpServer, &QTcpServer::newConnection, this, &Widget::onNewConnection);
}

Widget::~Widget() {
    tcpServer->close();
}

void Widget::startServer() {
    QString ip = ipEdit->text();
    quint16 port = portEdit->text().toUShort();

    // 手動設定 IP 位置進行監聽
    if (tcpServer->listen(QHostAddress(ip), port)) {
        logEdit->append(QString("伺服器成功啟動於 %1:%2").arg(ip).arg(port));
        startBtn->setEnabled(false); // 啟動後禁止再點擊
        ipEdit->setReadOnly(true);
        portEdit->setReadOnly(true);
    } else {
        logEdit->append("錯誤: 無法啟動伺服器，請檢查 IP 是否正確。");
    }
}

void Widget::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Widget::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Widget::onDisconnected);
    logEdit->append("新連線已進入，等待登入訊息...");
}

void Widget::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QString type = obj["type"].toString();

    if (type == "login") {
        QString nickname = obj["nickname"].toString();
        clientList[nickname] = socket;
        logEdit->append(QString("使用者登入: %1").arg(nickname));
        updateUserList();
    }
    else if (type == "chat") {
        QString target = obj["target"].toString();
        QString sender = obj["sender"].toString();
        if (clientList.contains(target)) {
            clientList[target]->write(data); // 一對一轉發
            logEdit->append(QString("訊息轉發: %1 -> %2").arg(sender).arg(target));
        }
    }
}

void Widget::onDisconnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        QString nickname = clientList.key(socket);
        if (!nickname.isEmpty()) {
            clientList.remove(nickname);
            logEdit->append(QString("使用者離開: %1").arg(nickname));
            updateUserList();
        }
        socket->deleteLater();
    }
}

void Widget::updateUserList() {
    QJsonObject res;
    res["type"] = "userlist";
    QJsonArray users;
    for (const QString &name : clientList.keys()) {
        users.append(name);
    }
    res["users"] = users;

    QByteArray data = QJsonDocument(res).toJson();
    for (QTcpSocket *s : clientList.values()) {
        s->write(data);
    }
}
