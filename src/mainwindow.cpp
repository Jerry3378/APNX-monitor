// mainwindow.cpp
// test.txt 디자인 — 여백 + 정보 위계 + 카드 형태 그대로 구현

#include "mainwindow.h"
#include "plcworker.h"
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_worker = new PlcWorker(this);
    connect(m_worker, &PlcWorker::sensorDataUpdated, this, &MainWindow::onSensorDataUpdated);
    connect(m_worker, &PlcWorker::connectionStatusChanged, this, &MainWindow::onConnectionStatusChanged);
    connect(m_worker, &PlcWorker::logMessage, this, &MainWindow::onLogMessage);
    setupUi();
}
MainWindow::~MainWindow() {
    if (m_worker && m_worker->isRunning()) { m_worker->requestStop(); m_worker->wait(); }
}

QFrame* MainWindow::makeCard() {
    auto *f = new QFrame(this);
    f->setFrameShape(QFrame::StyledPanel);
    f->setStyleSheet("QFrame { background:#ffffff; border:1px solid #e0e0e0; border-radius:10px; }");
    return f;
}

void MainWindow::setupUi() {
    // 전체 배경 + 여백 — test.txt처럼 카드 사이 여백을 넉넉히
    auto *central = new QWidget(this);
    central->setStyleSheet("background:#f5f5f7;");
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(12);

    // ── Header: APNX Monitor + ● Connected — 최상위 위계
    auto *header = makeCard();
    auto *hLay = new QHBoxLayout(header);
    hLay->setContentsMargins(16,12,16,12);
    auto *title = new QLabel("APNX Monitor", header);
    title->setStyleSheet("font-size:16px; font-weight:700; color:#111; border:none;");
    m_statusDot = new QLabel("●", header);
    m_statusDot->setStyleSheet("color:#c62828; font-size:14px; border:none;");
    m_statusLabel = new QLabel("Disconnected", header);
    m_statusLabel->setStyleSheet("font-size:12px; color:#666; border:none;");
    hLay->addWidget(title);
    hLay->addStretch();
    hLay->addWidget(m_statusDot);
    hLay->addWidget(m_statusLabel);
    root->addWidget(header);

    // ── DEVICE CONTROL 카드 — 두 번째 위계
    auto *devCard = makeCard();
    auto *devLay = new QVBoxLayout(devCard);
    devLay->setContentsMargins(16,14,16,14);
    devLay->setSpacing(10);
    auto *devTitle = new QLabel("DEVICE CONTROL", devCard);
    devTitle->setStyleSheet("font-size:10px; font-weight:700; color:#888; letter-spacing:1px; border:none;");
    devLay->addWidget(devTitle);
    auto *infoGrid = new QGridLayout();
    infoGrid->setHorizontalSpacing(24);
    infoGrid->setVerticalSpacing(6);
    auto *lbDev = new QLabel("Device", devCard); lbDev->setStyleSheet("font-size:10px; color:#999; border:none;");
    auto *lbMode = new QLabel("Mode", devCard); lbMode->setStyleSheet("font-size:10px; color:#999; border:none;");
    m_deviceValue = new QLabel("/dev/myusb0", devCard); m_deviceValue->setStyleSheet("font-size:12px; font-weight:600; color:#111; border:none;");
    m_modeValue = new QLabel("Hardware", devCard); m_modeValue->setStyleSheet("font-size:12px; font-weight:600; color:#111; border:none;");
    infoGrid->addWidget(lbDev, 0, 0); infoGrid->addWidget(m_deviceValue, 1, 0);
    infoGrid->addWidget(lbMode, 0, 1); infoGrid->addWidget(m_modeValue, 1, 1);
    devLay->addLayout(infoGrid);
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);
    m_btnConnectDevice = new QPushButton("Connect", devCard);
    m_btnSimulate = new QPushButton("Start", devCard);
    m_btnStop = new QPushButton("Stop", devCard);
    // 버튼 위계: Connect는 외곽선, Start는 강조, Stop은 중립
    m_btnConnectDevice->setStyleSheet("QPushButton { padding:8px 16px; border:1px solid #ccc; border-radius:6px; background:#fff; } QPushButton:hover { background:#f0f0f0; }");
    m_btnSimulate->setStyleSheet("QPushButton { padding:8px 16px; border:none; border-radius:6px; background:#1976d2; color:#fff; } QPushButton:hover { background:#1565c0; }");
    m_btnStop->setStyleSheet("QPushButton { padding:8px 16px; border:1px solid #ddd; border-radius:6px; background:#fff; } QPushButton:hover { background:#fce4ec; }");
    btnRow->addWidget(m_btnConnectDevice);
    btnRow->addStretch();
    btnRow->addWidget(m_btnSimulate);
    btnRow->addWidget(m_btnStop);
    devLay->addLayout(btnRow);
    root->addWidget(devCard);
    connect(m_btnConnectDevice, &QPushButton::clicked, this, &MainWindow::onConnectDeviceClicked);
    connect(m_btnSimulate, &QPushButton::clicked, this, &MainWindow::onSimulateClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    // ── ANALOG TAGS 카드 — 핵심 정보, 가장 넓은 여백
    auto *tagCard = makeCard();
    auto *tagLay = new QVBoxLayout(tagCard);
    tagLay->setContentsMargins(16,14,16,14);
    tagLay->setSpacing(12);
    auto *tagTitle = new QLabel("ANALOG TAGS", tagCard);
    tagTitle->setStyleSheet("font-size:10px; font-weight:700; color:#888; letter-spacing:1px; border:none;");
    tagLay->addWidget(tagTitle);
    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);
    for (int i=0;i<CHANNELS;i++) {
        m_box[i] = new QFrame(tagCard);
        m_box[i]->setFrameShape(QFrame::StyledPanel);
        m_box[i]->setStyleSheet("QFrame { background:#fcfcfc; border:1px solid #eaeaea; border-radius:10px; }");
        auto *vbox = new QVBoxLayout(m_box[i]);
        vbox->setContentsMargins(14,12,14,14);
        vbox->setSpacing(4);
        m_tagName[i] = new QLabel(QString("TAG %1").arg(i+1), m_box[i]);
        m_tagName[i]->setStyleSheet("font-size:10px; font-weight:600; color:#999; border:none;");
        m_addr[i] = new QLabel(QString("0x%1").arg(0x300 + i*2, 4, 16, QLatin1Char('0')).toUpper(), m_box[i]);
        m_addr[i]->setStyleSheet("font-size:10px; color:#bbb; border:none;");
        m_value[i] = new QLabel("--", m_box[i]);
        m_value[i]->setAlignment(Qt::AlignRight);
        m_value[i]->setStyleSheet("font-size:22px; font-weight:700; color:#111; border:none; padding-top:8px;");
        vbox->addWidget(m_tagName[i]);
        vbox->addWidget(m_addr[i]);
        vbox->addStretch();
        vbox->addWidget(m_value[i]);
        grid->addWidget(m_box[i], i/2, i%2);
    }
    tagLay->addLayout(grid);
    root->addWidget(tagCard);

    // ── PROTOCOL LOG 카드 — 하위 위계, 모노스페이스
    auto *logCard = makeCard();
    auto *logLay = new QVBoxLayout(logCard);
    logLay->setContentsMargins(16,14,16,14);
    logLay->setSpacing(8);
    auto *logTitle = new QLabel("PROTOCOL LOG", logCard);
    logTitle->setStyleSheet("font-size:10px; font-weight:700; color:#888; letter-spacing:1px; border:none;");
    logLay->addWidget(logTitle);
    m_log = new QTextEdit(logCard);
    m_log->setReadOnly(true);
    m_log->setStyleSheet("QTextEdit { background:#fafafa; border:1px solid #eee; border-radius:6px; font-family:monospace; font-size:10px; color:#333; }");
    m_log->setFixedHeight(110);
    m_log->setPlaceholderText("TX / RX 패킷 로그 — 랜덤 4개 주소 반복 (정지 전까지)");
    logLay->addWidget(m_log);
    root->addWidget(logCard);

    root->addStretch();
    setCentralWidget(central);
    setWindowTitle("APNX Monitor");
    resize(560, 640);
}

void MainWindow::onConnectDeviceClicked() {
    if (m_worker->isRunning()) { m_worker->requestStop(); m_worker->wait(); }
    m_modeValue->setText("Hardware");
    m_worker->setSimulation(false);
    m_worker->start();
}
void MainWindow::onSimulateClicked() {
    if (m_worker->isRunning()) { m_worker->requestStop(); m_worker->wait(); }
    m_modeValue->setText("Simulation");
    m_worker->setSimulation(true);
    m_worker->start();
}
void MainWindow::onStopClicked() { m_worker->requestStop(); m_log->append("정지 요청됨."); }

void MainWindow::onSensorDataUpdated(const QVector<double> &values, quint8 cmd, quint8 id) {
    Q_UNUSED(cmd); Q_UNUSED(id);
    for (int i=0;i<CHANNELS;i++) {
        if (i < values.size()) {
            int v = static_cast<int>(values[i]);
            // test.txt처럼 천 단위 콤마
            m_value[i]->setText(QString("%L1").arg(v));
        } else m_value[i]->setText("--");
    }
}
void MainWindow::onConnectionStatusChanged(const QString &status) {
    bool ok = status.contains("연결됨") || status.contains("Connected");
    m_statusDot->setStyleSheet(QString("color:%1; font-size:14px; border:none;").arg(ok ? "#2e7d32" : "#c62828"));
    m_statusLabel->setText(ok ? "Connected" : "Disconnected");
    m_statusLabel->setStyleSheet(QString("font-size:12px; color:%1; border:none;").arg(ok ? "#2e7d32" : "#666"));
    m_log->append("[상태] " + status);
}
void MainWindow::onLogMessage(const QString &msg) { m_log->append(msg); }
