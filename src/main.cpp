// main.cpp
// 시작점 — QApplication 만들고 MainWindow 띄우기 

#include "mainwindow.h"
#include <QApplication>   // Qt GUI 응용 프로그램 관리자

int main(int argc, char *argv[])
{
    // QApplication: Qt GUI 프로그램은 반드시 하나 있어야 이벤트/창 관리 가능
    QApplication app(argc, argv);

    // 메인 창 객체 생성 (생성자에서 UI 구성 + 워커 준비)
    MainWindow window;

    // 창을 화면에 보이게 함
    window.show();

    // 이벤트 루프 시작: 버튼 클릭/타이머 등 이벤트를 무한히 기다리며 처리.
    // 사용자가 창을 닫으면 exec() 가 끝나고 return 으로 빠져나옴.
    return app.exec();
}
