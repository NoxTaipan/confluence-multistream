#pragma once

#include "pch.h"
#include "push-widget.h"

class MultiOutputWidget : public QWidget
{
    Q_OBJECT
public:
    MultiOutputWidget(QWidget* parent = 0);

    std::vector<PushWidget*> GetAllPushWidgets();
    void SaveConfig();
    void LoadConfig();
    void OnOBSEvent(obs_frontend_event event);

public slots:  // ✅ CHANGED: Declare RefreshUI as a slot
    void RefreshUI();
    
    // Websocket configuration methods
    bool AddNewTarget(const QString& name, const QString& protocol = "RTMP");
    bool CloneTarget(const QString& sourceId, const QString& newName, const QString& newStreamKey = "");
    bool UpdateTargetName(const QString& targetId, const QString& newName);
    bool UpdateTargetStreamKey(const QString& targetId, const QString& streamKey);
    bool UpdateTargetServiceParam(const QString& targetId, const QString& key, const QString& value);
    bool DeleteTarget(const QString& targetId);
    bool UpdateSyncStart(const QString& targetId, bool syncStart);
    bool UpdateSyncStop(const QString& targetId, bool syncStop);
    PushWidget* FindPushWidgetById(const QString& targetId);

private:
    void UpdateMainStreamButton();

    QWidget* container_ = 0;
    QScrollArea scroll_;
    QVBoxLayout* itemLayout_ = 0;
    QVBoxLayout* layout_ = 0;
    QPushButton* mainStreamButton_ = 0;
};