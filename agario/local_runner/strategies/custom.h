#ifndef CUSTOM_H
#define CUSTOM_H

#include "strategy.h"
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>


class Custom : public Strategy
{
    Q_OBJECT

protected:
    QProcess *solution;
    bool is_running;
    QMetaObject::Connection finish_connection;

signals:
    void error(int, QString);

public:
    explicit Custom(int _id, const QString &_path) :
        Strategy(_id),
        solution(new QProcess(this))
    {
        solution->start(_path);
        finish_connection = connect(solution, SIGNAL(finished(int)), this, SLOT(on_finished(int)));
        connect(solution, SIGNAL(readyReadStandardError()), this, SLOT(on_error()));
        is_running = true;
        send_config();
    }

    virtual ~Custom() {
        Constants &ins = Constants::instance();
        if (solution) {
            disconnect(finish_connection);
            PlayerArray pa;
            CircleArray ca;
            QString message = prepare_state(pa, ca);
            int sent = solution->write(message.toStdString().c_str());
            solution->waitForBytesWritten(5000);
            bool success = solution->waitForReadyRead(ins.RESP_TIMEOUT * 1000);
            if (!success) {
                solution->waitForFinished(5000);
            }
            solution->close();
            delete solution;
        }
    }

    virtual Direct tickEvent(const PlayerArray &fragments, const CircleArray &objects) {
        if (! is_running) {
            return Direct(0, 0);
        }
        QString message = prepare_state(fragments, objects);
        qDebug() << message;
        int sent = solution->write(message.toStdString().c_str());
        if (sent == -1) {
            emit error(id, "Can't write to process");
            return Direct(0, 0);
        }
        Constants &ins = Constants::instance();
        QByteArray cmdBytes = "";
        while (! cmdBytes.endsWith('\n')) {
            bool success = solution->waitForReadyRead(ins.RESP_TIMEOUT * 1000);
            if (! success) {
                cmdBytes.append(solution->readAllStandardOutput());
                cmdBytes.append(solution->readAllStandardError());
                cmdBytes.append(solution->readAll());
                qDebug() << cmdBytes;
                emit error(id, "Can't wait for process answer (limit expired)");
                return Direct(0, 0);
            }
            cmdBytes.append(solution->readLine());
        }

        QJsonObject json = parse_answer(cmdBytes);
        QStringList keys = json.keys();
        if (! keys.contains("X") || ! keys.contains("Y")) {
            emit error(id, "No X or Y keys in answer json");
            return Direct(0, 0);
        }

        double x = json.value("X").toDouble(0.0);
        double y = json.value("Y").toDouble(0.0);
        Direct result(x, y);
        if (keys.contains("Split")) {
            result.split = json.value("Split").toBool(false);
        }
        if (keys.contains("Eject")) {
            result.eject = json.value("Eject").toBool(false);
        }
        return result;
    }

public slots:
    void on_finished(int code) {
        is_running = false;
        emit error(id, "Process finished with code " + QString::number(code));
    }

    void on_error() {
        QByteArray err_data =  solution->readAllStandardError();
        emit error(id, QString(err_data));
    }

public:
    void send_config() {
        QJsonDocument jsonDoc(Constants::instance().toJson());
        QString message = QString(jsonDoc.toJson(QJsonDocument::Compact)) + "\n";

        qDebug() << message;
        int sent = solution->write(message.toStdString().c_str());
        if (sent == 0) {
            emit error(id, "Can't write config to process");
        }
    }

    QString prepare_state(const PlayerArray &fragments, const CircleArray &visibles) {
        QJsonArray mineArray;
        for (Player *player : fragments) {
            mineArray.append(player->toJson(true));
        }
        QJsonArray objectsArray;
        for (Circle *circle : visibles) {
            objectsArray.append(circle->toJson());
        }
        QJsonObject json;
        json.insert("Mine", mineArray);
        json.insert("Objects", objectsArray);

        QJsonDocument jsonDoc(json);
        return QString(jsonDoc.toJson(QJsonDocument::Compact)) + "\n";
    }

    QJsonObject parse_answer(QByteArray &data) {
        QJsonObject empty;
        if (data.length() < 3) {
            return empty;
        }
        if (data[data.length() - 1] == '\n') {
            data = data.left(data.length() - 1);
        }

        QJsonParseError err;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &err);
        if (jsonDoc.isNull()) {
            return empty;
        }
        return jsonDoc.object();
    }
};

#endif // CUSTOM_H
