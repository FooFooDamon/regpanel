/*
 * Private widget classes of this project.
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

#include "private_widgets.hpp"

#include <set>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QJsonObject>
#include <QJsonArray>

#include "qt_print.hpp"

int resize_table_height(QTableWidget *table, bool header_row_visible)
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

/******************************** BigSpinBox begin ********************************/

QValidator::State BigSpinBox::validate(QString &input, int &pos) const/* override */
{
    QString copy(input);

    //qtCDebugV(::, "input: %s, pos: %d\n", copy.toStdString().c_str(), pos);
    if (copy.startsWith("0x"))
        copy.remove(0, 2);

    pos -= copy.size() - copy.trimmed().size(); // FIXME: ??
    copy = copy.trimmed();
    if (copy.isEmpty())
        return QValidator::Intermediate;

    const std::string copy_str = copy.toStdString();
    char *end_ptr;
    uint64_t val = strtoull(copy_str.c_str(), &end_ptr, this->displayIntegerBase());

    if (/*'\0' != copy_str[0] && */'\0' == *end_ptr) // Entire string is valid.
        return (val >= m_minimum64 && val <= m_maximum64) ? QValidator::Acceptable : QValidator::Invalid;

    return QValidator::Invalid;
}

/*
 * NOTE 1: This reimplemented function affects the initial default and current values
 *      of each RegFullValuesRow item. Without this function,
 *      a value greater than INT32_MAX (0x7fffffff) will become 0,
 *      but the reason is uncertain yet.
 *
 * NOTE 2: Meanwhile, it causes a new bug that the displaying text of combo box fails to get updated
 *      when the user clicks the Up/Down buttons or presses the keyboard Up/Down arrows instead of
 *      inputting the expected value directly.
 *      To solve this bug, I have to reimplement one more function stepBy() after this one.
 */
QString BigSpinBox::textFromValue(int val/* This value is truncated and thus not used. */) const/* override */
{
    const QString &text = this->lineEdit()->displayText();
    int base = this->displayIntegerBase();
    uint64_t value = text.toULongLong(nullptr, base); // Converted from instant text instead of using the old m_value64.

    qtCDebugV(::, "%s: base = %d, m_value64 = 0x%lx, text = %s, result = 0x%lx\n",
        this->name().c_str(), base, m_value64, text.toStdString().c_str(), value);

    return QString::number(value, base);
}

/*
 * NOTE: This reimplemented function is for fixing a bug caused by textFromValue() above.
 */
void BigSpinBox::stepBy(int steps)/* override */
{
    if (0 == steps)
        return;

    uint64_t val = this->lineEdit()->displayText().toULongLong(nullptr, this->displayIntegerBase()); // this->value()
    uint64_t min = this->minimum();
    uint64_t max = this->maximum();

    if ((steps < 0 && val <= min) || (steps > 0 && val >= max))
        return;

    uint64_t absolute_val = (steps < 0) ? -steps : steps;

    if (steps > 0)
        this->setValue((max - val >= absolute_val) ? (val + absolute_val) : max);
    else
        this->setValue((val - min >= absolute_val) ? (val - absolute_val) : min);
}

void BigSpinBox::setValue(uint64_t val)
{
    if (/*val == m_value64 || */val < m_minimum64 || val > m_maximum64)
        return;

    QString text = this->prefix() + QString::number(val, this->displayIntegerBase());

    m_value64 = val;
    this->lineEdit()->setText(text);
    qtCDebugV(::, "%s: val = 0x%lx, text = %s, displayText() = %s\n",
        this->name().c_str(), val, text.toStdString().c_str(), this->lineEdit()->displayText().toStdString().c_str());

    //emit this->valueChanged(val);
    emit this->textChanged(this->lineEdit()->displayText());
}

/******************************** BigSpinBox end ********************************/

/******************************** RegFullValuesRow begin ********************************/

RegFullValuesRow::RegFullValuesRow(QWidget *parent, const QString &name_prefix,
    uint64_t default_value, uint64_t current_value)
    : QTableWidget(1, 5, parent)
    , m_def_label("Default:  ", this)
    , m_def_value(BigSpinBox::ShowStyle::HEX, this)
    , m_curr_label("Current:  ", this)
    , m_curr_value(BigSpinBox::ShowStyle::HEX, this)
{
    m_def_label.setObjectName(name_prefix + "_full_values_def_label");
    m_def_value.setObjectName(name_prefix + "_full_values_def_val");
    m_def_value.setRange(0, UINT64_MAX);
    m_def_value.setValue(default_value);
    m_def_value.setReadOnly(true);

    m_curr_label.setObjectName(name_prefix + "_full_values_curr_label");
    m_curr_value.setObjectName(name_prefix + "_full_values_curr_val");
    m_curr_value.setRange(0, UINT64_MAX);
    m_curr_value.setValue(current_value);
    m_curr_value.setReadOnly(true);
    if (!m_curr_value.isReadOnly())
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

RegFullValuesRow::~RegFullValuesRow()
{
    //this->clear();
    //this->setRowCount(0);
}

/******************************** RegFullValuesRow end ********************************/

/******************************** RegBitsDescCell begin ********************************/

RegBitsDescCell::RegBitsDescCell(QWidget *parent, const QString &name_prefix, const QString &title, const QString &hint,
    uint64_t value, uint64_t value_max, BigSpinBox::ShowStyle style,
    const QJsonObject *enum_dict, bool is_readonly)
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
        std::vector<uint16_t> bad_indexes;
        uint64_t bad_key = 0xffff;
        std::set<uint64_t> key_digits;

        m_enum = new QComboBox(this);
        m_enum->setObjectName(name_prefix + "_desc_enum");
        //m_enum->setDisabled(is_readonly);
        m_enum_values.reserve(enum_dict->size());
        bad_indexes.reserve(enum_dict->size());
        for (QJsonObject::const_iterator iter = enum_dict->begin(); enum_dict->end() != iter; ++iter)
        {
            const std::string &key_str = iter.key().toStdString();
            char *ptr;
            uint64_t key_digit = strtoull(key_str.c_str(), &ptr, 16);

            if (0 == key_digit && key_str.c_str() == ptr)
            {
                m_badvalue_index = index++;
                bad_indexes.push_back(m_badvalue_index);
            }
            else
            {
                key_digits.insert(key_digit);
            }

            m_enum_values.push_back(key_digit);

            m_enum->addItem(iter.value().toString("Invalid"));
        }
        for (uint16_t i = 0; i < (uint16_t)0xffff; ++i)
        {
            if (key_digits.end() == key_digits.find(i))
            {
                bad_key = i;
                break;
            }
        }
        for (auto bad_index : bad_indexes)
        {
            m_enum_values[bad_index] = bad_key;
        }

        this->connect(m_enum, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_enumbox_currentIndexChanged(int)));
    } // if (enum_dict)
    else
    {
        m_digit = new BigSpinBox(style, this);
        m_digit->setObjectName(name_prefix + "_desc_digit");
        //m_digit->setReadOnly(is_readonly);
        m_digit->setRange(0, value_max);
        m_digit->setStyleSheet(is_readonly ? "background-color: darkgray; color: white;"
            : "background-color: " SOFT_GREEN_COLOR "; color: black;");
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

RegBitsDescCell::RegBitsDescCell(RegBitsDescCell &&src)
    : QTableWidget(src.rowCount(), src.columnCount(), src.parentWidget())
    , m_title(src.m_title.text(), this)
    , m_digit(src.m_digit)
    , m_enum(src.m_enum)
    , m_enum_values(std::move(src.m_enum_values))
    , m_badvalue_index(src.m_badvalue_index)
{
    src.setParent(nullptr);
    src.m_digit = nullptr;
    src.m_enum = nullptr;
    m_digit->setParent(this);
    m_enum->setParent(this);
}

RegBitsDescCell::~RegBitsDescCell()
{
    //this->clear();
    //this->setRowCount(0);

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

void RegBitsDescCell::sync(uint64_t value)
{
    if (m_digit)
    {
        this->disconnect(m_digit, SIGNAL(textChanged(const QString &)),
            this, SLOT(on_digitbox_textChanged(const QString &)));

        m_digit->setValue(value);

        this->connect(m_digit, SIGNAL(textChanged(const QString &)),
            this, SLOT(on_digitbox_textChanged(const QString &)));
    }
    else
    {
        for (size_t i = 0; i < m_enum_values.size(); ++i)
        {
            if (value == m_enum_values[i])
            {
                this->disconnect(m_enum, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(on_enumbox_currentIndexChanged(int)));

                m_enum->setCurrentIndex(i);

                this->connect(m_enum, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(on_enumbox_currentIndexChanged(int)));

                return;
            }
        }

        if (INVALID_INDEX != m_badvalue_index)
        {
            this->disconnect(m_enum, SIGNAL(currentIndexChanged(int)),
                this, SLOT(on_enumbox_currentIndexChanged(int)));

            m_enum->setCurrentIndex(m_badvalue_index);

            this->connect(m_enum, SIGNAL(currentIndexChanged(int)),
                this, SLOT(on_enumbox_currentIndexChanged(int)));
        }
    }
}

void RegBitsDescCell::on_digitbox_textChanged(const QString &text)
{
    auto *bits_table = dynamic_cast<QTableWidget *>(this->parentWidget()->parentWidget());
    int desc_column = bits_table->columnCount() - 1;
    BigSpinBox *relevant_digit = nullptr;

    for (int i = 0; i < bits_table->rowCount(); ++i)
    {
        if (bits_table->cellWidget(i, desc_column) == this)
        {
            relevant_digit = dynamic_cast<BigSpinBox *>(bits_table->cellWidget(i, 2));
            break;
        }
    }

    if (relevant_digit)
        relevant_digit->setValue(strtoull(text.toStdString().c_str(), nullptr, text.startsWith("0x") ? 16 : 10));
}

void RegBitsDescCell::on_enumbox_currentIndexChanged(int index)
{
    if (index < 0)
        return;

    auto *bits_table = dynamic_cast<QTableWidget *>(this->parentWidget()->parentWidget());
    int desc_column = bits_table->columnCount() - 1;
    BigSpinBox *relevant_digit = nullptr;

    for (int i = 0; i < bits_table->rowCount(); ++i)
    {
        if (bits_table->cellWidget(i, desc_column) == this)
        {
            relevant_digit = dynamic_cast<BigSpinBox *>(bits_table->cellWidget(i, 2));
            break;
        }
    }

    if (relevant_digit)
        relevant_digit->setValue(m_enum_values[index]);
}

/******************************** RegBitsDescCell end ********************************/

/******************************** RegBitsTable begin ********************************/

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
    BITS_ITEM_DESC_MISSING,
    BITS_ITEM_DESC_TODO,
    BITS_ITEM_DESC_RESERVED,
    BITS_ITEM_DESC_ENUM,
    BITS_ITEM_DESC_BOOL,
    BITS_ITEM_DESC_INVBOOL, // inverse bool
    BITS_ITEM_DESC_DECIMAL,
    BITS_ITEM_DESC_UDECIMAL, // unsigned decimal
    BITS_ITEM_DESC_HEX
};

static BitsItemDesc check_bits_item_desc_type(const char *desc)
{
    if (0 == strcasecmp(desc, "missing"))
        return BITS_ITEM_DESC_MISSING;

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

    if (0 == strcasecmp(desc, "udecimal"))
        return BITS_ITEM_DESC_UDECIMAL;

    if (0 == strcasecmp(desc, "hex"))
        return BITS_ITEM_DESC_HEX;

    return BITS_ITEM_DESC_UNKNOWN;
}

static inline uint64_t u64_lshift(uint64_t value, uint8_t shift)
{
    return (shift >= 64) ? 0 : (value << shift);
}

static inline uint64_t u64_rshift(uint64_t value, uint8_t shift)
{
    return (shift >= 64) ? 0 : (value >> shift);
}

RegBitsTable::RegBitsTable(QWidget *parent, const QString &name_prefix,
    const char *dict_key, const QJsonArray &dict_value,
    uint64_t default_value, uint64_t current_value)
    : QTableWidget(dict_value.count(), 4, parent)
{
    QStringList header_texts;
    int value_size = dict_value.count();
    auto gen_bits_mask = [](int8_t high, int8_t low) {
        return ~u64_lshift(UINT64_MAX, high - low + 1);
    };
    auto extract_from_full_value = [&gen_bits_mask](uint64_t full_value, int8_t high, int8_t low) {
        return (full_value >> low) & gen_bits_mask(high, low);
    };

    this->setObjectName(name_prefix + "_bits");
    //this->setDragEnabled(false); // doesn't work
    this->setContentsMargins(0, 0, 0, 0);
    this->setHorizontalHeaderLabels(header_texts << "Bits" << "Default" << "Current" << "Description");
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
        bool is_readonly = (0 == attr_arr[1].toString().compare("RO", Qt::CaseInsensitive));

        this->m_ranges.push_back(new QLabel(QString::fromStdString(bits_range), this));
        this->m_ranges.back()->setObjectName(cell_name_prefix);
        this->setCellWidget(i, 0, this->m_ranges.back());

        this->m_def_values.push_back(new BigSpinBox(BigSpinBox::ShowStyle::HEX, this));
        this->m_def_values.back()->setObjectName(cell_name_prefix + "_defval");
        this->m_def_values.back()->setReadOnly(true);
        this->m_def_values.back()->setStyleSheet("background-color: darkgray; color: white;");
        this->m_def_values.back()->setRange(0, value_max);
        this->m_def_values.back()->setValue(
            extract_from_full_value(default_value, range_pair.first, range_pair.second));
        this->setCellWidget(i, 1, this->m_def_values.back());

        this->m_curr_values.push_back(new BigSpinBox(BigSpinBox::ShowStyle::HEX, this));
        this->m_curr_values.back()->setObjectName(cell_name_prefix + "_currval");
        if (is_readonly)
        {
            this->m_curr_values.back()->setReadOnly(true);
            this->m_curr_values.back()->setStyleSheet("background-color: darkgray; color: white;");
        }
        else
        {
            this->m_curr_values.back()->setStyleSheet("background-color: " SOFT_GREEN_COLOR "; color: black;");
        }
        this->m_curr_values.back()->setRange(0, value_max);
        this->m_curr_values.back()->setValue(curr_value);
        this->setCellWidget(i, 2, this->m_curr_values.back());
        this->connect(this->m_curr_values.back(), SIGNAL(textChanged(const QString &)),
            this, SLOT(on_currval_textChanged(const QString &)));

        if (desc_type > BITS_ITEM_DESC_RESERVED)
        {
            BigSpinBox::ShowStyle show_style = (BITS_ITEM_DESC_DECIMAL == desc_type) ? BigSpinBox::ShowStyle::DECIMAL
                : ((BITS_ITEM_DESC_UDECIMAL == desc_type) ? BigSpinBox::ShowStyle::UDECIMAL
                    : BigSpinBox::ShowStyle::HEX);
            bool enumerable = (desc_type >= BITS_ITEM_DESC_ENUM && desc_type <= BITS_ITEM_DESC_INVBOOL);
            QJsonObject bool_dict({ { "0", "false" }, { "1", "true" } });
            QJsonObject invbool_dict({ { "0", "true" }, { "1", "false" } });
            const QJsonObject &enum_dict = (BITS_ITEM_DESC_ENUM == desc_type) ? dict.value("desc").toObject()
                : ((BITS_ITEM_DESC_BOOL == desc_type) ? bool_dict : invbool_dict);

            this->m_desc_items.push_back(
                new RegBitsDescCell(this, cell_name_prefix, attr_arr[3].toString(),
                    (attr_size > 4) ? attr_arr[4].toString() : "",
                    curr_value, value_max, show_style, (enumerable ? &enum_dict : nullptr), is_readonly)
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

RegBitsTable::~RegBitsTable()
{
    //this->clear();
    //this->setRowCount(0);

    for (auto &i : m_ranges)
    {
        delete i;
    }
    std::vector<QLabel *>().swap(m_ranges);

    for (auto &i : m_def_values)
    {
        delete i;
    }
    std::vector<BigSpinBox *>().swap(m_def_values);

    for (auto &i : m_curr_values)
    {
        delete i;
    }
    std::vector<BigSpinBox *>().swap(m_curr_values);

    for (auto &i : m_desc_items)
    {
        delete i;
    }
    std::vector<QWidget *>().swap(m_desc_items);
}

void RegBitsTable::on_currval_textChanged(const QString &text)
{
    auto *changed_bits = dynamic_cast<BigSpinBox *>(this->sender()/* QObject::sender() */);
    std::pair<int8_t, int8_t> bits_range_pair;
    RegBitsDescCell *desc_cell = nullptr;
    QLabel *desc_label = nullptr;

    for (size_t i = 0; i < m_curr_values.size(); ++i)
    {
        if (changed_bits == m_curr_values[i])
        {
            QWidget *desc_item = m_desc_items[i];

            desc_cell = dynamic_cast<RegBitsDescCell *>(desc_item);
            desc_label = dynamic_cast<QLabel *>(desc_item);

            bits_range_pair = check_bits_range(m_ranges[i]->text().toStdString().c_str());

            break;
        }
    }

    const std::string &bits_obj_name = changed_bits ? changed_bits->objectName().toStdString() : "<UNKNOWN>";

    if (nullptr == desc_cell && nullptr == desc_label)
    {
        qtCErrV(::, "Can not match a desc widget with emitter: %s\n", bits_obj_name.c_str());

        return;
    }

    const std::string &text_str = text.toStdString();
    uint64_t bits_value = strtoull(text_str.c_str(), nullptr, text.startsWith("0x") ? 16 : 10);
    auto *outer_table = dynamic_cast<QTableWidget *>(this->parentWidget()->parentWidget());
    auto *full_values_table = dynamic_cast<RegFullValuesRow *>(outer_table->cellWidget(1, 0));
    uint64_t full_value = full_values_table->current_value();
    uint64_t mask_high = u64_lshift(UINT64_MAX, bits_range_pair.first + 1);
    uint64_t mask_low = u64_rshift(UINT64_MAX, 64 - bits_range_pair.second);
    uint64_t mask = mask_high | mask_low;
    uint64_t full_value_updated = (full_value & mask) | (bits_value << bits_range_pair.second);

    qtCDebugV(::, "%s: text = %s, bits_value = 0x%lx, mask_high = 0x%lx, mask_low = 0x%lx, mask = 0x%lx,"
        " full_value = 0x%lx, result = 0x%lx\n",
        bits_obj_name.c_str(), text_str.c_str(), bits_value, mask_high, mask_low, mask,
        full_value, full_value_updated);

    full_values_table->sync(full_value_updated);

    if (desc_cell)
        desc_cell->sync(bits_value);
}

/******************************** RegBitsTable end ********************************/

/*
 * ================
 *   CHANGE LOG
 * ================
 *
 * >>> 2024-10-01, Man Hung-Coeng <udc577@126.com>:
 *  01. Initial commit.
 *
 * >>> 2024-10-08, Man Hung-Coeng <udc577@126.com>:
 *  01. Rename class U64SpinBox to BigSpinBox and improve it a little
 *      (not supporting signed decimal yet).
 *  02. Add a new description type "missing" to support the case that
 *      the official doesn't provide any info.
 *
 * >>> 2024-10-15, Man Hung-Coeng <udc577@126.com>:
 *  01. Fix the error of synchronizing the "Others" option
 *      of Description pull-down list.
 */

