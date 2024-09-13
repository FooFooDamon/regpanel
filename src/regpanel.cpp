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

    scan_config_directory(config_dir);

    emit this->lstVendor->currentIndexChanged(-1);
    emit this->lstAddrBaseMethod->currentIndexChanged(this->lstAddrBaseMethod->currentIndex());
    emit this->chkboxAsInput->stateChanged(this->chkboxAsInput->isChecked());
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

    if (!this->chkboxAsInput->isChecked())
    {
        if (this->refresh_register_tables(this->json(), this->lstModule->currentText().toStdString().c_str()))
        {
            this->info_box("Refresh",
                QString("Refreshed register tables for module:\n\n") + this->lstModule->currentText());
        }
        else
        {
            this->error_box("Refresh",
                QString("Failed to refresh register tables for module:\n\n") + this->lstModule->currentText());
        }
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

#if 0
    if (this->lstModule->currentIndex() >= 0 && !this->chkboxAsInput->isChecked())
    {
        if (this->refresh_register_tables(this->json(), this->lstModule->currentText().toStdString().c_str()))
        {
            this->info_box("Refresh",
                QString("Refreshed register tables for module:\n\n") + this->lstModule->currentText());
        }
        else
        {
            this->error_box("Refresh",
                QString("Failed to refresh register tables for module:\n\n") + this->lstModule->currentText());
        }
    }
#endif
}

void RegPanel::on_btnConvert_clicked(void)
{
    this->warning_box("Unimplemented", "TODO");
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

bool RegPanel::refresh_register_tables(const QJsonDocument &json, const char *module_name)
{
    this->warning_box("Unimplemented", __FUNCTION__);

    return false;
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
 */

