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
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDebug>

class DRegExp : public QRegExp
{
public:
    explicit DRegExp(const QString &pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive,
                     PatternSyntax syntax = RegExp)
        : QRegExp(pattern, cs, syntax)
    {

    }

    bool exactMatch(const QString &str) const
    {
        if (!isNot)
            return QRegExp::exactMatch(str);

        return !QRegExp::exactMatch(str);
    }

    DRegExp(const DRegExp &rx)
        : QRegExp(rx)
    {
        isNot = rx.isNot;
    }

    DRegExp &operator=(const DRegExp &rx)
    {
        QRegExp::operator =(rx);
        isNot = rx.isNot;
    }

    bool isNot = false;
};

static QList<DRegExp> white_list;
static bool exec = false;
static QDir::Filters flags = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;

void removeFile(const QString &file, bool onlyFile, bool onlyDir)
{
    QFileInfo info(file);

    if (info.isDir() && onlyFile)
        return;

    if (info.isFile() && onlyDir)
        return;

    for (const DRegExp &re : white_list) {
        if (re.exactMatch(file)) {
            qDebug() << "Matched by the white list, pattern:" << re.pattern() << "file:" << file << "is not:" << re.isNot;

            return;
        }
    }

    if (info.isDir()) {
        for (const QFileInfo &f : QDir(file).entryInfoList(flags))
            removeFile(f.absoluteFilePath(), onlyFile, onlyDir);
    }

    if (info.exists()) {
        qInfo() << "will do remove:" << file;

        if (!exec)
            return;

        if (info.isDir()) {
            if (!QDir::current().rmdir(file)) {
                qWarning() << "Faile remove dir:" << file;
            }
        } else {
            QFile tmp_file(file);

            if (!tmp_file.remove(file)) {
                qWarning() << "Faile remove file:" << file << "error:" << tmp_file.errorString();
            }
        }

        return;
    }

    for (const QFileInfo &f : info.absoluteDir().entryInfoList(QStringList() << info.fileName(),
                                                               flags)) {
        removeFile(f.absoluteFilePath(), onlyFile, onlyDir);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    for (const QString &arg : app.arguments()) {
        if (QStringLiteral("--exec") == arg)
            exec = true;
        else if (QStringLiteral("--hidden") == arg)
            flags |= QDir::Hidden;
    }

    QFile white_list_file(CONFIG_PATH "/white.txt");
    QFile black_list_file(CONFIG_PATH "/black.txt");

    white_list << DRegExp(white_list_file.fileName(), Qt::CaseSensitive, QRegExp::Wildcard);
    white_list << DRegExp(black_list_file.fileName(), Qt::CaseSensitive, QRegExp::Wildcard);
    white_list << DRegExp(app.applicationFilePath(), Qt::CaseSensitive, QRegExp::Wildcard);

    qDebug() << "+++++white list begin+++++";

    if (white_list_file.open(QFile::ReadOnly)) {
        while (!white_list_file.atEnd()) {
            QByteArray line_data = white_list_file.readLine().trimmed();

            if (line_data.isEmpty() || line_data.startsWith("#") || line_data.startsWith("//")) {
                qDebug() << "Skip:" << QString::fromUtf8(line_data);

                continue;
            }

            qDebug() << QString::fromUtf8(line_data);

            bool is_not = false;

            if (line_data.startsWith("!")) {
                is_not = true;
                line_data = line_data.mid(1);
            } else if (line_data.startsWith("=")) {
                line_data = QRegExp::escape(QString::fromUtf8(line_data.mid(1))).toUtf8();
            }

            if (!line_data.startsWith("~/") && !line_data.startsWith("/")) {
                qWarning() << "!!!---Invalid Row---!!!";

                continue;
            }

            const QString &line = QString::fromUtf8(line_data);

            DRegExp re(line.startsWith("~/") ? QDir::home().absoluteFilePath(line.mid(2)) : line,
                       Qt::CaseSensitive, QRegExp::Wildcard);

            re.isNot = is_not;
            white_list << re;
        }
    }

    qDebug() << "+++++white list end+++++";
    qDebug() << "-----black list begin-----";

    if (black_list_file.open(QFile::ReadOnly)) {
        while (!black_list_file.atEnd()) {
            QByteArray line_data = black_list_file.readLine().trimmed();

            if (line_data.isEmpty() || line_data.startsWith("#") || line_data.startsWith("//")) {
                qDebug() << "Skip:" << QString::fromUtf8(line_data);

                continue;
            }

            qDebug() << QString::fromUtf8(line_data);

            bool only_file = false;
            bool only_dir = false;

            if (line_data.startsWith("f")) {
                only_file = true;
                line_data = line_data.mid(1);
            } else if (line_data.startsWith("d")) {
                only_dir = true;
                line_data = line_data.mid(1);
            }

            if (!line_data.startsWith("~/") && !line_data.startsWith("/")) {
                qWarning() << "!!!---Invalid Row---!!!";

                continue;
            }

            const QString &line = QString::fromUtf8(line_data);

            if (line.startsWith("~/"))
                removeFile(QDir::home().absoluteFilePath(line.mid(2)), only_file, only_dir);
            else
                removeFile(line, only_file, only_dir);
        }
    }

    qDebug() << "-----black list end-----";

    return 0;
}
