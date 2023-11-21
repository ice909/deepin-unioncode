// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "collaborators.h"
#include "services/window/windowservice.h"
#include "mainframe/cvskeeper.h"
#include "base/abstractaction.h"
#include "base/abstractcentral.h"
#include <QAction>

using namespace dpfservice;
void Collaborators::initialize()
{

}

bool Collaborators::start()
{
    auto &ctx = dpfInstance.serviceContext();
    WindowService *windowService = ctx.service<WindowService>(WindowService::name());
    if (windowService) {
        if (windowService->addNavigation) {
            windowService->addNavigation(MWNA_GIT, "git");
            windowService->addNavigation(MWNA_SVN, "svn");
        }
        if (windowService->addCentralNavigation) {
            windowService->addCentralNavigation(MWNA_GIT,
                                                new AbstractCentral(CVSkeeper::instance()->gitMainWidget()));
            windowService->addCentralNavigation(MWNA_SVN,
                                                new AbstractCentral(CVSkeeper::instance()->svnMainWidget()));
        }
    }
    return true;
}

dpf::Plugin::ShutdownFlag Collaborators::stop()
{
    return Sync;
}
