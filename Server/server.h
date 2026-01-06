#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include<QTcpSocket>
#include<QVector>
#include<QTime>

class server: public QTcpServer
{
    Q_OBJECT
private:
    QVector<QTcpSocket*> Sockets;
    QByteArray Data;
    void SendToClient(QString username, QString str);
    qint16 DataSize;

public:
    server();
    //QTcpSocket *socket;
public slots:
    void incomingConnection(qintptr SoketDescription);
    void SlotReadyRead();

};

#endif // SERVER_H
