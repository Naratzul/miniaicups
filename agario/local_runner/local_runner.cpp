#include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[]) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    Constants::initialize(env);

    QApplication a(argc, argv);

    QVector<QString> commands;
    commands.reserve(argc);

    for (auto it = argv; it != argv + argc; ++it) {
        commands.append(*it);
    }

    bool batch_mode = commands.contains("--batch-mode");
    QString file_name = "result.txt";
    for (const auto& cmd: commands) {
        if (cmd.contains("--save-results=")) {
            file_name = cmd.mid(cmd.indexOf('=') + 1);
        }
    }

    MainWindow window(batch_mode, file_name);
    if (!batch_mode) {
        window.show();
    }

    return a.exec();
}
