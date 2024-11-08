/*
 * Register Panel GUI class.
 *
 * Copyright (c) 2024 Man Hung-Coeng <udc577@126.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "regpanel.hpp"

#include "versions.h"

#include <QtCore/QDir>
#include <QtGui/QtGui>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>

#include "qt_print.hpp"
#include "private_widgets.hpp"

#if 0
#define ABORT(errcode)                          QApplication::exit(errcode)
#else
#define ABORT(errcode)                          exit(errcode)
#endif

const QTextCodec *G_TEXT_CODEC = QTextCodec::codecForName("UTF8"/*"GB2312"*/);

RegPanel::RegPanel(const char *config_dir, QWidget *parent)
    : QDialog(parent)
    , m_config_dir(config_dir)
    , m_prev_vendor_idx(-1)
    , m_prev_chip_idx(-1)
    , m_prev_file_idx(-1)
    , m_prev_module_idx(-1)
{
    setupUi(this);

    this->setFixedSize(this->geometry().size());
    this->setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    this->setWindowTitle(QString::asprintf("%s [%s]", this->windowTitle().toStdString().c_str(), FULL_VERSION()));
    this->lblPoweredBy->setText(this->lblPoweredBy->text() + " " + QT_VERSION_STR);
    this->scrlViewContents->setLayout(this->vlayoutRegTables);

    scan_config_directory(config_dir);

    emit this->lstVendor->currentIndexChanged(-1);
    emit this->lstAddrBaseMethod->currentIndexChanged(this->lstAddrBaseMethod->currentIndex());
    emit this->chkboxAsInput->stateChanged(this->chkboxAsInput->isChecked());
}

RegPanel::~RegPanel()
{
    //this->clear_register_tables(); // FIXME: Seems unnecessary.
    //delete this->scrollArea; // FIXME: Same with this and other widgets made by *.ui file.
    // NOTE: Certainly there're some memory leaks, but it's too few to worry.
}

#define SHOW_MSG_BOX(_type, _title, _text)                      do { \
    bool visible = this->isVisible(); \
    QMessageBox::_type(visible ? this : this->parentWidget(), visible ? _title : this->windowTitle(), _text); \
} while (0)

void RegPanel::info_box(const QString &title, const QString &text)
{
    SHOW_MSG_BOX(information, title, text);
}

void RegPanel::warning_box(const QString &title, const QString &text)
{
    SHOW_MSG_BOX(warning, title, text);
}

void RegPanel::error_box(const QString &title, const QString &text)
{
    SHOW_MSG_BOX(critical, title, text);
}

void RegPanel::closeEvent(QCloseEvent *event)/* override */
{
    QMessageBox::StandardButton button = QMessageBox::question(
        this, "", "Exit now ?", QMessageBox::Yes | QMessageBox::No);

    if (QMessageBox::Yes == button)
        event->accept();
    else
        event->ignore();
}

void RegPanel::on_tab_currentChanged(int index)
{
    if (1 != this->tab->currentIndex()) // Not the tab page showing conversion result.
        return;

    const int vendor_idx = this->lstVendor->currentIndex();
    const int chip_idx = this->lstChip->currentIndex();
    const int file_idx = this->lstFile->currentIndex();
    const int module_idx = this->lstModule->currentIndex();

    if (vendor_idx == this->m_prev_vendor_idx
        && chip_idx == this->m_prev_chip_idx
        && file_idx == this->m_prev_file_idx
        && module_idx == this->m_prev_module_idx)
    {
        return;
    }

    if (module_idx < 0)
    {
        this->error_box("No Module", "No further operations can be performed without a valid module!");
        this->tab->disconnect(this->tab, SIGNAL(currentChanged(int)), this, SLOT(on_tab_currentChanged(int)));
        this->tab->setCurrentIndex(0); // Switch back to the "Load" page.
        this->tab->connect(this->tab, SIGNAL(currentChanged(int)), this, SLOT(on_tab_currentChanged(int)));

        return;
    }

    this->m_prev_vendor_idx = vendor_idx;
    this->m_prev_chip_idx = chip_idx;
    this->m_prev_file_idx = file_idx;
    this->m_prev_module_idx = module_idx;

    const QString &module_name = this->lstModule->currentText();
    const QJsonObject &modules_dict = this->json().object().value(module_name).toObject();

    this->m_reg_addr_map.clear();
    for (const QString &k : modules_dict.keys())
    {
        const std::string &key = k.toStdString();

        if (!k.startsWith("__"))
            this->m_reg_addr_map[strtoull(key.c_str(), nullptr, 16)] = key;
    }

    if (this->chkboxAsInput->isChecked())
        return;

    this->clear_register_tables();

    int count = this->make_register_tables(this->json(), module_name);

    if (count > 0)
    {
        this->info_box("Load",
            QString::asprintf("Loaded %d register tables for module:\n\n%s", count, module_name.toStdString().c_str()));
    }
    else
    {
        this->error_box("Load",
            QString::asprintf("Failed to load register tables for module:\n\n%s", module_name.toStdString().c_str()));
    }

    this->grpboxView->setTitle(QString::asprintf("View: %d item(s) below", (count >= 0) ? count : 0));
}

void RegPanel::on_lstVendor_currentIndexChanged(int index)
{
    const auto &vendors = this->vendors();

    if (index < 0)
    {
        this->disconnect(this->lstVendor, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstVendor_currentIndexChanged(int)));

        this->lstVendor->clear();
        for (const auto &vendor : vendors)
        {
            this->lstVendor->addItem(QString::fromStdString(vendor.first));
        }

        this->connect(this->lstVendor, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstVendor_currentIndexChanged(int)));
    }

    emit this->lstChip->currentIndexChanged(-1);
}

void RegPanel::on_lstChip_currentIndexChanged(int index)
{
    if (index < 0)
    {
        int vendor_idx = this->lstVendor->currentIndex();

        this->disconnect(this->lstChip, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstChip_currentIndexChanged(int)));

        this->lstChip->clear();
        if (vendor_idx >= 0)
        {
            for (const auto &chip : this->vendors()[vendor_idx].second)
            {
                this->lstChip->addItem(QString::fromStdString(chip.first));
            }
        }

        this->connect(this->lstChip, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstChip_currentIndexChanged(int)));
    }

    emit this->lstFile->currentIndexChanged(-1);
}

void RegPanel::on_lstFile_currentIndexChanged(int index)
{
    if (index < 0)
    {
        int vendor_idx = this->lstVendor->currentIndex();
        int chip_idx = this->lstChip->currentIndex();

        this->disconnect(this->lstFile, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstFile_currentIndexChanged(int)));

        this->lstFile->clear();
        if (vendor_idx >= 0 && chip_idx >= 0)
        {
            for (const auto &file : this->vendors()[vendor_idx].second[chip_idx].second)
            {
                this->lstFile->addItem(QString::fromStdString(file));
            }
        }

        this->connect(this->lstFile, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstFile_currentIndexChanged(int)));
    }

    emit this->lstModule->currentIndexChanged(-1);
}

void RegPanel::on_lstModule_currentIndexChanged(int index)
{
    if (index < 0)
    {
        int vendor_idx = this->lstVendor->currentIndex();
        int chip_idx = this->lstChip->currentIndex();
        int file_idx = this->lstFile->currentIndex();

        this->disconnect(this->lstModule, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstModule_currentIndexChanged(int)));

        this->lstModule->clear();
        if (vendor_idx >= 0 && chip_idx >= 0 && file_idx >= 0)
        {
            QString path = QString::fromStdString(this->config_dir()) + QDir::separator()
                + this->lstVendor->currentText() + QDir::separator()
                + this->lstChip->currentText() + QDir::separator()
                + this->lstFile->currentText();

            if (this->load_config_file(path.toStdString().c_str()))
            {
                const QJsonArray &modules = this->json().object().value("__modules__").toArray();

                for (const auto &m : modules)
                {
                    this->lstModule->addItem(m.toString());
                }
            }

            if (this->lstModule->count() > 0)
                index = 0;
        }

        this->connect(this->lstModule, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_lstModule_currentIndexChanged(int)));
    }

    if (index < 0)
        this->error_box("Target Missing", "No readable configuration files or valid modules.");
}

void RegPanel::on_lstDelimeter_currentIndexChanged(int index)
{
    emit this->chkboxAsInput->stateChanged(this->chkboxAsInput->isChecked());
}

void RegPanel::on_lstAddrBaseMethod_currentIndexChanged(int index)
{
    bool ignored = (0 == this->lstAddrBaseMethod->itemText(index).compare("Ignore", Qt::CaseInsensitive));

    this->spnboxAddrBase->setDisabled(ignored);
    this->spnboxAddrBase->setReadOnly(ignored);
    this->spnboxAddrBase->setStyleSheet(ignored ? "background-color: darkgray; color: white;"
        : "background-color: " SOFT_GREEN_COLOR "; color: black;");
}

enum
{
    CURLY_BRACES,
    SQUARE_BRACKETS
};

void RegPanel::on_chkboxAsInput_stateChanged(int checked)
{
    QPalette palette = this->txtInput->palette();
    int delim_index = this->lstDelimeter->currentIndex();
    char left_delim = (CURLY_BRACES == delim_index) ? '{' : '[';
    char right_delim = (CURLY_BRACES == delim_index) ? '}' : ']';
    QString placeholder_text = checked ? QString::asprintf("Input Address-Value pairs here. For example:\n"
            "%c 0x0040, 0x0101 %c,\n%c 0x0080, 0xabab %c", left_delim, right_delim, left_delim, right_delim)
        : QString("");

    this->txtInput->setPlaceholderText(placeholder_text);
    this->txtInput->setReadOnly(!checked);
    this->txtInput->setStyleSheet(checked ? "background-color: " SOFT_GREEN_COLOR "; color: black;"
        : "background-color: darkgray; color: white;");
    palette.setColor(QPalette::PlaceholderText, QColor("darkgray"));
    this->txtInput->setPalette(palette);
}

void RegPanel::on_btnConvert_clicked(void)
{
    if (this->lstModule->currentIndex() < 0)
    {
        this->error_box("No Target", QString::asprintf("No available module, please check your configuration again."));

        return;
    }

    const QString &module_name = this->lstModule->currentText();
    int count;

    if (this->chkboxAsInput->isChecked())
    {
        this->clear_register_tables();

        if ((count = this->make_register_tables(*this->txtInput, module_name)) > 0)
            this->info_box("Convert", QString::asprintf("Converted %d register tables from text box.", count));
        else
            this->error_box("Convert", "Failed to convert register tables from text box!");

        this->grpboxView->setTitle(QString::asprintf("View: %d item(s) below", (count >= 0) ? count : 0));
    }
    else
    {
        if ((count = this->generate_register_array_items(module_name, *this->txtInput)) > 0)
            this->info_box("Generate", QString::asprintf("Generated %d register array items to text box.", count));
        else
            this->error_box("Generate", "Failed to generate register array items to text box!");
    }
}

void RegPanel::scan_config_directory(const char *config_dir)
{
    QDir dir(config_dir);

    if (!dir.exists())
    {
        this->error_box("Directory Error", QString::asprintf("Non-existent or unreadable directory:\n\n%s", config_dir));

        ABORT(EXIT_FAILURE);
    }

    char dir_delim = QDir::separator().toLatin1();
    auto dir_filters = QDir::Filter::Dirs | QDir::Filter::Readable | QDir::Filter::NoDotAndDotDot;
    auto file_filters = QDir::Filter::Files | QDir::Filter::Readable | QDir::Filter::NoDotAndDotDot;
    auto sort_flags = QDir::SortFlag::Name;
    const auto &vendors = dir.entryList(dir_filters, sort_flags);

    if (vendors.empty())
    {
        this->error_box("Directory Error", QString::asprintf("No readable vendor directories within:\n\n%s", config_dir));

        ABORT(EXIT_FAILURE);
    }

    this->m_vendors.reserve(vendors.count());
    for (const auto &vendor_name : vendors)
    {
        auto v = this->m_vendors.insert(this->m_vendors.end(), { vendor_name.toStdString(), std::vector<ChipItem>() });
        QDir vdir(QString::asprintf("%s%c%s", dir.path().toStdString().c_str(), dir_delim, v->first.c_str()));
        const auto &chips = vdir.entryList(dir_filters, sort_flags);

        if (chips.empty())
            continue;

        v->second.reserve(chips.count());
        for (const auto &chip_name : chips)
        {
            auto c = v->second.insert(v->second.end(), { chip_name.toStdString(), std::vector<std::string>() });
            QDir cdir(QString::asprintf("%s%c%s", vdir.path().toStdString().c_str(), dir_delim, c->first.c_str()));
            const auto &files = cdir.entryList(file_filters, sort_flags);

            if (files.empty())
                continue;

            c->second.reserve(files.count());
            for (const auto &file_name : files)
            {
                c->second.push_back(file_name.toStdString());
            }
        }
    }
}

bool RegPanel::load_config_file(const char *path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
    {
        this->error_box("File Error",
            QString::asprintf("Failed to read file:\n\n%s\n\nReason:\n\n", path) + file.errorString());

        return false;
    }

    QJsonParseError err;
    QJsonDocument &doc = this->json();

    doc = QJsonDocument::fromJson(file.readAll(), &err);

    if (QJsonParseError::NoError != err.error)
    {
        this->error_box("JSON Error", err.errorString());

        return false;
    }

    const QJsonObject &obj = doc.object();
    const QJsonValue &val = obj.value("__modules__");

    if (val.isNull() || val.isUndefined())
    {
        this->error_box("Invalid Format", QString::asprintf("There's no __modules__ array, err: %d", val.type()));

        return false;
    }

    if (!val.isArray())
    {
        this->error_box("Invalid Format", "__modules__ is NOT an array!");

        return false;
    }

    const QJsonArray &arr = val.toArray();

    if (arr.empty())
    {
        this->error_box("Invalid Format", "Empty __modules__ array!");

        return false;
    }

    for (const auto &m : arr)
    {
        if (!m.isString())
        {
            this->error_box("Invalid Format", "__modules__ is NOT a pure string-array!");

            return false;
        }

        const QString &module_name = m.toString();

        if (!obj.contains(module_name))
        {
            this->error_box("Invalid Format", QString("Cannot find module: ") + module_name);

            return false;
        }

        if (!obj.value(module_name).isObject())
        {
            this->error_box("Invalid Format", QString("Module[") + module_name + "] is NOT a dictionary!");

            return false;
        }
    }

    return true;
}

static QLineEdit* make_register_title(QWidget *parent, const QString &table_prefix, const QString &title_text)
{
    auto *reg_title = new QLineEdit(title_text, parent);
    QFont title_font;

    reg_title->setObjectName(table_prefix + "_title");
    //reg_title->setStyleSheet("background: transparent; border: none; color: #ffa348;");
    reg_title->setStyleSheet("background: transparent; border: none; color: orange;");
    title_font.setBold(true);
    title_font.setPointSize(22);
    reg_title->setFont(title_font);
    reg_title->setReadOnly(true);

    return reg_title;
}

QTableWidget* RegPanel::make_register_table(QWidget *parent, const QString &name_prefix,
    const QString &dict_key, const QJsonArray &dict_value,
    uint64_t default_value, uint64_t current_value)
{
    auto *outer_table = new QTableWidget(4, 1, parent);
    auto *title_row = make_register_title(outer_table, name_prefix, dict_key);
    auto *full_values_row = new RegFullValuesRow(outer_table, name_prefix, default_value, current_value);
    auto *bits_table = new RegBitsTable(outer_table, name_prefix,
        dict_key.toStdString().c_str(), dict_value, default_value, current_value);

    outer_table->setRowHeight(0, title_row->height());
    outer_table->setCellWidget(0, 0, title_row);
    //outer_table->item(0, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    outer_table->setRowHeight(1, resize_table_height(full_values_row, /* header_row_visible = */false));
    outer_table->setCellWidget(1, 0, full_values_row);

#if 0 // doesn't work
    resize_table_height(bits_table, /* header_row_visible = */true);
    outer_table->resizeRowToContents(2);
#else
    outer_table->setRowHeight(2, resize_table_height(bits_table, /* header_row_visible = */true));
#endif
    outer_table->setCellWidget(2, 0, bits_table);

    outer_table->setObjectName(name_prefix + "_holder");
    outer_table->setShowGrid(false);
    outer_table->verticalHeader()->setVisible(false);
    outer_table->horizontalHeader()->setVisible(false);
    outer_table->horizontalHeader()->setStretchLastSection(true); // Auto-stretch for the final columns
    resize_table_height(outer_table, /* header_row_visible = */false);

    return outer_table;
}

static QString find_referenced_register_if_any(const QJsonObject &modules_dict,
    const QString &orig_key, const QJsonArray &orig_value)
{
    for (const QJsonValue &item : orig_value)
    {
        if (!item.isObject())
            continue;

        const QJsonObject &dict = item.toObject();

        if (!dict.contains("ref"))
            continue;

        const QJsonValue &ref_val = dict.value("ref");

        if (!ref_val.isString())
            continue;

        QString ref_key(ref_val.toString());

        return modules_dict.contains(ref_key) ? ref_key : QString("");
    }

    return QString("");
}

static uint64_t get_default_value(const QJsonObject &modules_dict, const QString &key)
{
    if (!modules_dict.contains("__defaults__"))
        return 0;

    const QJsonValue &def = modules_dict.value("__defaults__");

    if (!def.isObject())
        return 0;

    const QJsonObject &def_dict = def.toObject();

    if (!def_dict.contains(key))
        return 0;

    const QJsonValue &def_val = def_dict.value(key);

    return def_val.isString() ? strtoull(def_val.toString().toStdString().c_str(), nullptr, 16) : 0;
}

int RegPanel::make_register_tables(const QJsonDocument &json, const QString &module_name)
{
    const QJsonObject &modules_dict = json.object().value(module_name).toObject();
    QVBoxLayout *vlayout = this->vlayoutRegTables;
    QWidget *scroll_widget = vlayout->parentWidget();
    int table_count = 0;
    int i = 0;

    for (QJsonObject::const_iterator iter = modules_dict.begin(); modules_dict.end() != iter; ++iter)
    {
        const QString &orig_key = iter.key();

        ++i;

        if (orig_key.startsWith("__"))
            continue;

        const QJsonValue &orig_value = iter.value();

        if (!orig_value.isArray())
        {
            qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", i, orig_key.toStdString().c_str());
            continue;
        }

        const QJsonArray &orig_val_arr = orig_value.toArray();
        QString dest_key = find_referenced_register_if_any(modules_dict, orig_key, orig_val_arr);
        QString name_prefix = QString::asprintf("reg[%d]", i);
        uint64_t default_value = get_default_value(modules_dict, orig_key);
        QTableWidget *reg_table;

        if (dest_key.isEmpty())
        {
            reg_table = this->make_register_table(scroll_widget, name_prefix, orig_key, orig_val_arr,
                default_value, default_value);
        }
        else
        {
            qtCDebugV(::, "Redirecting reg[%s] to reg[%s] ...\n",
                orig_key.toStdString().c_str(), dest_key.toStdString().c_str());

            const QJsonValue &dest_value = modules_dict.value(dest_key);

            if (!dest_value.isArray())
            {
                qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", i, dest_key.toStdString().c_str());
                continue;
            }

            reg_table = this->make_register_table(scroll_widget, name_prefix, orig_key, dest_value.toArray(),
                default_value, default_value);
        }

        vlayout->addWidget(reg_table, /* stretch = */0, Qt::AlignTop);

        if ((++table_count) < 2)
        {
            auto msg_handler = qInstallMessageHandler(nullptr); // Restore to the default one for auto-newline.

            reg_table->dumpObjectTree();
            qInstallMessageHandler(msg_handler); // Restore to the customized one.
        }
    }

    return table_count;
}

#define IS_HEX_CHAR(c)          (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

int RegPanel::make_register_tables(const QTextEdit &textbox, const QString &module_name)
{
    QVBoxLayout *vlayout = this->vlayoutRegTables;
    QWidget *scroll_widget = vlayout->parentWidget();
    int delim_index = this->lstDelimeter->currentIndex();
    const char left_delim = (CURLY_BRACES == delim_index) ? '{' : '[';
    const char right_delim = (CURLY_BRACES == delim_index) ? '}' : ']';
    const QJsonObject &doc_dict = this->json().object();
    const QJsonObject &modules_dict = doc_dict.value(module_name).toObject();
    const QString &offset_method = this->lstAddrBaseMethod->currentText();
    char offset_op = (0 == offset_method.compare("Ignore", Qt::CaseInsensitive)) ? '\0'
        : ((0 == offset_method.compare("Add", Qt::CaseInsensitive)) ? '+' : '-');
    uint64_t addr_offset = ('\0' == offset_op) ? 0 : this->spnboxAddrBase->value();
    uint64_t addr = 0;
    uint64_t value = 0;
    const std::string &input = textbox.toPlainText().toStdString();
    const char *ptr = input.c_str();
    int table_seq = 1;

#define IS_DELIM_OR_NULL(c)     ((left_delim == (c)) || (right_delim == (c)) || ('\0' == (c)))

    while (true)
    {
        if (left_delim != *ptr)
            while (left_delim != *++ptr && '\0' != *ptr);

        if ('\0' == *ptr) // Empty box, or the final array item.
            break;

        if (left_delim != *ptr)
        {
            this->error_box("Invalid Format", QString::asprintf("No %c for item[%d]!\n\nConversion aborted!",
                left_delim, table_seq));
            break;
        }

        char c;

        do
        {
            c = *++ptr;

            if (IS_HEX_CHAR(c) || IS_DELIM_OR_NULL(c))
                break;
        }
        while (true);

        if (IS_DELIM_OR_NULL(c))
        {
            this->error_box("Invalid Format", QString::asprintf("No address for item[%d]!\n\nConversion aborted!",
                table_seq));
            break;
        }

        char *end_ptr;

        addr = strtoull(ptr, &end_ptr, 16);
        if ('+' == offset_op)
            addr += addr_offset;
        else
            addr -= addr_offset;

        if (/*'\0' != *ptr && */'\0' == *end_ptr) // Entire string has been parsed.
        {
            this->error_box("Invalid Format", QString::asprintf("No value for item[%d]!\n\nConversion aborted!",
                table_seq));
            break;
        }

        ptr = end_ptr;
        do
        {
            c = *++ptr;

            if (IS_HEX_CHAR(c) || IS_DELIM_OR_NULL(c))
                break;
        }
        while (true);

        if (IS_DELIM_OR_NULL(c))
        {
            this->error_box("Invalid Format", QString::asprintf("No value for item[%d]!\n\nConversion aborted!",
                table_seq));
            break;
        }

        value = strtoull(ptr, &end_ptr, 16);

        ptr = end_ptr;
        do
        {
            c = *ptr++;

            if ((right_delim == c) || (left_delim == c) || ('\0' == c))
                break;
        }
        while (true);

        if (right_delim != c)
        {
            this->error_box("Invalid Format", QString::asprintf("No %c for item[%d]!\n\nConversion aborted!",
                right_delim, table_seq));
            break;
        }

        auto orig_key_iter = this->m_reg_addr_map.find(addr);

        if (this->m_reg_addr_map.end() == orig_key_iter)
        {
            qtCErrV(::, "[%d] No such a register with address = 0x%lx\n", table_seq, addr);
            continue;
        }

        QString orig_key = QString::fromStdString(orig_key_iter->second);
        const QJsonValue &orig_value = modules_dict.value(orig_key);

        if (!orig_value.isArray())
        {
            qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", table_seq, orig_key.toStdString().c_str());
            continue;
        }

        const QJsonArray &orig_val_arr = orig_value.toArray();
        QString dest_key = find_referenced_register_if_any(modules_dict, orig_key, orig_val_arr);
        QString name_prefix = QString::asprintf("reg[%d]", table_seq);
        uint64_t default_value = get_default_value(modules_dict, orig_key);
        QTableWidget *reg_table;

        if (dest_key.isEmpty())
        {
            reg_table = this->make_register_table(scroll_widget, name_prefix, orig_key, orig_val_arr,
                default_value, value);
        }
        else
        {
            const QJsonValue &dest_value = modules_dict.value(dest_key);

            if (!dest_value.isArray())
            {
                qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", table_seq, dest_key.toStdString().c_str());
                continue;
            }

            reg_table = this->make_register_table(scroll_widget, name_prefix, orig_key, dest_value.toArray(),
                default_value, value);
        }

        vlayout->addWidget(reg_table, /* stretch = */0, Qt::AlignTop);

        ++table_seq;
    } // while (true)

    if (table_seq <= 1)
    {
        if (input.size() > 0)
            this->error_box("Conversion Error", "Select the correct delimiter type, "
                "and write address-value pairs according to the placeholder text.");
        else
            this->error_box("Empty Contents", "Are you kidding?!");
    }

    return table_seq - 1;
}

void RegPanel::clear_register_tables(void)
{
    QVBoxLayout *vlayout = this->vlayoutRegTables;
    QWidget *scroll_widget = vlayout->parentWidget();
    bool print_flag = true;
    auto is_reg_widget = [](const std::string &widget_name) {
        return (0 == widget_name.compare(0, 4, "reg["));
    };

    for (auto &i : scroll_widget->children())
    {
        const std::string &iname = i->objectName().toStdString();

        if (!is_reg_widget(iname))
            continue;

        auto *outer_table = dynamic_cast<QTableWidget *>(i);
        auto *title_cell = dynamic_cast<QLineEdit *>(outer_table->cellWidget(0, 0));
        auto *full_values_cell = dynamic_cast<RegFullValuesRow *>(outer_table->cellWidget(1, 0));
        auto *bits_table_cell = dynamic_cast<RegBitsTable *>(outer_table->cellWidget(2, 0));

        qtCDebugV(::, "\tDeleting: %s (%s)\n", title_cell->objectName().toStdString().c_str(),
            title_cell->text().toStdString().c_str());
        delete title_cell;

        if (print_flag) qtCDebugV(::, "\tDeleting: %s\n", full_values_cell->objectName().toStdString().c_str());
        delete full_values_cell;

        if (print_flag) qtCDebugV(::, "\tDeleting: %s\n", bits_table_cell->objectName().toStdString().c_str());
        delete bits_table_cell;

        qtCDebugV(::, "Deleting: %s\n", iname.c_str());
        vlayout->removeWidget(outer_table);
        //outer_table->clear();
        //outer_table->setRowCount(0);
        delete outer_table;

        print_flag = false;
    } // for (auto &i : scroll_widget->children())
}

#define DEFAULT_BITWIDTH        32

static inline int get_bitwidth(const QJsonObject &doc_dict, const QString &key)
{
    if (!doc_dict.contains(key))
        return DEFAULT_BITWIDTH;

    const QJsonValue &width_val = doc_dict.value(key);
    int result = width_val.isDouble() ? width_val.toDouble()
        : (width_val.isString() ? atoi(width_val.toString().toStdString().c_str()) : DEFAULT_BITWIDTH);

    return (8 == result || 16 == result || 32 == result || 64 == result) ? result : DEFAULT_BITWIDTH;
}

static inline const char* bitwidth_format_string(int bitwidth)
{
    if (8 == bitwidth)
        return "0x%02lx";

    if (16 == bitwidth)
        return "0x%04lx";

    if (64 == bitwidth)
        return "0x%016lx";

    return "0x%08lx";
}

int RegPanel::generate_register_array_items(const QString &module_name, const QTextEdit &textbox)
{
    QVBoxLayout *vlayout = this->vlayoutRegTables;
    QWidget *scroll_widget = vlayout->parentWidget();
    int delim_index = this->lstDelimeter->currentIndex();
    const char left_delim = (CURLY_BRACES == delim_index) ? '{' : '[';
    const char right_delim = (CURLY_BRACES == delim_index) ? '}' : ']';
    const QJsonObject &doc_dict = this->json().object();
    const char *addr_width_fmt = bitwidth_format_string(get_bitwidth(doc_dict, "__addr_bits__"));
    const char *value_width_fmt = bitwidth_format_string(get_bitwidth(doc_dict, "__data_bits__"));
    const QString &offset_method = this->lstAddrBaseMethod->currentText();
    char offset_op = (0 == offset_method.compare("Ignore", Qt::CaseInsensitive)) ? '\0'
        : ((0 == offset_method.compare("Add", Qt::CaseInsensitive)) ? '+' : '-');
    uint64_t addr_offset = ('\0' == offset_op) ? 0 : this->spnboxAddrBase->value();
    uint64_t addr = 0;
    uint64_t value = 0;
    QString result;
    int count = 0;
    auto is_reg_widget = [](const std::string &widget_name) {
        return (0 == widget_name.compare(0, 4, "reg["));
    };

    for (auto &i : scroll_widget->children())
    {
        if (!is_reg_widget(i->objectName().toStdString()))
            continue;

        auto *outer_table = dynamic_cast<QTableWidget *>(i);
        auto *title_cell = dynamic_cast<QLineEdit *>(outer_table->cellWidget(0, 0));
        auto *full_values_cell = dynamic_cast<RegFullValuesRow *>(outer_table->cellWidget(1, 0));

        addr = strtoull(title_cell->text().toStdString().c_str(), nullptr, 16);
        //addr = title_cell->text().toULongLong(nullptr, 16); // will fail due to the "0x" prefix.
        if ('+' == offset_op)
            addr += addr_offset;
        else
            addr -= addr_offset;

        value = full_values_cell->current_value();

        result.append(left_delim).append(' ')
            .append(QString::asprintf(addr_width_fmt, addr)).append(", ")
            .append(QString::asprintf(value_width_fmt, value))
            .append(' ').append(right_delim).append(",\n");

        ++count;
    } // for (auto &i : scroll_widget->children())

    this->txtInput->setText(result);

    return count;
}

/*
 * ================
 *   CHANGE LOG
 * ================
 *
 * >>> 2024-09-09, Man Hung-Coeng <udc577@126.com>:
 *  01. Initial commit.
 *
 * >>> 2024-09-10, Man Hung-Coeng <udc577@126.com>:
 *  01. Add config_dir to the parameter list of constructor and
 *      check whether the configuration directory exist.
 *
 * >>> 2024-09-13, Man Hung-Coeng <udc577@126.com>:
 *  01. Implement most widget dynamic update logics
 *      except for those of register tables.
 *  02. Append Qt version to the powered-by info at the bottom.
 *
 * >>> 2024-09-25, Man Hung-Coeng <udc577@126.com>:
 *  01. Support generating register graphical tables from configuration file.
 *
 * >>> 2024-09-26, Man Hung-Coeng <udc577@126.com>:
 *  01. Support conversion between graphical tables and text box.
 *
 * >>> 2024-10-01, Man Hung-Coeng <udc577@126.com>:
 *  01. Optimize the loop traversal logic of clear_register_tables()
 *      and generate_register_array_items().
 *  02. Remove resize_table_height() and all private widget classes
 *      to another new created header file and its source file.
 *
 * >>> 2024-10-04, Man Hung-Coeng <udc577@126.com>:
 *  01. Support capturing window close event.
 *  02. Support refreshing tables only when the tab page is switched.
 *
 * >>> 2024-10-09, Man Hung-Coeng <udc577@126.com>:
 *  01. Fix the error of getting default value for items
 *      that reference configurations of other registers.
 *
 * >>> 2024-11-08, Man Hung-Coeng <udc577@126.com>:
 *  01. Fix the error of displaying table title for items
 *      that reference configurations of other registers.
 */

