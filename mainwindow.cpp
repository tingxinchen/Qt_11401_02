#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tcpServer(nullptr)
    , serverPort(0)
{
    ui->setupUi(this);
    setWindowTitle("Qt 通訊軟體");
    
    loadFriendsList();
    
    // Initialize server
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
}

MainWindow::~MainWindow()
{
    saveFriendsList();
    delete ui;
}

void MainWindow::on_addFriendButton_clicked()
{
    bool ok;
    QString friendName = QInputDialog::getText(this, "新增好友", 
                                               "好友名稱:", 
                                               QLineEdit::Normal,
                                               "", &ok);
    
    if (ok && !friendName.isEmpty()) {
        // Check if friend already exists
        QList<QListWidgetItem*> items = ui->friendsList->findItems(friendName, Qt::MatchExactly);
        if (items.isEmpty()) {
            ui->friendsList->addItem(friendName);
            saveFriendsList();
            QMessageBox::information(this, "成功", QString("已新增好友: %1").arg(friendName));
        } else {
            QMessageBox::warning(this, "錯誤", "好友已存在!");
        }
    }
}

void MainWindow::on_removeFriendButton_clicked()
{
    QListWidgetItem *currentItem = ui->friendsList->currentItem();
    if (currentItem) {
        QString friendName = currentItem->text();
        
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "刪除好友", 
                                     QString("確定要刪除好友 %1 嗎?").arg(friendName),
                                     QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            delete currentItem;
            
            // Close chat window if open
            if (chatWindows.contains(friendName)) {
                chatWindows[friendName]->close();
                delete chatWindows[friendName];
                chatWindows.remove(friendName);
            }
            
            saveFriendsList();
        }
    } else {
        QMessageBox::warning(this, "錯誤", "請先選擇一個好友!");
    }
}

void MainWindow::on_friendsList_itemDoubleClicked(QListWidgetItem *item)
{
    QString friendName = item->text();
    
    // Check if chat window already exists
    if (!chatWindows.contains(friendName)) {
        ChatWindow *chatWindow = new ChatWindow(friendName, this);
        chatWindows[friendName] = chatWindow;
        
        // Connect signals from chat window
        connect(chatWindow, &ChatWindow::destroyed, this, [this, friendName]() {
            chatWindows.remove(friendName);
        });
    }
    
    chatWindows[friendName]->show();
    chatWindows[friendName]->raise();
    chatWindows[friendName]->activateWindow();
}

void MainWindow::on_startServerButton_clicked()
{
    if (!tcpServer->isListening()) {
        bool ok;
        int port = QInputDialog::getInt(this, "啟動伺服器", 
                                       "請輸入伺服器連接埠:", 
                                       8888, 1024, 65535, 1, &ok);
        
        if (ok) {
            if (tcpServer->listen(QHostAddress::Any, port)) {
                serverPort = port;
                ui->startServerButton->setText("停止伺服器");
                ui->serverStatusLabel->setText(QString("伺服器運行中 (連接埠: %1)").arg(port));
                QMessageBox::information(this, "成功", 
                                       QString("伺服器已在連接埠 %1 啟動").arg(port));
            } else {
                QMessageBox::critical(this, "錯誤", 
                                    QString("無法啟動伺服器: %1").arg(tcpServer->errorString()));
            }
        }
    } else {
        tcpServer->close();
        ui->startServerButton->setText("啟動伺服器");
        ui->serverStatusLabel->setText("伺服器已停止");
        QMessageBox::information(this, "提示", "伺服器已停止");
    }
}

void MainWindow::newConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::clientDisconnected);
    
    ui->serverStatusLabel->setText(QString("伺服器運行中 (連接埠: %1) - 已連接客戶端").arg(serverPort));
}

void MainWindow::readyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    QByteArray data = socket->readAll();
    QString message = QString::fromUtf8(data);
    
    // Parse message format: "FRIEND_NAME:MESSAGE_TEXT"
    int separatorIndex = message.indexOf(':');
    if (separatorIndex > 0) {
        QString friendName = message.left(separatorIndex);
        QString messageText = message.mid(separatorIndex + 1);
        
        // Store socket-friend mapping if not exists
        if (!socketToFriend.contains(socket)) {
            socketToFriend[socket] = friendName;
        }
        
        // Open or update chat window
        if (!chatWindows.contains(friendName)) {
            ChatWindow *chatWindow = new ChatWindow(friendName, this);
            chatWindows[friendName] = chatWindow;
            chatWindow->show();
            
            connect(chatWindow, &ChatWindow::destroyed, this, [this, friendName]() {
                chatWindows.remove(friendName);
            });
        }
        
        chatWindows[friendName]->receiveMessage(messageText);
        chatWindows[friendName]->setSocket(socket);
    }
}

void MainWindow::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        socketToFriend.remove(socket);
        socket->deleteLater();
    }
}

void MainWindow::saveFriendsList()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir;
    if (!dir.exists(configPath)) {
        dir.mkpath(configPath);
    }
    
    QFile file(configPath + "/friends.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (int i = 0; i < ui->friendsList->count(); ++i) {
            out << ui->friendsList->item(i)->text() << "\n";
        }
        file.close();
    }
}

void MainWindow::loadFriendsList()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(configPath + "/friends.txt");
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                ui->friendsList->addItem(line);
            }
        }
        file.close();
    }
}
