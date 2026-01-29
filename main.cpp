/**
 *  \file       main.cpp
 *  \brief      Fynix main window
 *
 *  \version    1.0
 *  \date       Jan 29, 2026
 *  \author     Xavier Descarrega - DEVE embedded designs <info@deve.tech>
 *
 *  \copyright  MIT License
 *
 *              Copyright (c) 2016 Xavier Descarrega
 *
 *              Permission is hereby granted, free of charge, to any person obtaining a copy
 *              of this software and associated documentation files (the "Software"), to deal
 *              in the Software without restriction, including without limitation the rights
 *              to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *              copies of the Software, and to permit persons to whom the Software is
 *              furnished to do so, subject to the following conditions:
 *
 *              The above copyright notice and this permission notice shall be included in all
 *              copies or substantial portions of the Software.
 *
 *              THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *              IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *              FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *              AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER
 *              LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *              OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *              SOFTWARE.
 *
 */

#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    QIcon appIcon(":/icon/logo.png"); // path to your logo file
    a.setWindowIcon(appIcon);

    QCoreApplication::setApplicationName(PROJECT_NAME);
    QCoreApplication::setApplicationVersion(PROJECT_VERSION);

    MainWindow w;
    w.show();
    return a.exec();
}
