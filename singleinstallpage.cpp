#include "singleinstallpage.h"
#include "deblistmodel.h"

#include <QVBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QApplication>

#include <QApt/DebFile>
#include <QApt/Transaction>

using QApt::DebFile;
using QApt::Transaction;


const QString holdTextInRect(const QFontMetrics &fm, const QString &text, const QRect &rect)
{
    const int textFlag = Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop;

    if (rect.contains(fm.boundingRect(rect, textFlag, text)))
        return text;

    QString str(text + "...");

    while (true)
    {
        if (str.size() < 4)
            break;

        QRect boundingRect = fm.boundingRect(rect, textFlag, str);
        if (rect.contains(boundingRect))
            break;

        str.remove(str.size() - 4, 1);
    }

    return str;
}

SingleInstallPage::SingleInstallPage(DebListModel *model, QWidget *parent)
    : QWidget(parent),

      m_packagesModel(model),

      m_packageIcon(new QLabel),
      m_packageName(new QLabel),
      m_packageVersion(new QLabel),
      m_packageDescription(new QLabel),
      m_tipsLabel(new QLabel),
      m_progress(new QProgressBar),
      m_installButton(new QPushButton),
      m_uninstallButton(new QPushButton),
      m_reinstallButton(new QPushButton),
      m_confirmButton(new QPushButton)
{
    m_packageIcon->setText("icon");
    m_packageIcon->setFixedSize(64, 64);
    m_packageName->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    m_packageVersion->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_tipsLabel->setAlignment(Qt::AlignCenter);
    m_tipsLabel->setStyleSheet("QLabel {"
                               "color: red;"
                               "}");

    m_progress->setMinimum(0);
    m_progress->setMaximum(100);
    m_progress->setFixedHeight(8);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);

    m_installButton->setText(tr("Install"));
    m_uninstallButton->setText(tr("Remove"));
    m_reinstallButton->setText(tr("Reinstall"));
    m_confirmButton->setText(tr("OK"));
    m_packageDescription->setWordWrap(true);
    m_packageDescription->setMaximumHeight(80);
    m_packageDescription->setFixedWidth(220);

    QLabel *packageName = new QLabel;
    packageName->setText(tr("Package: "));
    packageName->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

    QLabel *packageVersion = new QLabel;
    packageVersion->setText(tr("Version: "));
    packageVersion->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    QGridLayout *itemInfoLayout = new QGridLayout;
    itemInfoLayout->addWidget(packageName, 0, 0);
    itemInfoLayout->addWidget(m_packageName, 0, 1);
    itemInfoLayout->addWidget(packageVersion, 1, 0);
    itemInfoLayout->addWidget(m_packageVersion, 1, 1);
    itemInfoLayout->setSpacing(0);
    itemInfoLayout->setVerticalSpacing(10);
    itemInfoLayout->setMargin(0);

    QHBoxLayout *itemLayout = new QHBoxLayout;
    itemLayout->addStretch();
    itemLayout->addWidget(m_packageIcon);
    itemLayout->addLayout(itemInfoLayout);
    itemLayout->addStretch();
    itemLayout->setSpacing(10);
    itemLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *btnsLayout = new QHBoxLayout;
    btnsLayout->addStretch();
    btnsLayout->addWidget(m_installButton);
    btnsLayout->addWidget(m_uninstallButton);
    btnsLayout->addWidget(m_reinstallButton);
    btnsLayout->addWidget(m_confirmButton);
    btnsLayout->addStretch();
    btnsLayout->setSpacing(30);
    btnsLayout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->addStretch();
    contentLayout->addLayout(itemLayout);
    contentLayout->addSpacing(30);
    contentLayout->addWidget(m_packageDescription);
    contentLayout->addStretch();
    contentLayout->addWidget(m_tipsLabel);
    contentLayout->addWidget(m_progress);
    contentLayout->addSpacing(15);
    contentLayout->addLayout(btnsLayout);
    contentLayout->setSpacing(0);
    contentLayout->setMargin(0);

    QHBoxLayout *centralLayout = new QHBoxLayout;
    centralLayout->addStretch();
    centralLayout->addLayout(contentLayout);
    centralLayout->addStretch();
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(100, 0, 100, 20);

    setLayout(centralLayout);

    connect(m_installButton, &QPushButton::clicked, this, &SingleInstallPage::install);
    connect(m_reinstallButton, &QPushButton::clicked, this, &SingleInstallPage::install);
    connect(m_uninstallButton, &QPushButton::clicked, this, &SingleInstallPage::uninstallCurrentPackage);
    connect(m_confirmButton, &QPushButton::clicked, qApp, &QApplication::quit);

    connect(model, &DebListModel::workerFinished, this, &SingleInstallPage::workerFinished);
    connect(model, &DebListModel::workerStarted, this, &SingleInstallPage::workerStarted);
    connect(model, &DebListModel::transactionProgressChanged, this, &SingleInstallPage::onWorkerProgressChanged);

    QTimer::singleShot(1, this, &SingleInstallPage::setPackageInfo);
}

void SingleInstallPage::install()
{
    m_packagesModel->installAll();
}

void SingleInstallPage::uninstallCurrentPackage()
{
    m_packagesModel->uninstallPackage(0);
}

void SingleInstallPage::workerStarted()
{
    m_progress->setVisible(true);
    m_progress->setValue(0);
    m_tipsLabel->clear();

    m_installButton->setVisible(false);
    m_reinstallButton->setVisible(false);
    m_uninstallButton->setVisible(false);
    m_confirmButton->setVisible(false);
}

void SingleInstallPage::workerFinished()
{
    m_progress->setVisible(false);
    m_confirmButton->setVisible(true);
}

void SingleInstallPage::onWorkerProgressChanged(const int progress)
{
    m_progress->setValue(progress);
}

void SingleInstallPage::setPackageInfo()
{
    DebFile *package = m_packagesModel->preparedPackages().first();

    const QIcon icon = QIcon::fromTheme("application-vnd.debian.binary-package", QIcon::fromTheme("debian-swirl"));

    m_packageIcon->setPixmap(icon.pixmap(m_packageIcon->size()));
    m_packageName->setText(package->packageName());
    m_packageVersion->setText(package->version());

    // set package description
    const QRect boundingRect = QRect(0, 0, m_packageDescription->width(), m_packageDescription->maximumHeight());
    const QFontMetrics fm(m_packageDescription->font());
    m_packageDescription->setText(holdTextInRect(fm, package->longDescription(), boundingRect));

    // package install status
    const QModelIndex index = m_packagesModel->index(0);
    const int installStat = index.data(DebListModel::PackageVersionStatusRole).toInt();

    const bool installed = installStat != DebListModel::NotInstalled;
    const bool installedSameVersion = installStat == DebListModel::InstalledSameVersion;
    m_installButton->setVisible(!installed || !installedSameVersion);
    m_uninstallButton->setVisible(installedSameVersion);
    m_reinstallButton->setVisible(installedSameVersion);
    m_confirmButton->setVisible(false);

    if (installed)
    {
        if (!installedSameVersion)
            m_tipsLabel->setText(tr("Other version installed"));
        else
            m_tipsLabel->setText(tr("Same version installed"));

        return;
    }

    // package depends status
    const int dependsStat = index.data(DebListModel::PackageDependsStatusRole).toInt();
    if (dependsStat == DebListModel::DependsBreak)
    {
        m_tipsLabel->setText(tr("Broken Dependencies"));
        m_installButton->setVisible(false);
        m_reinstallButton->setVisible(false);
        m_confirmButton->setVisible(true);
    }
}
