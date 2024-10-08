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

#ifndef __PRIVATE_WIDGETS_HPP__
#define __PRIVATE_WIDGETS_HPP__

#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>

class QComboBox;

#define SOFT_GREEN_COLOR                        "#c7edcc"

int resize_table_height(QTableWidget *table, bool header_row_visible);

class BigSpinBox : public QSpinBox
{
    Q_OBJECT

    Q_PROPERTY(uint64_t value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(uint64_t minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(uint64_t maximum READ maximum WRITE setMaximum)

private:
    Q_DISABLE_COPY_MOVE(BigSpinBox);

public:
    enum ShowStyle
    {
        HEX,
        UDECIMAL,
        DECIMAL,
    };

    explicit BigSpinBox(enum ShowStyle style = HEX, QWidget *parent = nullptr)
        : QSpinBox(parent)
        , m_show_style(style)
        , m_value64(0)
        , m_minimum64(0)
        , m_maximum64(99)
    {
        if (HEX == style)
        {
            this->setDisplayIntegerBase(16);
            this->setPrefix("0x");
        }
        else
        {
            this->setDisplayIntegerBase(10);
        }
    }

public:
    inline std::string name(void) const
    {
        return this->objectName().toStdString();
    }

    inline enum ShowStyle show_style(void) const
    {
        return m_show_style;
    }

    inline uint64_t value(void) const
    {
        return m_value64;
    }

    inline uint64_t minimum(void) const
    {
        return m_minimum64;
    }

    inline void setMinimum(uint64_t min) // FIXME: Support signed type.
    {
        if (min <= m_maximum64)
        {
            int64_t signed_min = static_cast<int64_t>(min);

            m_minimum64 = min;
            // For automatically disabling the Down button of combo box if reaching the bottom.
            QSpinBox::setMinimum((signed_min < INT32_MIN) ? INT32_MIN : signed_min);
        }
    }

    inline uint64_t maximum(void) const
    {
        return m_maximum64;
    }

    inline void setMaximum(uint64_t max) // FIXME: Support signed type.
    {
        if (max >= m_minimum64)
        {
            m_maximum64 = max;
            // For automatically disabling the Up button of combo box if reaching the top.
            QSpinBox::setMaximum((max > INT32_MAX) ? INT32_MAX : max);
        }
    }

    inline void setRange(uint64_t min, uint64_t max) // FIXME: Support signed type.
    {
        if (max >= min)
        {
            setMinimum(min);
            setMaximum(max);
        }
    }

protected:
    QValidator::State validate(QString &input, int &pos) const override;
    //int valueFromText(const QString &text) const override;
    QString textFromValue(int val) const override;

public:
    void stepBy(int steps) override;

public Q_SLOTS:
    void setValue(uint64_t val);

Q_SIGNALS:
  void valueChanged(uint64_t); // optional, only needed if Q_PROPERTY(... setValue ..) above contains NOTIFY

private:
    enum ShowStyle m_show_style;
    uint64_t m_value64;
    uint64_t m_minimum64;
    uint64_t m_maximum64;
};

class RegFullValuesRow : public QTableWidget
{
private:
    Q_DISABLE_COPY_MOVE(RegFullValuesRow);

public:
    RegFullValuesRow() = delete;

    RegFullValuesRow(QWidget *parent, const QString &name_prefix, uint64_t default_value, uint64_t current_value);

    ~RegFullValuesRow();

    inline void sync(uint64_t current_value)
    {
        m_curr_value.setValue(current_value);
    }

    inline uint64_t current_value(void)
    {
        return m_curr_value.value();
    }

private:
    QLabel m_def_label;
    BigSpinBox m_def_value;
    QLabel m_curr_label;
    BigSpinBox m_curr_value;
};

class RegBitsDescCell : public QTableWidget
{
    Q_OBJECT

private:
    Q_DISABLE_COPY(RegBitsDescCell);

public:
    RegBitsDescCell() = delete;

    RegBitsDescCell(QWidget *parent, const QString &name_prefix, const QString &title, const QString &hint,
        uint64_t value, uint64_t value_max, BigSpinBox::ShowStyle style,
        const QJsonObject *enum_dict, bool is_readonly);

    RegBitsDescCell(RegBitsDescCell &&src);

    ~RegBitsDescCell();

    void sync(uint64_t value);

private slots:
    void on_digitbox_textChanged(const QString &text);

    void on_enumbox_currentIndexChanged(int index);

private:
    QLineEdit m_title;
    BigSpinBox *m_digit;
    QComboBox *m_enum;
    std::vector<uint64_t> m_enum_values;
    uint16_t m_badvalue_index;
    static const uint16_t INVALID_INDEX = 0xffff;
};

class RegBitsTable : public QTableWidget
{
    Q_OBJECT

private:
    Q_DISABLE_COPY_MOVE(RegBitsTable);

public:
    RegBitsTable() = delete;

    RegBitsTable(QWidget *parent, const QString &name_prefix,
        const char *dict_key, const QJsonArray &dict_value,
        uint64_t default_value, uint64_t current_value);

    ~RegBitsTable();

private slots:
    void on_currval_textChanged(const QString &text);

private:
    std::vector<QLabel *> m_ranges;
    std::vector<BigSpinBox *> m_def_values;
    std::vector<BigSpinBox *> m_curr_values;
    std::vector<QWidget *> m_desc_items;
};

#endif /* #ifndef __PRIVATE_WIDGETS_HPP__ */

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
 */

