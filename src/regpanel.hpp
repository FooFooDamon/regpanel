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

#ifndef __REGPANEL_HPP__
#define __REGPANEL_HPP__

#include <QJsonDocument>

#include "ui_regpanel.h"

class QTableWidget;

class RegPanel : public QDialog, public Ui_Dialog
{
    Q_OBJECT

public:
    RegPanel() = delete;
    RegPanel(const char *config_dir, QWidget *parent = nullptr);
    ~RegPanel();

public:
    void info_box(const QString &title, const QString &text);
    void warning_box(const QString &title, const QString &text);
    void error_box(const QString &title, const QString &text);

public:
    inline const std::string& config_dir(void) const
    {
        return m_config_dir;
    }

    typedef std::pair<std::string, std::vector<std::string>> ChipItem;

    typedef std::pair<std::string, std::vector<ChipItem>> VendorItem;

    inline const std::vector<VendorItem>& vendors(void) const
    {
        return m_vendors;
    }

    inline QJsonDocument& json(void)
    {
        return m_json;
    }

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_tab_currentChanged(int index);
    void on_lstVendor_currentIndexChanged(int index);
    void on_lstChip_currentIndexChanged(int index);
    void on_lstFile_currentIndexChanged(int index);
    void on_lstModule_currentIndexChanged(int index);
    void on_lstDelimeter_currentIndexChanged(int index);
    void on_lstAddrBaseMethod_currentIndexChanged(int index);
    void on_chkboxAsInput_stateChanged(int checked);
    void on_btnConvert_clicked(void);

private:
    void scan_config_directory(const char *config_dir);
    bool load_config_file(const char *path);
    QTableWidget* make_register_table(QWidget *parent, const QString &name_prefix,
        const QString &dict_key, const QJsonArray &dict_value,
        uint64_t default_value, uint64_t current_value);
    int make_register_tables(const QJsonDocument &json, const QString &module_name);
    int make_register_tables(const QTextEdit &textbox, const QString &module_name);
    void clear_register_tables(void);
    int generate_register_array_items(const QString &module_name, const QTextEdit &textbox);

private:
    std::string m_config_dir;
    std::vector<VendorItem> m_vendors;
    QJsonDocument m_json;
    std::map<uint64_t, std::string> m_reg_addr_map;
    int m_prev_vendor_idx;
    int m_prev_chip_idx;
    int m_prev_file_idx;
    int m_prev_module_idx;
};

#endif /* #ifndef __REGPANEL_HPP__ */

/*
 * ================
 *   CHANGE LOG
 * ================
 *
 * >>> 2024-09-09, Man Hung-Coeng <udc577@126.com>:
 *  01. Initial commit.
 *
 * >>> 2024-09-10, Man Hung-Coeng <udc577@126.com>:
 *  01. Add config_dir to the parameter list of constructor.
 *
 * >>> 2024-09-13, Man Hung-Coeng <udc577@126.com>:
 *  01. Add necessary widget slots (aka: callback functions for state change)
 *      and some auxiliary variables/functions.
 *
 * >>> 2024-09-25, Man Hung-Coeng <udc577@126.com>:
 *  01. Add functions of generating register graphical tables
 *      and array item texts.
 *
 * >>> 2024-10-04, Man Hung-Coeng <udc577@126.com>:
 *  01. Add closeEvent() for capturing window close event.
 *  02. Add on_tab_currentChanged() and m_prev_*_idx to support
 *      refreshing tables only when the tab page is switched.
 */

