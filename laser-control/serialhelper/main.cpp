#include "operatorform.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    // 高DPI支持设置（Qt 5.6+）
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    // Qt 5.14+ 使用新的高DPI缩放策略
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );
#endif

    QApplication a(argc, argv);

    // 设置全局默认编码为 UTF-8，防止中文乱码
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    operatorForm w;
    w.show();

    return a.exec();
}
