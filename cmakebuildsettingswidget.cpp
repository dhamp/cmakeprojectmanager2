/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cmakebuildsettingswidget.h"

#include "configmodel.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/pathchooser.h>
#include <utils/itemviews.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// CMakeBuildSettingsWidget:
// --------------------------------------------------------------------

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new QSortFilterProxyModel)
{
    QTC_CHECK(bc);

    setDisplayName(tr("CMake"));

    auto vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    auto container = new Utils::DetailsWidget;
    container->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(container);

    auto details = new QWidget(container);
    container->setWidget(details);

    auto mainLayout = new QGridLayout(details);
    mainLayout->setMargin(0);
    mainLayout->setColumnStretch(1, 10);

    auto project = static_cast<CMakeProject *>(bc->target()->project());

    auto buildDirChooser = new Utils::PathChooser;
    buildDirChooser->setBaseFileName(project->projectDirectory());
    buildDirChooser->setFileName(bc->buildDirectory());
    connect(buildDirChooser, &Utils::PathChooser::rawPathChanged, this,
            [this, project](const QString &path) {
                m_configModel->flush(); // clear out config cache...
                project->changeBuildDirectory(m_buildConfiguration, path);
            });

    int row = 0;
    mainLayout->addWidget(new QLabel(tr("Build directory:")), row, 0);
    mainLayout->addWidget(buildDirChooser->lineEdit(), row, 1);
    mainLayout->addWidget(buildDirChooser->buttonAtIndex(0), row, 2);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    m_errorLabel = new QLabel;
    m_errorLabel->setPixmap(Core::Icons::ERROR.pixmap());
    m_errorLabel->setVisible(false);
    m_errorMessageLabel = new QLabel;
    m_errorMessageLabel->setVisible(false);
    auto boxLayout = new QHBoxLayout;
    boxLayout->addWidget(m_errorLabel);
    boxLayout->addWidget(m_errorMessageLabel);
    mainLayout->addLayout(boxLayout, row, 0, 1, 3, Qt::AlignHCenter);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    auto tree = new Utils::TreeView;
    connect(tree, &Utils::TreeView::activated,
            tree, [tree](const QModelIndex &idx) { tree->edit(idx); });
    m_configView = tree;
    m_configFilterModel->setSourceModel(m_configModel);
    m_configFilterModel->setFilterKeyColumn(2);
    m_configFilterModel->setFilterFixedString(QLatin1String("0"));
    m_configView->setModel(m_configFilterModel);
    m_configView->setMinimumHeight(300);
    m_configView->setRootIsDecorated(false);
    m_configView->setUniformRowHeights(true);
    new Utils::HeaderViewStretcher(m_configView->header(), 1);
    m_configView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_configView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_configView->setFrameShape(QFrame::NoFrame);
    m_configView->hideColumn(2); // Hide isAdvanced column
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_configView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicator::Large, findWrapper);
    m_progressIndicator->attachToWidget(findWrapper);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { m_progressIndicator->show(); });

    mainLayout->addWidget(findWrapper, row, 0, 1, 2);

    auto buttonLayout = new QVBoxLayout;
    m_editButton = new QPushButton(tr("&Edit"));
    buttonLayout->addWidget(m_editButton);
    m_resetButton = new QPushButton(tr("&Reset"));
    m_resetButton->setEnabled(false);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_showAdvancedCheckBox = new QCheckBox(tr("Advanced"));
    buttonLayout->addWidget(m_showAdvancedCheckBox);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    mainLayout->addLayout(buttonLayout, row, 2);

    ++row;
    m_reconfigureButton = new QPushButton(tr("Apply Configuration Changes"));
    m_reconfigureButton->setEnabled(false);
    mainLayout->addWidget(m_reconfigureButton, row, 0, 1, 3);

    updateAdvancedCheckBox();
    setError(bc->error());

    connect(project, &CMakeProject::parsingStarted, this, [this]() {
        updateButtonState();
        m_showProgressTimer.start();
    });
    connect(project, &CMakeProject::buildDirectoryDataAvailable,
            this, [this, project, buildDirChooser](ProjectExplorer::BuildConfiguration *bc) {
        updateButtonState();
        if (m_buildConfiguration == bc) {
            m_configModel->setConfiguration(project->currentCMakeConfiguration());
            buildDirChooser->triggerChanged(); // refresh valid state...
        }
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
    });

    connect(m_configModel, &QAbstractItemModel::dataChanged,
            this, &CMakeBuildSettingsWidget::updateButtonState);
    connect(m_configModel, &QAbstractItemModel::modelReset,
            this, &CMakeBuildSettingsWidget::updateButtonState);

    connect(m_showAdvancedCheckBox, &QCheckBox::stateChanged,
            this, &CMakeBuildSettingsWidget::updateAdvancedCheckBox);

    connect(m_resetButton, &QPushButton::clicked, m_configModel, &ConfigModel::resetAllChanges);
    connect(m_reconfigureButton, &QPushButton::clicked, this, [this, project]() {
        project->setCurrentCMakeConfiguration(m_configModel->configurationChanges());
    });
    connect(m_editButton, &QPushButton::clicked, this, [this]() {
        QModelIndex idx = m_configView->currentIndex();
        if (idx.column() != 1)
            idx = idx.sibling(idx.row(), 1);
        m_configView->setCurrentIndex(idx);
        m_configView->edit(idx);
    });

    connect(bc, &CMakeBuildConfiguration::errorOccured, this, &CMakeBuildSettingsWidget::setError);
}

void CMakeBuildSettingsWidget::setError(const QString &message)
{
    bool showWarning = !message.isEmpty();
    m_errorLabel->setVisible(showWarning);
    m_errorLabel->setToolTip(message);
    m_errorMessageLabel->setVisible(showWarning);
    m_errorMessageLabel->setText(message);
    m_errorMessageLabel->setToolTip(message);

    m_configView->setVisible(!showWarning);
    m_editButton->setVisible(!showWarning);
    m_resetButton->setVisible(!showWarning);
    m_showAdvancedCheckBox->setVisible(!showWarning);
    m_reconfigureButton->setVisible(!showWarning);
}

void CMakeBuildSettingsWidget::updateButtonState()
{
    auto project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
    const bool isParsing = project->isParsing();
    const bool hasChanges = m_configModel->hasChanges();
    m_resetButton->setEnabled(hasChanges && !isParsing);
    m_reconfigureButton->setEnabled((hasChanges || m_configModel->hasCMakeChanges()) && !isParsing);
}

void CMakeBuildSettingsWidget::updateAdvancedCheckBox()
{
    // Switch between Qt::DisplayRole (everything is "0") and Qt::EditRole (advanced is "1").
    m_configFilterModel->setFilterRole(m_showAdvancedCheckBox->isChecked() ? Qt::EditRole : Qt::DisplayRole);
}

} // namespace Internal
} // namespace CMakeProjectManager
