/**
 *  \file       WidgetTreeComboBox.hpp
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

#ifndef WIDGETTREECOMBOBOX_HPP
#define WIDGETTREECOMBOBOX_HPP

#include <QApplication>
#include <QStyledItemDelegate>
#include <QComboBox>

// Custom delegate to handle hover cursor for dropdown items
class HoverDelegate : public QStyledItemDelegate
{
    public:

        using QStyledItemDelegate::QStyledItemDelegate;

        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override
        {
            if (event->type() == QEvent::HoverEnter)
            {
                QApplication::setOverrideCursor(Qt::PointingHandCursor); // Change cursor to hand pointer
            }
            else if (event->type() == QEvent::HoverLeave)
            {
                QApplication::restoreOverrideCursor(); // Restore default cursor
            }

            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
};

// Custom QComboBox to change cursor on hover
class WidgetTreeComboBox : public QComboBox
{
        Q_OBJECT

    public:

        uint32_t DefaultValIdx;
        bool isDummy, TriggerDataChange;

        using QComboBox::QComboBox; // Inherit constructors

        WidgetTreeComboBox(QWidget *parent = nullptr, int firstInt = -1, int SecondInt= -1, uint32_t DefaultValIdx = 0);
        void setDummy(bool);
        void setIdx(uint32_t Idx);

    private:

        int m_firstInt;
        int m_secondInt;

    signals:

        void editingFinishedWithInts(int firstInt, int secondInt);

    protected:

        void wheelEvent( QWheelEvent * e) override;
        void paintEvent(QPaintEvent *event) override;

        bool eventFilter(QObject *obj, QEvent *event) override;

    protected:

        void enterEvent(QEnterEvent *event) override;

        void leaveEvent(QEvent *event) override;
};

#endif // WIDGETTREECOMBOBOX_HPP
