/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakeprojectnodes.h"

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeProjectNode::CMakeProjectNode(CMakeProject *project, const Utils::FileName &fileName)
    : ProjectExplorer::ProjectNode(fileName),
      m_project(project)
{
}

bool CMakeProjectNode::showInSimpleTree() const
{
    // TODO
    return true;
}

QList<ProjectExplorer::ProjectAction> CMakeProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::ProjectAction>()
        << ProjectExplorer::AddNewFile
        << ProjectExplorer::EraseFile
        << ProjectExplorer::Rename;
}

bool CMakeProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool CMakeProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool CMakeProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool CMakeProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool CMakeProjectNode::removeFiles(const QStringList &filePaths,  QStringList *notRemoved)
{
    Q_UNUSED(filePaths)
    Q_UNUSED(notRemoved)
    return false;
}

bool CMakeProjectNode::deleteFiles(const QStringList &filePaths)
{
    return m_project->eraseFiles(filePaths);
}

bool CMakeProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project->renameFile(filePath, newFilePath);
}
