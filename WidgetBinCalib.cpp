/**
 *  \file       WidgetBinCalib.hpp
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

#include "WidgetBinCalib.hpp"

#include <QHeaderView>
#include <qapplication.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qstandarditemmodel.h>
#include <QPainter>
#include <QLineEdit>
#include <QStylePainter>
#include <QFileDialog>
#include <QColor>
#include <QTreeWidget>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QEvent>
#include <QRegularExpression>

#include "WidgetTreeTextBox.hpp"
#include "WidgetTreeComboBox.hpp"

//QList<QString> headers = {"Symbol", "Address", "Size", "Type"};

class  ColumnHighlightTreeWidget: public QTreeWidget
{
   // Q_OBJECT

public:
    ColumnHighlightTreeWidget(QWidget* parent = nullptr)
        : QTreeWidget(parent) {}

    // Highlight columns starting from this index
    void setHighlightFromColumn(int col)
    {
        m_startColumn = col;
        viewport()->update(); // redraw
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QTreeWidget::paintEvent(event); // draw tree normally

        if (m_startColumn < 0 || m_startColumn >= columnCount())
            return;

        QPainter painter(viewport());
        painter.setPen(QPen(Qt::blue, 2));       // border color
        painter.setBrush(QColor(0,0,255,30));    // optional translucent fill

        // Draw rectangle for each column from m_startColumn onwards
        for (int col = m_startColumn; col < columnCount(); ++col)
        {
            int x = header()->sectionPosition(col);
            int w = header()->sectionSize(col);
            int y = 0;
            int h = viewport()->height();

            painter.drawRect(x, y, w-1, h-1);
        }
    }

private:
    int m_startColumn = -1;
};

#include <QTreeWidget>
#include <QPainter>
#include <QHeaderView>
#include <QScrollBar>

WidgetTreeComboBox::WidgetTreeComboBox(QWidget *parent, int firstInt, int secondInt, uint32_t DefaultValIdx) : QComboBox(parent), m_firstInt(firstInt), m_secondInt(secondInt)
{
    // Enable hover tracking for the dropdown list
    view()->setMouseTracking(true);

    this->Int1 = firstInt;
    this->Int2 = secondInt;
    this->DefaultValIdx = DefaultValIdx;
    this->isDummy = false;

     setStyleSheet(
         "QComboBox {"
         "  background: none;"
         "  border: none;"
         "  padding: 0px;"
         "  margin: 0px;"
         "}"

         "QComboBox::drop-down {"
         "  border: none;"
         "}"

         "QComboBox::down-arrow {"
         "  image: none;"
         "}"


         "QComboBox::item:selected {"
         "  background-color: rgba(128,128,128,120);"
         "}"

         /* 🔹 POPUP (dropdown list) */
         "QComboBox QAbstractItemView {"
         "  background-color: rgb(255,255,255);"   /* or any solid color */
         "  color: black;"
         "  border: none;"
         "  selection-background-color: rgba(128,128,128,120);"
         "}"
         );

   //  Connect the editingFinished signal to a lambda
    connect(this, &QComboBox::currentIndexChanged, this, [this]() {
        emit editingFinishedWithInts(m_firstInt, m_secondInt);
    });

    // Install event filter on the dropdown list
    view()->viewport()->installEventFilter(this);
}

void WidgetTreeComboBox::setDummy(bool dummy)
{
    this->isDummy = dummy;
}

void WidgetTreeComboBox::wheelEvent( QWheelEvent * e)
{
    e->ignore();
}

void WidgetTreeComboBox::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // Draw the combo box frame first
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // Set the font for the **displayed text only**
    QFont f = this->font();                 // start with the combo's font
    f.setBold(currentIndex() != this->DefaultValIdx);
    painter.setFont(f);

    // Draw the label (current text) in the proper rectangle
    QRect textRect = style()->subControlRect(
        QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this
        );

    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(textRect, Qt::AlignCenter, currentText());
}


bool WidgetTreeComboBox::eventFilter(QObject *obj, QEvent *event)
{
    // Only proceed if this combo box is editable
    if (this->isDummy)
        return QComboBox::eventFilter(obj, event);

    if (obj == view()->viewport()) {
        if (event->type() == QEvent::HoverEnter) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor); // Change cursor to hand pointer
        } else if (event->type() == QEvent::HoverLeave) {
            QApplication::restoreOverrideCursor(); // Restore default cursor
        }
    }
    return QComboBox::eventFilter(obj, event);
}


void WidgetTreeComboBox::enterEvent(QEnterEvent *event)
{
    std::cout << "isdummy " << (int)isDummy << std::endl;

    if (!isDummy) {
        setCursor(Qt::PointingHandCursor); // hand pointer on hover
    }
    QComboBox::enterEvent(event);
}

void WidgetTreeComboBox::leaveEvent(QEvent *event)
{
    if (!isDummy) {
        unsetCursor(); // restore default
    }
    QComboBox::leaveEvent(event);
}

WidgetTreeTextBox::WidgetTreeTextBox(QWidget *parent, bool showTable, uint32_t Idx, int firstInt, int secondInt, float DefaultVal) : QLineEdit(parent), m_firstInt(firstInt), m_secondInt(secondInt)
{
    this->showTable = showTable;
    this->Idx = Idx;
    this->DefaultVal = DefaultVal;

    // Connect the editingFinished signal to a lambda
    connect(this, &WidgetTreeTextBox::editingFinished, this, [this](){
                emit editingFinishedWithInts(m_firstInt, m_secondInt);
    });

    connect(this, &QLineEdit::textChanged, this, &WidgetTreeTextBox::onTextChanged);

    // Center the text in the QLineEdit
    this->setAlignment(Qt::AlignCenter);

    // Appearance
    setAlignment(Qt::AlignCenter);
    setFrame(false);
    //setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    this->setStyleSheet(
        "QLineEdit {"
        "  border: none;"
        "  background-color: transparent;"
        "}"
        "QLineEdit:hover {"
        "  background-color: grey;"
        "}"
        );

    if (showTable)
    {
        this->setReadOnly(true);
        this->setCursor(Qt::PointingHandCursor);
    }
}

void WidgetTreeTextBox::onTextChanged(const QString &text)
{
    if (text.toFloat() != this->DefaultVal)
    {
        this->setStyleSheet("QLineEdit { font-weight: bold; }");
    }
    else
    {
        this->setStyleSheet("QLineEdit { font-weight: normal; }");
    }
}

void WidgetTreeTextBox::mouseReleaseEvent(QMouseEvent *e)
{

#if(0)
    editFlag = true;
    IntVer = this->Int1;
    newTable->setRowCount(0);
    newTable->setColumnCount(0);
    cout << "mouse released " << (int)DataMemInfo[this->Int1][this->Int2].Length.size() << endl;

    selectedTableIdx = this->Idx;
    uint32_t yLen = 1;
    if (DataMemInfo[this->Int1][this->Idx].Length.size() > 2)
    {
        cout << "two dim array" << endl;
        yLen = DataMemInfo[this->Int1][this->Idx].Length.at(1);
    }

    this->yLen = yLen;

    // cout << "ylen " << (int)yLen << " xLen " << DataMemInfo[this->Idx].Length.at(0) << endl;

    if (this->showTable)
    {
        newTable->setVisible(true);
        for (uint32_t j = 0 ; j < yLen ; j++)
        {
            newTable->insertRow(0);
            newTable->setRowHeight(0,20);
        }

        for (uint32_t i = 0 ; i < DataMemInfo[this->Int1][this->Idx].Length.at(0) ; i++)
        {
            newTable->insertColumn(i);
            newTable->setColumnWidth(i, 60);
            //cout << "address " << std::hex << (int)DataMemInfo[this->Idx].Address + i*2 << endl;
            //this->pDialog->ui_Dialog_DWARFViewer->tableWidget->setItem(0, i, new QTableWidgetItem(QString::number(Mem2.ReadMem_uint16(0, 0, DataMemInfo[this->Idx].Address + (i*2)))));

        }

        newTable->loadData(this->Int1, this->Idx, yLen);
        newTable->parseData();
        cout << "table loaded TRUE" << endl;
        newTable->isTableLoaded = true;
        //connect(newTable, SIGNAL(cellChanged(int, int) ),this, SLOT(on_table_itemChanged(int, int)) );
        //  parseTable();
        // }



        // this->pDialog->on_table_itemChanged()


    }
    else
    {
        newTable->setVisible(false);
        // disconnect(newTable, SIGNAL(cellChanged(int, int) ),this, SLOT(on_table_itemChanged(int, int)) );
        cout << "table loaded FALSE" << endl;
        newTable->isTableLoaded = false;

    }

    //connect(this->pDialog->ui_Dialog_DWARFViewer->tableWidget, SIGNAL(cellChanged(int, int) ),this, SLOT(Dialog_DWARFViewer::on_table_itemChanged(int, int)) );
#endif
    QLineEdit::mouseReleaseEvent(e);
}

// Custom header class to add a clickable icon
class ClickableHeader : public QHeaderView
{
    private:
        enum HeaderIcon {
            Icon_Close = 0,
            Icon_Save  = 1,
            Icon_None  = -1
        };

        struct ColumnInfo
        {
            std::string Name;
            uint32_t Size;
            bool isFile;
        };

        int m_hoveredSection = -1;
        int m_hoveredIconIndex = -1;
        QVector<ColumnInfo> m_columns;
        QTreeWidget *parentT;

    public:

        using IconClickedCallback = std::function<void(int section, int icon, string filename)>;
        IconClickedCallback onIconClicked;

        using BinFileCloseCallback = std::function<void(int section, int icon, string filename)>;
        IconClickedCallback onBinFileClose;

        ClickableHeader(Qt::Orientation orientation, QWidget *parent = nullptr) : QHeaderView(orientation, parent)
        {
            parentT = (QTreeWidget *)parent;
            // Enable click events on the header
            setSectionsClickable(true);
            setMouseTracking(true);
            m_hoveredSection = -1;
            m_columns.clear();
        }

        void addColumn(const QString& Name, uint32_t Size, bool isFile = false)
        {
            m_columns.push_back({Name.toStdString(), Size, isFile});
            if (parentT)
            {
                int colIndex = static_cast<int>(m_columns.size() - 1); // index of new column

                parentT->setColumnCount(m_columns.size());          // update column count
                parentT->setColumnWidth(colIndex, Size);
                // Optional: stretch the last column
                if (colIndex == parentT->columnCount() - 1)
                {
                    parentT->header()->setStretchLastSection(false);
                }

            }
            parentT->header()->setStretchLastSection(true);
            update();
        }

        void removeColumn(uint32_t Idx)
        {
            m_columns.remove(Idx);
            parentT->setColumnCount(parentT->columnCount() -1);
            //update();
        }

        // Show/hide a specific column by index
        void setColumnVisible(int index, bool visible)
        {
            if (parentT && index >= 0 && index < parentT->columnCount())
            {
                parentT->setColumnHidden(index, !visible);
            }
        }

    protected:

        void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override
        {
            if (logicalIndex < 0 || logicalIndex >= m_columns.size())
                return;

            const ColumnInfo& col = m_columns[logicalIndex];

            painter->save();




            constexpr int IconSize     = 20;
            constexpr int IconSpacing  = 2;
            constexpr int IconPadding  = 2;   // <-- THIS is the padding you want

            // --- Draw background for hovered icon only ---
            if (col.isFile)
            {
                // --- Draw border around the whole header section ---
               // QPen borderPen(Qt::gray, 1); // color & thickness
               // painter->setPen(borderPen);
                //painter->setBrush(Qt::NoBrush);
                //painter->drawRect(rect.adjusted(0, 0, -1, -1)); // -1 because drawRect includes bottom/right edges

                int xRight = rect.right() - IconSize - IconSpacing;
                int yTop   = rect.center().y() - IconSize / 2;

                QRect iconBgRects[2];
                iconBgRects[0] = QRect(xRight, yTop, IconSize, IconSize);
                xRight -= IconSize + IconSpacing;
                iconBgRects[1] = QRect(xRight, yTop, IconSize, IconSize);

                // Draw hover background for icons
                for (int i = 0; i < 2; ++i)
                {
                    if (m_hoveredSection == logicalIndex && m_hoveredIconIndex == i)
                    {
                        painter->setBrush(QColor("#cce4ff"));
                        painter->setPen(Qt::NoPen);
                        painter->drawRoundedRect(iconBgRects[i], 4.0, 4.0);
                    }
                }

                // Draw text centered in remaining space
                int totalIconWidth = 2 * (IconSize + 2 + IconSpacing);
                QRect textRect = rect.adjusted(0, 0, -totalIconWidth, 0);

                QFontMetrics fm(painter->font());
                QString elidedText = fm.elidedText(QString::fromStdString(col.Name),
                                                   Qt::ElideRight,
                                                   textRect.width());

                painter->setPen(Qt::black);

                // Center text horizontally in the available space
                QRectF centeredTextRect = textRect;
                painter->drawText(centeredTextRect, Qt::AlignVCenter | Qt::AlignHCenter, elidedText);

                // Draw icons
                QIcon iconClose(":/icon/close.svg");
                QIcon iconSave(":/icon/save-file.svg");

                QRect iconInner0 = iconBgRects[0].adjusted(
                    IconPadding, IconPadding, -IconPadding, -IconPadding);

                QRect iconInner1 = iconBgRects[1].adjusted(
                    IconPadding, IconPadding, -IconPadding, -IconPadding);

                iconClose.paint(painter, iconInner0, Qt::AlignCenter);
                iconSave.paint(painter, iconInner1, Qt::AlignCenter);
            }
            else
            {

                // Fill background
                QColor headerBgColor = QColor(250, 250, 250); // light grey
                painter->fillRect(rect, headerBgColor);

                // Non-file columns: just draw the text centered
                QFontMetrics fontMetrics(painter->font());
                QString elidedText = fontMetrics.elidedText(QString::fromStdString(col.Name),
                                                            Qt::ElideRight,
                                                            rect.width());
                painter->drawText(rect, Qt::AlignVCenter | Qt::AlignHCenter, elidedText);
            }

            painter->restore();
        }

        int iconAtPosition(int logicalIndex, const QPoint& pos) const
        {
            if (logicalIndex < 0 || logicalIndex >= m_columns.size())
                return Icon_None;

            const ColumnInfo& col = m_columns[logicalIndex];
            if (!col.isFile)
                return Icon_None;

            constexpr int IconSize    = 20;
            constexpr int IconSpacing = 2;

            int xRight  = sectionViewportPosition(logicalIndex)
                         + sectionSize(logicalIndex)
                         - IconSize
                         - IconSpacing;

            int y = rect().center().y() - IconSize / 2;

            QRect closeRect(xRight, y, IconSize, IconSize);
            xRight -= IconSize + IconSpacing;
            QRect saveRect(xRight, y, IconSize, IconSize);

            if (closeRect.contains(pos)) return Icon_Close;
            if (saveRect.contains(pos))  return Icon_Save;

            return Icon_None;
        }

        void mouseReleaseEvent(QMouseEvent *event) override
        {
            int section = logicalIndexAt(event->pos());
            if (section < 0)
                return;

            int icon = iconAtPosition(section, event->pos());

            switch (icon)
            {
            case Icon_Save:
            {
                qDebug() << "Save icon pressed on section" << section;
                QString fname = QFileDialog::getSaveFileName(
                    this,
                    "Save calibrated binary",
                    ".",
                    "Intel hex (*.hex)"
                    );
                if (!fname.isEmpty())
                {
                    onIconClicked(section, static_cast<int>(icon), fname.toStdString());
                }
                event->accept();
                return;
            }

            case Icon_Close:
                onBinFileClose(section, static_cast<int>(icon), "");
                qDebug() << "Close icon pressed on section" << section;
                // handle close
                event->accept();
                return;

            default:
                break;
            }

            QHeaderView::mouseReleaseEvent(event);
        }

        void mouseMoveEvent(QMouseEvent *event) override
        {
            // Default: no hover
            int hoveredSection = -1;
            int hoveredIconIndex = -1;

            // Get the section under the mouse
            int section = logicalIndexAt(event->pos());

            if (section >= 0 && section < m_columns.size())
            {
                const ColumnInfo& col = m_columns[section];

                if (col.isFile)
                {
                    constexpr int IconSize = 20;
                    constexpr int IconSpacing = 4;

                    int sectionX = sectionViewportPosition(section);
                    int sectionWidth = sectionSize(section);
                    int yCenter = height() / 2;

                    // Rightmost (last) icon
                    QRect iconRects[2];
                    iconRects[0] = QRect(sectionX + sectionWidth - IconSize - IconSpacing,
                                         yCenter - IconSize/2,
                                         IconSize,
                                         IconSize);

                    // Second-last icon
                    iconRects[1] = QRect(sectionX + sectionWidth - 2*(IconSize + IconSpacing),
                                         yCenter - IconSize/2,
                                         IconSize,
                                         IconSize);

                    QPoint mousePos = event->pos();

                    for (int i = 0; i < 2; ++i)
                    {
                        if (iconRects[i].contains(mousePos))
                        {
                            hoveredSection = section;
                            hoveredIconIndex = i;
                            break;
                        }
                    }
                }
            }

            // Only update and repaint if the hovered icon changed
            if (hoveredSection != m_hoveredSection || hoveredIconIndex != m_hoveredIconIndex)
            {
                m_hoveredSection = hoveredSection;
                m_hoveredIconIndex = hoveredIconIndex;
                update(); // repaint the header
            }

            // Call base implementation for other header interactions
            QHeaderView::mouseMoveEvent(event);
        }

        // Make sure to also clear hover when mouse leaves header
        void leaveEvent(QEvent *event) override
        {
            if (m_hoveredSection != -1 || m_hoveredIconIndex != -1)
            {
                m_hoveredSection = -1;
                m_hoveredIconIndex = -1;
                update();
            }
            QHeaderView::leaveEvent(event);
        }
};

ClickableHeader *header;
bool IsViewAdvanced;

QTreeWidget  *m_treeWidget;   // Left: Hierarchy/Symbols
QTreeWidget *m_symbolTree;


QTreeWidgetItem* copyItemWithoutColumn(QTreeWidgetItem* item, int colToRemove)
{
    // Create new item with one less column
    int oldCount = item->columnCount();
    QTreeWidgetItem* newItem = new QTreeWidgetItem(oldCount - 1);

    // Copy data, skipping the column to remove
    int j = 0;
    for (int i = 0; i < oldCount; ++i)
    {
        if (i == colToRemove)
            continue;

        newItem->setText(j, item->text(i));
        // Optional: copy icons, colors, tooltips, etc.
        newItem->setIcon(j, item->icon(i));
        newItem->setBackground(j, item->background(i));
        newItem->setForeground(j, item->foreground(i));
        j++;
    }

    // Recursively copy children
    for (int c = 0; c < item->childCount(); ++c)
    {
        QTreeWidgetItem* childCopy = copyItemWithoutColumn(item->child(c), colToRemove);
        newItem->addChild(childCopy);
    }

    return newItem;
}

BinCalibToolWidget::BinCalibToolWidget(QWidget *parent, FileBin_ELF *elf) : QWidget(parent)
{
    this->ELFData = elf;
    this->BaseFileData.clear();
    IsViewAdvanced = false;
    //this->BaseFileCnt = 0;
    // 1. Main Vertical Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); // no margins
    mainLayout->setSpacing(0);                   // no spacing

    // 2. Toolbar
    m_toolBar = new QToolBar("Calibration Tools", this);
    m_toolBar->setIconSize(QSize(20, 20));
    m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // Set spacing between buttons in the toolbar layout
    if (auto layout = m_toolBar->layout()) {
        layout->setSpacing(8);   // space between buttons
        layout->setContentsMargins(4, 2, 4, 2); // top, left, bottom, right
    }

    // 5. Apply padding via stylesheet (applies to all buttons in the toolbar)
    m_toolBar->setStyleSheet(R"(
    QToolButton {
        padding: 2px;              /* adds space around the icon */
        margin: 2px;               /* optional: space between buttons */
        border: none;              /* flat look */
        background-color: transparent;
border-radius: 4px;        /* Slightly rounded corners */
    }
    QToolButton:checked {
        background-color: #80aee0; /* Fynix blue for checked buttons */
        color: white;
    }
    QToolButton:hover {
        background-color: #e6f0fa; /* subtle hover */
    }

 QToolButton:checked:hover {
        background-color: #80aee0;
    }
)");

    // Optional: remove toolbar border / background
   // m_toolBar->setStyleSheet("QToolBar { border: none; background: transparent; }");

    // Add dummy actions
    QAction* actionViewAdvanced = m_toolBar->addAction(QIcon(":/icon/view_advanced.svg"), "View advanced");
    QAction* actionLoadFile = m_toolBar->addAction(QIcon(":/icon/folder-open.svg"), "Open file...");
   // QAction *reloadAct = m_toolBar->addAction("Info");
    actionViewAdvanced->setCheckable(true);
    actionViewAdvanced->setChecked(true);
   // m_toolBar->addSeparator();

    mainLayout->addWidget(m_toolBar);

    // 3. Splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(2); // make splitter handle thin
    m_splitter->setStyleSheet(R"(
    QSplitter::handle {
        background-color: #f3f3f3; /* thin line color */
    }
)");

    // 4. Tree Widget
    m_treeWidget = new QTreeWidget(this);



    m_treeWidget->setHeaderLabels({"Calibratable files"});
    m_treeWidget->setFrameShape(QFrame::NoFrame); // remove frame
    m_treeWidget->setIndentation(12); // optional, remove tree indentation
    m_treeWidget->setFocusPolicy(Qt::NoFocus);
    m_treeWidget->setStyleSheet(
        "QTreeWidget { border: none; background: transparent; }"
        );

    // 5. Table Widget
    m_symbolTree = new QTreeWidget(this);


    //ColumnHighlightTreeWidget* tree = new ColumnHighlightTreeWidget(this);
    //tree->setColumnCount(6);

    //auto overlay = new ColumnOverlay(m_rightTree, 4); // column index 3+
    //overlay->show();

    // Make the 3rd column (index 2) have a border
   // m_rightTree->setItemDelegate(new ColumnBorderDelegate(4));
    header = new ClickableHeader(Qt::Horizontal, m_symbolTree);

    header->onIconClicked = [this](int section, int icon, string filename){
        qDebug() << "Icon clicked in section" << section << "icon" << icon;
        this->BaseFileData.at(section-4)->mem->Save(filename);
    };

    header->onBinFileClose = [this](int section, int icon, string filename){
        qDebug() << "Icon clicked in section" << section << "icon" << icon;
        //this->BaseFileData.at(section-4)->mem->Save(filename);

        int columnToRemove = section; // column index to remove
        int oldCount = m_symbolTree->columnCount();
        int newCount = oldCount - 1;




        // Backup header labels
        //QStringList newHeaders;
        //for (int i = 0; i < oldCount; ++i)
      //  {
            //if (i == columnToRemove)
           //     continue;
       //     newHeaders << m_symbolTree->headerItem()->text(i);
       // }

        // Copy top-level items
        QList<QTreeWidgetItem*> topLevelCopies;
        for (int i = 0; i < m_symbolTree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* orig = m_symbolTree->topLevelItem(i);
            QTreeWidgetItem* copy = copyItemWithoutColumn(orig, columnToRemove);
            topLevelCopies.append(copy);
        }

        header->removeColumn(columnToRemove);

        // Clear tree and set new column count
        //m_symbolTree->clear();
        //m_symbolTree->setColumnCount(newCount);
        //m_symbolTree->setHeaderLabels(newHeaders);
        //header = newHeaders;

        // Reinsert items
        for (auto* item : topLevelCopies)
        {
            m_symbolTree->addTopLevelItem(item);
        }


       // int columnToRemove = section; // assuming section is the column index

        //if (columnToRemove >= 0 && columnToRemove < m_symbolTree->columnCount())
        //{
            // Hide or remove the column
           // m_symbolTree->setColumnHidden(columnToRemove, true);

            // Optional: update internal data if you track BaseFileData
            if (columnToRemove >= 4) // if your data columns start at 4
            {
                int dataIdx = columnToRemove - 4;
                if (dataIdx < BaseFileData.size())
                {
                    BaseFileData.erase(BaseFileData.begin() + dataIdx);
                }
            }
       // }
    };


    m_symbolTree->setHeader(header);
    header->addColumn("Symbol", 260);
    header->addColumn("Address", 100);
    header->addColumn("Size", 80);
    header->addColumn("Type", 60);



    //header->addColumn("Symbol", 260);
   // m_rightTree->setColumnCount(3);
   // m_rightTree->setHeaderLabels(headers);

   // m_rightTree->header()->stretchLastSection();
   // m_rightTree->setHeaderLabels(headers);
    //m_rightTree->setColumnWidth(0,260);
   // m_rightTree->setColumnWidth(1,80);
    //m_rightTree->setColumnWidth(2,40);
    //m_rightTree->setColumnWidth(3,60);
    //m_rightTree->setColumnWidth(4,250);
    //m_rightTree->setRootIsDecorated(false); // if you don’t want expandable tree icons

    m_symbolTree->setStyleSheet(
        "QTreeWidget {"
        "    border: none;"
        "    background-color: white;"        /* base color for even rows */
        "}"
        "QTreeWidget::item {"
        "    background-color: #E8E8E8;"      /* even rows */
        "    height: 22px;"
        "}"
        "QTreeWidget::item:alternate {"
        "    background-color: #F5F5F5;"      /* odd rows */
        "    height: 22px;"
        "}"
        "QTreeWidget::item:hover {"
        "   background-color: #CCE4FF;"     /* remove default light blue hover */
        "}"
        );

    m_symbolTree->setAlternatingRowColors(true);
    //m_symbolTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    //m_symbolTree->setAllColumnsShowFocus(true);
    m_symbolTree->setIndentation(12); // optional, remove tree indentation
    m_symbolTree->header()->setStretchLastSection(true);

    // Prevent horizontal resizing when vertical scrollbar appears
    m_symbolTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_symbolTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    //m_rightTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //m_rightTree->set


    // for (int row = 0; row < m_rightTree->topLevelItemCount(); ++row)
    // {
    //     QTreeWidgetItem* item = m_rightTree->topLevelItem(row);
    //     for (int col = 0; col < m_rightTree->columnCount(); ++col)
    //     {
    //         item->setBackground(col, QBrush(QColor(0,0,255,30)));
    //     }
    // }

    // Add widgets to splitter
    m_splitter->addWidget(m_treeWidget);
    m_splitter->addWidget(m_symbolTree);

    m_splitter->setSizes({180, 600});
    // Set stretch factors
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);

    // 6. Add splitter to layout
    mainLayout->addWidget(m_splitter);



    // 3. Connect the triggered signal to a slot or lambda
    connect(actionViewAdvanced, &QAction::triggered, this, [this, actionViewAdvanced](bool checked)
            {

                IsViewAdvanced = checked;
                m_symbolTree->setColumnHidden(1, !IsViewAdvanced);
                 m_symbolTree->setColumnHidden(2, !IsViewAdvanced);
                  m_symbolTree->setColumnHidden(3, !IsViewAdvanced);
            });

    connect(actionLoadFile, &QAction::triggered, this, [this, actionLoadFile](bool checked)
            {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open File"),
            QString(),                       // initial directory
            tr("ELF Files (*.elf);;Intel HEX Files (*.hex);;All Files (*)")            // filter
            );

        if (fileName.isEmpty())
            return;

        QFileInfo fileInfo(fileName);
        // Get the file extension (e.g., "txt", "jpg")
        QString extension = fileInfo.suffix().toLower();

        // Check the file extension
        if (extension == "elf")
        {
            qDebug() << "Dropped master file:" << fileName;
            //loadElf(fileName.toStdString());


            //IsMasterFileLoaded = true;
        }
        else if (extension == "hex")
        {
            FileBin_IntelHex_Memory *newBaseFile = new FileBin_IntelHex_Memory();
            newBaseFile->Load(fileName.toStdString().c_str(), LIB_FIRMWAREBIN_HEX);
            this->Calib_BaseFile_AddNew(fileName.toStdString(), newBaseFile);
            //qDebug() << "Dropped image file:" << fileName;
            ///if (!Mem2[this->BaseFileCnt].Load(filePath.toStdString().c_str(), LIB_FIRMWAREBIN_HEX))
            //{
            //    cout << "Error loading" << endl;
            //    return;
            // }
            //else
            //{
            //   cout << "Loading finished" << endl;
           // AddNewBaseFile(fileName.toStdString());

            //  Parse_Master_Symbol();
            // parseHexFile();
            // isFileLoaded = true;
            //}
        }
        else
        {
           // qDebug() << "Dropped file with unsupported extension:" << filePath;
        }


            });

}


//uint32_t SymbolValIdx;

 void Calib_MasterWidget(void);

 void BinCalibToolWidget::Calib_BaseFile_DataParse(FileBin_VarInfoType* node, uint32_t BaseFileIdx, FileBin_IntelHex_Memory *newFileBin)
 {
    uint32_t childIdx = 0;
    //SymbolValIdx = 0;
    QTreeWidget *tree = m_symbolTree;

    for (int i = 0; i < this->BaseFileData.at(BaseFileIdx)->data.size(); i++)
    {
        if (!this->BaseFileData.at(BaseFileIdx)->data.at(i)->node)
        {
            break;
        }

        switch (this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->DataType)
        {
            case FileBin_VARINFO_TYPE_UINT8:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_uint8(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_SINT8:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_sint8(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_UINT16:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_uint16(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_SINT16:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_sint16(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_UINT32:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_uint32(this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_SINT32:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_sint32(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_FLOAT32:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                lineedit->setText(QString::number(newFileBin->ReadMem_float32(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_ENUM:
            {
                WidgetTreeComboBox *dataWidget = (WidgetTreeComboBox *)this->BaseFileData.at(BaseFileIdx)->data.at(i)->WidgetData;
                dataWidget->setCurrentIndex(newFileBin->ReadMem_uint8(nullptr, 0, this->BaseFileData.at(BaseFileIdx)->data.at(i)->node->Addr));
                break;
            }
            default:
            {
                break;
            }
        }
    }
 }

 void BinCalibToolWidget::BinMemWrite(uint32_t BinIdx, uint32_t SymbolIdx)
 {
    if (BinIdx > (this->BaseFileData.size() - 1))
    {
        cout << "Invalid binary file index: " << BinIdx << endl;
        return;
    }

    if (SymbolIdx > (this->BaseFileData.at(BinIdx)->data.size() - 1))
    {
        cout << "Invalid symbol index: " << SymbolIdx << endl;
        return;
    }

    if (!this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node)
    {
        return;
    }

    //cout << "Writing memory BinFile Idx: " << BinIdx << " Symbol Idx: " << SymbolIdx << " Addr: 0x" << std::hex << this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr <<  endl;

    switch(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->DataType)
    {
        case FileBin_VARINFO_TYPE_BOOLEAN:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_boolean(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_UINT8:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint8(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_SINT8:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_sint8(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_UINT16:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint16(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_SINT16:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_sint16(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_UINT32:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint32(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_SINT32:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_sint32(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_FLOAT32:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_float32(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_ENUM:
        {
            WidgetTreeComboBox* comboBox = (WidgetTreeComboBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint8(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->Addr, comboBox->currentIndex());
            break;
        }

        default:
        {
            cout << "Undefiend datatype" << endl;
        }
    }
}

void BinCalibToolWidget::Calib_BaseFile_WidgetPopulate(FileBin_VarInfoType* node, QTreeWidgetItem* item, uint32_t BaseFileIdx)
{

    uint32_t childIdx = 0;
    QTreeWidget *tree = m_symbolTree;

    while (node)
    {
        // Skip qualifier nodes for the row but still recurse into children
        if (!node->isQualifier)
        {
            /* Multi-dimensional symbol */
            if (node->Size.size() > 1)
            {
                WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, true, childIdx, BaseFileIdx, childIdx, 23);

                // Build array string
                QStringList dims;
                for (uint32_t i = 0; i < node->Size.size() - 1; ++i)
                {
                    dims << QString::number(node->Size[i]);
                }

                widgetData->setText("<" + dims.join(" x ") + ">");

                tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);
               // SymbolValIdx++;
               // childIdx++;
            }
            else
            {
                if (FileBin_VARINFO_TYPE_BOOLEAN == node->DataType)
                {
                   std::vector<uint8_t> raw = this->ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                   bool value = 0;
                   if (raw.size() >= 1)
                   {
                       value = static_cast<uint8_t>(raw[0]);
                   }

                   WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                   QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts,
                                    [this, &widgetData](int firstInt, int secondInt)
                                    {
                                        this->BinMemWrite(firstInt, secondInt);
                                    });

                   tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, widgetData);

                   SymbolDataType *newDataInfo = new SymbolDataType();
                   newDataInfo->node = node;
                   newDataInfo->WidgetData = widgetData;
                   BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);

                   //childIdx++;
                  /// SymbolValIdx++;
                }
                else if (FileBin_VARINFO_TYPE_UINT8 == node->DataType)
                {
                    std::vector<uint8_t> raw = this->ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    uint8_t value = 0;
                    if (raw.size() >= 1)
                    {
                        value = static_cast<uint8_t>(raw[0]);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts,
                                    [this, &widgetData](int firstInt, int secondInt)
                                    {
                                        this->BinMemWrite(firstInt, secondInt);
                                    });

                    tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, widgetData);

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);

                    //childIdx++;
                    //SymbolValIdx++;
                }
                else if (FileBin_VARINFO_TYPE_SINT8 == node->DataType)
                {
                     std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    int8_t value = 0;
                    if (raw.size() >= 1)
                    {
                        value = static_cast<int8_t>(raw[0]);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, 0, value);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, &widgetData](int firstInt, int secondInt) {
                        BinMemWrite(firstInt, secondInt);
                    });

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);

                    //SymbolValIdx++;
                   // childIdx++;
                }
                else if (FileBin_VARINFO_TYPE_UINT16 == node->DataType)
                {
                    std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    uint16_t value = 0;
                    if (raw.size() >= 2)
                    {
                        value = static_cast<uint16_t>(raw[0])
                        | (static_cast<uint16_t>(raw[1]) << 8);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, 0, value);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts,
                                      [this, &widgetData](int firstInt, int secondInt)
                                      {
                                          BinMemWrite(firstInt, secondInt);
                                      });

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                     SymbolDataType *newDataInfo = new SymbolDataType();
                     newDataInfo->node = node;
                     newDataInfo->WidgetData = widgetData;
                     BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);

                   // SymbolValIdx++;
                    //childIdx++;
                }
                else if (FileBin_VARINFO_TYPE_SINT16 == node->DataType)
                {
                    std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    int16_t value = 0;
                    if (raw.size() >= 2)
                    {
                        value = static_cast<int16_t>(raw[0])
                        | (static_cast<int16_t>(raw[1]) << 8);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, 0, value);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts,
                                     [this, &widgetData](int firstInt, int secondInt)
                                     {
                                         BinMemWrite(firstInt, secondInt);
                                     });

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                    //SymbolValIdx++;
                   // childIdx++;
                }
                else if (FileBin_VARINFO_TYPE_UINT32 == node->DataType)
                {
                     std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                     uint32_t value = 0;
                     if (raw.size() >= 2)
                     {
                        value = static_cast<uint32_t>(raw[0])
                            | (static_cast<uint32_t>(raw[1]) << 8)
                            | (static_cast<uint32_t>(raw[2]) << 16)
                            | (static_cast<uint32_t>(raw[3]) << 24);
                     }

                     WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, 0, value);

                     QObject::connect(widgetData,
                                      &WidgetTreeTextBox::editingFinishedWithInts,
                                      [this, &widgetData](int firstInt, int secondInt)
                                     {
                                          BinMemWrite(firstInt, secondInt);
                                      });

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                }
                else if (FileBin_VARINFO_TYPE_SINT32 == node->DataType)
                {


                    std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    int32_t value = 0;
                    if (raw.size() >= 2)
                    {
                        value = static_cast<int32_t>(raw[0])
                        | (static_cast<int32_t>(raw[1]) << 8)
                            | (static_cast<int32_t>(raw[2]) << 16)
                            | (static_cast<int32_t>(raw[3]) << 24);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, 0, value);

                    QObject::connect(widgetData,
                                     &WidgetTreeTextBox::editingFinishedWithInts,
                                     [this, &widgetData](int firstInt, int secondInt)
                                     {
                                         BinMemWrite(firstInt, secondInt);
                                     });

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                }
                else if (FileBin_VARINFO_TYPE_FLOAT32 == node->DataType)
                {
                    std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    float value = 0.0f;

                    if (raw.size() >= 4)
                    {
                        uint32_t tmp =
                            static_cast<uint32_t>(raw[0]) |
                            (static_cast<uint32_t>(raw[1]) << 8) |
                            (static_cast<uint32_t>(raw[2]) << 16) |
                            (static_cast<uint32_t>(raw[3]) << 24);

                        std::memcpy(&value, &tmp, sizeof(float));
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, 0, value);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, &widgetData](int firstInt, int secondInt) {
                        BinMemWrite(firstInt, secondInt);
                    });

                    tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, widgetData);

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);

                   // SymbolValIdx++;
                   // childIdx++;
                }
                else if (FileBin_VARINFO_TYPE_ENUM == node->DataType)
                {
                    uint8_t val;

                    if (node->Size.size() > 0)
                    {
                     std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));
                        val = raw.at(0);
                    }

                    WidgetTreeComboBox *widgetData = new WidgetTreeComboBox(this, BaseFileIdx, 0, val);

                    FileBin_DWARF_VarInfoType *enumNode = node->child->child->child;

                    while (enumNode)
                    {
                        widgetData->addItem(QString::fromStdString(std::string(enumNode->data.begin(), enumNode->data.end())));
                        enumNode = enumNode->next;
                    }

                    QObject::connect(widgetData, &WidgetTreeComboBox::editingFinishedWithInts, [this](int firstInt, int secondInt) {
                        BinMemWrite(firstInt, secondInt);
                    });

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    //QTreeWidgetItem* childItem = item->child(childIdx);


                    //childItem->setBackground(4 + BaseFileIdx, QBrush(QColor(255, 0, 0)));

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);

                   // SymbolValIdx++;
                    //childIdx++;
                }
                else
                {
                    WidgetTreeComboBox *dummyItem = new WidgetTreeComboBox(this, BaseFileIdx, 0, 0);
                    dummyItem->setDummy(true);
                    tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, dummyItem);
                    qDebug() << "Unknown data type";
                }
            }

            if (node->child)
            {
                this->Calib_BaseFile_WidgetPopulate(node->child, item->child(childIdx), BaseFileIdx);
            }
        }
        else if (node->child)
        {
            this->Calib_BaseFile_WidgetPopulate(node->child, item, BaseFileIdx);
        }

        node = node->next;
        childIdx++;
    }
}

static QString formatSize(const std::vector<uint32_t>& size)
{
    if (size.empty())
        return {};

    if (size.size() == 1)
        return QString::number(size.front());

    QStringList dims;
    dims.reserve(size.size());
    for (uint32_t dim : size)
        dims << QString::number(dim);

    return QString("<%1>").arg(dims.join(" x "));
}

static QString extractDisplayName(const FileBin_DWARF_VarInfoType* node)
{
    if (node->data.empty())
        return QStringLiteral("unnamed");

    QString name = QString::fromStdString(
        std::string(node->data.begin(), node->data.end())
        );

    if (node->elementType == FILEBIN_DWARF_ELEMENT_COMPILE_UNIT) {
        name = QFileInfo(name).completeBaseName();
    }

    return name;
}

static QString formatType(uint32_t typeId)
{
    switch (static_cast<FileBin_DWARF_VarInfoLenType>(typeId))
    {
        case FileBin_VARINFO_TYPE_BOOLEAN:  return QStringLiteral("bool");
        case FileBin_VARINFO_TYPE_UINT8:    return QStringLiteral("uint8");
        case FileBin_VARINFO_TYPE_SINT8:    return QStringLiteral("sint8");
        case FileBin_VARINFO_TYPE_UINT16:   return QStringLiteral("uint16");
        case FileBin_VARINFO_TYPE_SINT16:   return QStringLiteral("sint16");
        case FileBin_VARINFO_TYPE_UINT32:   return QStringLiteral("uint32");
        case FileBin_VARINFO_TYPE_SINT32:   return QStringLiteral("sint32");
        case FileBin_VARINFO_TYPE_FLOAT32:  return QStringLiteral("float32");
        case FileBin_VARINFO_TYPE_FLOAT64:  return QStringLiteral("float64");
        case FileBin_VARINFO_TYPE_ENUM:     return QStringLiteral("[enum]");
        case FileBin_VARINFO_TYPE_STRUCT:   return QStringLiteral("[struct]");
        case FileBin_VARINFO_TYPE__UNKNOWN:
        default:
            return QStringLiteral("");
    }
}

void populateTreeWidgetRecursive(FileBin_DWARF_VarInfoType* node, QTreeWidget* treeWidget, QTreeWidgetItem* parentItem = nullptr)
{
    while (node)
    {
        // Skip qualifier nodes for the row but still recurse into children
        if (!node->isQualifier)
        {
            // --- Prepare column texts ---
            QString name = extractDisplayName(node); // your function
            QString addr = QStringLiteral("0x") + QString::number(node->Addr, 16).toUpper();
            QString size = formatSize(node->Size);   // your function
            QString type = formatType(node->DataType);   // your function

            // --- Create the tree item ---
            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setText(0, name);
            item->setText(1, addr);
            item->setTextAlignment(1, Qt::AlignCenter);
            item->setText(2, size);
            item->setTextAlignment(2, Qt::AlignCenter);
            item->setText(3, type);
            item->setTextAlignment(3, Qt::AlignCenter);

            if (node->data.empty())
            {
                QFont f = item->font(0);
                f.setItalic(true);
                item->setFont(0, f);
            }

            // --- Add item to the tree ---
            if (parentItem)
                parentItem->addChild(item);
            else
                treeWidget->addTopLevelItem(item);

            // --- Insert extra column widget (QLineEdit) ---
            //QLineEdit* editBox = new QLineEdit(treeWidget);
            //editBox->setText("23");               // default value
            //treeWidget->setItemWidget(item, 3, editBox); // column 3

            // --- Recurse into children ---
            if (node->child)
                populateTreeWidgetRecursive(node->child, treeWidget, item);
        }
        else if (node->child)
        {
            // Qualifier node: skip row but recurse into children
            populateTreeWidgetRecursive(node->child, treeWidget, parentItem);
        }

        node = node->next; // move to sibling
    }
}

void BinCalibToolWidget::populateTreeWidgetRecursive(FileBin_DWARF_VarInfoType* node, QTreeWidgetItem* parentItem)
{
    while (node)
    {
        // Skip qualifier nodes for the row but still recurse into children
        if (!node->isQualifier)
        {
            // --- Prepare column texts ---
            QString name = extractDisplayName(node); // your function
            QString addr = QStringLiteral("0x") + QString::number(node->Addr, 16).toUpper();
            QString size = formatSize(node->Size);   // your function
            QString type = formatType(node->DataType);   // your function

            // --- Create the tree item ---
            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setText(0, name);
            item->setText(1, addr);
            item->setTextAlignment(1, Qt::AlignCenter);
            item->setText(2, size);
            item->setTextAlignment(2, Qt::AlignCenter);
            item->setText(3, type);
            item->setTextAlignment(3, Qt::AlignCenter);

            if (node->data.empty())
            {
                QFont f = item->font(0);
                f.setItalic(true);
                item->setFont(0, f);
            }

            // --- Add item to the tree ---
            if (parentItem)
                parentItem->addChild(item);
            //else
               // treeWidget->addTopLevelItem(item);


            //QTreeWidgetItem* dummy = new QTreeWidgetItem();
            //dummy->setHidden(true);
            //item->addChild(dummy);
            // --- Insert extra column widget (QLineEdit) ---
            //QLineEdit* editBox = new QLineEdit(treeWidget);
            //editBox->setText("23");               // default value
            //treeWidget->setItemWidget(item, 3, editBox); // column 3

            // --- Recurse into children ---
            if (node->child)
                populateTreeWidgetRecursive(node->child, item);
        }
        else if (node->child)
        {
            // Qualifier node: skip row but recurse into children
            populateTreeWidgetRecursive(node->child, parentItem);
        }

        node = node->next; // move to sibling
    }
}

void BinCalibToolWidget::onTreeItemClicked(QTreeWidgetItem* item, int column)
{
    /* Index of the selected Unit (tree row) */
    uint32_t cuIdx = m_treeWidget->indexOfTopLevelItem(item);

    FileBin_DWARF_VarInfoType* node = this->SymbolData;

    /* Fetch symbol node for the selected Compilation Unit */
    while (node && cuIdx--)
    {
        node = node->next;
    }

    m_symbolTree->clear();

    // Recursively populate children under invisible root
    populateTreeWidgetRecursive(node->child, m_symbolTree->invisibleRootItem());

    /* Set selected Compilation Unit */
    this->selectedSymbolData = node->child;

    for (uint32_t i = 0 ; i < this->BaseFileData.size(); i++)
    {
        this->BaseFileData.at(i)->data.clear();
        this->Calib_BaseFile_WidgetPopulate(node->child, m_symbolTree->invisibleRootItem(), i);
        this->Calib_BaseFile_DataParse(nullptr, i, this->BaseFileData.at(i)->mem);
    }
}

void BinCalibToolWidget::Calib_BaseFile_AddNew(std::string filename, FileBin_IntelHex_Memory *newFileBin)
{
    SymbolDataInfo *basefile = new SymbolDataInfo();
    basefile->filename = filename;
    basefile->mem = newFileBin;
    this->BaseFileData.push_back(basefile);

    header->addColumn(QFileInfo(QString::fromStdString(filename)).fileName(), 140, true);

    if (selectedSymbolData)
    {
        FileBin_DWARF_VarInfoType *cuSymbolList = this->selectedSymbolData;

        while (cuSymbolList)
        {
            this->Calib_BaseFile_WidgetPopulate(cuSymbolList, m_symbolTree->invisibleRootItem(),this->BaseFileData.size()-1);
            cuSymbolList = cuSymbolList->next;
        }
        this->Calib_BaseFile_DataParse(nullptr, this->BaseFileData.size() - 1, newFileBin);
    }
}

static std::string TagToString(uint32_t tag)
{
    switch (tag)
    {
    case FILEBIN_DWARF_ELEMENT_COMPILE_UNIT:    return "COMPILE UNIT";
    case FILEBIN_DWARF_ELEMENT_VOLATILE:        return "VOLATILE";
    case FILEBIN_DWARF_ELEMENT_ENUMERATION:     return "ENUMERATION";
    case FILEBIN_DWARF_ELEMENT_ARRAY:           return "ARRAY";
    case FILEBIN_DWARF_ELEMENT_TYPEDEF:         return "TYPEDEF";
    case FILEBIN_DWARF_ELEMENT_BASE_TYPE:       return "BASE TYPE";
    case FILEBIN_DWARF_ELEMENT_STRUCTURE:       return "STRUCTURE";
    case FILEBIN_DWARF_ELEMENT_MEMBER:          return "MEMBER";
    case FILEBIN_DWARF_ELEMENT_VARIABLE:        return "VARIABLE";
    case FILEBIN_DWARF_ELEMENT_CONSTANT:        return "CONSTANT";
    default:                                    return "";
    }
}

void BinCalibToolWidget::Calib_MasterStruct(FileBin_VarInfoType* node)
{
    m_treeWidget->clear();  // clear existing items
    this->selectedSymbolData = nullptr;

    static QFont italicFont;
    italicFont.setItalic(true);

    this->SymbolData = node;

    while (node)
    {
        QString displayName;
        if (node->data.empty()) {
            displayName = "unnamed";
        } else {
            QString fullPath = QString::fromUtf8(reinterpret_cast<const char*>(node->data.data()), node->data.size());
            QFileInfo fi(fullPath);
            displayName = fi.fileName(); // only the file name, without path
        }

        //QString tagStr = QString::fromStdString(TagToString(node->elementType));

        // Create a QTreeWidgetItem for this node
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, displayName);  // column 0
        //item->setText(1, tagStr);       // column 1

        // Make read-only
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        if (node->data.empty())
            item->setFont(0, italicFont);

        // If node has children, add a dummy child to show the expand arrow
        if (node->child)
        {
            //QTreeWidgetItem* placeholder = new QTreeWidgetItem();
            //placeholder->setText(0, "Loading...");
            //placeholder->setFlags(Qt::ItemIsEnabled);  // must be enabled to show expand arrow
            //placeholder->setData(0, Qt::UserRole + 1, true); // mark as placeholder

            //item->addChild(placeholder);

            // Store pointer to actual child
            //item->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<void*>(node->child)));
        }

        // Add top-level item to tree widget
        m_treeWidget->addTopLevelItem(item);

        node = node->next;


    }

    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &BinCalibToolWidget::onTreeItemClicked);
}

#if(0)
void BinCalibToolWidget::Parse_Master_CompilationUnit()
{
    this->UI_CompileUnitTree->clear();

    FileBin_DWARF_VarInfoType* node = DWARFData->SymbolRoot;
    uint8_t nodeCnt = 0;

    while (node != nullptr)
    {
        QString path = QString::fromStdString(std::string(node->data.begin(), node->data.end()));
        path = QDir::fromNativeSeparators(path);

        QString fileName = path.mid(path.lastIndexOf('/') + 1);

        // Skip non-matching entries if only compile units are not requested
        if (!this->IsExpert && !fileName.endsWith("PBCfg.c")) {
            node = node->next;
            nodeCnt++;
            continue;
        }

        this->UI_SymbolTree->setColumnHidden(1, !this->IsExpert);
        this->UI_SymbolTree->setColumnHidden(2, !this->IsExpert);
        this->UI_SymbolTree->setColumnHidden(3, !this->IsExpert);
        this->UI_SymbolTree->setColumnHidden(4, !this->IsControlUnitReadActive);

        QTreeWidgetItem* treeItem = new QTreeWidgetItem(this->UI_CompileUnitTree);
        treeItem->setText(0, QString::number(nodeCnt));
        treeItem->setText(1, fileName);

        node = node->next;
        nodeCnt++;
    }
}
#endif


void BinCalibToolWidget::handleApplyPatch() {
    // Internal library logic to modify the HEX file
    // ...
    //emit calibrationFinished("output_calibrated.hex");
}
