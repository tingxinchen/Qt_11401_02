#include "chatwindow.h"
#include "ui_chatwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>

ChatWindow::ChatWindow(const QString &friendName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
    , friendName(friendName)
    , tcpSocket(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(QString("與 %1 聊天").arg(friendName));
    
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &ChatWindow::socketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &ChatWindow::socketDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(tcpSocket, &QAbstractSocket::errorOccurred, this, &ChatWindow::socketError);
#else
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &ChatWindow::socketError);
#endif
    
    ui->connectionStatusLabel->setText("未連接");
}

ChatWindow::~ChatWindow()
{
    if (tcpSocket) {
        tcpSocket->disconnectFromHost();
    }
    delete ui;
}

void ChatWindow::on_sendButton_clicked()
{
    QString message = ui->messageInput->text().trimmed();
    
    if (message.isEmpty()) {
        return;
    }
    
    if (!tcpSocket || tcpSocket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "錯誤", "未連接到伺服器!");
        return;
    }
    
    // Format: "FRIEND_NAME:MESSAGE_TEXT"
    QString formattedMessage = QString("%1:%2").arg(friendName, message);
    QByteArray data = formattedMessage.toUtf8();
    
    tcpSocket->write(data);
    tcpSocket->flush();
    
    appendMessage("我", message);
    ui->messageInput->clear();
}

void ChatWindow::on_connectButton_clicked()
{
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
        ui->connectButton->setText("連接");
        ui->connectionStatusLabel->setText("未連接");
    } else {
        bool ok;
        QString host = QInputDialog::getText(this, "連接到伺服器", 
                                            "伺服器IP位址:", 
                                            QLineEdit::Normal,
                                            "127.0.0.1", &ok);
        if (!ok) return;
        
        int port = QInputDialog::getInt(this, "連接到伺服器", 
                                       "伺服器連接埠:", 
                                       8888, 1024, 65535, 1, &ok);
        if (!ok) return;
        
        ui->connectionStatusLabel->setText("連接中...");
        tcpSocket->connectToHost(host, port);
    }
}

void ChatWindow::on_messageInput_returnPressed()
{
    on_sendButton_clicked();
}

void ChatWindow::receiveMessage(const QString &message)
{
    appendMessage(friendName, message);
}

void ChatWindow::setSocket(QTcpSocket *socket)
{
    if (tcpSocket && tcpSocket != socket) {
        // Disconnect all signals from old socket
        disconnect(tcpSocket, nullptr, this, nullptr);
        tcpSocket->disconnectFromHost();
        tcpSocket->deleteLater();
        tcpSocket = nullptr;  // Clear pointer before assigning new socket
    }
    
    tcpSocket = socket;
    
    if (tcpSocket) {
        connect(tcpSocket, &QTcpSocket::connected, this, &ChatWindow::socketConnected);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &ChatWindow::socketDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(tcpSocket, &QAbstractSocket::errorOccurred, this, &ChatWindow::socketError);
#else
        connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                this, &ChatWindow::socketError);
#endif
        
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
            ui->connectButton->setText("斷開");
            ui->connectionStatusLabel->setText("已連接");
        }
    }
}

void ChatWindow::socketConnected()
{
    ui->connectButton->setText("斷開");
    ui->connectionStatusLabel->setText("已連接");
    appendMessage("系統", "已連接到伺服器");
}

void ChatWindow::socketDisconnected()
{
    ui->connectButton->setText("連接");
    ui->connectionStatusLabel->setText("未連接");
    appendMessage("系統", "已斷開連接");
}

void ChatWindow::socketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QMessageBox::critical(this, "連接錯誤", tcpSocket->errorString());
    ui->connectionStatusLabel->setText("連接失敗");
}

void ChatWindow::appendMessage(const QString &sender, const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage = QString("[%1] %2: %3").arg(timestamp, sender, message);
    ui->chatDisplay->append(formattedMessage);
}
