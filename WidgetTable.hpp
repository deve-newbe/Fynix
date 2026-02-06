#ifndef WIDGETTABLE_HPP
#define WIDGETTABLE_HPP

#include <qtablewidget.h>

class WidgetTable : public QTableWidget
{
    Q_OBJECT
public:
    using QTableWidget::QTableWidget; // inherit constructors
     QVector<QVector<QString>> copiedData;

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // WIDGETTABLE_HPP
