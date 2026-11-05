#include "appquitter.hpp"

#include <QCoreApplication>
#include <QTimer>

void AppQuitter::quit() const
{
    QTimer::singleShot(0, qApp, []() { QCoreApplication::exit(0); });
}
