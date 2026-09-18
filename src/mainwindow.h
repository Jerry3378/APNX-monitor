// mainwindow.h
// test.txt 디자인 — 여백 + 정보 위계 + 카드 형태

#pragma once
#include <QMainWindow>
#include <QVector>
#include <QString>
class PlcWorker;
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QGridLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private slots:
    void onConnectDeviceClicked();
    void onSimulateClicked();
    void onStopClicked();
    void onSensorDataUpdated(const QVector<double> &values, quint8 cmd, quint8 id);
    void onConnectionStatusChanged(const QString &status);
    void onLogMessage(const QString &msg);
private:
    QLabel *m_statusDot; // ●
    QLabel *m_statusLabel;
    QLabel *m_deviceValue;
    QLabel *m_modeValue;
    QPushButton *m_btnConnectDevice;
    QPushButton *m_btnSimulate;
    QPushButton *m_btnStop;
    static const int CHANNELS = 4;
    QFrame *m_box[CHANNELS];
    QLabel *m_value[CHANNELS];
    QLabel *m_tagName[CHANNELS];
    QLabel *m_addr[CHANNELS];
    QTextEdit *m_log;
    PlcWorker *m_worker;
    void setupUi();
    QFrame* makeCard();
};
