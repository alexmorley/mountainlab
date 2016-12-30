/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 7/6/2016
*******************************************************/

#include "mlcommon.h"
#include "cachemanager/cachemanager.h"
#include "taskprogress/taskprogress.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QThread>
#include <QCoreApplication>
#include <QUrl>
#include <QDir>
#include <QCryptographicHash>
#include <math.h>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QJsonArray>
#include <QSettings>
#include "mlnetwork.h"

#ifdef QT_GUI_LIB
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QMessageBox>
#endif

QString system_call_return_output(QString cmd);

QString TextFile::read(const QString& fname, QTextCodec* codec)
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream ts(&file);
    if (codec != 0)
        ts.setCodec(codec);
    QString ret = ts.readAll();
    file.close();
    return ret;
}

bool TextFile::write(const QString& fname, const QString& txt, QTextCodec* codec)
{
    /*
     * Modification on 5/23/16 by jfm
     * We don't want an program to try to read this while we have only partially completed writing the file.
     * Therefore we now create a temporary file and then copy it over
     */

    QString tmp_fname = fname + ".tf." + MLUtil::makeRandomId(6) + ".tmp";

    //if a file with this name already exists, we need to remove it
    //(should we really do this before testing whether writing is successful? I think yes)
    if (QFile::exists(fname)) {
        if (!QFile::remove(fname)) {
            qWarning() << "Problem in TextFile::write. Could not remove file even though it exists" << fname;
            return false;
        }
    }

    //write text to temporary file
    QFile file(tmp_fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Problem in TextFile::write. Could not open for writing... " << tmp_fname;
        return false;
    }
    QTextStream ts(&file);
    if (codec != 0) {
        ts.setAutoDetectUnicode(false);
        ts.setCodec(codec);
    }
    ts << txt;
    ts.flush();
    file.close();

    //check the contents of the file (is this overkill?)
    QString txt_test = TextFile::read(tmp_fname, codec);
    if (txt_test != txt) {
        QFile::remove(tmp_fname);
        qWarning() << "Problem in TextFile::write. The contents of the file do not match what was expected." << fname;
        return false;
    }

    //finally, rename the file
    if (!QFile::rename(tmp_fname, fname)) {
        qWarning() << "Problem in TextFile::write. Unable to rename file at the end of the write command" << fname;
        return false;
    }

    return true;
}

QChar make_random_alphanumeric()
{
    static int val = 0;
    val++;
    QTime time = QTime::currentTime();
    QString code = time.toString("hh:mm:ss:zzz");
    code += QString::number(qrand() + val);
    code += QString::number(QCoreApplication::applicationPid());
    code += QString::number((long)QThread::currentThreadId());
    int num = qHash(code);
    if (num < 0)
        num = -num;
    num = num % 36;
    if (num < 26)
        return QChar('A' + num);
    else
        return QChar('0' + num - 26);
}
QString make_random_id_22(int numchars)
{
    QString ret;
    for (int i = 0; i < numchars; i++) {
        ret.append(make_random_alphanumeric());
    }
    return ret;
}

QString MLUtil::makeRandomId(int numchars)
{
    return make_random_id_22(numchars);
}

bool MLUtil::threadInterruptRequested()
{
    return QThread::currentThread()->isInterruptionRequested();
}

bool MLUtil::inGuiThread()
{
    return (QThread::currentThread() == QCoreApplication::instance()->thread());
}

QString find_ancestor_path_with_name(QString path, QString name)
{
    if (name.isEmpty())
        return "";
    while (QFileInfo(path).fileName() != name) {
        path = QFileInfo(path).path();
        if (!path.contains(name))
            return ""; //guarantees that we eventually exit the while loop
    }
    return path; //the directory name must equal the name argument
}

QString MLUtil::mountainlabBasePath()
{
    return find_ancestor_path_with_name(qApp->applicationDirPath(), "mountainlab");
}

void mkdir_if_doesnt_exist(const QString& path)
{
    if (!QDir(path).exists()) {
        QDir(QFileInfo(path).path()).mkdir(QFileInfo(path).fileName());
    }
}

QString MLUtil::mlLogPath()
{
    QString ret = mountainlabBasePath() + "/log";
    mkdir_if_doesnt_exist(ret);
    return ret;
}

QString MLUtil::resolvePath(const QString& basepath, const QString& path)
{
    if (QFileInfo(path).isRelative()) {
        return basepath + "/" + path;
    }
    else
        return path;
}

void MLUtil::mkdirIfNeeded(const QString& path)
{
    mkdir_if_doesnt_exist(path);
}

#include "sumit.h"
QString MLUtil::computeSha1SumOfFile(const QString& path)
{
    //printf("Looking up sha1: %s\n",path.toUtf8().data());
    return sumit(path, 0, MLUtil::tempPath());
    /*
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return "";
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    file.close();
    QString ret = QString(hash.result().toHex());
    return ret;
    */
}
QString MLUtil::computeSha1SumOfFileHead(const QString& path, long num_bytes)
{
    return sumit(path, num_bytes, MLUtil::tempPath());
}

static QString s_temp_path = "";
QString MLUtil::tempPath()
{
    if (!s_temp_path.isEmpty())
        return s_temp_path;

    QString tmp = MLUtil::configResolvedPath("general", "temporary_path");
    if (!QDir(tmp).mkpath("mountainlab")) {
        qCritical() << "Unable to create temporary directory: " + tmp + "/mountainlab";
        abort();
    }

    s_temp_path = tmp + "/mountainlab";
    return s_temp_path;
}

QVariant clp_string_to_variant(const QString& str);

CLParams::CLParams(int argc, char* argv[])
{
    this->success = true; //let's be optimistic!

    //find the named and unnamed parameters checking for errors along the way
    for (int i = 1; i < argc; i++) {
        QString str = QString(argv[i]);
        if (str.startsWith("--")) {
            int ind2 = str.indexOf("=");
            QString name = str.mid(2, ind2 - 2);
            QString val = "";
            if (ind2 >= 0)
                val = str.mid(ind2 + 1);
            if (name.isEmpty()) {
                this->success = false;
                this->error_message = "Problem with parameter: " + str;
                return;
            }
            QVariant val2 = clp_string_to_variant(val);
            if (this->named_parameters.contains(name)) {
                QVariant tmp = this->named_parameters[name];
                QVariantList list;
                if (tmp.type() == QVariant::List) {
                    list = tmp.toList();
                }
                else {
                    list.append(tmp);
                }
                list.append(val2);
                this->named_parameters[name] = list;
            }
            else {
                this->named_parameters[name] = val2;
            }
        }
        else {
            this->unnamed_parameters << str;
        }
    }
}

bool clp_is_long(const QString& str)
{
    bool ok;
    str.toLongLong(&ok);
    return ok;
}

bool clp_is_int(const QString& str)
{
    bool ok;
    str.toInt(&ok);
    return ok;
}

bool clp_is_float(const QString& str)
{
    bool ok;
    str.toFloat(&ok);
    return ok;
}

QVariant clp_string_to_variant(const QString& str)
{
    if (clp_is_long(str))
        return str.toLongLong();
    if (clp_is_int(str))
        return str.toInt();
    if (clp_is_float(str))
        return str.toFloat();
    return str;
}

double MLCompute::min(const QVector<double>& X)
{
    return *std::min_element(X.constBegin(), X.constEnd());
}

double MLCompute::max(const QVector<double>& X)
{
    return *std::max_element(X.constBegin(), X.constEnd());
}

double MLCompute::sum(const QVector<double>& X)
{
    return std::accumulate(X.constBegin(), X.constEnd(), 0.0);
}

double MLCompute::mean(const QVector<double>& X)
{
    if (X.isEmpty())
        return 0;
    double s = sum(X);
    return s / X.count();
}

double MLCompute::stdev(const QVector<double>& X)
{
    double sumsqr = std::inner_product(X.constBegin(), X.constEnd(), X.constBegin(), 0.0);
    double sum = std::accumulate(X.constBegin(), X.constEnd(), 0.0);
    int ct = X.count();
    if (ct >= 2) {
        return sqrt((sumsqr - sum * sum / ct) / (ct - 1));
    }
    else
        return 0;
}

double MLCompute::dotProduct(const QVector<double>& X1, const QVector<double>& X2)
{
    if (X1.count() != X2.count())
        return 0;
    return std::inner_product(X1.constBegin(), X1.constEnd(), X2.constBegin(), 0.0);
}

double MLCompute::norm(const QVector<double>& X)
{
    return sqrt(dotProduct(X, X));
}

double MLCompute::dotProduct(const QVector<float>& X1, const QVector<float>& X2)
{
    if (X1.count() != X2.count())
        return 0;
    return std::inner_product(X1.constBegin(), X1.constEnd(), X2.constBegin(), 0.0);
}

double MLCompute::norm(const QVector<float>& X)
{
    return sqrt(dotProduct(X, X));
}

double MLCompute::correlation(const QVector<double>& X1, const QVector<double>& X2)
{
    if (X1.count() != X2.count())
        return 0;
    long N = X1.count();
    double mean1 = mean(X1);
    double stdev1 = stdev(X1);
    double mean2 = mean(X2);
    double stdev2 = stdev(X2);
    if ((stdev1 == 0) || (stdev2 == 0))
        return 0;
    QVector<double> Y1(N);
    QVector<double> Y2(N);
    for (long i = 0; i < N; i++) {
        Y1[i] = (X1[i] - mean1) / stdev1;
        Y2[i] = (X2[i] - mean2) / stdev2;
    }
    return dotProduct(Y1, Y2);
}

double MLCompute::norm(long N, const double* X)
{
    return sqrt(dotProduct(N, X, X));
}

double MLCompute::dotProduct(long N, const double* X1, const double* X2)
{
    return std::inner_product(X1, X1 + N, X2, 0.0);
}

double MLCompute::dotProduct(long N, const float* X1, const float* X2)
{

    return std::inner_product(X1, X1 + N, X2, 0.0);
}

QString MLUtil::computeSha1SumOfString(const QString& str)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(str.toLatin1());
    return QString(hash.result().toHex());
}

double MLCompute::sum(long N, const double* X)
{
    return std::accumulate(X, X + N, 0.0);
}

double MLCompute::mean(long N, const double* X)
{
    if (!N)
        return 0;
    return sum(N, X) / N;
}

double MLCompute::max(long N, const double* X)
{
    return N ? *std::max_element(X, X + N) : 0;
}

double MLCompute::min(long N, const double* X)
{
    return N ? *std::min_element(X, X + N) : 0;
}

QList<int> MLUtil::stringListToIntList(const QStringList& list)
{
    QList<int> ret;
    ret.reserve(list.size());
    foreach (QString str, list) {
        ret << str.toInt();
    }
    return ret;
}

QStringList MLUtil::intListToStringList(const QList<int>& list)
{
    QStringList ret;
    ret.reserve(list.size());
    foreach (int a, list) {
        ret << QString::number(a);
    }
    return ret;
}

void MLUtil::fromJsonValue(QByteArray& X, const QJsonValue& val)
{
    X = QByteArray::fromBase64(val.toString().toLatin1());
}

void MLUtil::fromJsonValue(QList<int>& X, const QJsonValue& val)
{
    X.clear();
    QByteArray ba;
    MLUtil::fromJsonValue(ba, val);
    QDataStream ds(&ba, QIODevice::ReadOnly);
    while (!ds.atEnd()) {
        int val;
        ds >> val;
        X << val;
    }
}

void MLUtil::fromJsonValue(QVector<int>& X, const QJsonValue& val)
{
    X.clear();
    QByteArray ba;
    MLUtil::fromJsonValue(ba, val);
    QDataStream ds(&ba, QIODevice::ReadOnly);
    while (!ds.atEnd()) {
        int val;
        ds >> val;
        X << val;
    }
}

void MLUtil::fromJsonValue(QVector<double>& X, const QJsonValue& val)
{
    X.clear();
    QByteArray ba;
    MLUtil::fromJsonValue(ba, val);
    QDataStream ds(&ba, QIODevice::ReadOnly);
    while (!ds.atEnd()) {
        double val;
        ds >> val;
        X << val;
    }
}

QJsonValue MLUtil::toJsonValue(const QByteArray& X)
{
    return QString(X.toBase64());
}

QJsonValue MLUtil::toJsonValue(const QList<int>& X)
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    for (int i = 0; i < X.count(); i++) {
        ds << X[i];
    }
    return toJsonValue(ba);
}

QJsonValue MLUtil::toJsonValue(const QVector<int>& X)
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    for (int i = 0; i < X.count(); i++) {
        ds << X[i];
    }
    return toJsonValue(ba);
}

QJsonValue MLUtil::toJsonValue(const QVector<double>& X)
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    for (int i = 0; i < X.count(); i++) {
        ds << X[i];
    }
    return toJsonValue(ba);
}

QByteArray MLUtil::readByteArray(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray ret = file.readAll();
    file.close();
    return ret;
}

bool MLUtil::writeByteArray(const QString& path, const QByteArray& X)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open file for writing byte array: " + path;
        return false;
    }
    if (file.write(X) != X.count()) {
        qWarning() << "Problem writing byte array: " + path;
        return false;
    }
    file.close();
    return true;
}

double MLCompute::min(long N, const float* X)
{
    return N ? *std::min_element(X, X + N) : 0;
}

double MLCompute::max(long N, const float* X)
{
    return N ? *std::max_element(X, X + N) : 0;
}

double MLCompute::sum(long N, const float* X)
{
    return std::accumulate(X, X + N, 0.0);
}

double MLCompute::mean(long N, const float* X)
{
    if (!N)
        return 0;
    return sum(N, X) / N;
}

double MLCompute::norm(long N, const float* X)
{
    return sqrt(dotProduct(N, X, X));
}

double MLCompute::median(const QVector<double>& X)
{
    if (X.isEmpty())
        return 0;
    QVector<double> Y = X;
    qSort(Y);
    if (Y.count() % 2 == 0) {
        return (Y[Y.count() / 2 - 1] + Y[Y.count() / 2]) / 2;
    }
    else {
        return Y[Y.count() / 2];
    }
}

/////////////////////////////////////////////////////////////////////////////

QString make_temporary_output_file_name(QString processor_name, QMap<QString, QVariant> args_inputs, QMap<QString, QVariant> args_parameters, QString output_pname)
{
    QJsonObject tmp;
    tmp["processor_name"] = processor_name;
    tmp["inputs"] = QJsonObject::fromVariantMap(args_inputs);
    tmp["parameters"] = QJsonObject::fromVariantMap(args_parameters);
    tmp["output_pname"] = output_pname;
    QString tmp_json = QJsonDocument(tmp).toJson();
    QString code = MLUtil::computeSha1SumOfString(tmp_json);
    return CacheManager::globalInstance()->makeLocalFile(code + "-prv-" + output_pname);
}

void run_process(QString processor_name, QMap<QString, QVariant> inputs, QMap<QString, QVariant> outputs, QMap<QString, QVariant> parameters, bool force_run)
{
    TaskProgress task("Running: " + processor_name);
    QStringList args;
    args << "run-process" << processor_name;
    {
        QStringList keys = inputs.keys();
        foreach (QString key, keys) {
            args << QString("--%1=%2").arg(key).arg(inputs[key].toString());
        }
    }
    {
        QStringList keys = outputs.keys();
        foreach (QString key, keys) {
            args << QString("--%1=%2").arg(key).arg(outputs[key].toString());
        }
    }
    {
        QStringList keys = parameters.keys();
        foreach (QString key, keys) {
            args << QString("--%1=%2").arg(key).arg(parameters[key].toString());
        }
    }
    if (force_run) {
        args << "--_force_run";
    }
    QString exe = MLUtil::mountainlabBasePath() + "/mountainprocess/bin/mountainprocess";
    task.log() << "Running process:" << args.join(" ");
    QProcess P;
    P.start(exe, args);
    if (!P.waitForStarted()) {
        task.error() << "Problem starting process.";
        return;
    }
    while (!P.waitForFinished(100)) {
        if (MLUtil::inGuiThread()) {
            qApp->processEvents();
        }
    }
}

QString system_call_return_output(QString cmd)
{
    QProcess process;
    process.start(cmd);
    process.waitForStarted();
    process.waitForFinished(-1);
    return process.readAllStandardOutput().trimmed();
}

QString locate_file_with_checksum(QString checksum, QString fcs, long size, bool allow_downloads)
{
    QString extra_args = "";
    if (!allow_downloads)
        extra_args += "--local-only";
    QString cmd = QString("prv locate --checksum=%1 --fcs=%2 --size=%3 %4").arg(checksum).arg(fcs).arg(size).arg(extra_args);
    QString ret = system_call_return_output(cmd);
    //QStringList lines = ret.split("\n");
    //return lines.last();
    return ret;
}

QString download_file_to_temp_dir(QString url)
{
    QString tmp_fname = CacheManager::globalInstance()->makeLocalFile(MLUtil::computeSha1SumOfString(url) + "." + make_random_id_22(5) + ".download");
    QFile::remove(tmp_fname);
    QString cmd = QString("curl %1 -o %2").arg(url).arg(tmp_fname);
    system_call_return_output(cmd);
    if (!QFile::exists(tmp_fname))
        return "";
    return tmp_fname;
}

class Downloader : public QThread {
public:
    //input
    QString url;
    QString output_path;

    //output
    bool success = false;

    void run()
    {
        QFile::remove(output_path);

        QString cmd = QString("curl -s %1 -o %2").arg(url).arg(output_path);
        if (system(cmd.toUtf8().data()) != 0) {
            QFile::remove(output_path);
        }
        success = QFile::exists(output_path);
    }
};

QString concatenate_files_to_temporary_file(QStringList file_paths)
{
    /// Witold, this function should be improved by streaming the read/writes
    foreach (QString str, file_paths) {
        if (str.isEmpty())
            return "";
    }

    QString code = file_paths.join(";");
    QString tmp_fname = CacheManager::globalInstance()->makeLocalFile(MLUtil::computeSha1SumOfString(code) + "." + make_random_id_22(5) + ".concat.download");
    QFile f(tmp_fname);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << __FUNCTION__ << "Unable to open file for writing: " + tmp_fname;
        return "";
    }
    foreach (QString str, file_paths) {
        QFile g(str);
        if (!g.open(QIODevice::ReadOnly)) {
            qWarning() << __FUNCTION__ << "Unable to open file for reading: " + str;
            f.close();
            return "";
        }
        f.write(g.readAll());
        g.close();
    }

    f.close();
    return tmp_fname;
}

QString parallel_download_file_from_prvfileserver_to_temp_dir(QString url, long size, int num_downloads)
{

    QString tmp_fname = CacheManager::globalInstance()->makeLocalFile() + ".parallel_download";
    MLNetwork::PrvParallelDownloader downloader;
    downloader.destination_file_name = tmp_fname;
    downloader.size = size;
    downloader.source_url = url;
    downloader.num_threads = num_downloads;

    downloader.start();
    bool done = false;
    while (!done) {
        if (downloader.waitForFinished(100)) {
            done = true;
        }
        else {
            if (MLUtil::inGuiThread()) {
                qApp->processEvents();
            }
        }
    }

    if (downloader.success)
        return tmp_fname;
    else
        return "";
}

QString MLUtil::configResolvedPath(const QString& group, const QString& key)
{
    QString ret = MLUtil::configValue(group, key).toString();
    return QDir(MLUtil::mountainlabBasePath()).filePath(ret);
}

QStringList MLUtil::configResolvedPathList(const QString& group, const QString& key)
{
    QJsonArray array = MLUtil::configValue(group, key).toArray();
    QStringList ret;
    for (int i = 0; i < array.count(); i++) {
        ret << QDir(MLUtil::mountainlabBasePath()).filePath(array[i].toString());
    }
    return ret;
}

QJsonValue MLUtil::configValue(const QString& group, const QString& key)
{
    QString json1;
#ifdef Q_OS_LINUX
    /* lookup order:
     * ~/.config/mountainlab/
     * /etc/mountainlab/
     * /usr/local/share/mountainlab/
     * /usr/share/mountainlab/
     */
    /// TODO: Extend with QStandardPaths to make it platform independent
    static const QStringList globalDirs = {
        QDir::homePath()+"/.config/mountainlab",
        "/etc/mountainlab", "/usr/local/share/mountainlab", "/usr/share/mountainlab"
    };
    for(int i = 0; i < globalDirs.size() && json1.isEmpty(); ++i)
        json1 = TextFile::read(globalDirs.at(i) + "/mountainlab.default.json");
#endif
    if (json1.isEmpty())
        json1 = TextFile::read(MLUtil::mountainlabBasePath() + "/mountainlab.default.json");
    QJsonParseError err1;
    QJsonObject obj1 = QJsonDocument::fromJson(json1.toUtf8(), &err1).object();
    if (err1.error != QJsonParseError::NoError) {
        qWarning() << err1.errorString();
        qWarning() << "Error parsing mountainlab.default.json.";
        abort();
        return QJsonValue();
    }
    QJsonObject obj2;
    /* lookup order:
     * ~/.config/mountainlab/
     *
     *
     */
    static const QStringList userDirs = {
#ifdef Q_OS_LINUX
        QDir::homePath()+"/.config/mountainlab",
#endif
        MLUtil::mountainlabBasePath()
    };
    QString json2;
    for(int i = 0; i < userDirs.size() && json2.isEmpty(); ++i) {
        if (!QFile::exists(userDirs.at(i) + "/mountainlab.user.json"))
            continue;
        json2 = TextFile::read(userDirs.at(i) + "/mountainlab.user.json");
        // we want to break if the file exists, even if it is empty
        break;
    }
    if (!json2.isEmpty()) {
        QJsonParseError err2;
        obj2 = QJsonDocument::fromJson(json2.toUtf8(), &err2).object();
        if (err2.error != QJsonParseError::NoError) {
            qWarning() << err2.errorString();
            qWarning() << "Error parsing mountainlab.user.json.";
            abort();
            return QJsonValue();
        }
    }
    if (!group.isEmpty()) {
        obj1 = obj1[group].toObject();
        obj2 = obj2[group].toObject();
    }
    QJsonValue ret = obj1[key];
    if (obj2.contains(key)) {
        ret = obj2[key];
    }
    return ret;
}

QString locate_prv(const QJsonObject& obj)
{
    QString path0 = obj["original_path"].toString();
    QString checksum0 = obj["original_checksum"].toString();
    QString fcs0 = obj["original_fcs"].toString();
    long size0 = obj["original_size"].toVariant().toLongLong();
    QString ret = locate_file_with_checksum(checksum0, fcs0, size0, false);
    if (ret.isEmpty()) {
        if (QFile::exists(path0)) {
            if (QFileInfo(path0).size() == size0) {
                if (MLUtil::computeSha1SumOfFile(path0) == checksum0) {
                    qWarning() << "Using original path----- " + path0;
                    return path0;
                }
            }
        }
    }
    return ret;
}

QStringList MLUtil::toStringList(const QVariant& val)
{
    QStringList ret;
    if (val.type() == QVariant::List) {
        QVariantList list = val.toList();
        for (int i = 0; i < list.count(); i++) {
            ret << list[i].toString();
        }
    }
    else {
        ret << val.toString();
    }
    return ret;
}

bool MLUtil::matchesFastChecksum(QString path, QString fcs)
{
    if (fcs.isEmpty())
        return true;

    int ind0 = fcs.indexOf("-");
    QString fcs_name = fcs.mid(0, ind0);
    QString fcs_value = fcs.mid(ind0 + 1);

    if (fcs_name == "head1000") {
        return (MLUtil::computeSha1SumOfFileHead(path, 1000) == fcs_value);
    }
    else {
        qWarning() << "Unknown fcs name: " + fcs;
        return true;
    }
}
