/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2010-2014 by Andrea Diamantini <adjam7 at gmail dot com>
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* ============================================================ */


// Self Includes
#include "adblocksettingwidget.h"

// Auto Includes
#include "rekonq.h"

// KDE Includes
#include <KLocalizedString>
#include <KSharedConfig>

// Qt Includes
#include <QString>
#include <QStandardPaths>
#include <QIcon>

#include <QWhatsThis>
#include <QListWidgetItem>


AdBlockSettingWidget::AdBlockSettingWidget(KSharedConfig::Ptr config, QWidget *parent)
    : QWidget(parent)
    , _changed(false)
    , _adblockConfig(config)
{
    setupUi(this);

    // icon buttons
    okButton->setIcon( QIcon::fromTheme( QL1S("dialog-ok") ));
    cancelButton->setIcon( QIcon::fromTheme( QL1S("dialog-cancel") ));

    hintLabel->setText(i18n("<qt>Filter expression (e.g. <tt>http://www.example.com/ad/*</tt>, <a href=\"filterhelp\">more information</a>):"));
    connect(hintLabel, SIGNAL(linkActivated(QString)), this, SLOT(slotInfoLinkActivated(QString)));

    manualFiltersListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    tabWidget->setTabText(0, i18n("Automatic Filters"));
    tabWidget->setTabText(1, i18n("Manual Filters"));

    // FIXME
//     searchLine->setListWidget(manualFiltersListWidget);

    insertButton->setIcon(QIcon::fromTheme( QL1S("list-add") ));
    connect(insertButton, SIGNAL(clicked()), this, SLOT(insertRule()));

    removeButton->setIcon(QIcon::fromTheme( QL1S("list-remove") ));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeRule()));

    load();

    spinBox->setSuffix(QL1S(" ") + i18n("days"));

    // emit changed signal
    connect(checkEnableAdblock, SIGNAL(stateChanged(int)),   this, SLOT(hasChanged()));
    connect(checkHideAds,       SIGNAL(stateChanged(int)),   this, SLOT(hasChanged()));
    connect(spinBox,            SIGNAL(valueChanged(int)),   this, SLOT(hasChanged()));

    connect(automaticFiltersListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(hasChanged()));
    
    // always save on close
    connect(okButton, SIGNAL(clicked()), this, SLOT(saveAndClose()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
}


void AdBlockSettingWidget::slotInfoLinkActivated(const QString &url)
{
    Q_UNUSED(url)

    const QString hintHelpString = i18n("<qt><p>Enter an expression to filter. Filters can be defined as either:"
                                  "<ul><li>a shell-style wildcard, e.g. <tt>http://www.example.com/ads*</tt>, "
                                  "the wildcards <tt>*?[]</tt> may be used</li>"
                                  "<li>a full regular expression by surrounding the string with '<tt>/</tt>', "
                                  "e.g. <tt>/\\/(ad|banner)\\./</tt></li></ul>"
                                  "<p>Any filter string can be preceded by '<tt>@@</tt>' to whitelist (allow) any matching URL, "
                                  "which takes priority over any blacklist (blocking) filter.");

    QWhatsThis::showText(QCursor::pos(), hintHelpString);
}


void AdBlockSettingWidget::insertRule()
{
    const QString rule = addFilterLineEdit->text();
    if (rule.isEmpty())
        return;

    const int numberItem(manualFiltersListWidget->count());
    for (int i = 0; i < numberItem; ++i) {
        if (manualFiltersListWidget->item(i)->text() == rule) {
            addFilterLineEdit->clear();
            return;
        }
    }
    manualFiltersListWidget->addItem(rule);
    addFilterLineEdit->clear();
    hasChanged();
}


void AdBlockSettingWidget::removeRule()
{
    QList<QListWidgetItem *> select = manualFiltersListWidget->selectedItems();
    if (select.isEmpty()) {
        return;
    }
    Q_FOREACH (QListWidgetItem *item, select) {
        delete item;
    }
    hasChanged();
}


void AdBlockSettingWidget::load()
{
    // General settings
    KConfigGroup settingsGroup(_adblockConfig, "Settings");

    const bool isAdBlockEnabled = settingsGroup.readEntry("adBlockEnabled", false);
    checkEnableAdblock->setChecked(isAdBlockEnabled);

    // update enabled status
    checkHideAds->setEnabled(isAdBlockEnabled);
    tabWidget->setEnabled(isAdBlockEnabled);

    const bool areImageFiltered = settingsGroup.readEntry("hideAdsEnabled", false);
    checkHideAds->setChecked(areImageFiltered);

    const int days = settingsGroup.readEntry("updateInterval", 7);
    spinBox->setValue(days);

    // ------------------------------------------------------------------------------

    // automatic filters
    KConfigGroup autoFiltersGroup(_adblockConfig, "FiltersList");

    int i = 1;
    QString n = QString::number(i);

    while (autoFiltersGroup.hasKey( QL1S("FilterName-") + n))
    {
        bool filterEnabled = autoFiltersGroup.readEntry( QL1S("FilterEnabled-") + n, false);
        QString filterName = autoFiltersGroup.readEntry( QL1S("FilterName-") + n, QString());

        QListWidgetItem *subItem = new QListWidgetItem(automaticFiltersListWidget);
        subItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        if (filterEnabled)
            subItem->setCheckState(Qt::Checked);
        else
            subItem->setCheckState(Qt::Unchecked);

        subItem->setText(filterName);

        i++;
        n = QString::number(i);
    }

    // ------------------------------------------------------------------------------

    // local filters
    const QString localRulesFilePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QL1C('/') + QL1S("adblockrules_local");

    QFile ruleFile(localRulesFilePath);
    if (!ruleFile.open(QFile::ReadOnly | QFile::Text))
    {
        qDebug() << "Unable to open rule file" << localRulesFilePath;
        return;
    }

    QTextStream in(&ruleFile);
    while (!in.atEnd())
    {
        const QString stringRule = in.readLine();
        QListWidgetItem *subItem = new QListWidgetItem(manualFiltersListWidget);
        subItem->setText(stringRule);
    }
}


void AdBlockSettingWidget::saveAndClose()
{
    if (!_changed)
    {
        close();
        return;
    }

    // General settings
    KConfigGroup settingsGroup(_adblockConfig, "Settings");

    settingsGroup.writeEntry("adBlockEnabled", checkEnableAdblock->isChecked());
    settingsGroup.writeEntry("hideAdsEnabled", checkHideAds->isChecked());
    settingsGroup.writeEntry("updateInterval", spinBox->value());

    // automatic filters
    KConfigGroup autoFiltersGroup(_adblockConfig, "FiltersList");
    for (int i = 0; i < automaticFiltersListWidget->count(); ++i)
    {
        QListWidgetItem *subItem = automaticFiltersListWidget->item(i);
        bool active = true;
        if (subItem->checkState() == Qt::Unchecked)
            active = false;

        const QString n = QString::number(i + 1);
        autoFiltersGroup.writeEntry( QL1S("FilterEnabled-") + n, active);
    }

    // local filters
    const QString localRulesFilePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QL1C('/') + QL1S("adblockrules_local");

    QFile ruleFile(localRulesFilePath);
    if (!ruleFile.open(QFile::WriteOnly | QFile::Text))
    {
        qDebug() << "Unable to open rule file" << localRulesFilePath;
        return;
    }

    QTextStream out(&ruleFile);
    for (int i = 0; i < manualFiltersListWidget->count(); ++i)
    {
        QListWidgetItem *subItem = manualFiltersListWidget->item(i);
        const QString stringRule = subItem->text();
        if (!stringRule.trimmed().isEmpty())
            out << stringRule << '\n';
    }

    // -------------------------------------------------------------------------------
    _changed = false;
    emit changed(false);
    
    emit okClicked();
    
    // bye bye
    deleteLater();
}


void AdBlockSettingWidget::hasChanged()
{
    // update enabled status
    checkHideAds->setEnabled(checkEnableAdblock->isChecked());
    tabWidget->setEnabled(checkEnableAdblock->isChecked());
    _changed = true;
    emit changed(true);
}


bool AdBlockSettingWidget::changed() const
{
    return _changed;
}
