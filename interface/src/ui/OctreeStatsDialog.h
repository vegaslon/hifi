//
//  OctreeStatsDialog.h
//  interface/src/ui
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeStatsDialog_h
#define hifi_OctreeStatsDialog_h

#include <QDialog>
#include <QFormLayout>
#include <QLabel>

#include <OctreeSceneStats.h>

#define MAX_STATS 100

class OctreeStatsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    OctreeStatsDialog(QWidget* parent, NodeToOctreeSceneStats* model);
    ~OctreeStatsDialog();

signals:
    void closed();

public slots:
    void reject() override;
    void moreless(const QString& link);

protected:
    // State <- data model held by BandwidthMeter
    void paintEvent(QPaintEvent*) override;

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*) override;

    int AddStatItem(const char* caption, unsigned colorRGBA = 0);
    void RemoveStatItem(int item);
    void showAllOctreeServers();

    void showOctreeServersOfType(NodeType_t serverType);

private:
    enum details {
        LESS,
        MORE,
        MOST
    };

    QFormLayout* _form { nullptr };
    QLabel* _labels[MAX_STATS];
    NodeToOctreeSceneStats* _model { nullptr };
    int _statCount { 0 };

    int _octreeServerLabel;

    int _sendingMode;
    int _serverElements;
    int _localElements;
    int _localElementsMemory;

    int _entityUpdateTime;
    int _entityUpdates;
    int _processedPackets;
    int _processedPacketsElements;
    int _processedPacketsEntities;
    int _processedPacketsTiming;
    int _outboundEditPackets;
    
    const int SAMPLES_PER_SECOND { 10 };
    SimpleMovingAverage _averageUpdatesPerSecond { SAMPLES_PER_SECOND };
    quint64 _lastWindowAt { usecTimestampNow() };
    quint64 _lastKnownTrackedEdits { 0 };

    quint64 _lastRefresh { 0 };

    details _extraServerDetails { LESS };
};

#endif // hifi_OctreeStatsDialog_h
