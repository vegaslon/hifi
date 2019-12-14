//
//  MappingRequest.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_MappingRequest_h
#define hifi_MappingRequest_h

#include <QtCore/QObject>

#include "AssetUtils.h"
#include "AssetClient.h"

class MappingRequest : public QObject {
    Q_OBJECT
public:
    enum Error {
        NoError,
        NotFound,
        NetworkError,
        PermissionDenied,
        InvalidPath,
        InvalidHash,
        UnknownError
    };

    virtual ~MappingRequest();

    Q_INVOKABLE void start();
    Error getError() const { return _error; }
    Q_INVOKABLE QString getErrorString() const;

protected:
    Error _error { NoError };
    MessageID _mappingRequestID { INVALID_MESSAGE_ID };

private:
    virtual void doStart() = 0;
};


class GetMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    GetMappingRequest(const AssetUtils::AssetPath& path);

    AssetUtils::AssetHash getHash() const { return _hash;  }
    AssetUtils::AssetPath getRedirectedPath() const { return _redirectedPath; }
    bool wasRedirected() const { return _wasRedirected; }

signals:
    void finished(GetMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetUtils::AssetPath _path;
    AssetUtils::AssetHash _hash;


    AssetUtils::AssetPath _redirectedPath;
    bool _wasRedirected { false };
};

class SetMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    SetMappingRequest(const AssetUtils::AssetPath& path, const AssetUtils::AssetHash& hash);

    AssetUtils::AssetPath getPath() const { return _path;  }
    AssetUtils::AssetHash getHash() const { return _hash;  }

signals:
    void finished(SetMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetUtils::AssetPath _path;
    AssetUtils::AssetHash _hash;
};

class DeleteMappingsRequest : public MappingRequest {
    Q_OBJECT
public:
    DeleteMappingsRequest(const AssetUtils::AssetPathList& path);

signals:
    void finished(DeleteMappingsRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetUtils::AssetPathList _paths;
};

class RenameMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    RenameMappingRequest(const AssetUtils::AssetPath& oldPath, const AssetUtils::AssetPath& newPath);

signals:
    void finished(RenameMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetUtils::AssetPath _oldPath;
    AssetUtils::AssetPath _newPath;
};

class GetAllMappingsRequest : public MappingRequest {
    Q_OBJECT
public:
    AssetUtils::AssetMappings getMappings() const { return _mappings;  }

signals:
    void finished(GetAllMappingsRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetUtils::AssetMappings _mappings;
};

class SetBakingEnabledRequest : public MappingRequest {
    Q_OBJECT
public:
    SetBakingEnabledRequest(const AssetUtils::AssetPathList& path, bool enabled);

signals:
    void finished(SetBakingEnabledRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetUtils::AssetPathList _paths;
    bool _enabled;
};


#endif // hifi_MappingRequest_h
