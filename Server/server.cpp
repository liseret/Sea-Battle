#include "server.h"

server::server() {
    if (this->listen(QHostAddress::Any,777)){
        qDebug()<<"start";
    }
    else{
        qDebug()<<"error";
    }
    DataSize=0;
}


void server::incomingConnection(qintptr SoketDescription){
    QTcpSocket* NewClient = new QTcpSocket;
    NewClient->setSocketDescriptor(SoketDescription);
    QString* clientUsername = new QString();
    NewClient->setProperty("username", QVariant::fromValue(clientUsername));
    NewClient->setProperty("hasSentGreeting", false);

    connect(NewClient, &QTcpSocket::readyRead, this, &server::SlotReadyRead);
    connect(NewClient, &QTcpSocket::disconnected, this, [this, NewClient](){
        qDebug() << "Client disconnected";
        QString* usernamePtr = NewClient->property("username").value<QString*>();
        QString username = usernamePtr ? *usernamePtr : "Unknown";
        if (!username.isEmpty() && username != "Unknown") {
            SendToClient("SERVER", username + " disconnected");
        }
        Sockets.removeOne(NewClient);
        delete usernamePtr;
        NewClient->deleteLater();
    });

    Sockets.push_back(NewClient);
    qDebug() << "Client connected:" << SoketDescription<< "Total clients:" << Sockets.size();
}

void server::SlotReadyRead(){
    QTcpSocket* CurrClient = qobject_cast<QTcpSocket*>(sender());

    if (!CurrClient) {
        return;
    }
    QDataStream in(CurrClient);
    in.setVersion(QDataStream::Qt_5_15);

    if (in.status() == QDataStream::Ok){
        while (true){
            if (DataSize == 0){
                if(CurrClient->bytesAvailable() < 2){
                    break;
                }
                in >> DataSize;
            }
            if (CurrClient->bytesAvailable() < DataSize){
                break;
            }

            QString str;
            QString user;
            QTime time;
            in >> user >> time >> str;
            DataSize = 0;

            QString* usernamePtr = CurrClient->property("username").value<QString*>();
            bool hasSentGreeting = CurrClient->property("hasSentGreeting").toBool();
            if (usernamePtr && usernamePtr->isEmpty() && !user.isEmpty()) {
                *usernamePtr = user;

                if (!hasSentGreeting) {
                    SendToClient("SERVER", user + " connected to the c4atik!");
                    CurrClient->setProperty("hasSentGreeting", true);
                }
            }
            if (!str.isEmpty()) {
                SendToClient(user, str);
            }
        }
    }
    else{
        qDebug() << "read error :(";
    }
}

void server::SendToClient(QString username, QString str){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    out << quint16(0) << username << QTime::currentTime() << str;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    qDebug() << "Sending to" << Sockets.size() << "clients";

    for (int i = 0; i < Sockets.size(); i++) {
        if (Sockets[i] && Sockets[i]->state() == QTcpSocket::ConnectedState) {
            Sockets[i]->write(block);
        }
    }
}

