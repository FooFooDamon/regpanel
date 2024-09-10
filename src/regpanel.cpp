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

const QTextCodec *G_TEXT_CODEC = QTextCodec::codecForName("UTF8"/*"GB2312"*/);

RegPanel::RegPanel(const char *config_dir, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    this->setFixedSize(this->geometry().size());
    this->setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    this->setWindowTitle(QString::asprintf("%s [%s]", this->windowTitle().toStdString().c_str(), FULL_VERSION()));

    QDir dir(config_dir);

    if (!dir.exists())
    {
        QMessageBox::critical(parent, this->windowTitle(),
            QString::asprintf("Directory does not exist:\n\n%s", config_dir));

        //QApplication::exit(EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
}

void RegPanel::on_btnConvert_clicked(void)
{
    // TODO
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
 */

