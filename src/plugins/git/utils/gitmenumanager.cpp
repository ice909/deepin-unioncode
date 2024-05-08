// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gitmenumanager.h"
#include "client/gitclient.h"

#include "base/abstractaction.h"
#include "services/window/windowservice.h"
#include "services/project/projectservice.h"
#include "services/editor/editorservice.h"

#include <QMenu>

using namespace dpfservice;

GitMenuManager::GitMenuManager(QObject *parent)
    : QObject(parent)
{
}

void GitMenuManager::actionHandler(QAction *act, GitType type)
{
    const auto &filePath = act->property(GitFilePath).toString();
    bool isProject = act->property(GitIsProject).toBool();
    bool ret = false;
    switch (type) {
    case GitLog:
        ret = GitClient::instance()->gitLog(filePath, isProject);
        break;
    case GitBlame:
        ret = GitClient::instance()->blameFile(filePath);
        break;
    case GitDiff:
        ret = GitClient::instance()->gitDiff(filePath, isProject);
    default:
        break;
    }

    if (ret) {
        auto dockName = winSer->getCurrentDockName(Position::Central);
        if (dockName == GitWindow)
            return;

        GitClient::instance()->setLastCentralWidget(dockName);
        winSer->showWidgetAtPosition(GitWindow, Position::Central, true);
    }
}

GitMenuManager *GitMenuManager::instance()
{
    static GitMenuManager ins;
    return &ins;
}

void GitMenuManager::initialize(dpfservice::WindowService *service)
{
    if (!service)
        return;

    winSer = service;
    auto initAction = [&](QAction *action, const QString &id = QString(),
                          const QString &description = QString(),
                          const QKeySequence &key = QKeySequence()) -> AbstractAction * {
        auto actionImpl = new AbstractAction(action, this);
        if (!key.isEmpty())
            actionImpl->setShortCutInfo(id, description, key);
        return actionImpl;
    };

    gitAct = new QAction("&Git", this);
    auto gitActImpl = initAction(gitAct);
    service->addAction(MWM_TOOLS, gitActImpl);

    createGitSubMenu();
    gitAct->setMenu(&gitSubMenu);
}

void GitMenuManager::setupProjectMenu()
{
    auto activeProjInfo = dpfGetService(dpfservice::ProjectService)->getActiveProjectInfo();
    if (!activeProjInfo.isVaild() || !GitClient::instance()->checkRepositoryExist(activeProjInfo.workspaceFolder())) {
        curProjectAct->setEnabled(false);
        return;
    }

    QFileInfo info(activeProjInfo.workspaceFolder());

    curProjectAct->setEnabled(true);
    projectLogAct->setProperty(GitFilePath, activeProjInfo.workspaceFolder());
    projectLogAct->setText(tr("Log of \"%1\"").arg(info.fileName()));

    projectDiffAct->setProperty(GitFilePath, activeProjInfo.workspaceFolder());
    projectDiffAct->setText(tr("Diff of \"%1\"").arg(info.fileName()));
}

void GitMenuManager::setupFileMenu(const QString &filePath)
{
    QString file = filePath;
    if (file.isEmpty()) {
        auto editorService = dpfGetService(EditorService);
        if (!editorService)
            return;

        file = editorService->currentFile();
    }

    if (file.isEmpty() || !GitClient::instance()->checkRepositoryExist(file)) {
        curFileAct->setEnabled(false);
        return;
    }

    curFileAct->setEnabled(true);
    QFileInfo info(file);

    fileLogAct->setProperty(GitFilePath, file);
    fileLogAct->setText(tr("Log of \"%1\"").arg(info.fileName()));

    fileBlameAct->setProperty(GitFilePath, file);
    fileBlameAct->setText(tr("Blame of \"%1\"").arg(info.fileName()));

    fileDiffAct->setProperty(GitFilePath, file);
    fileDiffAct->setText(tr("Diff of \"%1\"").arg(info.fileName()));
}

QAction *GitMenuManager::gitAction() const
{
    return gitAct;
}

void GitMenuManager::createGitSubMenu()
{
    curFileAct = gitSubMenu.addAction(tr("Current File"));
    curFileAct->setEnabled(false);
    curFileAct->setMenu(&fileSubMenu);
    createFileSubMenu();

    curProjectAct = gitSubMenu.addAction(tr("Current Project"));
    curProjectAct->setEnabled(false);
    curProjectAct->setMenu(&projectSubMenu);
    createProjectSubMenu();
}

void GitMenuManager::createFileSubMenu()
{
    fileLogAct = new QAction(this);
    connect(fileLogAct, &QAction::triggered, this, std::bind(&GitMenuManager::actionHandler, this, fileLogAct, GitLog));

    fileBlameAct = new QAction(this);
    connect(fileBlameAct, &QAction::triggered, this, std::bind(&GitMenuManager::actionHandler, this, fileBlameAct, GitBlame));

    fileDiffAct = new QAction(this);
    connect(fileDiffAct, &QAction::triggered, this, std::bind(&GitMenuManager::actionHandler, this, fileDiffAct, GitDiff));

    fileSubMenu.addAction(fileLogAct);
    fileSubMenu.addAction(fileBlameAct);
    fileSubMenu.addAction(fileDiffAct);
}

void GitMenuManager::createProjectSubMenu()
{
    projectLogAct = new QAction(this);
    projectLogAct->setProperty(GitIsProject, true);
    connect(projectLogAct, &QAction::triggered, this, std::bind(&GitMenuManager::actionHandler, this, projectLogAct, GitLog));

    projectDiffAct = new QAction(this);
    projectDiffAct->setProperty(GitIsProject, true);
    connect(projectDiffAct, &QAction::triggered, this, std::bind(&GitMenuManager::actionHandler, this, projectDiffAct, GitDiff));

    projectSubMenu.addAction(projectLogAct);
    projectSubMenu.addAction(projectDiffAct);
}
