/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QFile>
#include <QDir>
#include <QDebug>

static QList<QRegExp> white_list;
static bool exec = false;
static QDir::Filters flags = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;

void removeFile(const QString &file)
{
    for (const QRegExp &re : white_list) {
        if (re.exactMatch(file)) {
            qDebug() << "Matched by the white list, pattern:" << re.pattern() << "file:" << file;

            return;
        }
    }

    QFileInfo info(file);

    if (info.isDir()) {
        for (const QFileInfo &f : QDir(file).entryInfoList(flags))
            removeFile(f.absoluteFilePath());
    }

    if (info.exists()) {
        qInfo() << "will remove:" << file;

        if (!exec)
            return;

        if (!QFile::remove(file)) {
            qWarning() << "Faile remove file:" << file;
        }

        return;
    }

    for (const QFileInfo &f : info.absoluteDir().entryInfoList(QStringList() << info.fileName(),
                                                               flags)) {
        removeFile(f.absoluteFilePath());
    }
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (QByteArray("--exec") == argv[i])
            exec = true;
        else if (QByteArray("--hidden") == argv[i])
            flags |= QDir::Hidden;
    }

    if (argc > 1 && argv[1] == QByteArray("--exec"))
        exec = true;

    QFile white_list_file(QDir::home().absoluteFilePath(".config/auto_clean/white.txt"));
    QFile black_list_file(QDir::home().absoluteFilePath(".config/auto_clean/black.txt"));

    qDebug() << "+++++white list begin+++++";

    if (white_list_file.open(QFile::ReadOnly)) {
        while (!white_list_file.atEnd()) {
            const QString &line = QString::fromUtf8(white_list_file.readLine().trimmed());

            if (line.isEmpty() || line.startsWith("#") || line.startsWith("//"))
                continue;

            qDebug() << line;

            QRegExp re(line.startsWith("~/") ? QDir::home().absoluteFilePath(line.mid(2)) : line,
                       Qt::CaseSensitive, QRegExp::Wildcard);

            white_list << re;
        }
    }

    qDebug() << "+++++white list end+++++";
    qDebug() << "-----black list begin-----";

    if (black_list_file.open(QFile::ReadOnly)) {
        while (!black_list_file.atEnd()) {
            const QString &line = QString::fromUtf8(black_list_file.readLine().trimmed());

            if (line.isEmpty() || line.startsWith("#") || line.startsWith("//"))
                continue;

            qDebug() << line;

            if (line.startsWith("~/"))
                removeFile(QDir::home().absoluteFilePath(line.mid(2)));
            else
                removeFile(line);
        }
    }

    qDebug() << "-----black list end-----";

    return 0;
}
