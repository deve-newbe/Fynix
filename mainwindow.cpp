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
Q_DECLARE_METATYPE(FileBin_VarInfoType*)

FileBin_ELF *elf;
FileBin_DWARF *dwarf;


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
    case FileBin_VARINFO_TYPE_FLOAT32:   return QStringLiteral("float32");
    case FileBin_VARINFO_TYPE_FLOAT64:   return QStringLiteral("float64");
    case FileBin_VARINFO_TYPE_ENUM:     return QStringLiteral("enum");
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

void populateTopLevelSymbol(FileBin_VarInfoType* node, QStandardItem* parentItem)
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

QStandardItemModel *model, *modelSymbol;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(
        QString("%1 v%2")
            .arg(QCoreApplication::applicationName())
            .arg(QCoreApplication::applicationVersion())
        );

    ui->splitter->setSizes(QList<int>() << 800 << 100);

    qRegisterMetaType<FileBin_VarInfoType*>("FileBin_VarInfoType*");

    model = new QStandardItemModel(this);

    elf = new FileBin_ELF();
    dwarf = new FileBin_DWARF();

    setAcceptDrops(true);

    ui->treeView->setModel(model);

    modelSymbol = new QStandardItemModel(this);

    ui->treeView_2->setModel(modelSymbol);
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

    qDebug() << "Selected symbol:"
             << (node->data.empty() ? "unnamed" : QString::fromUtf8(reinterpret_cast<const char*>(node->data.data()), node->data.size()))
             << " Addr:" << QString::asprintf("0x%X", node->Addr);

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



        TreeElementType* rootNode = dwarf->root;

       // if (!rootNode)
        {
        //    return;
        }

        QStandardItem* rootItem = model->invisibleRootItem();

        ui->treeView->setUpdatesEnabled(false);
        populateTopLevel(rootNode, rootItem);
        ui->treeView->setUpdatesEnabled(true);

        FileBin_VarInfoType* rootNodeSymbol = dwarf->Symbol;

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
                loadElf(filePath.toStdString());

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



