#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#include "cpu.hpp"
#include "dialogwindow.hpp"
#include "mytablewidgetitem.hpp"
#include "tablewidget.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QCloseEvent>
#include <QSettings>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , str_count(0)
    , isUntitled(true) //по умолчанию файл не имеет конкретного имени
    , isOpen(false) //по умолчанию в программе нет открытых файлов
    , isEdited(false) //по умолчанию файл не считается отредактированным
    , lastItemCreated(false) //по умолчанию false т.к. первая ячейка считается созданной только
                              //после отработки createTableItem в первой итерации цикла
    , list()

{
    ui->setupUi(this);

    connect(ui->action_Open, &QAction::triggered,
            this, &MainWindow::action_open_triggered); //связывание действия открытия и вызовы метода open
    connect(ui->action_New, &QAction::triggered,
            this, &MainWindow::action_newFile_triggered); //связывание действия создания нового файла и вызова метода newFile
    connect(ui->action_Save, &QAction::triggered,
            this, &MainWindow::action_save_triggered); //связывание действия сохранения и вызова метода save

    connect(ui->action_Exit, &QAction::triggered,
            this, &MainWindow::close); //связывание действия закрытия окна и вызова метода close
    connect(ui->action_Exit_All, &QAction::triggered,
            qApp, &QApplication::closeAllWindows);

    connect(ui->action_Add, &QAction::triggered,
            this, &MainWindow::action_add_triggered); //связывание действия добавления данных и вызов метода add
    connect(ui->action_Edit, &QAction::triggered,
            this, &MainWindow::action_edit_triggered); //связывание действия редактирования данных и вызов метода edit
    connect(ui->action_Delete, &QAction::triggered,
            this, &MainWindow::action_delete_triggered); //связывание действия удаления данных и вызов метода delete

    connect(ui->action_AboutProgram, &QAction::triggered,
            this, &MainWindow::action_aboutProgram_triggered);
    connect(ui->action_AboutAuthor, &QAction::triggered,
            this, &MainWindow::action_aboutAuthor_triggered);

    connect(ui->lineEdit, SIGNAL(editingFinished()),
            this, SLOT(lineEditSearch_editingFinished())); //связывание действия завершения редактирования строки lineEdit
                                                           //и вызова метода для поиска

    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(CustomMenuRequested(QPoint))); //связывание вызова контекстного меню

    //связывание сигнала изменения ячейки в таблице и слота item_edited
    connect(ui->tableWidget, &QTableWidget::cellChanged, this, &MainWindow::item_edited);

    ui->tableWidget->setSortingEnabled(true); //включаем сортировку для столбцов

    loadSettings(); //вызов функции загрузки настроек

    qmPath = qApp->applicationDirPath() + "/translations"; //устанавливаем путь к папке с переводами
    createLanguageMenu(); //создаем меню выбора языка

    set_language(active_lang); //устанавливаем язык приложения

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); //устанавливаем режим выделения только строк при выборе

    int static filenum = 0; //номер untitled файла

    currentFileName = QString("untitled%1.txt").arg(filenum++); //название файла при создании окна по умолчанию

    setWindowTitle(currentFileName); //установка названия текущего окна значением currentFileName
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*Блок действий меню File-----------------------------------------------------*/

//отработка триггера действия создания нового файла
void MainWindow::action_newFile_triggered()
{
    MainWindow *mainWindow = new MainWindow;
    mainWindow->show();
}

//отработка триггера действия открытия файла
void MainWindow::action_open_triggered()
{
    lastItemCreated = false;
    if (may_be_save()) //сохранить данные в текущем файле при открытии нового?
    {
        QString str = QFileDialog::getOpenFileName(this); //из диалогового окна открытия файла получаем путь до файла
        if (!str.isEmpty()) //если путь нормальный
        {
            openFile(str); //считываем данные из файла
        }
        else
        {
            return;
        }
        setCurrentFile(str); //устанавливаем в названии имя открытого файла

        ui->tableWidget->horizontalHeader()->show();

        createTableHeader(); //создаем заголовок таблицы

        str_count = number_of_datastrings(str); //считаем количество строк в файле
        for (int i = 0; i < str_count; i++)
            createTableItem(i); //создаем строки с данными в таблице
        lastItemCreated = true; //последняя ячейка заполнена, значит можно обрабатывать сигналы изменения ячейки
        isEdited = false; //файл считается неотредактированным

        isOpen = true; //файл считается открытым
        if (isOpen) //если файл считается открытым, то включаем пункты редактировать запись и удалить запись
        {
            ui->action_Edit->setEnabled(true);
            ui->action_Delete->setEnabled(true);
        }
    }
}

//отработка триггера действия сохранения
bool MainWindow::action_save_triggered()
{
    if (isUntitled) //если у файла нет названия
    {
        QString fileName = QFileDialog::getSaveFileName(this, //выбираем файл в который нужно сохранить(название файла)
                                                        tr("Save as"),
                                                        currentFileName);
        if (fileName.isEmpty()) //если файл не выбрали выводим предупреждение
        {
            QMessageBox::StandardButton ret;
            ret = QMessageBox::warning(this,
                                       tr("Warning"),
                                       tr("You have not selected a file"),
                                       QMessageBox::StandardButton::Ok);
            if (ret == QMessageBox::StandardButton::Ok) //если пользователь нажал сохранить вызываем триггер save
                return false;
        }

        return saveFile(fileName); //передаем в сохранение имя выбранного файла
    }
    else
    {
        return saveFile(currentFileName); //иначе сохраняем файл с текущим названием
    }
}


/*Блок действий меню Edit-----------------------------------------------------*/

//отработка триггера действия добавления данных
void MainWindow::action_add_triggered()
{
    isEdited = true;
    if(!lastItemCreated) createTableHeader();
    DialogWindow dlgwin; //создание объекта диалоговога окна
    dlgwin.setWindowTitle(tr("Add"));
    dlgwin.exec(); //запускаем диалоговое окно

    if(dlgwin.result() == DialogWindow::Rejected)
    {
        if (ui->tableWidget->rowCount() == 0)
        {
            ui->tableWidget->horizontalHeader()->hide();
        }
        isEdited = false;
        return; //если пользователь нажимает cancel
    }
    CPU new_cpu; //создаем объект добавляемого процессора

    //заполняем поля объекта данными из диалогового окна
    new_cpu.setManufacturer(dlgwin.getManufacturer());
    new_cpu.setModel(dlgwin.getModel());
    new_cpu.setCost(dlgwin.getCost());
    new_cpu.setSocket(dlgwin.getSocket());
    new_cpu.setCore_num(dlgwin.getCore_num());
    new_cpu.setProc_speed(dlgwin.getProc_speed());
    new_cpu.setMem_type(dlgwin.getMem_type());
    new_cpu.setMem_freq(dlgwin.getMem_freq());

    list.addToList(new_cpu); //добавляем новый процессор в список

    str_count += 1; //увеличиваем кол-во позиций в списке

    createTableItem(ui->tableWidget->rowCount()); //добавляем новый процессор в таблицу

    ui->action_Edit->setEnabled(true);
    ui->action_Delete->setEnabled(true);
}

//отработка триггера действия редактирования данных
void MainWindow::action_edit_triggered()
{
    isEdited = true;
    bool ok; //переменная в которую QInputDialog запишет нажатие на кнопку отмены или подтверждения
    int row = QInputDialog::getInt(this, QString (tr("Input")),
                                           QString (tr("Enter the line number in the table:")),
                                           1, 1, str_count, 1, &ok) - 1;
    //в row записывается вводимый пользователем номер строки (-1 т.к. в списке счет идет от 0)

    if(!ok)
    {
        isEdited = false;
        return; //если не было подтверждения в QInputDialog, то отменяем изменение
    }
    DialogWindow dlgwin; //создаем объект диалогового окна
    dlgwin.setWindowTitle(tr("Edit"));

    //заполняем поля диалогового окна данными выбраной строки в таблице
    dlgwin.setManufacturer(list[row]->cpu.getManufacturer());
    dlgwin.setModel(list[row]->cpu.getModel());
    dlgwin.setCost(list[row]->cpu.getCost());
    dlgwin.setSocket(list[row]->cpu.getSocket());
    dlgwin.setCore_num(list[row]->cpu.getCore_num());
    dlgwin.setProc_speed(list[row]->cpu.getProc_speed());
    dlgwin.setMem_type(list[row]->cpu.getMem_type());
    dlgwin.setMem_freq(list[row]->cpu.getMem_freq());

    dlgwin.exec(); //запускаем диалоговое окно
    if(dlgwin.result() == DialogWindow::Rejected)
    {
        isEdited = false;
        return; //если пользователь нажал отмена, то отменяем изменение
    }
    //устанавливаем новые значения
    list[row]->cpu.setManufacturer(dlgwin.getManufacturer());
    list[row]->cpu.setModel(dlgwin.getModel());
    list[row]->cpu.setCost(dlgwin.getCost());
    list[row]->cpu.setSocket(dlgwin.getSocket());
    list[row]->cpu.setCore_num(dlgwin.getCore_num());
    list[row]->cpu.setProc_speed(dlgwin.getProc_speed());
    list[row]->cpu.setMem_type(dlgwin.getMem_type());
    list[row]->cpu.setMem_freq(dlgwin.getMem_freq());

    createTableItem(row, false); //обновляем строку в таблице
}

//отработка триггера действия удаления данных
void MainWindow::action_delete_triggered()
{
    isEdited = true;
    bool ok; //переменная в которую QInputDialog запишет нажатие на кнопку отмены или подтверждения
    int row = QInputDialog::getInt(this, QString (tr("Input")),
                                           QString (tr("Enter the line number in the table:")),
                                           1, 1, str_count, 1, &ok) - 1;
    //в row записывается вводимый пользователем номер строки (-1 т.к. в списке счет идет от 0)

    if(!ok)
    {
        isEdited = false;
        return; //если не было подтверждения в QInputDialog, то отменяем изменение
    }
    list.deleteFromList(row); //удаляем запись из списка

    str_count -= 1; //уменьшаем счетчик количества записей в файле

    ui->tableWidget->removeRow(row); //удаляем строку из таблицы

    if (ui->tableWidget->rowCount() == 0) //если запись была последней в таблице, то убираем горизонтальный заголовок
        ui->tableWidget->horizontalHeader()->hide();
}


/*Блок действий меню About----------------------------------------------------*/

//отработка триггера действия выведения данных о программе
void MainWindow::action_aboutProgram_triggered()
{
    QMessageBox::about(this, tr("About program"), tr("Processor accounting program"));
}

//отработка триггера действия выведения данных об авторе программы
void MainWindow::action_aboutAuthor_triggered()
{
    QMessageBox::about(this, tr("About author"), tr("This program was developed by IEUIS-2-6 student Zabolotnov Nikolay Vladimirovich"));
}

//------------------------------------------------------------------------------

//отработка введения поискового запроса в lineEdit
void MainWindow::lineEditSearch_editingFinished()
{
    for(int i=ui->tableWidget->rowCount();i>=0;i--) //очищаем таблицу
    {
        ui->tableWidget->removeRow(i);
    }
    QString trg = ui->lineEdit->text(); //получаем поисковый запрос из lineEdit
    bool is_int; //введенный запрос int?
    int val = trg.toInt(&is_int); //записываем в отдельную переменную запрос преобразованный в int
    for (int i = 0; i < str_count; i++) //проходимся по всем значениям в списке данных
    {
        if ((!is_int && QString::fromStdString(list[i]->cpu.getManufacturer()).contains(trg)) //если запрос не число и запрос совпадает
                                                                                              //с полем хранимого в списке элемента, то отрисовываем элемент
                                                                                              //списка в таблице
                || (!is_int && QString::fromStdString(list[i]->cpu.getModel()).contains(trg))
                || (is_int && (list[i]->cpu.getCost() == val)) //если запрос число и запрос совпадает
                                                               //с полем хранимого элемента в списке элемента,
                                                               //то отрисовываем элемент списка в таблице
                || (!is_int && QString::fromStdString(list[i]->cpu.getSocket()).contains(trg))
                || (is_int && (list[i]->cpu.getCore_num() == val))
                || (is_int && (list[i]->cpu.getProc_speed() == val))
                || (!is_int && QString::fromStdString(list[i]->cpu.getMem_type()).contains(trg))
                || (is_int && (list[i]->cpu.getMem_freq() == val)))
                    createTableItem(i);
    }
}


/*Блок контекстного меню и его действий---------------------------------------*/

//создание кастомного контекстного меню
void MainWindow::CustomMenuRequested(QPoint pos)
{
    QMenu *menu = new QMenu(this); //создаем объект контекстного меню

    QAction *editRow = new QAction(tr("Edit"), this); //создание действия редактировать
    QAction *deleteRow = new QAction(tr("Delete"), this); //создание действия удалить

    connect(editRow, SIGNAL(triggered()), this, SLOT(cont_editRow()));     //обработчик вызова диалога редактирования
    connect(deleteRow, SIGNAL(triggered()), this, SLOT(cont_deleteRow())); //обработчик удаления записи

    menu->addAction(editRow); //добавляем действия в меню
    menu->addAction(deleteRow);

    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos)); //вызов контекстного меню
}

//действие редактирования вызываемое из контекстного меню
void MainWindow::cont_editRow()
{
    isEdited = true;
    int row = ui->tableWidget->selectionModel()->currentIndex().row(); //получаем номер выбранной строки в таблице

    DialogWindow dlgwin; //создаем объект диалогового окна
    dlgwin.setWindowTitle(tr("Edit"));

    //заполняем поля диалогового окна данными выбраной строки в таблице
    dlgwin.setManufacturer(list[row]->cpu.getManufacturer());
    dlgwin.setModel(list[row]->cpu.getModel());
    dlgwin.setCost(list[row]->cpu.getCost());
    dlgwin.setSocket(list[row]->cpu.getSocket());
    dlgwin.setCore_num(list[row]->cpu.getCore_num());
    dlgwin.setProc_speed(list[row]->cpu.getProc_speed());
    dlgwin.setMem_type(list[row]->cpu.getMem_type());
    dlgwin.setMem_freq(list[row]->cpu.getMem_freq());

    dlgwin.exec(); //запускаем диалоговое окно
    if(dlgwin.result() == DialogWindow::Rejected)
    {
        isEdited = false;
        return; //если пользователь нажал отмена, то отменяем изменение
    }

    //устанавливаем новые значения
    list[row]->cpu.setManufacturer(dlgwin.getManufacturer());
    list[row]->cpu.setModel(dlgwin.getModel());
    list[row]->cpu.setCost(dlgwin.getCost());
    list[row]->cpu.setSocket(dlgwin.getSocket());
    list[row]->cpu.setCore_num(dlgwin.getCore_num());
    list[row]->cpu.setProc_speed(dlgwin.getProc_speed());
    list[row]->cpu.setMem_type(dlgwin.getMem_type());
    list[row]->cpu.setMem_freq(dlgwin.getMem_freq());

    createTableItem(row, false); //обновляем строку в таблице
}

//действие удаления вызываемое из контекстного меню
void MainWindow::cont_deleteRow()
{
    int row = ui->tableWidget->selectionModel()->currentIndex().row(); //получаем номер выбранной строки в таблице

    if(row >= 0) // проверка на выбор строки
    {
        if (QMessageBox::warning(this, //если пользователь подтверждает удаление, то удаляем, если нет, то отменяем изменения
                                 tr("Confirmation"),
                                 tr("Are you sure you want to delete this entry?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        {
            return;
        }
        else
        {
            list.deleteFromList(row);

            str_count -= 1;

            ui->tableWidget->removeRow(row);

            isEdited = true; //документ считаеся отредактированым

            if (ui->tableWidget->rowCount() == 0) //если запись была последней в таблице, то убираем горизонтальный заголовок
            {
                isEdited = false;
                ui->tableWidget->horizontalHeader()->hide();
            }
        }
    }
}

//------------------------------------------------------------------------------

//если ячейка в таблице была отредактирована - сработает этот метод
void MainWindow::item_edited()
{
    if(lastItemCreated)
    {
        isEdited = true;
    }
}

//перехвает closeEvent`a для того чтобы сделать доп. действия по закрытию программы
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (may_be_save())
    {
        saveSettings(active_lang, pos(), size());
        event->accept();
    }
    else
    {
        saveSettings(active_lang, pos(), size());
        event->ignore();
    }
}

//------------------------------------------------------------------------------

//метод для сохранения настроек программы
void MainWindow::saveSettings(QString locale, QPoint pos, QSize dim)
{
    QSettings settings("settings.ini", QSettings::IniFormat); //создаем файл для настроек и объект для обращения к этому файлу

    settings.beginGroup("Settings"); //начало группы настроек "Settings"
    settings.setValue("language", locale); //сохраняем язык
    settings.endGroup(); //закрываем группу "Settings"

    settings.beginGroup("WindowGeometry"); //начало группы настроек "WindowGeometry"
    settings.setValue("pos", pos); //сохраняем позицию окна
    settings.setValue("dim", dim); //сохраняем размеры окна
    settings.endGroup(); //закрываем группу "WindowGeometry"
}

//метод для загрузки настроек программы
void MainWindow::loadSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat); //создаем файл для настроек и объект для обращения к этому файлу

    settings.beginGroup("WindowGeometry"); //начало группы настроек "WindowGeometry"
    resize(settings.value("dim", QSize(1000,1000)).toSize()); //устанавливаем размеры окна
    move(settings.value("pos", QPoint(200, 200)).toPoint()); //устанавливаем позицию окна
    settings.endGroup(); //закрываем группу "WindowGeometry"

    settings.beginGroup("Settings"); //начало группы настроек "Settings"
    active_lang = settings.value("language").toString(); //устанавливаем язык приложения
    settings.endGroup(); //закрываем группу "Settings"
}

//метод обрабатывающий файл при открытии
void MainWindow::openFile(const QString &fileName)
{
    std::string stdstr_fileName = fileName.toStdString(); //записываем QString строку в STD строку
    std::fstream in_file(stdstr_fileName, std::ios_base::in); //открытие файлового потока
    if (!in_file.is_open())
    {
        QMessageBox::warning(this,
                             tr("Read error"),
                             tr("Unable to read file %1")
                             .arg(fileName));
    }
    std::string in_str; //цельная строка получаемая из файла
    while (!in_file.eof())
    {
        std::getline(in_file, in_str); //получение строки вида "...;...;...;...;"
        if (in_str.empty()) continue;

        std::stringstream file_str(in_str); //создание потока строк
        std::string record; //строка для хранения конкретной записи (;...;)
        const int num_of_rec = 8; //количество записей в цельной строке из файла
        std::string temp[num_of_rec]; //массив строк для хранения записей полученной парсингом
        int j = 0; //счетчик индекса для массива строк
        while (getline(file_str, record, ';')) //парсинг потока строк,
            //запись в строку record с разделителем ;
        {
            temp[j] = record;
            j++;
        }
        CPU cpu; //добавляемый объект
        //заполнение полей объекта данными
        cpu.setManufacturer(temp[0]);
        cpu.setModel(temp[1]);
        cpu.setCost(stoi(temp[2]));
        cpu.setSocket(temp[3]);
        cpu.setCore_num(stoi(temp[4]));
        cpu.setProc_speed(stoi(temp[5]));
        cpu.setMem_type(temp[6]);
        cpu.setMem_freq(stoi(temp[7]));

        list.addToList(cpu); //добавление позиции в список
    }
    in_file.close();
    isOpen = true; //файл считается открытым
}

//метод обрабатывающий файл при сохранении
bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,
                             tr("Error"),
                             tr("Can't write to file %1:\n%2")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    std::string stdstr_fileName = fileName.toStdString(); //записываем QString строку в STD строку
    std::fstream out_file(stdstr_fileName, std::ios_base::out); //открываем файл на вывод

    customList::Item *temp = list.head;
    while (temp != nullptr) //опускаемся пока не встретим конец
    {
        //запись данных в файл
        out_file << temp->cpu.getManufacturer() << ';'
                 << temp->cpu.getModel() << ';'
                 << temp->cpu.getCost() << ';'
                 << temp->cpu.getSocket() << ';'
                 << temp->cpu.getCore_num() << ';'
                 << temp->cpu.getProc_speed() << ';'
                 << temp->cpu.getMem_type() << ';'
                 << temp->cpu.getMem_freq() << ';' << '\n';
        temp = temp->next; //записываем адрес следующего элемента
    }
    isEdited = false;
    out_file.close();
    return true;
}

//"захочет ли пользователь сохранить данные в текущем файле при ...?"
bool MainWindow::may_be_save()
{
    if (isEdited) //если какая-либо позиция таблицы отредактирована
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this,
                                   tr("File changed"),
                                   tr("The file has been edited\n"
                                      "Do you want to commit your changes?"),
                                   QMessageBox::Save |
                                   QMessageBox::Discard |
                                   QMessageBox::Cancel);
        if (ret == QMessageBox::Save) //если пользователь нажал сохранить вызываем триггер save
            return action_save_triggered();
        else if (ret == QMessageBox::Cancel) //если пользователь нажал отмена - отменяем открытие нового файла
            return false;
    }
    return true;
}

//создает оглавление таблицы
void MainWindow::createTableHeader()
{
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->tableWidget->setColumnCount(8);

    QStringList HorizontalHeader;

    HorizontalHeader.append(tr("Manufacturer"));
    HorizontalHeader.append(tr("Model"));
    HorizontalHeader.append(tr("Cost"));
    HorizontalHeader.append(tr("Socket"));
    HorizontalHeader.append(tr("Number of cores"));
    HorizontalHeader.append(tr("Clock frequency"));
    HorizontalHeader.append(tr("Memory type"));
    HorizontalHeader.append(tr("Memory clock speed"));

    ui->tableWidget->setHorizontalHeaderLabels(HorizontalHeader);
}

//создает строку заполненную данными в таблице
void MainWindow::createTableItem(int i, bool new_item)
{
    const int num_of_param = 8; //количество столбцов

    int pos; //позиция добавляемого/редактируемого элемента
    if(new_item) //если элемент добавляют, то он добавляется в конец таблицы
    {
        pos = ui->tableWidget->rowCount();
        ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);}
    else //иначе позиция = номер отредактированной строки
    {
        pos = i;
    }

    QTableWidgetItem *Item[num_of_param]; //массив ячеек для строки таблицы

    //ячейки заполняются из списка
    Item[0] = new QTableWidgetItem(QString::fromStdString(list[i]->cpu.getManufacturer()));
    Item[1] = new QTableWidgetItem(QString::fromStdString(list[i]->cpu.getModel()));
    Item[2] = new MyTableWidgetItem(QString::number(list[i]->cpu.getCost()));
    Item[3] = new QTableWidgetItem(QString::fromStdString(list[i]->cpu.getSocket()));
    Item[4] = new MyTableWidgetItem(QString::number(list[i]->cpu.getCore_num()));
    Item[5] = new MyTableWidgetItem(QString::number(list[i]->cpu.getProc_speed()));
    Item[6] = new QTableWidgetItem(QString::fromStdString(list[i]->cpu.getMem_type()));
    Item[7] = new MyTableWidgetItem(QString::number(list[i]->cpu.getMem_freq()));

    //таблица заполняется данными
    for (int j = 0; j < num_of_param; j++)
    {
        ui->tableWidget->setItem(pos, j, Item[j]);
    }
}

//устанавливает в название окна название текущего открытого файла в этом окне
void MainWindow::setCurrentFile(const QString &filename)
{
    currentFileName = filename;
    ui->tableWidget->setWindowModified(false);
    setWindowModified(false);
    isUntitled = false; //окно больше не будет считаться неназванным

    QString showName; //отображаемое название файла

    if (currentFileName.isEmpty()) //если у файла нет названия, назваем его untitled
    {
        showName = "untitled.txt";
        currentFileName = showName;
    }
    else //если у файла есть название+путь
        showName = croppedFileName(currentFileName); //вызываем функцию чтобы вывести только название файла - без пути

    setWindowTitle(tr("%1[*]")
                   .arg(showName));
}

//вырезает из названия файла путь к нему
QString MainWindow::croppedFileName(const QString &FullFilename)
{
    return QFileInfo(FullFilename).fileName();
}

//считывает количество строк в файле с данными
int MainWindow::number_of_datastrings(const QString& filename)
{
    std::string stdstr_fileName = filename.toStdString(); //записываем QString строку в STD строку
    std::fstream in_file(stdstr_fileName, std::ios_base::in); //открытие файлового потока
    int ctn = 0; //количество строк в файле
    std::string text; //считываемая строка
    while (!in_file.eof()) //цикл для счета количества строк в файле
    {
        getline(in_file, text); //считывается строка
        if(text.size()) //проверяется размер строки
            ++ctn; //если строка непустая, то количество строк в файле увеличивается
    }
    return ctn; //возвращаем кол-во строк в файле
}

//метод для уставновки конктреного языка в интерфейсе приложения
void MainWindow::set_language(QString locale)
{
    if (locale.isEmpty()) return; //если нет "локали", то return
        if (!qtLngTranslator.load("coursework_" + locale, qmPath)) //если не получается получить доступ к файлу с переводами
        {
            return;
        }
        active_lang = locale; //активный язык окна = локали
        qApp->installTranslator(&qtLngTranslator); //устанавливаем перевод
        ui->retranslateUi(this); //отрисовываем интерфейс в соответствии с переводом
        createTableHeader(); //создаем заголовок таблицы
        ui->tableWidget->horizontalHeader()->hide();
        ui->tableWidget->horizontalHeader()->show();
}

//метод для смены языка приложения
void MainWindow::switchLanguage()
{
    set_language(languageActionGroup->checkedAction()->data().toString()); //смена языка
}

//метод для создания меню языков
void MainWindow::createLanguageMenu()
{
    languageActionGroup = new QActionGroup(this); //создаем группу действий
    connect(languageActionGroup, &QActionGroup::triggered,
                        this, &MainWindow::switchLanguage); //связываем триггер действия в меню с методом смены языка

    QDir dir(qmPath); //создаем объект папки с переводами

    QStringList fileNames = dir.entryList(QStringList("coursework_*.qm")); //создаем список файлов с переводами

    for (int i = 0; i < fileNames.size(); i++)
    {
        QString locale = fileNames[i]; //устанавливаем локаль
        locale.remove(0, locale.indexOf('_') + 1); //вырезаем из локали coursework_
        locale.truncate(locale.lastIndexOf('.')); //выразем .qm

        QTranslator translator;
        translator.load(fileNames[i], qmPath); //загружаем перевод

        QString language = translator.translate("MainWindow", "English");

        QAction *action = new QAction(tr("%1")
                                     .arg(language),
                                     this);

        action->setCheckable(true);
        action->setData(locale);

        ui->menu_Settings->addAction(action);

        languageActionGroup->addAction(action);

        if (locale == active_lang)
        {
            action->setChecked(true);
        }
    }

}
