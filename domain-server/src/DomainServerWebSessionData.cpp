//
//  DomainServerWebSessionData.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-07-21.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainServerWebSessionData.h"

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

DomainServerWebSessionData::DomainServerWebSessionData() :
    _username(),
    _roles()
{
    
}

DomainServerWebSessionData::DomainServerWebSessionData(const QJsonObject& userObject) :
    _roles()
{
    _username = userObject["username"].toString();
    
    // pull each of the roles and throw them into our set
    foreach(const QJsonValue& rolesValue, userObject["roles"].toArray()) {
        _roles.insert(rolesValue.toString());
    }
}

DomainServerWebSessionData::DomainServerWebSessionData(const DomainServerWebSessionData& otherSessionData) : QObject() {
    _username = otherSessionData._username;
    _roles = otherSessionData._roles;
}

DomainServerWebSessionData& DomainServerWebSessionData::operator=(const DomainServerWebSessionData& otherSessionData) {
    DomainServerWebSessionData temp(otherSessionData);
    swap(temp);
    return *this;
}

void DomainServerWebSessionData::swap(DomainServerWebSessionData& otherSessionData) {
    using std::swap;
    
    swap(_username, otherSessionData._username);
    swap(_roles, otherSessionData._roles);
}

QDataStream& operator<<(QDataStream &out, const DomainServerWebSessionData& session) {
    out << session._username << session._roles;
    return out;
}

QDataStream& operator>>(QDataStream &in, DomainServerWebSessionData& session) {
    in >> session._username >> session._roles;
    return in;
}

