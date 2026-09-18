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
    void UpdateMainStreamStats();
    void CheckConfluenceStatus();
    void RestartConfluenceServer();
    void RepairConfluenceServer();
    // Reaplica Tr() a todo lo que este dock tiene visible ahora mismo -
    // conectada a los botones de banderita. Los textos que ya se recalculan
    // solos a partir de estado en vivo (CheckConfluenceStatus,
    // UpdateMainStreamButton) simplemente se vuelven a llamar en vez de
    // guardar una copia separada del estado actual.
    void RetranslateUi();

    QWidget* container_ = 0;
    QScrollArea scroll_;
    QVBoxLayout* itemLayout_ = 0;
    QVBoxLayout* layout_ = 0;
    QPushButton* mainStreamButton_ = 0;
    QPushButton* addTargetBtn_ = 0;
    QPushButton* startAllBtn_ = 0;
    QPushButton* stopAllBtn_ = 0;
    QPushButton* langEsBtn_ = 0;
    QPushButton* langEnBtn_ = 0;
    // Fila del stream principal (Twitch), con el mismo formato icono+nombre+punto
    // de estado que usan las filas de target en PushWidgetImpl - ver UpdateBadge()
    // en push-widget.cpp para el mismo patron de carga de icono.
    QLabel* mainStreamBadge_ = 0;
    QLabel* mainStreamDot_ = 0;
    // Linea de stats (duracion/bitrate/FPS), mismo calculo y formato que
    // PushWidgetImpl::UpdateStreamStatus pero sobre el output principal de OBS
    // (obs_frontend_get_streaming_output) en vez de un output propio del plugin.
    QLabel* mainStreamMsg_ = 0;
    QTimer* mainStreamStatsTimer_ = 0;
    std::chrono::steady_clock::time_point mainStreamBeginTime_;
    std::chrono::steady_clock::time_point mainStreamLastInfoTime_;
    uint64_t mainStreamTotalFrames_ = 0;
    uint64_t mainStreamTotalBytes_ = 0;

    // Confluence (Stream Info / Chat / Overlay) corre como un servidor Node
    // aparte de este plugin - este bloque solo lo monitorea/reinicia desde
    // el dock nativo para no depender de abrir el dock del navegador para
    // notar si se cayo.
    QLabel* confluenceDot_ = 0;
    QLabel* confluenceStatusLabel_ = 0;
    QPushButton* confluenceRestartBtn_ = 0;
    QPushButton* confluenceRepairBtn_ = 0;
    QTimer* confluenceCheckTimer_ = 0;
};