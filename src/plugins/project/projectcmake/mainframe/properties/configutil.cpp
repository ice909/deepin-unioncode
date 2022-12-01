/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhouyi<zhouyi1@uniontech.com>
 *
 * Maintainer: zhouyi<zhouyi1@uniontech.com>
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

#include "configutil.h"
#include "mainframe/cmakeasynparse.h"
#include "mainframe/cmakegenerator.h"
#include "targetsmanager.h"

#include "services/option/optionmanager.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QVector>
#include <QFileInfo>
#include <QDir>
namespace config {

class ConfigUtilPrivate
{
    friend class ConfigUtil;
    ConfigureParam configureParam;
    QMap<ConfigType, QString> configTypeStringMap;
};

ConfigUtil::ConfigUtil(QObject *parent)
    : QObject(parent)
    , d(new ConfigUtilPrivate())
{
    d->configTypeStringMap = {{Debug, "Debug"},
                        {Release, "Release"}};
}

ConfigUtil::~ConfigUtil()
{
    if (d) {
        delete d;
    }
}

ConfigUtil *ConfigUtil::instance()
{
    static ConfigUtil ins;
    return &ins;
}

ConfigureParam *ConfigUtil::getConfigureParamPointer()
{
    return &d->configureParam;
}

QString ConfigUtil::getNameFromType(ConfigType type)
{
    if (d->configTypeStringMap.contains(type)) {
        return d->configTypeStringMap.value(type);
    }

    return "Unkown";
}

ConfigType ConfigUtil::getTypeFromName(QString name)
{
    ConfigType type = d->configTypeStringMap.key(name);
    return type;
}

bool ConfigUtil::isNeedConfig(const QString &workspace, ConfigureParam &param)
{
    QString propertyFile = getConfigPath(workspace);
    if (QFileInfo(propertyFile).exists() || QFileInfo(propertyFile).isFile()) {
        readConfig(propertyFile, param);
        if (!param.buildConfigures.isEmpty()) {
            return false;
        }
    }
    return true;
}

dpfservice::ProjectInfo ConfigUtil::createProjectInfo(const ConfigureParam *param)
{
    dpfservice::ProjectInfo info;
    for (auto iter = param->buildConfigures.begin(); iter != param->buildConfigures.end(); ++iter) {
        if (d->configureParam.tempSelType == iter->type) {
            info.setLanguage(param->language);
            info.setKitName(CmakeGenerator::toolKitName());
            info.setWorkspaceFolder(param->workspace);
            info.setBuildType(ConfigUtil::instance()->getNameFromType(iter->type));
            info.setBuildFolder(iter->directory);
            info.setBuildProgram(OptionManager::getInstance()->getCMakeToolPath());

            QStringList configArguments;
            configArguments << "-S";
            configArguments << info.workspaceFolder();
            configArguments << "-B";
            configArguments << info.buildFolder();
            configArguments << "-G";
            configArguments << CDT_PROJECT_KIT::get()->CDT4_GENERATOR;
            configArguments << "-DCMAKE_BUILD_TYPE=" + info.buildType();
            configArguments << "-DCMAKE_EXPORT_COMPILE_COMMANDS=1";
            info.setConfigCustomArgs(configArguments);

            QStringList buildArguments;
            buildArguments << "--build";
            buildArguments << ".";
            buildArguments << "--target";
            buildArguments << "all";
            info.setBuildCustomArgs(buildArguments);

            QStringList cleanArguments;
            cleanArguments << "--build";
            cleanArguments << ".";
            cleanArguments << "--target";
            cleanArguments << "clean";
            info.setCleanCustomArgs(buildArguments);
        }
    }

    return info;
}

void ConfigUtil::configProject(const ConfigureParam *param)
{
    dpfservice::ProjectInfo info = createProjectInfo(param);
    if (info.isVaild()) {
        emit configureDone(info);
    }
}

void ConfigUtil::checkConfigInfo(const QString &buildType, const QString &directory)
{
    ConfigType type = getTypeFromName(buildType);
    auto iter = d->configureParam.buildConfigures.begin();
    for (; iter != d->configureParam.buildConfigures.end(); ++iter) {
        if (type == iter->type) {
            if (!directory.isEmpty())
                iter->directory = directory;
            QString cfgFile = iter->directory + QDir::separator() +
                    TargetsManager::instance()->getCMakeConfigFile();
            if (!QFileInfo(cfgFile).isFile()) {
                configProject(&d->configureParam);
            }
        }
    }
}

QString ConfigUtil::getConfigPath(const QString &projectPath)
{
    return CustomPaths::projectCachePath(projectPath) + QDir::separator() + "project.properties";
}

void ConfigUtil::readConfig(const QString &filePath, ConfigureParam &param)
{
    param.clear();

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream stream(&file);
        stream >> param;
        file.close();
    }
}

void ConfigUtil::saveConfig(const QString &filePath, const ConfigureParam &param)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadWrite)) {
        QDataStream stream(&file);
        stream << param;
        file.close();
    }
}

void ConfigUtil::updateProjectInfo(dpfservice::ProjectInfo &info, const ConfigureParam *param)
{
    if (!param)
        return;

    for (auto iter = param->buildConfigures.begin(); iter != param->buildConfigures.end(); ++iter) {
        if (d->configureParam.defaultType == iter->type) {
            info.setLanguage(param->language);
            info.setKitName(CmakeGenerator::toolKitName());
            info.setWorkspaceFolder(param->workspace);
            info.setBuildType(ConfigUtil::instance()->getNameFromType(iter->type));
            info.setBuildFolder(iter->directory);
            info.setBuildProgram(OptionManager::getInstance()->getCMakeToolPath());

            QStringList arguments;
            arguments << "-S";
            arguments << info.workspaceFolder();
            arguments << "-B";
            arguments << info.buildFolder();
            arguments << "-G";
            arguments << CDT_PROJECT_KIT::get()->CDT4_GENERATOR;
            arguments << "-DCMAKE_BUILD_TYPE=" + info.buildType();
            arguments << "-DCMAKE_EXPORT_COMPILE_COMMANDS=1";
            info.setConfigCustomArgs(arguments);

            for (auto iterStep = iter->steps.begin(); iterStep != iter->steps.end(); ++iterStep) {
                QStringList arguments;
                arguments << "--build";
                arguments << ".";
                if (!iterStep->targetName.isEmpty()) {
                    arguments << "--target";
                    arguments << iterStep->targetName;
                }
                if (!iterStep->arguments.isEmpty()) {
                    arguments << "--";
                    arguments << iterStep->arguments;
                }
                if (iterStep->type == StepType::Build) {
                    info.setBuildCustomArgs(arguments);
                } else if (iterStep->type == StepType::Clean) {
                    info.setCleanCustomArgs(arguments);
                }
            }

            info.setRunProgram(OptionManager::getInstance()->getCMakeToolPath());
            auto iterRun = iter->runConfigure.params.begin();
            for (; iterRun != iter->runConfigure.params.end(); ++iterRun) {
                if (iterRun->targetName == iter->runConfigure.defaultTargetName) {
                    QStringList arguments;
                    arguments << iterRun->arguments;
                    arguments << iterRun->targetPath;
                    info.setRunCustomArgs(arguments);
                    break;
                }
            }

            break;
        }
    }
}

} //namespace config
