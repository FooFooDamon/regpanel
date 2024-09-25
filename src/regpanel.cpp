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
#include <QTableWidget>
#include <QHeaderView>

#include "qt_print.hpp"

#if 0
#define ABORT(errcode)                          QApplication::exit(errcode)
#else
#define ABORT(errcode)                          exit(errcode)
#endif

#define SOFT_GREEN_COLOR                        "#c7edcc"

const QTextCodec *G_TEXT_CODEC = QTextCodec::codecForName("UTF8"/*"GB2312"*/);

RegPanel::RegPanel(const char *config_dir, QWidget *parent)
    : QDialog(parent)
    , m_config_dir(config_dir)
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
    {
        this->error_box("Target Missing", "No readable configuration files or valid modules.");

        return;
    }

    const QString &module_name = this->lstModule->currentText();
    const QJsonObject &modules_dict = this->json().object().value(module_name).toObject();

    this->m_reg_addr_map.clear();
    for (const QString &k : modules_dict.keys())
    {
        const std::string &key = k.toStdString();

        if (!k.startsWith("__"))
            this->m_reg_addr_map[strtoull(key.c_str(), nullptr, 16)] = key;
    }

    if (!this->chkboxAsInput->isChecked())
    {
        int count;

        this->clear_register_tables();

        if ((count = this->make_register_tables(this->json(), module_name)) > 0)
        {
            this->info_box("Refresh",
                QString::asprintf("Refreshed %d register tables for module:\n\n%s", count, module_name.toStdString().c_str()));

            this->tab->setCurrentIndex(1);
        }
        else
        {
            this->error_box("Refresh",
                QString::asprintf("Failed to refresh register tables for module:\n\n%s", module_name.toStdString().c_str()));
        }

        this->grpboxView->setTitle(QString::asprintf("View: %d item(s) below", (count >= 0) ? count : 0));
    }
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
    this->spnboxAddrBase->setStyleSheet(ignored ? "background-color: darkgray; color: black;"
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

static int resize_table_height(QTableWidget *table, bool header_row_visible)
{
    int h;
    int table_height = 0;
    //const std::string &table_name = table->objectName().toStdString();

    if (header_row_visible)
    {
        h = table->horizontalHeader()->height();
        table_height += h;
        //qtDebugV(::, "%s: table_height += %d\n", table_name.c_str(), h);
    }

    for (int r = 0; r < table->rowCount(); ++r)
    {
        h = table->rowHeight(r);
        table_height += h;
        //qtDebugV(::, "%s: table_height += %d\n", table_name.c_str(), h);
    }

    //qtDebugV("Before setting: min/max height = %d/%d\n", table->minimumHeight(), table->maximumHeight());
    table->setMinimumHeight(table_height);
    table->setMaximumHeight(table_height);
    //qtDebugV("After setting, min/max height = %d/%d\n", table->minimumHeight(), table->maximumHeight());
    table->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    //table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    return table_height;
}

void RegPanel::on_btnConvert_clicked(void)
{
    if (this->lstModule->currentIndex() < 0)
    {
        this->error_box("No Target", QString::asprintf("No available module, please check your configuration again."));

        return;
    }

    if (0 == qEnvironmentVariable("REGPANEL_TEST", "0").toInt())
    {
        this->warning_box("Unimplemented", __PRETTY_FUNCTION__);

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

class RegFullValuesRow : public QTableWidget
{
private:
    Q_DISABLE_COPY_MOVE(RegFullValuesRow);

public:
    RegFullValuesRow() = delete;

    RegFullValuesRow(QWidget *parent, const QString &name_prefix, uint64_t default_value, uint64_t current_value)
        : QTableWidget(1, 5, parent)
        , m_def_label("Default:  0x", this)
        , m_def_value(this)
        , m_curr_label("Current:  0x", this)
        , m_curr_value(this)
    {
        m_def_label.setObjectName(name_prefix + "_full_values_def_label");
        m_def_value.setObjectName(name_prefix + "_full_values_def_val");
        m_def_value.setDisplayIntegerBase(16);
        m_def_value.setRange(0, 0x7fffffff); // FIXME: unsigned
        m_def_value.setValue(default_value);
        m_def_value.setReadOnly(true);

        m_curr_label.setObjectName(name_prefix + "_full_values_curr_label");
        m_curr_value.setObjectName(name_prefix + "_full_values_curr_val");
        m_curr_value.setDisplayIntegerBase(16);
        m_curr_value.setRange(0, 0x7fffffff); // FIXME: unsigned
        m_curr_value.setValue(current_value);
        m_curr_value.setStyleSheet("background-color: " SOFT_GREEN_COLOR "; color: black;");

        this->setObjectName(name_prefix + "_full_values");
        this->setShowGrid(false);
        this->horizontalHeader()->setVisible(false);
        this->verticalHeader()->setVisible(false);
        this->setCellWidget(0, 0, &m_def_label);
        this->setCellWidget(0, 1, &m_def_value);
        // NOTE: Leave column 2 blank on purpose.
        this->setCellWidget(0, 3, &m_curr_label);
        this->setCellWidget(0, 4, &m_curr_value);
        this->setColumnWidth(0, m_def_label.width());
        this->setColumnWidth(3, m_curr_label.width());
    }

    ~RegFullValuesRow()
    {
        this->clear();
        this->setRowCount(0);
    }

    void sync(uint64_t current_value)
    {
        // FIXME: disconnect slot
        m_curr_value.setValue(current_value);
        // FIXME: connect slot again
    }

// TODO: m_curr_value slot

private:
    QLabel m_def_label;
    QSpinBox m_def_value;
    QLabel m_curr_label;
    QSpinBox m_curr_value;
};

class RegBitsDescCell : public QTableWidget
{
private:
    Q_DISABLE_COPY(RegBitsDescCell);

public:
    RegBitsDescCell() = delete;

    const uint16_t INVALID_INDEX = 0xffff;

    RegBitsDescCell(QWidget *parent, const QString &name_prefix, const QString &title, const QString &hint,
        uint64_t value, uint64_t value_max, bool is_hex_value, const QJsonObject *enum_dict)
        : QTableWidget(2, 1, parent)
        , m_title(title, this)
        , m_digit(nullptr)
        , m_enum(nullptr)
        , m_badvalue_index(INVALID_INDEX)
    {
        QString title_style = QString::asprintf("QLineEdit{ background: transparent; border: none; %s }",
            hint.isEmpty() ? "" : "color: blue;");

        m_title.setObjectName(name_prefix + "_desc_title");
        if (!hint.isEmpty())
        {
            QFont font;

            font.setUnderline(true);
            m_title.setFont(font);
            title_style += "QToolTip { color: white; }";
            m_title.setToolTip(hint);
            m_title.setWhatsThis(hint);
        }
        m_title.setStyleSheet(title_style);
        m_title.setReadOnly(true);

        if (enum_dict)
        {
            int index = 0;

            m_enum = new QComboBox(this);
            m_enum->setObjectName(name_prefix + "_desc_enum");
            m_values.reserve(enum_dict->size());
            for (QJsonObject::const_iterator iter = enum_dict->begin(); enum_dict->end() != iter; ++iter)
            {
                const std::string &key_str = iter.key().toStdString();
                char *ptr;
                uint32_t key_digit = strtoul(key_str.c_str(), &ptr, 16);

                if (0 == key_digit && key_str.c_str() == ptr)
                    m_badvalue_index = index++;

                m_values.push_back(key_digit);

                m_enum->addItem(iter.value().toString("Invalid"));
            }
        }
        else
        {
            m_digit = new QSpinBox(this);
            m_digit->setObjectName(name_prefix + "_desc_digit");
            m_digit->setDisplayIntegerBase(is_hex_value ? 16 : 10);
            m_digit->setRange(0, value_max);
            m_digit->setStyleSheet("background-color: " SOFT_GREEN_COLOR "; color: black;");
            m_digit->setToolTip(is_hex_value ? "hex" : "decimal");
        }
        this->sync(value);

        this->setObjectName(name_prefix + "_desc");
        this->setShowGrid(false);
        this->verticalHeader()->setVisible(false);
        this->horizontalHeader()->setVisible(false);
        this->horizontalHeader()->setStretchLastSection(true); // Auto-stretch for the final columns
        this->setCellWidget(0, 0, &m_title);
        if (enum_dict)
            this->setCellWidget(1, 0, m_enum);
        else
            this->setCellWidget(1, 0, m_digit);
    }

    RegBitsDescCell(RegBitsDescCell &&src)
        : QTableWidget(src.rowCount(), src.columnCount(), src.parentWidget())
        , m_title(src.m_title.text(), this)
        , m_digit(src.m_digit)
        , m_enum(src.m_enum)
        , m_values(std::move(src.m_values))
        , m_badvalue_index(src.m_badvalue_index)
    {
        src.setParent(nullptr);
        src.m_digit = nullptr;
        src.m_enum = nullptr;
        m_digit->setParent(this);
        m_enum->setParent(this);
    }

    ~RegBitsDescCell()
    {
        this->clear();
        this->setRowCount(0);

        if (m_digit)
        {
            delete m_digit;
            m_digit = nullptr;
        }

        if (m_enum)
        {
            delete m_enum;
            m_enum = nullptr;
        }
    }

    void sync(uint32_t value)
    {
        // FIXME: disconnect slot

        if (m_digit)
            m_digit->setValue(value);
        else
        {
            for (size_t i = 0; i < m_values.size(); ++i)
            {
                if (value == m_values[i])
                {
                    m_enum->setCurrentIndex(i);

                    return;
                }
            }

            if (INVALID_INDEX != m_badvalue_index)
                m_enum->setCurrentIndex(m_badvalue_index);
        }

        // FIXME: connect slot again
    }

// TODO: m_digit slot and m_enum slot

private:
    QLineEdit m_title;
    QSpinBox *m_digit;
    QComboBox *m_enum;
    std::vector<uint32_t> m_values;
    uint16_t m_badvalue_index;
};

std::pair<int8_t, int8_t> check_bits_range(const char *range)
{
    std::pair<int8_t, int8_t> result = { -1, -1 };
    char *end_ptr;

    result.first = strtoul(range, &end_ptr, 10);

    if ((0 == result.first && range == end_ptr) // No digits at all.
        || result.first > 63)
    {
        result.first = -1;

        return result;
    }

    if ('\0' != *range && '\0' == *end_ptr) // Entire string has been parsed.
    {
        result.second = result.first;

        return result;
    }

    char *colon_ptr = (':' == *end_ptr) ? end_ptr : strchr(end_ptr, ':');

    if (nullptr == colon_ptr || '\0' == colon_ptr[1]) // Only a single bit.
    {
        result.second = result.first;

        return result;
    }

    result.second = atoi(colon_ptr + 1);

    if (result.second < 0)
    {
        result.first = -1;

        return result;
    }

    if (result.second > result.first)
        result.first = result.second = -1;

    return result;
}

enum BitsItemDesc
{
    BITS_ITEM_DESC_UNKNOWN,
    BITS_ITEM_DESC_TODO,
    BITS_ITEM_DESC_RESERVED,
    BITS_ITEM_DESC_ENUM,
    BITS_ITEM_DESC_BOOL,
    BITS_ITEM_DESC_INVBOOL,
    BITS_ITEM_DESC_DECIMAL,
    BITS_ITEM_DESC_HEX
};

static BitsItemDesc check_bits_item_desc_type(const char *desc)
{
    if (0 == strcasecmp(desc, "TODO"))
        return BITS_ITEM_DESC_TODO;

    if (0 == strcasecmp(desc, "reserved"))
        return BITS_ITEM_DESC_RESERVED;

    if (0 == strcasecmp(desc, "enum"))
        return BITS_ITEM_DESC_ENUM;

    if (0 == strcasecmp(desc, "bool"))
        return BITS_ITEM_DESC_BOOL;

    if (0 == strcasecmp(desc, "invbool"))
        return BITS_ITEM_DESC_INVBOOL;

    if (0 == strcasecmp(desc, "decimal"))
        return BITS_ITEM_DESC_DECIMAL;

    if (0 == strcasecmp(desc, "hex"))
        return BITS_ITEM_DESC_HEX;

    return BITS_ITEM_DESC_UNKNOWN;
}

class RegBitsTable : public QTableWidget
{
private:
    Q_DISABLE_COPY_MOVE(RegBitsTable);

public:
    RegBitsTable() = delete;

    RegBitsTable(QWidget *parent, const QString &name_prefix,
        const char *dict_key, const QJsonArray &dict_value,
        uint64_t default_value, uint64_t current_value)
        : QTableWidget(dict_value.count(), 4, parent)
    {
        QStringList header_texts;
        int value_size = dict_value.count();
        auto gen_bits_mask = [](int8_t high, int8_t low) {
            return ~(0xffffffffffffffffULL << (high - low + 1));
        };
        auto extract_from_full_value = [&gen_bits_mask](uint64_t full_value, int8_t high, int8_t low) {
            return (full_value >> low) & gen_bits_mask(high, low);
        };

        this->setObjectName(name_prefix + "_bits");
        //this->setDragEnabled(false); // doesn't work
        this->setContentsMargins(0, 0, 0, 0);
        this->setHorizontalHeaderLabels(header_texts << "Bits" << "Default" << "Current" << "Desc");
        this->horizontalHeader()->setStretchLastSection(true); // Auto-stretch for the final columns
        this->m_ranges.reserve(value_size);
        this->m_def_values.reserve(value_size);
        this->m_curr_values.reserve(value_size);
        this->m_desc_items.reserve(value_size);
        for (int i = 0; i < value_size; ++i)
        {
            const QJsonValue &item = dict_value[i];

            if (!item.isObject())
            {
                qtCErrV(::, "reg[%s]: item[%d] is not a dictionary/map!\n", dict_key, i);
                continue;
            }

            const QJsonObject &dict = item.toObject();

            if (!dict.contains("attr"))
            {
                qtCErrV(::, "reg[%s]: item[%d] does not contain an \"attr\" property!\n", dict_key, i);
                continue;
            }

            const QJsonValue &attr_val = dict.value("attr");

            if (!attr_val.isArray())
            {
                qtCErrV(::, "reg[%s]: item[%d]: Value of \"attr\" property is not an array!\n", dict_key, i);
                continue;
            }

            const QJsonArray &attr_arr = attr_val.toArray();
            int attr_size = attr_arr.count();

            if (attr_size < 3)
            {
                qtCErrV(::, "reg[%s]: item[%d].attr: Too few elements, just %d!\n", dict_key, i, attr_size);
                continue;
            }

            const std::string &bits_range = attr_arr[0].toString().toStdString();
            auto range_pair = check_bits_range(bits_range.c_str());

            if (range_pair.first < 0 || range_pair.second < 0)
            {
                qtCErrV(::, "reg[%s]: item[%d].attr: Invalid bits range: %s\n", dict_key, i, bits_range.c_str());
                continue;
            }

            const std::string &desc_type_str = attr_arr[2].toString().toStdString();
            auto desc_type = check_bits_item_desc_type(desc_type_str.c_str());

            if (BITS_ITEM_DESC_UNKNOWN == desc_type)
            {
                qtCErrV(::, "reg[%s]: item[%d].attr[%s]: Invalid description type: %s\n",
                    dict_key, i, bits_range.c_str(), desc_type_str.c_str());
                continue;
            }
            else if (BITS_ITEM_DESC_ENUM == desc_type)
            {
                if (!dict.contains("desc"))
                {
                    qtCErrV(::, "reg[%s]: item[%d] does not contain an \"desc\" property!\n", dict_key, i);
                    continue;
                }

                const QJsonValue &desc_val = dict.value("desc");

                if (!desc_val.isObject())
                {
                    qtCErrV(::, "reg[%s]: item[%d]: Value of \"desc\" property is not a dictionary/map!\n", dict_key, i);
                    continue;
                }

                if (desc_val.toObject().count() <= 0)
                {
                    qtCErrV(::, "reg[%s]: item[%d]: \"desc\" dictionary/map is empty!\n", dict_key, i);
                    continue;
                }
            }
            else if (desc_type > BITS_ITEM_DESC_RESERVED && attr_size < 4)
            {
                qtCErrV(::, "reg[%s]: item[%d].attr[%s]: Missing title for description type[%s]\n",
                    dict_key, i, bits_range.c_str(), desc_type_str.c_str());
                continue;
            }
            else
            {
                ; // nothing but for the sake of Code of Conduct
            }

            QString cell_name_prefix = name_prefix + QString::asprintf("_bits[%s]", bits_range.c_str());
            uint64_t curr_value = extract_from_full_value(current_value, range_pair.first, range_pair.second);
            uint64_t value_max = gen_bits_mask(range_pair.first, range_pair.second);

            this->m_ranges.push_back(new QLabel(QString::fromStdString(bits_range), this));
            this->m_ranges.back()->setObjectName(cell_name_prefix);
            this->setCellWidget(i, 0, this->m_ranges.back());

            this->m_def_values.push_back(new QSpinBox(this));
            this->m_def_values.back()->setObjectName(cell_name_prefix + "_defval");
            this->m_def_values.back()->setReadOnly(true);
            this->m_def_values.back()->setStyleSheet("background-color: darkgray; color: white;");
            this->m_def_values.back()->setDisplayIntegerBase(16);
            this->m_def_values.back()->setRange(0, value_max);
            this->m_def_values.back()->setValue(
                extract_from_full_value(default_value, range_pair.first, range_pair.second));
            this->setCellWidget(i, 1, this->m_def_values.back());

            this->m_curr_values.push_back(new QSpinBox(this));
            this->m_curr_values.back()->setObjectName(cell_name_prefix + "_currval");
            if (0 == attr_arr[1].toString().compare("RO", Qt::CaseInsensitive))
            {
                this->m_curr_values.back()->setReadOnly(true);
                this->m_curr_values.back()->setStyleSheet("background-color: darkgray; color: white;");
            }
            else
            {
                this->m_curr_values.back()->setStyleSheet("background-color: " SOFT_GREEN_COLOR "; color: black;");
            }
            this->m_curr_values.back()->setDisplayIntegerBase(16);
            this->m_curr_values.back()->setRange(0, value_max);
            this->m_curr_values.back()->setValue(curr_value);
            this->setCellWidget(i, 2, this->m_curr_values.back());

            if (desc_type > BITS_ITEM_DESC_RESERVED)
            {
                bool enumerable = (desc_type >= BITS_ITEM_DESC_ENUM && desc_type <= BITS_ITEM_DESC_INVBOOL);
                QJsonObject bool_dict({ { "0", "false" }, { "1", "true" } });
                QJsonObject invbool_dict({ { "0", "true" }, { "1", "false" } });
                const QJsonObject &enum_dict = (BITS_ITEM_DESC_ENUM == desc_type) ? dict.value("desc").toObject()
                    : ((BITS_ITEM_DESC_BOOL == desc_type) ? bool_dict : invbool_dict);

                this->m_desc_items.push_back(
                    new RegBitsDescCell(this, cell_name_prefix, attr_arr[3].toString(),
                        (attr_size > 4) ? attr_arr[4].toString() : "",
                        curr_value, value_max, /* is_hex_value = */(BITS_ITEM_DESC_HEX == desc_type),
                        enumerable ? &enum_dict : nullptr)
                );
                this->setRowHeight(i, resize_table_height(
                    dynamic_cast<RegBitsDescCell *>(this->m_desc_items.back()), /* header_row_visible = */false));
            }
            else
            {
                this->m_desc_items.push_back(new QLabel(QString::fromStdString(desc_type_str), this));
                this->m_desc_items.back()->setObjectName(cell_name_prefix + "_desc");
            }
            this->setCellWidget(i, 3, this->m_desc_items.back());
        } // for (int i : dict_value.count())
    }

    ~RegBitsTable()
    {
        this->clear();
        this->setRowCount(0);

        for (auto &i : m_ranges)
        {
            delete i;
        }
        std::vector<QLabel *>().swap(m_ranges);

        for (auto &i : m_def_values)
        {
            delete i;
        }
        std::vector<QSpinBox *>().swap(m_def_values);

        for (auto &i : m_curr_values)
        {
            delete i;
        }
        std::vector<QSpinBox *>().swap(m_curr_values);

        for (auto &i : m_desc_items)
        {
            delete i;
        }
        std::vector<QWidget *>().swap(m_desc_items);
    }

private:
    std::vector<QLabel *> m_ranges;
    std::vector<QSpinBox *> m_def_values;
    std::vector<QSpinBox *> m_curr_values;
    std::vector<QWidget *> m_desc_items;
};

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
    int i = -1;

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
        QTableWidget *reg_table;

        if (dest_key.isEmpty())
        {
            uint64_t default_value = get_default_value(modules_dict, orig_key);

            reg_table = this->make_register_table(scroll_widget, name_prefix, orig_key, orig_val_arr,
                default_value, default_value);
        }
        else
        {
            qtCDebugV(::, "Redirecting reg[%s] to reg[%s] ...\n",
                orig_key.toStdString().c_str(), dest_key.toStdString().c_str());

            if (!modules_dict.contains(dest_key))
            {
                qtCErrV(::, "[%d] No such a register: %s\n", i, dest_key.toStdString().c_str());
                continue;
            }

            const QJsonValue &dest_value = modules_dict.value(dest_key);

            if (!dest_value.isArray())
            {
                qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", i, dest_key.toStdString().c_str());
                continue;
            }

            uint64_t default_value = get_default_value(modules_dict, dest_key);

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

int RegPanel::make_register_tables(const QTextEdit &textbox, const QString &module_name)
{
    static int test_count = 0;
    int table_count = 0;

    if (0 == ((++test_count) % 2))
    {
        this->info_box("Test", "All register tables cleared");

        return table_count;
    }

    const QJsonObject &modules_dict = this->json().object().value(module_name).toObject();
    QVBoxLayout *vlayout = this->vlayoutRegTables;
    QWidget *scroll_widget = vlayout->parentWidget();

    for (int i = 0; i < 3; ++i)
    {
        int addr = i * 4;
        auto orig_key_iter = this->m_reg_addr_map.find(addr);

        if (this->m_reg_addr_map.end() == orig_key_iter)
        {
            qtCErrV(::, "[%d] No such a register with address = 0x%x\n", i, addr);
            continue;
        }

        QString orig_key = QString::fromStdString(orig_key_iter->second);
        const QJsonValue &orig_value = modules_dict.value(orig_key);

        if (!orig_value.isArray())
        {
            qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", i, orig_key.toStdString().c_str());
            continue;
        }

        const QJsonArray &orig_val_arr = orig_value.toArray();
        QString dest_key = find_referenced_register_if_any(modules_dict, orig_key, orig_val_arr);
        QString name_prefix = QString::asprintf("reg[%d]", i);
        QTableWidget *reg_table;

        if (dest_key.isEmpty())
        {
            uint64_t default_value = get_default_value(modules_dict, orig_key);

            reg_table = this->make_register_table(scroll_widget, name_prefix, orig_key, orig_val_arr,
                default_value, addr);
        }
        else
        {
            if (!modules_dict.contains(dest_key))
            {
                qtCErrV(::, "[%d] No such a register: %s\n", i, dest_key.toStdString().c_str());
                continue;
            }

            const QJsonValue &dest_value = modules_dict.value(dest_key);

            if (!dest_value.isArray())
            {
                qtCErrV(::, "[%d] Value of register[%s] is not an array!\n", i, dest_key.toStdString().c_str());
                continue;
            }

            uint64_t default_value = get_default_value(modules_dict, dest_key);

            reg_table = this->make_register_table(scroll_widget, name_prefix, dest_key, dest_value.toArray(),
                default_value, addr);
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

        for (auto &j : i->children())
        {
            bool is_for_table = false;

            for (auto &k : j->children())
            {
                const std::string &kname = k->objectName().toStdString();

                if (!is_reg_widget(kname))
                    continue;

                is_for_table = true;

                if (std::string::npos != kname.rfind("_title"))
                {
                    qtCDebugV(::, "\t\tDeleting: %s (%s)\n", kname.c_str(),
                        dynamic_cast<QLineEdit *>(k)->text().toStdString().c_str());
                    delete k;

                    continue;
                }

                if (print_flag) qtCDebugV(::, "\t\tDeleting: %s\n", kname.c_str());
                delete k;
            } // for (auto &k : j->children())

            if (!is_for_table)
                continue;

            if (print_flag) qtCDebugV(::, "\tNo need to delete: %s\n", j->objectName().toStdString().c_str());
            //delete j;
        } // for (auto &j : i->children())

        qtCDebugV(::, "Deleting: %s\n", iname.c_str());
        vlayout->removeWidget(dynamic_cast<QWidget *>(i));
        delete i;
        print_flag = false;
    } // for (auto &i : scroll_widget->children())
}

int RegPanel::generate_register_array_items(const QString &module_name, const QTextEdit &textbox)
{
    this->warning_box("Unimplemented", __PRETTY_FUNCTION__);

    return 0;
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
 */

