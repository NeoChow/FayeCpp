
#include "websocketqt.h"

#if defined(HAVE_SUITABLE_QT_VERSION)

#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>

#include "../fayecpp.h"

namespace FayeCpp {

	void WebSocketQt::connected()
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQt:" << "connected";
#endif
		this->onConnected();
	}

	void WebSocketQt::disconnected()
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		const QString err(_socket->errorString());
		if (err.isEmpty()) qDebug() << "SocketQt:" << "disconnected without error";
		else qDebug() << "SocketQt:" << "disconnected with error:" << err;
#endif
		this->onDisconnected();
	}

	void WebSocketQt::textMessageReceived(const QString & message)
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQt:" << "received text:" << message;
#endif
		QByteArray bytes(message.toUtf8());
		if (bytes.size() > 0)
		{
			this->onTextReceived((const char *)bytes.data());
		}
	}

	void WebSocketQt::binaryMessageReceived(const QByteArray & message)
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQt:" << "received bin:" << (const char *)message.data();
#endif
		if (message.size() > 0)
		{
			this->onDataReceived((const unsigned char *)message.data(), (REUInt32)message.size());
		}
	}

	void WebSocketQt::error(QAbstractSocket::SocketError error)
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQt:" << "ERROR:" << error;
#endif
		const QString err(_socket->errorString());
		if (err.isEmpty())
		{
			this->onError(REString("TODO: unprocessed qt socket error"));
		}
		else
		{
			this->onError(REString(err.toUtf8().data()));
		}
		(void)error;
	}

	void WebSocketQt::sendData(const unsigned char * data, const REUInt32 dataSize)
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQt:" << "send bin:" << (const char*)data;
#endif
		this->updateLastSendTime();
		_socket->sendBinaryMessage(QByteArray((char*)data, (int)dataSize));
	}

	void WebSocketQt::sendText(const char * text, const REUInt32 textSize)
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQt:" << "send text:" << text;
#endif
		this->updateLastSendTime();
		_socket->sendTextMessage(QString(text));
		(void)textSize;
	}

	QString WebSocketQt::toString(const REString & s)
	{
		return s.isNotEmpty() ? QString(s.UTF8String()) : QString("");
	}

	void WebSocketQt::setupSocketWithSSLDataSource(SSLDataSource * dataSource)
	{
		QSslConfiguration config;

		QFile localFile(WebSocketQt::toString(dataSource->clientLocalCertificateFilePath()));
		if (localFile.open(QIODevice::ReadOnly))
		{
			QSslCertificate cert(localFile.readAll());
			localFile.close();
			if (cert.isNull())
			{
#ifdef FAYECPP_DEBUG_MESSAGES
				qDebug() << "SocketQT: LocalCertificate is NULL";
#endif
			}
			else
			{
				config.setLocalCertificate(cert);
			}
		}

		QFile keyFile(WebSocketQt::toString(dataSource->clientPrivateKeyFilePath()));
		if (keyFile.open(QIODevice::ReadOnly))
		{
			QByteArray pp;
			pp.append(WebSocketQt::toString(dataSource->clientPrivateKeyPassPhrase()));
			QSslKey key(keyFile.readAll(),
						QSsl::Rsa,
						QSsl::Pem,
						QSsl::PrivateKey,
						pp);
			pp.clear();
			keyFile.close();
			if (key.isNull())
			{
#ifdef FAYECPP_DEBUG_MESSAGES
				qDebug() << "SocketQT: PrivateKey is NULL";
#endif
			}
			else
			{
				config.setPrivateKey(key);
			}
		}

		QFile caFile(WebSocketQt::toString(dataSource->clientCACertificateFilePath()));
		if (caFile.open(QIODevice::ReadOnly))
		{
			QSslCertificate cert(caFile.readAll());
			caFile.close();
			if (cert.isNull())
			{
#ifdef FAYECPP_DEBUG_MESSAGES
				qDebug() << "SocketQT: CACertificate is NULL";
#endif
			}
			else
			{
				QList<QSslCertificate> caList(config.caCertificates());
				caList.append(cert);
				config.setCaCertificates(caList);
			}
		}

		_socket->setSslConfiguration(config);
	}

	void WebSocketQt::connectToServer()
	{
#ifdef FAYECPP_DEBUG_MESSAGES
		qDebug() << "SocketQT:" << "start connect url:" << this->url().UTF8String();
#endif

		SSLDataSource * dataSource = this->sslDataSource();
		if (dataSource)
		{
			this->setupSocketWithSSLDataSource(dataSource);
		}

		_socket->open(QUrl(this->url().UTF8String()));
	}

	void WebSocketQt::disconnectFromServer()
	{
		_socket->close();
	}

	const REString WebSocketQt::name() const
	{
		return WebSocketQt::transportName();
	}
	
	WebSocketQt::WebSocketQt(ClassMethodWrapper<Client, void(Client::*)(Responce*), Responce> * processMethod) : QObject(), Transport(processMethod),
		_socket(new QWebSocket(QString(""), QWebSocketProtocol::Version13, this))
	{
		Q_CHECK_PTR(_socket);

		connect(_socket, SIGNAL(connected()), this, SLOT(connected()));
		connect(_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
		connect(_socket, SIGNAL(textMessageReceived(QString)), this, SLOT(textMessageReceived(QString)));
		connect(_socket, SIGNAL(binaryMessageReceived(QByteArray)), this, SLOT(binaryMessageReceived(QByteArray)));
		connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
	}

	WebSocketQt::~WebSocketQt()
	{
		disconnect(_socket, SIGNAL(connected()), this, SLOT(connected()));
		disconnect(_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
		disconnect(_socket, SIGNAL(textMessageReceived(QString)), this, SLOT(textMessageReceived(QString)));
		disconnect(_socket, SIGNAL(binaryMessageReceived(QByteArray)), this, SLOT(binaryMessageReceived(QByteArray)));
		disconnect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));

		_socket->close();
		delete _socket;
	}

	REString WebSocketQt::transportName()
	{
		return REString("websocket");
	}
	
}

#endif /* HAVE_SUITABLE_QT_VERSION */

