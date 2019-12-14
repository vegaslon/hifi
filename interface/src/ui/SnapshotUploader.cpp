//
//  SnapshotUploader.cpp
//  interface/src/ui
//
//  Created by Howard Stearns on 8/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SnapshotUploader.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>

#include <AddressManager.h>

#include "scripting/WindowScriptingInterface.h"

SnapshotUploader::SnapshotUploader(QUrl inWorldLocation, QString pathname) :
    _inWorldLocation(inWorldLocation),
    _pathname(pathname) {
}

void SnapshotUploader::uploadSuccess(QNetworkReply* reply) {
    const QString STORY_UPLOAD_URL = "/api/v1/user_stories";

    // parse the reply for the thumbnail_url
    QByteArray contents = reply->readAll();
    QJsonParseError jsonError;
    auto doc = QJsonDocument::fromJson(contents, &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        auto dataObject = doc.object().value("data").toObject();
        QString thumbnailUrl = dataObject.value("thumbnail_url").toString();
        QString imageUrl = dataObject.value("image_url").toString();
        QString snapshotID = dataObject.value("id").toString();
        QString originalImageFileName = dataObject.value("original_image_file_name").toString();
        auto addressManager = DependencyManager::get<AddressManager>();
        QString placeName = _inWorldLocation.authority(); // We currently only upload shareable places, in which case this is just host.
        QString currentPath = _inWorldLocation.path();

        // create json post data
        QJsonObject rootObject;
        QJsonObject userStoryObject;
        QJsonObject detailsObject;
        detailsObject.insert("image_url", imageUrl);
        if (dataObject.contains("shareable_url")) {
            detailsObject.insert("shareable_url", dataObject.value("shareable_url").toString());
        }
        detailsObject.insert("snapshot_id", snapshotID);
        detailsObject.insert("original_image_file_name", originalImageFileName);
        QString pickledDetails = QJsonDocument(detailsObject).toJson();
        userStoryObject.insert("details", pickledDetails);
        userStoryObject.insert("thumbnail_url", thumbnailUrl);
        userStoryObject.insert("place_name", placeName);
        userStoryObject.insert("path", currentPath);
        userStoryObject.insert("action", "snapshot");
        userStoryObject.insert("audience", "for_url");
        rootObject.insert("user_story", userStoryObject);

        auto accountManager = DependencyManager::get<AccountManager>();
        JSONCallbackParameters callbackParams(this, "createStorySuccess", "createStoryFailure");

        accountManager->sendRequest(STORY_UPLOAD_URL,
            AccountManagerAuth::Required,
            QNetworkAccessManager::PostOperation,
            callbackParams,
            QJsonDocument(rootObject).toJson());

    } else {
        emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(true, contents);
        this->deleteLater();
    }
}

void SnapshotUploader::uploadFailure(QNetworkReply* reply) {
    QString replyString = reply->readAll();
    qDebug() << "Error " << reply->errorString() << " uploading snapshot " << _pathname << " from " << _inWorldLocation;
    if (replyString.size() == 0) {
        replyString = reply->errorString();
    }
    replyString = replyString.left(1000); // Only print first 1000 characters of error
    qDebug() << "Snapshot upload reply error (truncated):" << replyString;
    emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(true, replyString); // maybe someday include _inWorldLocation, _filename?
    this->deleteLater();
}

void SnapshotUploader::createStorySuccess(QNetworkReply* reply) {
    QString replyString = reply->readAll();
    emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(false, replyString);
    this->deleteLater();
}

void SnapshotUploader::createStoryFailure(QNetworkReply* reply) {
    QString replyString = reply->readAll();
    qDebug() << "Error " << reply->errorString() << " uploading snapshot story " << _pathname << " from " << _inWorldLocation;
    if (replyString.size() == 0) {
        replyString = reply->errorString();
    }
    replyString = replyString.left(1000); // Only print first 1000 characters of error
    qDebug() << "Snapshot story upload reply error (truncated):" << replyString;
    emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(true, replyString);
    this->deleteLater();
}

