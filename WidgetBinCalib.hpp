/**
 *  \file       WidgetBinCalib.cpp
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

#ifndef WIDGETBINCALIB_H
#define WIDGETBINCALIB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>
#include <QSplitter>
#include <qtreewidget.h>
#include <qtoolbar.h>
#include "FileBin_DWARF.h"
#include "FileBin_ELF.h"
#include "FileBin_IntelHex.hpp"
#include "WidgetTreeTextBox.hpp"

// Data structure to pass info into the library
struct DwarfSymbol {
    QString name;
    uint32_t address;
    QString type;
};

typedef struct SymbolData
{
    FileBin_DWARF_VarInfoType* node;
    QWidget *WidgetData;
} SymbolDataType;

struct SymbolDataInfo {
    vector<SymbolDataType *>data;
    FileBin_IntelHex_Memory *mem;
    std::string filename;
};


class BinCalibToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BinCalibToolWidget(QWidget *parent = nullptr, FileBin_ELF * elf = nullptr);

    // Public API to feed the widget data
    void loadSymbols(FileBin_VarInfoType* node);
    void setTargetHexPath(const QString &path);
    void AddNewBaseFile(std::string filename);




    void Calib_MasterStruct(FileBin_VarInfoType* node); // Generate master struct of source ifles
    void Calib_MasterSymbolShow(QTreeWidgetItem* item, int column); // Add master symbol list according to selected file
    void Calib_BaseFile_AddNew(std::string filename, FileBin_IntelHex_Memory *newFileBin); // Add new base file
    void Calib_BaseFile_WidgetPopulate(FileBin_VarInfoType* node, QTreeWidgetItem* item, uint32_t rowIdx); // Add base file widgets
    void Calib_BaseFile_DataParse(FileBin_VarInfoType* node, uint32_t BaseFileIdx, FileBin_IntelHex_Memory *newFileBin); // Parse base file into widgets
    void Calib_BaseFile_Remove(uint32_t BaseFileIdx); // Add new base file


signals:
    // Signal emitted when the library finishes a task
    void calibrationSaved(const QString &newFilePath);
    void errorOccurred(const QString &message);

private slots:
   // void handleApplyPatch();
    void onTreeItemClicked(QTreeWidgetItem* item, int column);

private:

    FileBin_DWARF_VarInfoType* SymbolData;
    FileBin_DWARF_VarInfoType* selectedSymbolData = nullptr;
    FileBin_ELF* ELFData;
    //std::vector<FileBaseInfo> BaseFile;
    bool IsMasterFileLoaded;
    vector<SymbolDataInfo *> BaseFileData;
    //uint32_t BaseFileCnt;


    QPushButton *m_applyBtn;
    QString m_hexFilePath;

    QToolBar     *m_toolBar;

    QTableWidget *m_tableWidget;  // Right: Hex/Values
    QSplitter    *m_splitter;     // Resizable divider

    QTreeWidgetItem* copyItemWithoutColumn(QTreeWidgetItem* item, int colToRemove);
    void loadSymbolData(FileBin_VarInfoType* node, QTreeWidgetItem* item, uint32_t rowIdx, uint32_t colIdx, uint32_t BaseFileColIdx);
    void populateTreeWidgetRecursive(FileBin_DWARF_VarInfoType* node, QTreeWidgetItem* parentItem = nullptr);
   // void RefreshBaseFile(void);
    void BinMemWrite(FileBin_VarInfoType *InfoNode, uint32_t BinIdx, uint32_t SymbolIdx);



};

#endif // WIDGETBINCALIB_H
