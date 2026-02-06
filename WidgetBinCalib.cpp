/**
 *  \file       WidgetBinCalib.hpp
 *  \brief      Fynix binary calibration widget
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
#include <QScrollBar>
#include <QEvent>
#include <QRegularExpression>
#include <QClipboard>

#include "WidgetTreeTextBox.hpp"
#include "WidgetTreeComboBox.hpp"

WidgetTreeComboBox::WidgetTreeComboBox(QWidget *parent, int firstInt, int secondInt, uint32_t DefaultValIdx) : QComboBox(parent)
{
    // Enable hover tracking for the dropdown list
    view()->setMouseTracking(true);

    this->m_firstInt = firstInt;
    this->m_secondInt = secondInt;
    this->DefaultValIdx = DefaultValIdx;
    this->isDummy = false;
    this->TriggerDataChange = false;

    setStyleSheet(
        "QComboBox {"
        "  background: none;"
        "  border: none;"
        "  border-left: 1px solid rgb(180,180,180);  /* matches QPen(Qt::gray, 1) */"
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

        /* POPUP (dropdown list) */
        "QComboBox QAbstractItemView {"
        "  background-color: rgb(255,255,255);"   /* or any solid color */
        "  color: black;"
        "  border: none;"
        "  selection-background-color: rgba(128,128,128,120);"
        "}"
        );

    // Connect the editingFinished signal to a lambda
    connect(this, &QComboBox::currentIndexChanged, this, [this]() {
        if (this->TriggerDataChange)
        {
            emit editingFinishedWithInts(m_firstInt, m_secondInt);
        }
    });

    // Install event filter on the dropdown list
    view()->viewport()->installEventFilter(this);

    this->TriggerDataChange = true;
}

void WidgetTreeComboBox::setDummy(bool dummy)
{
    this->isDummy = dummy;
}

void WidgetTreeComboBox::setIdx(uint32_t Idx)
{
    this->TriggerDataChange = false;
    this->setCurrentIndex(Idx);
    this->TriggerDataChange = true;
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
    QRect textRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);

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
    if (!isDummy)
    {
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

void WidgetTreeTextBox::RefreshState(void)
{
    this->setProperty("valueChanged", (this->text().toFloat() != this->DefaultVal));

    this->setStyleSheet(R"(
        QLineEdit {
            background: none;
            border: none;
            border-left: 1px solid rgb(180,180,180);  /* matches QPen(Qt::gray, 1) */
            padding: 0px;
            margin: 0px;
        }

        QLineEdit[valueChanged="true"] {
            font-weight: bold;
        }

        QLineEdit[valueChanged="false"] {
            font-weight: normal;
        }
        )");
}

WidgetTreeTextBox::WidgetTreeTextBox(QWidget *parent, bool showTable, uint32_t Idx, int firstInt, int secondInt, float DefaultVal) : QLineEdit(parent), m_firstInt(firstInt), m_secondInt(secondInt)
{
    this->showTable = showTable;
    this->Idx = Idx;
    this->DefaultVal = DefaultVal;
    this->Init = false;


    //setFocusPolicy(Qt::ClickFocus); // normal click/focus behavior
    setFocusPolicy(Qt::NoFocus);

    setMouseTracking(false);          // mouse move alone won't trigger editing
    // Center the text in the QLineEdit
    this->setAlignment(Qt::AlignCenter);

    // Appearance
    setFrame(false);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression(R"(^\d*\.?\d*$)"), this);

    setValidator(validator);

    if (showTable)
    {
        this->setReadOnly(true);
        this->setCursor(Qt::PointingHandCursor);
        RefreshState();
    }
    else
    {
        // Connect the editingFinished signal to a lambda
        connect(this, &WidgetTreeTextBox::textEdited, this, [this](){
            RefreshState();
            if (this->Init)
            {
                emit editingFinishedWithInts(m_firstInt, m_secondInt);
            }
        });
    }

    this->Init = true;
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

void WidgetTreeTextBox::SetVal(const QString &text)
{
    this->setText(text);
    RefreshState();
}

void WidgetTreeTextBox::mousePressEvent(QMouseEvent *event)
{
    emit clickedOrFocused();
}

void WidgetTreeTextBox::mouseReleaseEvent(QMouseEvent* event)
{
    // Give focus explicitly when the user releases the mouse
    this->setFocus(Qt::MouseFocusReason);
    // Then call the base class so cursor and editing work normally
    QLineEdit::mouseReleaseEvent(event);
}


void WidgetTable::keyPressEvent(QKeyEvent* event)
{

    // Example: only allow numbers, decimal, and backspace
    if ((event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) || event->key() == Qt::Key_Period || event->key() == Qt::Key_Backspace)
    {
        QTableWidget::keyPressEvent(event); // pass to base
    }
    else if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier)
    {

        QItemSelectionModel* sel = selectionModel();
        if (!sel) return;

        QModelIndexList indexes = sel->selectedIndexes();
        if (indexes.isEmpty()) return;

        // Determine the range of rows and columns
        int minRow = rowCount(), maxRow = 0;
        int minCol = columnCount(), maxCol = 0;

        for (const QModelIndex& idx : indexes)
        {
            if (idx.row() < minRow) minRow = idx.row();
            if (idx.row() > maxRow) maxRow = idx.row();
            if (idx.column() < minCol) minCol = idx.column();
            if (idx.column() > maxCol) maxCol = idx.column();
        }

        // Build tab-separated text
        QString clipboardText;
        for (int r = minRow; r <= maxRow; ++r)
        {
            QStringList rowValues;
            for (int c = minCol; c <= maxCol; ++c)
            {
                QString value;

                if (auto editor = qobject_cast<WidgetTreeTextBox*>(cellWidget(r, c)))
                    value = editor->text();
                else if (auto itm = this->item(r, c))
                    value = itm->text();

                rowValues << value;
            }
            clipboardText += rowValues.join('\t');
            if (r < maxRow) clipboardText += "\r\n";
        }

        // Copy to system clipboard
        QClipboard* cp = QApplication::clipboard();
        cp->setText(clipboardText);

        // Optionally, store in memory as a 2D vector
        copiedData.clear();
        copiedData.resize(maxRow - minRow + 1);
        for (int r = 0; r <= maxRow - minRow; ++r)
        {
            copiedData[r].resize(maxCol - minCol + 1);
            for (int c = 0; c <= maxCol - minCol; ++c)
            {
                if (auto editor = qobject_cast<WidgetTreeTextBox*>(cellWidget(minRow + r, minCol + c)))
                    copiedData[r][c] = editor->text();
                else if (auto item = this->item(minRow + r, minCol + c))
                    copiedData[r][c] = item->text();
                else
                    copiedData[r][c] = "";
            }
        }

        return;

    }
    else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_V)
    {

        QString text = QApplication::clipboard()->text();
        QStringList rows = text.split(QRegularExpression("\r\n|\n"), Qt::SkipEmptyParts);

        int startRow = currentIndex().row();
        int startCol = currentIndex().column();

        for (int r = 0; r < rows.size(); ++r)
        {
            QStringList cols = rows[r].split('\t');
            for (int c = 0; c < cols.size(); ++c)
            {
                int row = startRow + r;
                int col = startCol + c;
                if (row < rowCount() && col < columnCount())
                {
                    // Update WidgetTreeTextBox if present
                    if (auto editor = qobject_cast<WidgetTreeTextBox*>(cellWidget(row, col)))
                        editor->setText(cols[c]);
                    else
                        setItem(row, col, new QTableWidgetItem(cols[c]));
                }
            }
        }
        return;

    }
    else
    {
        // ignore other keys
        event->ignore();
    }
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
        QTreeWidget *WidgetParent;

    public:

        using IconClickedCallback = std::function<void(int section, int icon, string filename)>;
        IconClickedCallback onIconClicked;

        using BinFileCloseCallback = std::function<void(int section, int icon, string filename)>;
        IconClickedCallback onBinFileClose;

        ClickableHeader(Qt::Orientation orientation, QWidget *parent = nullptr) : QHeaderView(orientation, parent)
        {
            WidgetParent = (QTreeWidget *)parent;
            m_hoveredSection = -1;
            m_columns.clear();
        }

        void addColumn(const QString& Name, uint32_t Size, bool isFile = false)
        {
            m_columns.push_back({Name.toStdString(), Size, isFile});
            if (WidgetParent)
            {
                int colIndex = static_cast<int>(m_columns.size() - 1); // index of new column

                WidgetParent->setColumnCount(m_columns.size());          // update column count
                WidgetParent->setColumnWidth(colIndex, Size);
                // Optional: stretch the last column
                if (colIndex == WidgetParent->columnCount() - 1)
                {
                    WidgetParent->header()->setStretchLastSection(false);
                }
            }
            WidgetParent->header()->setStretchLastSection(true);
            update();
        }

        void removeColumn(uint32_t Idx)
        {
            m_columns.remove(Idx);
            WidgetParent->setColumnCount(WidgetParent->columnCount() -1);
        }

        // Show/hide a specific column by index
        void setColumnVisible(int index, bool visible)
        {
            if (WidgetParent && index >= 0 && index < WidgetParent->columnCount())
            {
                WidgetParent->setColumnHidden(index, !visible);
            }
        }

    protected:

        void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override
        {
            if (logicalIndex < 0 || logicalIndex >= m_columns.size())
                return;

            const ColumnInfo& col = m_columns[logicalIndex];



            constexpr int IconSize     = 20;
            constexpr int IconSpacing  = 2;
            constexpr int IconPadding  = 2;   // <-- THIS is the padding you want


            // --- Draw background for hovered icon only ---
            if (col.isFile)
            {
                // --- Draw icons first ---
                int xRight = rect.right() - IconSize - IconSpacing;
                int yTop   = rect.center().y() - IconSize / 2;

                QRect iconBgRects[2];
                iconBgRects[0] = QRect(xRight, yTop, IconSize, IconSize);
                xRight -= IconSize + IconSpacing;
                iconBgRects[1] = QRect(xRight, yTop, IconSize, IconSize);

                // Draw hover backgrounds
                for (int i = 0; i < 2; ++i)
                {
                    if (m_hoveredSection == logicalIndex && m_hoveredIconIndex == i)
                    {
                        painter->setBrush(QColor("#cce4ff"));
                        painter->setPen(Qt::NoPen);
                        painter->drawRoundedRect(iconBgRects[i], 4.0, 4.0);
                    }
                }

                // Draw icons
                QIcon iconClose(":/icon/close.svg");
                QIcon iconSave(":/icon/save-file.svg");

                QRect iconInner0 = iconBgRects[0].adjusted(IconPadding, IconPadding, -IconPadding, -IconPadding);
                QRect iconInner1 = iconBgRects[1].adjusted(IconPadding, IconPadding, -IconPadding, -IconPadding);

                iconClose.paint(painter, iconInner0, Qt::AlignCenter);
                iconSave.paint(painter, iconInner1, Qt::AlignCenter);

                // --- Draw text centered in full rect ---
                QFontMetrics fm(painter->font());
                QString elidedText = fm.elidedText(QString::fromStdString(col.Name),
                                                   Qt::ElideRight,
                                                   rect.width()); // full width

                int textWidth = fm.horizontalAdvance(elidedText);

                // Center text in the **entire rect**
                int textX = rect.left() + (rect.width() - textWidth) / 2;

                // Draw text vertically centered
                QRectF drawRect(textX, rect.top(), textWidth, rect.height());
                painter->setPen(Qt::black);
                painter->drawText(drawRect, Qt::AlignVCenter, elidedText);
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

            if (logicalIndex > 0)
            {
                painter->save();

                QPen borderPen(Qt::gray, 1); // line color and thickness
                painter->setPen(borderPen);
                // Left edge
                //painter->drawLine(rect.left() + 0.5, rect.top(), rect.left() + 0.5, rect.bottom());
                //painter->drawLine(rect.left() + 1.5, rect.top(), rect.left() + 1.5, rect.bottom());

                painter->fillRect(rect.left(), rect.top(), 1, rect.height(), Qt::gray);

                painter->restore();  // restores pen, brush, etc.
            }
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

BinCalibToolWidget::BinCalibToolWidget(QWidget* parent, FileBin_ELF* elf)
    : QWidget(parent), ELFData(elf)
{
    IsViewAdvanced = true;
    BaseFileData.clear();

    // 1. Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 2. Toolbar
    m_toolBar = new QToolBar("Calibration Tools", this);
    m_toolBar->setIconSize(QSize(20, 20));
    m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    if (auto layout = m_toolBar->layout()) {
        layout->setSpacing(8);
        layout->setContentsMargins(4, 2, 4, 2);
    }

    m_toolBar->setStyleSheet(R"(
        QToolButton {
            padding: 2px;
            margin: 2px;
            border: none;
            background-color: transparent;
            border-radius: 4px;
        }
        QToolButton:checked {
            background-color: #80aee0;
            color: white;
        }
        QToolButton:hover {
            background-color: #e6f0fa;
        }
        QToolButton:checked:hover {
            background-color: #80aee0;
        }
    )");

    // Toolbar actions
    QAction* actionViewAdvanced = m_toolBar->addAction(QIcon(":/icon/view_advanced.svg"), "View advanced");
    QAction* actionLoadFile = m_toolBar->addAction(QIcon(":/icon/folder-open.svg"), "Open file...");
    actionViewAdvanced->setCheckable(true);
    actionViewAdvanced->setChecked(true);

    mainLayout->addWidget(m_toolBar);

    // 3. Splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(2);
    m_splitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #f3f3f3;
        }
    )");

    // 4. Tree Widget (left)
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({"Calibratable files"});
    m_treeWidget->setFrameShape(QFrame::NoFrame);
    m_treeWidget->setIndentation(12);
    m_treeWidget->setFocusPolicy(Qt::NoFocus);
    m_treeWidget->setStyleSheet(
        "QTreeWidget { border: none; background: transparent; }"
        "QTreeView::item { height: 22px; }"
        );

    // 5. Symbol Tree (right)
    m_symbolTree = new QTreeWidget(this);

    header = new ClickableHeader(Qt::Horizontal, m_symbolTree);
    m_symbolTree->setHeader(header);

    header->addColumn("Symbol", 260);
    header->addColumn("Address", 100);
    header->addColumn("Size", 80);
    header->addColumn("Type", 60);

    m_symbolTree->setStyleSheet(R"(
        QTreeWidget {
            border: none;
            background-color: white;
            padding: 0px;
            margin: 0px;
        }
        QTreeWidget::viewport {
            padding: 0px;
            margin: 0px;
        }
        QTreeWidget::item {
            background-color: #E8E8E8;
            height: 22px;
            padding: 0px;
            margin: 0px;
        }
        QTreeWidget::item:alternate {
            background-color: #F5F5F5;
            height: 22px;
        }
        QTreeWidget::item:hover {
            background-color: #CCE4FF;
        }
    )");

    m_symbolTree->setAlternatingRowColors(true);
    m_symbolTree->setIndentation(12);
    m_symbolTree->header()->setStretchLastSection(true);
    m_symbolTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_symbolTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_symbolTree->setFocusPolicy(Qt::NoFocus);
    //m_symbolTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

    m_symbolTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Connect header signals
    header->onIconClicked = [this](int section, int icon, std::string filename){
        this->BaseFileData.at(section-4)->mem->Save(filename);
    };

    header->onBinFileClose = [this](int section, int, std::string){
        int columnToRemove = section;

        // Backup top-level items
        QList<QTreeWidgetItem*> topLevelCopies;
        for (int i = 0; i < m_symbolTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* orig = m_symbolTree->topLevelItem(i);
            QTreeWidgetItem* copy = copyItemWithoutColumn(orig, columnToRemove);
            topLevelCopies.append(copy);
        }

        header->removeColumn(columnToRemove);

        // Reinsert copied items
        for (auto* item : topLevelCopies)
            m_symbolTree->addTopLevelItem(item);

        // Update internal BaseFileData
        if (columnToRemove >= 4) {
            int dataIdx = columnToRemove - 4;
            if (dataIdx < BaseFileData.size())
                BaseFileData.erase(BaseFileData.begin() + dataIdx);
        }
    };

    // --- Right vertical splitter: symbol tree on top, table + toolbar below ---
    rightSplitter = new QSplitter(Qt::Vertical);

    // 1️⃣ Symbol tree on top
    rightSplitter->addWidget(m_symbolTree);

    // 2️⃣ Table container with toolbar
    tableContainer = new QWidget();
    QVBoxLayout* tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    // Toolbar
    QToolBar* tableToolBar = new QToolBar("Table Tools", tableContainer);
    tableToolBar->setIconSize(QSize(18, 18));
    tableToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tableLayout->addWidget(tableToolBar);

    QAction *addRowAction = new QAction(QIcon(":/icon/add_row.svg"), "X", this);
    connect(addRowAction, &QAction::triggered, this, &BinCalibToolWidget::hideTable);

    // spacer pushes following actions to the right
    QWidget *spacer = new QWidget(tableToolBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    tableToolBar->addWidget(spacer);
    tableToolBar->addAction(addRowAction);
    // Table
    m_tableWidgetCalib = new WidgetTable(tableContainer);
    m_tableWidgetCalib->setRowCount(0);
    m_tableWidgetCalib->setColumnCount(0); // adjust as needed
    m_tableWidgetCalib->horizontalHeader()->setDefaultSectionSize(60); // width
    m_tableWidgetCalib->verticalHeader()->setDefaultSectionSize(20);   // height
    tableLayout->addWidget(m_tableWidgetCalib);

    // Initially hide table + toolbar if desired
    tableContainer->setVisible(false); // or false

    rightSplitter->addWidget(tableContainer);

    // Optional: set initial sizes
    rightSplitter->setSizes({300, 200}); // tree 300px, table 200px

    // --- Main horizontal splitter ---
    m_splitter->addWidget(m_treeWidget);     // left tree
    m_splitter->addWidget(rightSplitter);    // right vertical splitter



    // 6. Add widgets to splitter
   // m_splitter->addWidget(m_treeWidget);
    //m_splitter->addWidget(m_symbolTree);
    m_splitter->setSizes({180, 600});
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(m_splitter);

    // 7. Connect toolbar & tree signals
    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &BinCalibToolWidget::onTreeItemClicked);

    connect(actionViewAdvanced, &QAction::triggered, this, [this, actionViewAdvanced](bool checked){
        IsViewAdvanced = checked;
        m_symbolTree->setColumnHidden(1, !IsViewAdvanced);
        m_symbolTree->setColumnHidden(2, !IsViewAdvanced);
        m_symbolTree->setColumnHidden(3, !IsViewAdvanced);
    });

    connect(actionLoadFile, &QAction::triggered, this, [this](bool){
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open File"),
            QString(),
            tr("ELF Files (*.elf);;Intel HEX Files (*.hex);;All Files (*)")
            );

        if (fileName.isEmpty()) return;

        const QString ext = QFileInfo(fileName).suffix().toLower();
        if (ext == "elf") {
            qDebug() << "Dropped master file:" << fileName;
            // loadElf(fileName.toStdString());
        } else if (ext == "hex") {
            auto* newBaseFile = new FileBin_IntelHex_Memory();
            newBaseFile->Load(fileName.toStdString().c_str(), LIB_FIRMWAREBIN_HEX);
            this->Calib_BaseFile_AddNew(fileName.toStdString(), newBaseFile);
        }
    });
}


void BinCalibToolWidget::hideTable(void)
{
    tableContainer->setVisible(false);
}

QTreeWidgetItem* BinCalibToolWidget::copyItemWithoutColumn(QTreeWidgetItem* item, int colToRemove)
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


#if(0)
DataValue BinCalibToolWidget::readMem(uint32_t BaseFileIdx, uint32_t Addr, FileBin_IntelHex_Memory *newFileBin)
{
    const vector<SymbolDataType*> baseData = this->BaseFileData.at(BaseFileIdx)->data;

    switch (baseData.at(i)->node->DataType)
    {
    case FileBin_VARINFO_TYPE_UINT8:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_uint8(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_SINT8:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_sint8(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_UINT16:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_uint16(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_SINT16:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_sint16(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_UINT32:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_uint32(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_SINT32:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_sint32(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_FLOAT32:
    {
        WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
        lineedit->SetVal(QString::number(newFileBin->ReadMem_float32(baseData.at(i)->node->Addr)));
        break;
    }
    case FileBin_VARINFO_TYPE_ENUM:
    {
        WidgetTreeComboBox *dataWidget = (WidgetTreeComboBox *)baseData.at(i)->WidgetData;
        dataWidget->setIdx(newFileBin->ReadMem_uint8(baseData.at(i)->node->Addr));
        break;
    }
    default:
    {
        break;
    }
    }
}
#endif


 void BinCalibToolWidget::Calib_BaseFile_DataParse(FileBin_VarInfoType* node, uint32_t BaseFileIdx, FileBin_IntelHex_Memory *newFileBin)
 {
    uint32_t childIdx = 0;

    if (!newFileBin || BaseFileIdx >= BaseFileData.size())
    {
        std::cout << "ERROR, BinFile incorrect" << std::endl;
        return;
    }

    const vector<SymbolDataType*> baseData = this->BaseFileData.at(BaseFileIdx)->data;

    for (int i = 0; i < baseData.size(); i++)
    {
        if (!baseData.at(i)->node)
        {
            break;
        }

        switch (baseData.at(i)->node->DataType)
        {
            case FileBin_VARINFO_TYPE_UINT8:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_uint8(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_SINT8:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_sint8(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_UINT16:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_uint16(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_SINT16:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_sint16(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_UINT32:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_uint32(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_SINT32:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_sint32(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_FLOAT32:
            {
                WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
                lineedit->SetVal(QString::number(newFileBin->ReadMem_float32(baseData.at(i)->node->Addr)));
                break;
            }
            case FileBin_VARINFO_TYPE_ENUM:
            {
                WidgetTreeComboBox *dataWidget = (WidgetTreeComboBox *)baseData.at(i)->WidgetData;
                dataWidget->setIdx(newFileBin->ReadMem_uint8(baseData.at(i)->node->Addr));
                break;
            }
            default:
            {
                break;
            }
        }
    }
 }

 void BinCalibToolWidget::BinMemWrite(FileBin_VarInfoType *InfoNode, uint32_t BinIdx, uint32_t SymbolIdx)
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

    //cout << "Writing memory BinFile type: " << (int)InfoNode->DataType <<  " Idx: " << BinIdx << " Symbol Idx: " << SymbolIdx << " Addr: 0x" << std::hex << InfoNode->Addr <<  endl;

    switch(this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->node->DataType)
    {
        case FileBin_VARINFO_TYPE_BOOLEAN:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_boolean(InfoNode->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_UINT8:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint8(InfoNode->Addr, textBox->text().toUInt());
            break;
        }

        case FileBin_VARINFO_TYPE_SINT8:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_sint8(InfoNode->Addr, textBox->text().toInt());
            break;
        }

        case FileBin_VARINFO_TYPE_UINT16:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint16(InfoNode->Addr, textBox->text().toUInt());
            break;
        }

        case FileBin_VARINFO_TYPE_SINT16:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_sint16(InfoNode->Addr, textBox->text().toInt());
            break;
        }

        case FileBin_VARINFO_TYPE_UINT32:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint32(InfoNode->Addr, textBox->text().toUInt());
            break;
        }

        case FileBin_VARINFO_TYPE_SINT32:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_sint32(InfoNode->Addr, textBox->text().toInt());
            break;
        }

        case FileBin_VARINFO_TYPE_FLOAT32:
        {
            WidgetTreeTextBox* textBox = (WidgetTreeTextBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_float32(InfoNode->Addr, textBox->text().toFloat());
            break;
        }

        case FileBin_VARINFO_TYPE_ENUM:
        {

            WidgetTreeComboBox* comboBox = (WidgetTreeComboBox*)this->BaseFileData.at(BinIdx)->data.at(SymbolIdx)->WidgetData;
            this->BaseFileData.at(BinIdx)->mem->WriteMem_uint8(InfoNode->Addr, comboBox->currentIndex());
            break;
        }

        default:
        {
            cout << "Undefiend datatype" << endl;
        }
    }
}

void BinCalibToolWidget::GenerateTable(uint8_t BaseFileIdx, FileBin_VarInfoType* node)
{
    uint32_t xLen = node->Size.at(0);
    uint32_t yLen = 1;
    uint8_t dataSize = node->Size.back();
    vector<float> data;
    vector<float> defaultData;

    if (node->Size.size() > 2)
    {
        yLen = node->Size.at(1);
    }

    int rows = m_tableWidgetCalib->rowCount();
    int cols = m_tableWidgetCalib->columnCount();

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            QWidget *widget = m_tableWidgetCalib->cellWidget(row, col);
            if (widget)
            {
                m_tableWidgetCalib->removeCellWidget(row, col);
                delete widget; // free memory
            }
        }
    }

    m_tableWidgetCalib->clear();
    m_tableWidgetCalib->setRowCount(0);
    m_tableWidgetCalib->setColumnCount(0);

    switch (node->DataType)
    {
        case FileBin_VARINFO_TYPE_UINT8:
        {
            //WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
            //lineedit->SetVal(QString::number(newFileBin->ReadMem_uint8(baseData.at(i)->node->Addr)));
            break;
        }
        case FileBin_VARINFO_TYPE_SINT8:
        {
            //WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
            //lineedit->SetVal(QString::number(newFileBin->ReadMem_sint8(baseData.at(i)->node->Addr)));
            break;
        }
        case FileBin_VARINFO_TYPE_UINT16:
        {
            vector<uint8_t> defaultDataRaw = this->ELFData->readSymbolFromELF(node->Addr, xLen * yLen * 2);

            // Convert every two bytes into uint16_t
            for (size_t i = 0; i < xLen * yLen; i += 1)
            {
                uint16_t value  = this->BaseFileData.at(BaseFileIdx)->mem->ReadMem_uint16(node->Addr + i*2);
                data.push_back(value);

                value = defaultDataRaw.at(i*2) + (defaultDataRaw.at((i*2)+1) << 8);
                defaultData.push_back(value);
            }

            break;
        }
        case FileBin_VARINFO_TYPE_SINT16:
        {
            vector<uint8_t> defaultDataRaw = this->ELFData->readSymbolFromELF(node->Addr, xLen * yLen * 2);

            // Convert every two bytes into uint16_t
            for (size_t i = 0; i < xLen * yLen; i += 1)
            {
                uint16_t value  = this->BaseFileData.at(BaseFileIdx)->mem->ReadMem_sint16(node->Addr + i*2);
                data.push_back(value);

                value = defaultDataRaw.at(i*2) + (defaultDataRaw.at((i*2)+1) << 8);
                defaultData.push_back(value);
            }

            break;
        }
        case FileBin_VARINFO_TYPE_UINT32:
        {
           // WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
            //lineedit->SetVal(QString::number(newFileBin->ReadMem_uint32(baseData.at(i)->node->Addr)));
            break;
        }
        case FileBin_VARINFO_TYPE_SINT32:
        {
            //WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
            //lineedit->SetVal(QString::number(newFileBin->ReadMem_sint32(baseData.at(i)->node->Addr)));
            break;
        }
        case FileBin_VARINFO_TYPE_FLOAT32:
        {
           // WidgetTreeTextBox *lineedit = (WidgetTreeTextBox *)baseData.at(i)->WidgetData;
            //lineedit->SetVal(QString::number(newFileBin->ReadMem_float32(baseData.at(i)->node->Addr)));
            break;
        }
        default:
        {
            break;
        }
    }


    // Set table dimensions first
    m_tableWidgetCalib->setRowCount(yLen);
    m_tableWidgetCalib->setColumnCount(xLen);

    // Optional: set row heights and column widths
    //for (uint32_t j = 0; j < yLen; j++) {
    //    m_tableWidgetCalib->setRowHeight(j, 20);
   // }
   // for (uint32_t i = 0; i < xLen; i++) {
   //     m_tableWidgetCalib->setColumnWidth(i, 60);
    //}

    // Populate cells
    // Fill table
    m_tableWidgetCalib->blockSignals(true);
    for (int row = 0; row < yLen; ++row) {
        for (int col = 0; col < xLen; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(QString::number(data[row * xLen + col]));
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
            m_tableWidgetCalib->setItem(row, col, item);

            if (data[row * xLen + col] != defaultData[row * xLen + col])
            {
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
        }
    }
    m_tableWidgetCalib->blockSignals(false);


    disconnect(m_tableWidgetCalib, &QTableWidget::itemChanged, nullptr, nullptr);

    connect(m_tableWidgetCalib, &QTableWidget::itemChanged,
            this, [this, BaseFileIdx, node, dataSize, xLen](QTableWidgetItem *item){
                int row = item->row();
                int col = item->column();
                size_t offset = (row * xLen + col) * dataSize;
                uint16_t value = item->text().toUInt();
                this->BaseFileData.at(BaseFileIdx)->mem->WriteMem_uint16(
                    node->Addr + offset,
                    value
                    );

                // Set bold if changed
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
                //handleCellChange(row, col, newValue);

            });

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

                QObject::connect(widgetData, &WidgetTreeTextBox::clickedOrFocused, [this, node, BaseFileIdx]() {
                    tableContainer->setVisible(true);        // show table + toolbar
                    rightSplitter->setSizes({300, 200});     // optional: restore splitter sizes
                    this->GenerateTable(BaseFileIdx, node);
                });

                widgetData->setText("<" + dims.join(" x ") + ">");

                tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);
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

                    tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        this->BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
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

                    tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        this->BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                }
                else if (FileBin_VARINFO_TYPE_SINT8 == node->DataType)
                {
                     std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    int8_t value = 0;
                    if (raw.size() >= 1)
                    {
                        value = static_cast<int8_t>(raw[0]);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
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

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
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

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                }
                else if (FileBin_VARINFO_TYPE_UINT32 == node->DataType)
                {
                    std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    uint32_t value = 0;

                    if (raw.size() >= 4)
                    {
                        value = static_cast<uint32_t>(raw[0])
                            | (static_cast<uint32_t>(raw[1]) << 8)
                            | (static_cast<uint32_t>(raw[2]) << 16)
                            | (static_cast<uint32_t>(raw[3]) << 24);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                }
                else if (FileBin_VARINFO_TYPE_SINT32 == node->DataType)
                {
                    std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));

                    int32_t value = 0;

                    if (raw.size() >= 4)
                    {
                        value = static_cast<int32_t>(raw[0])
                            | (static_cast<int32_t>(raw[1]) << 8)
                            | (static_cast<int32_t>(raw[2]) << 16)
                            | (static_cast<int32_t>(raw[3]) << 24);
                    }

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    QObject::connect(widgetData,
                                     &WidgetTreeTextBox::editingFinishedWithInts,
                                     [this, node](int firstInt, int secondInt)
                                     {
                                         BinMemWrite(node, firstInt, secondInt);
                                     });

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

                    WidgetTreeTextBox *widgetData = new WidgetTreeTextBox(this, false, 0, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), value);

                    tree->setItemWidget(item->child(childIdx), 4+BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeTextBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        BinMemWrite(node, firstInt, secondInt);
                    });


                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
                }
                else if (FileBin_VARINFO_TYPE_ENUM == node->DataType)
                {
                    uint8_t val;

                    if (node->Size.size() > 0)
                    {
                        std::vector<uint8_t> raw = ELFData->readSymbolFromELF(node->Addr, node->Size.at(0));
                        val = raw.at(0);
                    }

                    WidgetTreeComboBox *widgetData = new WidgetTreeComboBox(this, BaseFileIdx, BaseFileData.at(BaseFileIdx)->data.size(), val);

                    FileBin_DWARF_VarInfoType *enumNode = node->child->child->child;

                    while (enumNode)
                    {
                        widgetData->addItem(QString::fromStdString(std::string(enumNode->data.begin(), enumNode->data.end())));
                        enumNode = enumNode->next;
                    }

                    tree->setItemWidget(item->child(childIdx), 4 + BaseFileIdx, widgetData);

                    QObject::connect(widgetData, &WidgetTreeComboBox::editingFinishedWithInts, [this, node](int firstInt, int secondInt) {
                        BinMemWrite(node, firstInt, secondInt);
                    });

                    SymbolDataType *newDataInfo = new SymbolDataType();
                    newDataInfo->node = node;
                    newDataInfo->WidgetData = widgetData;
                    BaseFileData.at(BaseFileIdx)->data.push_back(newDataInfo);
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

    if (node->elementType == FILEBIN_DWARF_ELEMENT_COMPILE_UNIT)
    {
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
            {
                parentItem->addChild(item);
            }
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
            {
                populateTreeWidgetRecursive(node->child, item);
            }
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

void BinCalibToolWidget::Calib_MasterStruct(FileBin_VarInfoType* node)
{
    m_treeWidget->clear();
    selectedSymbolData = nullptr;
    SymbolData = node;

    QFont italicFont;
    italicFont.setItalic(true);

    for (FileBin_VarInfoType* cur = node; cur != nullptr; cur = cur->next)
    {
        QString displayName = "unnamed";

        if (!cur->data.empty())
        {
            const QString fullPath = QString::fromUtf8(reinterpret_cast<const char*>(cur->data.data()), static_cast<int>(cur->data.size()));

            displayName = QFileInfo(fullPath).fileName();
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, displayName);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        if (cur->data.empty())
        {
            item->setFont(0, italicFont);
        }
    }
}
