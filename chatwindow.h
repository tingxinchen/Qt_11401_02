#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(const QString &friendName, QWidget *parent = nullptr);
    ~ChatWindow();
    
    void receiveMessage(const QString &message);
    void setSocket(QTcpSocket *socket);

private slots:
    void on_sendButton_clicked();
    void on_connectButton_clicked();
    void on_messageInput_returnPressed();
    void socketConnected();
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError error);

private:
    Ui::ChatWindow *ui;
    QString friendName;
    QTcpSocket *tcpSocket;
    
    void appendMessage(const QString &sender, const QString &message);
};

#endif // CHATWINDOW_H
