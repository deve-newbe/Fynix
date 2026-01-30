/**
 *  \file       WidgetTreeTextBox.hpp
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

#ifndef DIALOGBINEDITOR_LINEEDIT_H
#define DIALOGBINEDITOR_LINEEDIT_H

#include "FileBin_DWARF.h"
#include <QLineEdit>


//using namespace std;

class WidgetTreeTextBox : public QLineEdit
{
    Q_OBJECT

public slots:

    //  void on_table_itemChanged(int row, int col);

    // Q_OBJECT
private slots:
    void onTextChanged(const QString &text);

private:

    int m_firstInt;
    int m_secondInt;
    bool Init;


    virtual void mouseReleaseEvent(QMouseEvent *e);
    void RefreshState(void);

signals:
    void editingFinishedWithInts(int firstInt, int secondInt);
    void mouseReleasedWithInts(int firstInt, int secondInt);


public:
    uint32_t Int1, Int2;
    float DefaultVal;
   // BinCalibToolWidget *pDialog;
    bool showTable;
    uint32_t Idx;
    void (*pChangeCbk)(void * parent_class, int row, int col);
    void * ChangeCbk_class;
    QWidget *parent;
    uint32_t yLen;

    void parseTable();

    void SetVal(const QString &text);

    WidgetTreeTextBox(QWidget *parent, bool showTable, uint32_t Idx, int firstInt, int secondInt, float DefaultVal);// : QLineEdit(parent);


};


#endif // DIALOGBINEDITOR_LINEEDIT_H
