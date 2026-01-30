/**
 *  \file       mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "WidgetBinCalib.hpp"
#include "FileBin_DWARF.h"
#include <QMainWindow>
#include <qitemselectionmodel.h>
#include <qstandarditemmodel.h>

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private slots:
        void on_actionOpen_triggered(bool checked);

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void dragLeaveEvent(QDragLeaveEvent *event) override;

    private:
        Ui::MainWindow *ui;
        BinCalibToolWidget *ui_BinCalibWidget;

        void loadElf(std::string file_name);
        void onTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
        void displayBinaryFile(FileBin_VarInfoType *symbol);
        void populateTopLevelSymbol(FileBin_VarInfoType* node, QStandardItem* parentItem);
        void AddNewBaseFile(QString Filename);
};
#endif // MAINWINDOW_H
