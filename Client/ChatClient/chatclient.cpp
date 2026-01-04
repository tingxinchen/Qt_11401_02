#include "chatclient.h"
#include "ui_chatclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>//存好友名單
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDateTime> //傳送時間

ChatClient::ChatClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatClient)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    // 1. 接收訊息
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);

    // 2. 連線成功處理
    connect(socket, &QTcpSocket::connected, this, [=](){
        ui->chatDisplay->addItem("系統提示: 成功連線到伺服器！");
        ui->connectBtn->setEnabled(false);    // 停用連線按鈕
        ui->disconnectBtn->setEnabled(true);  // 啟用斷開按鈕
    });

    // 3. 偵測斷開連線
    connect(socket, &QTcpSocket::disconnected, this, [=](){
        ui->chatDisplay->addItem("系統提示: 連線已中斷。");
        ui->connectBtn->setEnabled(true);     // 恢復連線按鈕
        ui->disconnectBtn->setEnabled(false); // 停用斷開按鈕
        ui->userList->clear();               // 清空好友名單
        currentTarget = "";                  // 重置聊天目標
    });

    // 4. 連結好友名單選取
    connect(ui->userList, &QListWidget::itemClicked, this, &ChatClient::onUserSelected);
}

ChatClient::~ChatClient()
{
    socket->close();
    delete ui;
}

// --- 連線按鈕 ---
void ChatClient::on_connectBtn_clicked()
{
    myName = ui->nameEdit->text().trimmed();
    if (myName.isEmpty()) {
        myName = "用戶1";
        ui->nameEdit->setText(myName);
    }

    QString ip = ui->ipEdit->text();
    quint16 port = ui->Port->text().toUShort();

    socket->connectToHost(ip, port);

    if (socket->waitForConnected(3000)) {
        QJsonObject login;
        login["type"] = "login";
        login["nickname"] = myName;
        socket->write(QJsonDocument(login).toJson());
    } else {
        QMessageBox::critical(this, "錯誤", "連線失敗，請檢查伺服器是否開啟或 IP 是否正確");
    }
}

// --- 中斷連線按鈕 ---
void ChatClient::on_disconnectBtn_clicked()
{
    if (socket->isOpen()) {
        socket->disconnectFromHost();
    }
}

// --- 好友選取 ---
void ChatClient::onUserSelected()
{
    if (ui->userList->currentItem()) {
        currentTarget = ui->userList->currentItem()->text();
        ui->chatDisplay->addItem(QString("--- 正在與 %1 一對一聊天 ---").arg(currentTarget));
    }
}

// 訊息傳送
void ChatClient::on_sendBtn_clicked()
{
    if (currentTarget.isEmpty()) {
        QMessageBox::warning(this, "提醒", "請先點選左側好友名單選擇聊天對象");
        return;
    }

    QString message = ui->msgEdit->text();
    if (message.isEmpty()) return;

    QJsonObject chat;
    chat["type"] = "chat";
    chat["sender"] = myName;
    chat["target"] = currentTarget;
    chat["content"] = message;

    socket->write(QJsonDocument(chat).toJson());

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->chatDisplay->addItem(QString("[%1] 我 -> %2: %3").arg(time).arg(currentTarget).arg(message));
    ui->msgEdit->clear();
}

// --- 檔案傳送 ---
void ChatClient::on_fileBtn_clicked()
{
    if (currentTarget.isEmpty()) {
        QMessageBox::warning(this, "提醒", "請先點選對象再傳送檔案");
        return;
    }

    QString path = QFileDialog::getOpenFileName(this, "選擇檔案或圖片");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData = file.readAll();

        QJsonObject fileObj;
        fileObj["type"] = "chat";
        fileObj["sender"] = myName;
        fileObj["target"] = currentTarget;
        fileObj["fileName"] = QFileInfo(path).fileName();
        fileObj["fileContent"] = QString(fileData.toBase64());

        socket->write(QJsonDocument(fileObj).toJson());
        ui->chatDisplay->addItem(QString("已傳送檔案: %1").arg(fileObj["fileName"].toString()));
    }
}

// --- 接收訊息 ---
void ChatClient::onReadyRead()
{
    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "userlist") {
        ui->userList->clear();
        QJsonArray users = obj["users"].toArray();
        for (int i = 0; i < users.size(); ++i) {
            QString name = users[i].toString();
            if (name != myName) {
                ui->userList->addItem(name);
            }
        }
    }
    else if (type == "chat") {
        QString sender = obj["sender"].toString();
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");

        if (obj.contains("fileContent")) {
            QString fileName = obj["fileName"].toString();
            QByteArray fileData = QByteArray::fromBase64(obj["fileContent"].toString().toUtf8());

            if (QMessageBox::question(this, "收到檔案",
                                      QString("[%1] 來自 %2 的檔案：%3\n是否儲存？").arg(time).arg(sender).arg(fileName)) == QMessageBox::Yes) {

                QString savePath = QFileDialog::getSaveFileName(this, "儲存檔案", fileName);
                if (!savePath.isEmpty()) {
                    QFile file(savePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(fileData);
                        file.close();
                        ui->chatDisplay->addItem(QString("[系統]: %1 儲存成功").arg(fileName));
                    }
                }
            }
        } else {
            QString content = obj["content"].toString();
            ui->chatDisplay->addItem(QString("[%1] %2: %3").arg(time).arg(sender).arg(content));
        }
    }
}
