/**
 *  \file       mainwindow.cpp
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
#include "./ui_mainwindow.h"
#include "FileBin_ELF.h"
#include "FileBin_DWARF.h"
#include <string>
#include <QStandardItemModel>
#include <chrono>   // for timing
#include <iostream>
#include <QFileInfo>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QMetaType>
#include "WidgetBinCalib.hpp"
#include <QIcon>
#include <QPixmap>
#include <QIcon>
#include <QTransform>
#include "FileBin_IntelHex.hpp"

QIcon rotateIcon(const QIcon &icon, qreal angle)
{
    // Choose the actual pixmap (default size)
    QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(32, 32)));

    // Rotate the pixmap
    QTransform transform;
    transform.rotate(angle);
    QPixmap rotated = pixmap.transformed(transform, Qt::SmoothTransformation);

    // Return a new QIcon
    return QIcon(rotated);
}

Q_DECLARE_METATYPE(FileBin_VarInfoType*)

FileBin_ELF *elf;
FileBin_DWARF *dwarf;
vector<FileBin_IntelHex_Memory *> base;


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

void populateModelRecursive(TreeElementType* node, QStandardItem* parentItem)
{
    static QFont italicFont;
    italicFont.setItalic(true);

    while (node)
    {
        QString displayName = node->data.empty()
        ? "unnamed"
        : QString::fromUtf8(reinterpret_cast<const char*>(node->data.data()), node->data.size());

        QStandardItem* nameItem = new QStandardItem(displayName);
        QStandardItem* tagItem  = new QStandardItem(QString::fromStdString(TagToString(node->elementType)));

        // Make read-only
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        tagItem->setFlags(tagItem->flags() & ~Qt::ItemIsEditable);

        if (node->data.empty())
            nameItem->setFont(italicFont);

        parentItem->appendRow({ nameItem, tagItem });

        if (node->child)
            populateModelRecursive(node->child, nameItem);

        node = node->next;
    }
}

static QString formatSize(const std::vector<uint32_t>& size)
{
    if (size.empty())
        return {};

    if (size.size() == 1)
    {
        // Single element
        uint32_t val = size.front();
        return QString::number(val) + (val == 1 ? " byte" : " bytes");
    }

    // Multi-dimensional array
    QStringList dims;
    dims.reserve(size.size());
    for (uint32_t dim : size)
        dims << QString::number(dim);

    return QString("<%1> bytes").arg(dims.join("x"));
}

static QString extractDisplayName(const FileBin_VarInfoType* node)
{
    if (node->data.empty())
        return QStringLiteral("unnamed");

    QString name = QString::fromStdString(
        std::string(node->data.begin(), node->data.end())
        );

    if (node->elementType == FILEBIN_DWARF_ELEMENT_COMPILE_UNIT) {
        name = QFileInfo(name).fileName();
    }

    return name;
}

static QString formatType(uint32_t typeId)
{
    switch (static_cast<FileBin_DWARF_VarInfoLenType>(typeId))
    {
    case FileBin_VARINFO_TYPE_BOOLEAN:   return QStringLiteral("bool");
    case FileBin_VARINFO_TYPE_UINT8:     return QStringLiteral("uint8");
    case FileBin_VARINFO_TYPE_SINT8:     return QStringLiteral("sint8");
    case FileBin_VARINFO_TYPE_UINT16:    return QStringLiteral("uint16");
    case FileBin_VARINFO_TYPE_SINT16:    return QStringLiteral("sint16");
    case FileBin_VARINFO_TYPE_UINT32:    return QStringLiteral("uint32");
    case FileBin_VARINFO_TYPE_SINT32:    return QStringLiteral("sint32");
    case FileBin_VARINFO_TYPE_UINT64:    return QStringLiteral("uint64");
    case FileBin_VARINFO_TYPE_SINT64:    return QStringLiteral("sint64");
    case FileBin_VARINFO_TYPE_FLOAT32:   return QStringLiteral("float32");
    case FileBin_VARINFO_TYPE_FLOAT64:   return QStringLiteral("float64");
    case FileBin_VARINFO_TYPE_ENUM:     return QStringLiteral("[enum]");
    case FileBin_VARINFO_TYPE_STRUCT:     return QStringLiteral("[struct]");
    case FileBin_VARINFO_TYPE__UNKNOWN:
    default:
        return QStringLiteral("");
    }
}

void populateModelRecursiveSymbol(FileBin_VarInfoType* node, QStandardItem* parentItem)
{
    static QFont italicFont;
    italicFont.setItalic(true);

    static Qt::ItemFlags roFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled; // read-only

    while (node)
    {
        QStandardItem* nameItem = parentItem;

        if (!node->isQualifier)
        {
            QString name = node->data.empty()
            ? "unnamed"
            : QString::fromUtf8(reinterpret_cast<const char*>(node->data.data()), node->data.size());
            QString addr = QString::asprintf("0x%X", node->Addr);
            QString size = formatSize(node->Size);
            QString type = formatType(node->DataType);

            // Create QStandardItems for the row
            nameItem = new QStandardItem(name);
            QStandardItem* addrItem = new QStandardItem(addr);
            QStandardItem* sizeItem = new QStandardItem(size);
            QStandardItem* typeItem = new QStandardItem(type);

            // Read-only
            nameItem->setFlags(roFlags);
            addrItem->setFlags(roFlags);
            sizeItem->setFlags(roFlags);
            typeItem->setFlags(roFlags);

            // Center alignment for numeric fields
            addrItem->setTextAlignment(Qt::AlignHCenter);
            sizeItem->setTextAlignment(Qt::AlignHCenter);
            typeItem->setTextAlignment(Qt::AlignHCenter);

            // Italic for unnamed nodes
            if (node->data.empty())
                nameItem->setFont(italicFont);

            // Store the pointer to this node in Qt::UserRole
            nameItem->setData(QVariant::fromValue(node), Qt::UserRole);

            // Append row to parent
            parentItem->appendRow({ nameItem, addrItem, sizeItem, typeItem });
        }

        // Recursive call for children
        if (node->child)
            populateModelRecursiveSymbol(node->child, nameItem);

        node = node->next;
    }
}



void beautifyTreeView(Ui::MainWindow *ui, QTreeView* view)
{
    // QTreeView* view = ui->treeView;

    view->setRootIsDecorated(true);
    view->setUniformRowHeights(true);
    view->setAlternatingRowColors(true);
    view->setAllColumnsShowFocus(true);

    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);

    view->setExpandsOnDoubleClick(true);
    //view->setAnimated(true);

    view->header()->setStretchLastSection(true);
    view->header()->setDefaultAlignment(Qt::AlignCenter); // Center header text
    view->header()->setFixedHeight(25);         // smaller header height

    view->header()->resizeSection(0, 400); // Column 0 width = 200 pixels
    view->header()->resizeSection(1, 100); // Column 1 width = 150 pixels
    view->header()->resizeSection(2, 120); // Column 1 width = 150 pixels

    view->setStyleSheet(R"(
        QHeaderView::section {
            background-color: #e0e0e0;
            color: black;
            font: bold 9pt "Segoe UI";
            padding: 4px;
            border: 1px solid #ccc;
        }
        QTreeView {
            background-color: white;
            alternate-background-color: #f0f0f0;
            color: black;
            border: 1px solid #ccc;
            font: 8pt "Segoe UI";
        }

        QTreeView::item {
            padding: 2px 2px;
        }

        QTreeView::item:selected {
            background-color: #cce5ff;
            color: black;
        }
    )");
}


void populateTopLevel(TreeElementType* node, QStandardItem* parentItem)
{
    static QFont italicFont;
    italicFont.setItalic(true);

    while (node)
    {
        QString displayName = node->data.empty()
        ? "unnamed"
        : QString::fromUtf8(reinterpret_cast<const char*>(node->data.data()), node->data.size());

        QStandardItem* nameItem = new QStandardItem(displayName);
        QStandardItem* tagItem  = new QStandardItem(QString::fromStdString(TagToString(node->elementType)));

        // Make read-only
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        tagItem->setFlags(tagItem->flags() & ~Qt::ItemIsEditable);

        if (node->data.empty())
            nameItem->setFont(italicFont);

        parentItem->appendRow({ nameItem, tagItem });

        // If node has children, add a **dummy child** as a placeholder
        if (node->child)
        {
            QStandardItem* placeholder = new QStandardItem("Loading...");
            placeholder->setFlags(Qt::ItemIsEnabled);      // must be enabled to show expand arrow
            placeholder->setData(true, Qt::UserRole + 1);  // mark as placeholder
            nameItem->appendRow(placeholder);

            // Store a pointer to the actual TreeElementType in item data
            nameItem->setData(QVariant::fromValue(reinterpret_cast<void*>(node->child)), Qt::UserRole);
        }

        node = node->next;
    }
}

void MainWindow::populateTopLevelSymbol(FileBin_VarInfoType* node, QStandardItem* parentItem)
{
    static QFont italicFont;
    italicFont.setItalic(true);

    this->ui_BinCalibWidget->Calib_MasterStruct(node);

    while (node)
    {
        QString displayName = node->data.empty()
        ? "unnamed"
        : QString::fromUtf8(reinterpret_cast<const char*>(node->data.data()), node->data.size());

        QStandardItem* nameItem = new QStandardItem(displayName);
        QStandardItem* tagItem  = new QStandardItem(QString::fromStdString(TagToString(node->elementType)));

        // Make read-only
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        tagItem->setFlags(tagItem->flags() & ~Qt::ItemIsEditable);

        if (node->data.empty())
            nameItem->setFont(italicFont);

        parentItem->appendRow({ nameItem, tagItem });

        // If node has children, add a **dummy child** as a placeholder
        if (node->child)
        {
            QStandardItem* placeholder = new QStandardItem("Loading...");
            placeholder->setFlags(Qt::ItemIsEnabled);      // must be enabled to show expand arrow
            placeholder->setData(true, Qt::UserRole + 1);  // mark as placeholder
            nameItem->appendRow(placeholder);

            // Store a pointer to the actual TreeElementType in item data
            nameItem->setData(QVariant::fromValue(reinterpret_cast<void*>(node->child)), Qt::UserRole);
        }

        node = node->next;
    }



}

QStandardItemModel *model, *modelSymbol;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Flat button style
    QString style = R"(

/* The main menu bar container */
QMenuBar {
background-color: #f0f0f0;
    font-size: 11px;
    spacing: 0px;              /* No gap between File, Edit, etc. */
    min-height: 18px;          /* Forces a slim bar */
    border-bottom: 1px solid #d0d0d0;
}

/* Individual menu items (File, Edit, etc.) */
QMenuBar::item {
    background-color: transparent;
    padding: 4px 10px;         /* Horizontal spacing for small text */
    margin: 1px;
    border-radius: 3px;        /* Consistent rounded edges */
}

/* Hover state for menu items */
QMenuBar::item:selected {
    background-color: #e2e2e2; /* Subtle grey hover */
    color: #000000;
}

/* The actual dropdown menus */
QMenu {
    background-color: #ffffff;
    border: 1px solid #d0d0d0;
    font-size: 11px;
}

QMenu::item {
background: transparent;
    padding: 5px 6px;          /* 1px top/bottom is the extreme limit */
    margin: 0px;
    border-radius: 2px;
}

QMenu::item:selected {
    background-color: #80aee0; /* Fynix blue for the dropdown selection */
    color: white;
}


    /* QPushButton / QToolButton */
    QPushButton:checked, QToolButton:checked {
        background-color: #5aa0ff;
        color: white;
        border: none;
        border-radius: 0;
    }
    QPushButton, QToolButton {
        background-color: transparent;
        color: black;
        border: none;
        border-radius: 0;
    }
    QPushButton:hover, QToolButton:hover {
        background-color: #e6f0fa;
    }
    QPushButton:disabled, QToolButton:disabled {
        color: #888888;
        background-color: transparent;
        border: none;
    }

    /* QTreeWidget selected items */
    QTreeWidget::item:selected {
        background-color: #80aee0;
        color: black; /* optional: white text on blue */
    }

    QTreeWidget::item:selected:hover {
        background-color: #80aee0;
        color: black; /* optional: white text on blue */
    }

    /* QTreeWidget hover */
    QTreeWidget::item:hover {
        background-color: #d0e4ff;
    }

    /* Remove focus rectangle / dotted line */
    QTreeWidget::item:focus {
        outline: none;
    }

/* Tighten the toolbar container */
QToolBar {
    background-color: #f0f0f0;
    border: none;
    border-bottom: 1px solid #d0d0d0;
    spacing: 4px;               /* Minimal space between buttons */
    padding: 3px;               /* Minimal padding around the bar */
}

/* Compact Buttons */
QToolBar QToolButton {
background-color: transparent;
    border: none;
    border-radius: 4px;
    padding: 5px;               /* Balanced padding */
    margin: 0px;
}

QToolButton:hover {
    background-color: #d0e4ff;
}

QToolButton:checked {
    background-color: #80aee0;
}
)";

    this->setStyleSheet(style);


    // Apply to the whole main window
    //this->setStyleSheet(buttonStyle);
    elf = new FileBin_ELF();
    dwarf = new FileBin_DWARF();

    this->ui_BinCalibWidget = new BinCalibToolWidget(this, elf);

    // Force the menubar to be strictly the height of its contents
    ui->menubar->setContentsMargins(0, 0, 0, 0);

    // If using a layout for the window, ensure no gap between menu and toolbar
    this->layout()->setSpacing(0);

    setWindowTitle(
        QString("%1 v%2")
            .arg(QCoreApplication::applicationName())
            .arg(QCoreApplication::applicationVersion())
        );



    ui->splitter->setSizes(QList<int>() << 800 << 100);

    qRegisterMetaType<FileBin_VarInfoType*>("FileBin_VarInfoType*");

    model = new QStandardItemModel(this);



    setAcceptDrops(true);

    ui->treeView->setModel(model);

    ui->toolBar->setIconSize(QSize(20, 20));
    ui->toolBar->setFixedHeight(36);          // Total height including padding
    ui->toolBar->setVisible(false);

    modelSymbol = new QStandardItemModel(this);

    ui->treeView_2->setModel(modelSymbol);

    ui->treeView_2->setFocusPolicy(Qt::NoFocus);
    //ui->->setFocusPolicy(Qt::NoFocus);

    ui->tabWidget_2->addTab(this->ui_BinCalibWidget, "Calibrator");

    ui->tabWidget_2->setTabIcon(0, QIcon(":/icon/inspect.svg"));

    for (int i = 0; i < ui->tabWidget_2->count(); ++i) {
        ui->tabWidget_2->setTabText(i, "");  // remove text
    }

    QIcon homeIcon(":/icon/inspect.svg");
    ui->tabWidget_2->setTabIcon(0, rotateIcon(homeIcon, 90)); // rotate 45 degrees

    QIcon settingsIcon(":/icon/calibrate.svg");
    ui->tabWidget_2->setTabIcon(1, rotateIcon(settingsIcon, 90)); // rotate 90 degrees

    ui->tabWidget_2->setIconSize(QSize(34, 34)); // icon fits nicely inside 48x48 tab
    ui->tabWidget_2->tabBar()->setFocusPolicy(Qt::NoFocus);

    QTabBar* bar = ui->tabWidget_2->tabBar();

    // Change cursor when hovering over any tab
    //bar->setCursor(Qt::PointingHandCursor);  // hand cursor

    bar->setStyleSheet(R"(
    QTabBar::tab {
        width: 48px;
        height: 48px;
        padding-left: 3px;   /* reserve space for vertical bar */
        padding-right: 2px;
        padding-top: 0px;
        padding-bottom: 8px;
        margin: 0px;
        border: none;
        border-radius: 0px;
        background: #f3f3f3;
        qproperty-alignment: AlignCenter; /* center icon */
    }

    QTabBar::tab:selected {
        border-left: 3px solid #80aee0; /* vertical selection bar */
        padding-left: 0px; /* keep icon centered */
    }

    QTabBar::tab:hover:!selected {
        border-left: 3px solid #e6f0fa;  /* lighter blue */
        padding-left: 0px;               /* keep icon centered */
background: #e6f0fa;             /* subtle grey hover */
    }
)");

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::displayBinaryFile(FileBin_VarInfoType *symbol)
{
    if (!symbol || !elf)
        return;

    if (symbol->Size.empty())
        return;

    // Read symbol bytes from ELF
    std::vector<uint8_t> data = elf->readSymbolFromELF(symbol->Addr, symbol->Size.at(0));

    const int bytesPerLine = 16;        // 16 bytes per line
    QString hexDump;

    for (int i = 0; i < static_cast<int>(data.size()); i += bytesPerLine)
    {
        QString line;

        // First column: memory address
        line += QString("0x%1: ").arg(symbol->Addr, 8, 16, QChar('0')).toUpper();

        // Hex bytes column
        for (int j = 0; j < bytesPerLine; ++j)
        {
            if (i + j < static_cast<int>(data.size()))
                line += QString("%1 ").arg(static_cast<unsigned char>(data[i + j]), 2, 16, QChar('0')).toUpper();
            else
                line += "   ";
        }

        // ASCII representation column
        line += " |";
        for (int j = 0; j < bytesPerLine; ++j)
        {
            if (i + j < static_cast<int>(data.size()))
            {
                char c = data[i + j];
                line += (c >= 32 && c <= 126) ? QChar(c) : '.';
            }
            else
            {
                line += ' ';
            }
        }
        line += "|\n";

        hexDump += line;
    }

    // Display in QTextEdit
    ui->textEdit->setFontFamily("Courier"); // monospaced
    ui->textEdit->setPlainText(hexDump);
}

void MainWindow::onTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    if (selected.indexes().isEmpty())
        return;

    // Usually only care about the first column index
    QModelIndex index = selected.indexes().first();
    if (!index.isValid())
        return;

    QStandardItem* item = modelSymbol->itemFromIndex(index);
    if (!item)
        return;

    // Retrieve your pointer stored in Qt::UserRole
    QVariant var = item->data(Qt::UserRole);
    if (!var.isValid())
        return;

    FileBin_VarInfoType* node = var.value<FileBin_VarInfoType*>();
    if (!node)
        return;

    // Now, you can update your QTextEdit with the binary content
    displayBinaryFile(node);
}

void MainWindow::loadElf(std::string file_name)
{
    auto start = std::chrono::high_resolution_clock::now();

    //delete elf;
    //delete dwarf;
    //elf = nullptr;
    //dwarf = nullptr;

    model->clear();
    modelSymbol->clear();

    //ui->treeView->setModel(new QStandardItemModel());
    //ui->treeView_2->setModel(new QStandardItemModel());

    model->setHorizontalHeaderLabels({"Name", "Value"});
    modelSymbol->setHorizontalHeaderLabels({"Name", "Value", "Size", "Type"});

    elf->Parse(file_name);

    if (elf->IsDWARF())
    {
        dwarf->Parse(
            file_name,
            elf->GetAbbrevOffset(),
            elf->GetAbbrevLen(),
            elf->GetInfoOffset(),
            elf->GetInfoLen(),
            elf->GetStrOffset()
            );



        TreeElementType* rootNode = dwarf->DataRoot;

       // if (!rootNode)
        {
        //    return;
        }

        QStandardItem* rootItem = model->invisibleRootItem();

        ui->treeView->setUpdatesEnabled(false);
        populateTopLevel(rootNode, rootItem);
        ui->treeView->setUpdatesEnabled(true);

        FileBin_VarInfoType* rootNodeSymbol = dwarf->SymbolRoot;

        //if (!rootNodeSymbol)
        {
        //    return;
        }

        QStandardItem* rootItemSymbol = modelSymbol->invisibleRootItem();

        ui->treeView_2->setUpdatesEnabled(false);
        populateTopLevelSymbol(rootNodeSymbol, rootItemSymbol);
        ui->treeView_2->setUpdatesEnabled(true);

        // In MainWindow constructor
        connect(ui->treeView_2->selectionModel(),
                &QItemSelectionModel::selectionChanged,
                this, &MainWindow::onTreeSelectionChanged);

        // Lazy loading for DWARF tree
        connect(ui->treeView, &QTreeView::expanded, this,
                [this](const QModelIndex& index)
                {
                    auto model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
                    if (!model) return;

                    QStandardItem* item = model->itemFromIndex(index);
                    if (!item || item->rowCount() == 0) return;

                    QStandardItem* firstChild = item->child(0);
                    if (firstChild && firstChild->data(Qt::UserRole + 1).toBool())
                    {
                        item->removeRow(0);

                        QVariant var = item->data(Qt::UserRole);
                        if (var.isValid())
                        {
                            auto* childNode =
                                reinterpret_cast<TreeElementType*>(var.value<void*>());
                            populateModelRecursive(childNode, item);
                            item->setData(QVariant(), Qt::UserRole);
                        }
                    }
                });
        // Lazy loading for symbol tree
        connect(ui->treeView_2, &QTreeView::expanded, this,
                [this](const QModelIndex& index)
                {
                    auto model = qobject_cast<QStandardItemModel*>(ui->treeView_2->model());
                    if (!model) return;

                    QStandardItem* item = model->itemFromIndex(index);
                    if (!item || item->rowCount() == 0) return;

                    QStandardItem* firstChild = item->child(0);
                    if (firstChild && firstChild->data(Qt::UserRole + 1).toBool())
                    {
                        item->removeRow(0);

                        QVariant var = item->data(Qt::UserRole);
                        if (var.isValid())
                        {
                            auto* childNode =
                                reinterpret_cast<FileBin_VarInfoType*>(var.value<void*>());
                            populateModelRecursiveSymbol(childNode, item);
                            item->setData(QVariant(), Qt::UserRole);
                        }
                    }
                });
    }


    auto end = std::chrono::high_resolution_clock::now();
    uint32_t duration_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    float duration_ms = duration_us / 1000.0f;

    std::cout << "[INFO] Parsing time: "
              << duration_ms << " ms" << std::endl;

    // Publish parsing time to status bar
    this->statusBar()->showMessage(
        QString("Parsing completed in %1 ms").arg(duration_ms, 0, 'f', 2),
        0
        );

    beautifyTreeView(ui, ui->treeView);
    beautifyTreeView(ui, ui->treeView_2);
}




void MainWindow::AddNewBaseFile(QString Filename)
{
    FileBin_IntelHex_Memory *newBaseFile = new FileBin_IntelHex_Memory();
    newBaseFile->Load(Filename.toStdString().c_str(), LIB_FIRMWAREBIN_HEX);
    this->ui_BinCalibWidget->Calib_BaseFile_AddNew(Filename.toStdString(), newBaseFile);
}

void MainWindow::on_actionOpen_triggered(bool checked)
{
    Q_UNUSED(checked);

    QString fileNameQt = QFileDialog::getOpenFileName(
        this,
        tr("Open ELF File"),
        QString(),                       // default directory
        tr("ELF Files (*.elf);;All Files (*)")
        );

    if (fileNameQt.isEmpty())
    {
        // User cancelled dialog
        return;
    }

    std::string file_name = fileNameQt.toStdString();

    loadElf(file_name);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Check if the dragged data contains files or text
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText())
    {
        event->acceptProposedAction(); // Accept the drag operation
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    // Accept the drag move event (optional)
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    // Handle the dropped data
    if (event->mimeData()->hasUrls())
    {
        // Handle file drops
        for (const QUrl &url : event->mimeData()->urls())
        {
            QString filePath = url.toLocalFile(); // Get the local file path
            QFileInfo fileInfo(filePath); // Create a QFileInfo object

            // Get the file extension (e.g., "txt", "jpg")
            QString extension = fileInfo.suffix().toLower();

            // Check the file extension
            if (extension == "elf")
            {
                qDebug() << "Dropped master file:" << filePath;
                loadElf(filePath.toStdString());


                //IsMasterFileLoaded = true;
            }
            else if (extension == "hex")
            {
                qDebug() << "Dropped image file:" << filePath;
                ///if (!Mem2[this->BaseFileCnt].Load(filePath.toStdString().c_str(), LIB_FIRMWAREBIN_HEX))
                //{
                //    cout << "Error loading" << endl;
                //    return;
               // }
                //else
                //{
                 //   cout << "Loading finished" << endl;
                    AddNewBaseFile(filePath);

                  //  Parse_Master_Symbol();
                   // parseHexFile();
                   // isFileLoaded = true;
                //}
            }
            else
            {
                qDebug() << "Dropped file with unsupported extension:" << filePath;
            }
        }
    }

    event->acceptProposedAction(); // Accept the drop operation
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    // Handle drag leave event
    event->accept();
}



