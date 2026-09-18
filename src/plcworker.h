// plcworker.h
// /dev/myusb0 를 백그라운드에서 TX/RX 하는 스레드 — real_test.c 참조
//  real_test.c 의 plc_write/plc_read 처럼 패킷을 만들고 write()/read()로 주고받음

#pragma once
#include <QThread>
#include <QVector>
#include <QString>

class PlcWorker : public QThread {
    Q_OBJECT
public:
    explicit PlcWorker(QObject *parent = nullptr);
    void setSimulation(bool on);
    void requestStop();

signals:
    void sensorDataUpdated(const QVector<double> &values, quint8 cmd, quint8 id);
    void connectionStatusChanged(const QString &status);
    void logMessage(const QString &msg);

protected:
    void run() override;

private:
    bool m_simulation = false;
    bool m_stopRequested = false;

    void runRealTests(int fd);      
    void runSimulatedTests();      
};
