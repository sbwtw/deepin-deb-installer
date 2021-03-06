#include "packagesmanager.h"
#include "deblistmodel.h"

#include <QtConcurrent>

using namespace QApt;

QString relationName(const RelationType type)
{
    switch (type)
    {
    case LessOrEqual:       return "<=";
    case GreaterOrEqual:    return ">=";
    case LessThan:          return "<";
    case GreaterThan:       return ">";
    case Equals:            return "=";
    case NotEqual:          return "!=";
    default:;
    }

    return QString();
}

bool dependencyVersionMatch(const int result, const RelationType relation)
{
    switch (relation)
    {
    case LessOrEqual:       return result <= 0;
    case GreaterOrEqual:    return result >= 0;
    case LessThan:          return result < 0;
    case GreaterThan:       return result > 0;
    case Equals:            return result == 0;
    case NotEqual:          return result != 0;
    default:;
    }

    return true;
}

Backend *init_backend()
{
    Backend *b = new Backend;

    if (b->init())
        return b;

    qFatal("%s", b->initErrorMessage().toStdString().c_str());
    return nullptr;
}

PackagesManager::PackagesManager(QObject *parent)
    : QObject(parent)
{
    m_backendFuture = QtConcurrent::run(init_backend);
}

int PackagesManager::packageInstallStatus(const int index)
{
    if (m_packageInstallStatus.contains(index))
        return m_packageInstallStatus[index];

    const QString packageName = m_preparedPackages[index]->packageName();
    Backend *b = m_backendFuture.result();
    Package *p = b->package(packageName);

    int ret;
    do {
        const QString installedVersion = p->installedVersion();
        if (installedVersion.isEmpty())
        {
            ret = DebListModel::NotInstalled;
            break;
        }

        const QString packageVersion = m_preparedPackages[index]->version();
        const int result = Package::compareVersion(packageVersion, installedVersion);

        if (result == 0)
            ret = DebListModel::InstalledSameVersion;
        else if (result > 0)
            ret = DebListModel::InstalledLaterVersion;
        else
            ret = DebListModel::InstalledEarlierVersion;
    } while (false);

    m_packageInstallStatus.insert(index, ret);
    return ret;
}

int PackagesManager::packageDependsStatus(const int index)
{
    if (m_packageDependsStatus.contains(index))
        return m_packageDependsStatus[index];

    DebFile *deb = m_preparedPackages[index];

    int ret = DebListModel::DependsOk;

//    qDebug() << "conflicts:";
//    const auto &conflicts = deb->conflicts();
//    for (auto const &item : conflicts)
//    {
//        for (auto const &info : item)
//        {
//            qDebug() << info.packageName() << info.packageVersion();
//        }
//    }

    qDebug() << "depends:";
    const auto &depends = deb->depends();
    for (auto const &item : depends)
    {
        int tr = DebListModel::DependsBreak;
        for (auto const &info : item)
        {
            const int r = checkDependsPackageStatus(info);
            tr = std::min(r, tr);
        }
        ret = std::max(tr, ret);
        if (ret == DebListModel::DependsBreak)
            return ret;
    }

    return ret;
}

const QString PackagesManager::packageInstalledVersion(const int index)
{
    Q_ASSERT(m_packageInstallStatus.contains(index));
    Q_ASSERT(m_packageInstallStatus[index] == DebListModel::InstalledEarlierVersion ||
             m_packageInstallStatus[index] == DebListModel::InstalledLaterVersion);

    Backend *b = m_backendFuture.result();
    Package *p = b->package(m_preparedPackages[index]->packageName());

    return p->installedVersion();
}

int PackagesManager::checkDependsPackageStatus(const DependencyInfo &dependencyInfo)
{
    qDebug() << DependencyInfo::typeName(dependencyInfo.dependencyType())
             << dependencyInfo.packageName()
             << relationName(dependencyInfo.relationType())
             << dependencyInfo.packageVersion();

    Backend *b = m_backendFuture.result();
    Package *p = b->package(dependencyInfo.packageName());

    // package not found
    if (!p)
        return DebListModel::DependsBreak;

    if (dependencyInfo.packageVersion().isEmpty())
        return DebListModel::DependsOk;

    const RelationType relation = dependencyInfo.relationType();
    const QString &installedVersion = p->installedVersion();

    if (!installedVersion.isEmpty())
    {
        const int result = Package::compareVersion(installedVersion, dependencyInfo.packageVersion());
        if (dependencyVersionMatch(result, relation))
            return DebListModel::DependsOk;
        else
            return DebListModel::DependsBreak;
    } else {
        const int result = Package::compareVersion(p->version(), dependencyInfo.packageVersion());
        if (dependencyVersionMatch(result, relation))
            return DebListModel::DependsAvailable;
        else
            return DebListModel::DependsBreak;
    }
}
