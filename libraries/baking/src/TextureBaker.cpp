//
//  TextureBaker.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>

#include <image/Image.h>
#include <ktx/KTX.h>
#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include "ModelBakingLoggingCategory.h"

#include "TextureBaker.h"

const QString BAKED_TEXTURE_EXT = ".ktx";

TextureBaker::TextureBaker(const QUrl& textureURL, image::TextureUsage::Type textureType,
                           const QDir& outputDirectory, const QString& bakedFilename,
                           const QByteArray& textureContent) :
    _textureURL(textureURL),
    _originalTexture(textureContent),
    _textureType(textureType),
    _outputDirectory(outputDirectory),
    _bakedTextureFileName(bakedFilename)
{
    if (bakedFilename.isEmpty()) {
        // figure out the baked texture filename
        auto originalFilename = textureURL.fileName();
        _bakedTextureFileName = originalFilename.left(originalFilename.lastIndexOf('.')) + BAKED_TEXTURE_EXT;
    }
}

void TextureBaker::bake() {
    // once our texture is loaded, kick off a the processing
    connect(this, &TextureBaker::originalTextureLoaded, this, &TextureBaker::processTexture);

    if (_originalTexture.isEmpty()) {
        // first load the texture (either locally or remotely)
        loadTexture();
    } else {
        // we already have a texture passed to us, use that
        emit originalTextureLoaded();
    }
}

void TextureBaker::abort() {
    Baker::abort();

    // flip our atomic bool so any ongoing texture processing is stopped
    _abortProcessing.store(true);
}

void TextureBaker::loadTexture() {
    // check if the texture is local or first needs to be downloaded
    if (_textureURL.isLocalFile()) {
        // load up the local file
        QFile localTexture { _textureURL.toLocalFile() };

        if (!localTexture.open(QIODevice::ReadOnly)) {
            handleError("Unable to open texture " + _textureURL.toString());
            return;
        }

        _originalTexture = localTexture.readAll();

        emit originalTextureLoaded();
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

        networkRequest.setUrl(_textureURL);

        qCDebug(model_baking) << "Downloading" << _textureURL;

        // kickoff the download, wait for slot to tell us it is done
        auto networkReply = networkAccessManager.get(networkRequest);
        connect(networkReply, &QNetworkReply::finished, this, &TextureBaker::handleTextureNetworkReply);
    }
}

void TextureBaker::handleTextureNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded texture" << _textureURL;

        // store the original texture so it can be passed along for the bake
        _originalTexture = requestReply->readAll();

        emit originalTextureLoaded();
    } else {
        // add an error to our list stating that this texture could not be downloaded
        handleError("Error downloading " + _textureURL.toString() + " - " + requestReply->errorString());
    }
}

void TextureBaker::processTexture() {
    // the baked textures need to have the source hash added for cache checks in Interface
    // so we add that to the processed texture before handling it off to be serialized
    auto hashData = QCryptographicHash::hash(_originalTexture, QCryptographicHash::Md5);
    std::string hash = hashData.toHex().toStdString();

    // IMPORTANT: _originalTexture is empty past this point
    auto processedTexture = image::processImage(std::move(_originalTexture), _textureURL.toString().toStdString(),
                                                ABSOLUTE_MAX_TEXTURE_NUM_PIXELS, _textureType, _abortProcessing);
    processedTexture->setSourceHash(hash);

    if (shouldStop()) {
        return;
    }

    if (!processedTexture) {
        handleError("Could not process texture " + _textureURL.toString());
        return;
    }

    
    auto memKTX = gpu::Texture::serialize(*processedTexture);

    if (!memKTX) {
        handleError("Could not serialize " + _textureURL.toString() + " to KTX");
        return;
    }

    const char* data = reinterpret_cast<const char*>(memKTX->_storage->data());
    const size_t length = memKTX->_storage->size();

    // attempt to write the baked texture to the destination file path
    auto filePath = _outputDirectory.absoluteFilePath(_bakedTextureFileName);
    QFile bakedTextureFile { filePath };

    if (!bakedTextureFile.open(QIODevice::WriteOnly) || bakedTextureFile.write(data, length) == -1) {
        handleError("Could not write baked texture for " + _textureURL.toString());
    } else {
        _outputFiles.push_back(filePath);
    }

    qCDebug(model_baking) << "Baked texture" << _textureURL;
    setIsFinished(true);
}

void TextureBaker::setWasAborted(bool wasAborted) {
    Baker::setWasAborted(wasAborted);

    qCDebug(model_baking) << "Aborted baking" << _textureURL;
}
