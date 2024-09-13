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

class RegPanel : public QDialog, public Ui_Dialog
{
    Q_OBJECT

public:
    RegPanel() = delete;
    RegPanel(const char *config_dir, QWidget *parent = nullptr);

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

private slots:
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
    bool refresh_register_tables(const QJsonDocument &json, const char *module_name);

private:
    std::string m_config_dir;
    std::vector<VendorItem> m_vendors;
    QJsonDocument m_json;
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
 */

