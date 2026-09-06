#include "multi-output-widget.h"
#include "push-widget.h"
#include "output-config.h"
#include "plugin-support.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDialog>
#include <QTabWidget>
#include <QMainWindow>
#include <QFontDatabase>
#include <QProcess>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

// Ruta personal de este equipo - el repo confluence vive aparte de este
// plugin. La build para compartir con otros va a necesitar que esto sea
// configurable en vez de hardcodeado (ver decision de mantener la version
// personal y la de compartir separadas).
static const char* ConfluenceDir()
{
    return "C:/Users/noxta/Documents/Claude/confluence";
}

static QString LoadBrandFontFamily()
{
    static QString cachedFamily;
    if (!cachedFamily.isEmpty()) return cachedFamily;

    const char* fontPath = obs_module_file("fonts/IBMPlexSans.ttf");
    if (!fontPath) return QString();

    int fontId = QFontDatabase::addApplicationFont(QString::fromUtf8(fontPath));
    bfree((void*)fontPath);
    if (fontId == -1) return QString();

    auto families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) return QString();

    cachedFamily = families.first();
    return cachedFamily;
}

MultiOutputWidget::MultiOutputWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(obs_module_text("Title"));

    QString brandFont = LoadBrandFontFamily();
    QString fontStack = brandFont.isEmpty()
        ? QStringLiteral("\"Segoe UI\", sans-serif")
        : QStringLiteral("\"%1\", \"Segoe UI\", sans-serif").arg(brandFont);

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "QWidget { background-color: #0c0e13; color: #e6e8ee; font-size: 13px; font-family: %1; }"
        "QPushButton {"
        "  background-color: #1c202b; color: #e6e8ee; border: 1px solid #262b38;"
        "  border-radius: 6px; padding: 8px 10px; font-weight: 600; }"
        "QPushButton:hover { background-color: #232838; border-color: #6ee7b7; }"
        "QPushButton:pressed { background-color: #161922; }"
        "QPushButton:focus { border-color: #6ee7b7; }"
        "QLabel { color: #9096ac; }"
        "QLineEdit, QComboBox {"
        "  background-color: #161922; color: #e6e8ee; border: 1px solid #262b38;"
        "  border-radius: 6px; padding: 5px 7px; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #6ee7b7; }"
        "QScrollArea { background-color: #0c0e13; border: none; }"
        "QCheckBox { color: #e6e8ee; }"
        "#mainStreamButton { border-color: #6ee7b7; font-weight: 700; }"
    ).arg(fontStack));

    container_ = new QWidget(&scroll_);
    container_->setAttribute(Qt::WA_StyledBackground, true);
    layout_ = new QVBoxLayout(container_);
    layout_->setAlignment(Qt::AlignmentFlag::AlignTop);

    // branded header - same mark/title/suite-tag pattern as the Confluence
    // and Confluence Chat web docks, so all 3 read as one product family
    auto headerRow = new QWidget(container_);
    auto headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 4);
    headerLayout->setSpacing(8);
    auto brandMark = new QLabel(headerRow);
    brandMark->setFixedSize(16, 16);
    brandMark->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "  stop:0 #6ee7b7, stop:0.6 #9146ff, stop:1 #ff3b5c);"
        "border-radius: 5px;"
    );
    auto titleLabel = new QLabel(obs_module_text("Title"), headerRow);
    titleLabel->setStyleSheet(QStringLiteral("color: #e6e8ee; font-weight: 700; font-size: 15px; font-family: %1;").arg(fontStack));
    headerLayout->addWidget(brandMark);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerRow->setLayout(headerLayout);
    layout_->addWidget(headerRow);

    auto suiteTag = new QLabel(u8"CONFLUENCE SUITE", container_);
    suiteTag->setStyleSheet("color: #9096ac; font-size: 9px; font-weight: 600; margin-left: 24px; margin-bottom: 8px;");
    layout_->addWidget(suiteTag);

    // Estado del servidor Confluence (Stream Info/Chat/Overlay) - corre
    // aparte de este plugin, asi que antes la unica forma de notar que se
    // habia caido era abrir el dock del navegador y ver que no cargaba.
    auto confluenceRow = new QWidget(container_);
    auto confluenceLayout = new QHBoxLayout(confluenceRow);
    confluenceLayout->setContentsMargins(0, 0, 0, 8);
    confluenceLayout->setSpacing(6);
    confluenceDot_ = new QLabel(confluenceRow);
    confluenceDot_->setFixedSize(8, 8);
    confluenceStatusLabel_ = new QLabel(obs_module_text("Confluence.Checking"), confluenceRow);
    confluenceStatusLabel_->setStyleSheet("font-size: 11px;");
    confluenceRestartBtn_ = new QPushButton(obs_module_text("Btn.RestartConfluence"), confluenceRow);
    confluenceRestartBtn_->setStyleSheet("padding: 3px 9px; font-size: 11px; font-weight: 600;");
    QObject::connect(confluenceRestartBtn_, &QPushButton::clicked, [this]() {
        RestartConfluenceServer();
    });
    confluenceLayout->addWidget(confluenceDot_);
    confluenceLayout->addWidget(confluenceStatusLabel_);
    confluenceLayout->addStretch();
    confluenceLayout->addWidget(confluenceRestartBtn_);
    confluenceRow->setLayout(confluenceLayout);
    layout_->addWidget(confluenceRow);

    confluenceCheckTimer_ = new QTimer(this);
    confluenceCheckTimer_->setInterval(5000);
    QObject::connect(confluenceCheckTimer_, &QTimer::timeout, this, &MultiOutputWidget::CheckConfluenceStatus);
    confluenceCheckTimer_->start();
    CheckConfluenceStatus();

    // main OBS output (e.g. Twitch) start/stop - native obs-frontend-api, not a multi-rtmp target
    mainStreamButton_ = new QPushButton(container_);
    mainStreamButton_->setObjectName("mainStreamButton");
    QObject::connect(mainStreamButton_, &QPushButton::clicked, [this]() {
        if (obs_frontend_streaming_active())
            obs_frontend_streaming_stop();
        else
            obs_frontend_streaming_start();
    });
    layout_->addWidget(mainStreamButton_);
    UpdateMainStreamButton();

    // init widget
    auto addButton = new QPushButton(obs_module_text("Btn.NewTarget"), container_);
    QObject::connect(addButton, &QPushButton::clicked, [this]() {
        auto& global = GlobalMultiOutputConfig();
        auto newid = GenerateId(global);
        auto target = std::make_shared<OutputTargetConfig>();
        target->id = newid;
        global.targets.emplace_back(target);
        auto pushwidget = createPushWidget(newid, container_);
        itemLayout_->addWidget(pushwidget);
        if (pushwidget->ShowEditDlg())
            SaveConfig();
        else {
            auto it = std::find_if(global.targets.begin(), global.targets.end(), [newid](auto& x) {
                return x->id == newid;
            });
            if (it != global.targets.end())
                global.targets.erase(it);
            delete pushwidget;
        }
    });
    layout_->addWidget(addButton);

    // start all, stop all
    auto allBtnContainer = new QWidget(this);
    auto allBtnLayout = new QHBoxLayout();
    auto startAllButton = new QPushButton(obs_module_text("Btn.StartAll"), allBtnContainer);
    allBtnLayout->addWidget(startAllButton);
    auto stopAllButton = new QPushButton(obs_module_text("Btn.StopAll"), allBtnContainer);
    allBtnLayout->addWidget(stopAllButton);
    allBtnContainer->setLayout(allBtnLayout);
    layout_->addWidget(allBtnContainer);

    // Start/stop en orden (Twitch primero, despues cada target en el orden
    // de la lista) con un pequeno delay entre cada uno - no espera a que el
    // anterior confirme conexion, solo evita disparar todo en el mismo instante.
    QObject::connect(startAllButton, &QPushButton::clicked, [this]() {
        const int stepMs = 800;
        int delayMs = 0;
        QTimer::singleShot(delayMs, this, []() {
            if (!obs_frontend_streaming_active())
                obs_frontend_streaming_start();
        });
        delayMs += stepMs;
        for (auto x : GetAllPushWidgets()) {
            QTimer::singleShot(delayMs, this, [x]() { x->StartStreaming(); });
            delayMs += stepMs;
        }
    });
    QObject::connect(stopAllButton, &QPushButton::clicked, [this]() {
        const int stepMs = 800;
        int delayMs = 0;
        QTimer::singleShot(delayMs, this, []() {
            if (obs_frontend_streaming_active())
                obs_frontend_streaming_stop();
        });
        delayMs += stepMs;
        for (auto x : GetAllPushWidgets()) {
            QTimer::singleShot(delayMs, this, [x]() { x->StopStreaming(); });
            delayMs += stepMs;
        }
    });
    
    // load config
    itemLayout_ = new QVBoxLayout(container_);
    LoadConfig();
    layout_->addLayout(itemLayout_);

    // donate section (unchanged)
    if (std::string("\xe5\xa4\x9a\xe8\xb7\xaf\xe6\x8e\xa8\xe6\xb5\x81") == obs_module_text("Title"))
    {
        auto cr = new QWidget(container_);
        auto innerLayout = new QGridLayout(cr);
        innerLayout->setAlignment(Qt::AlignmentFlag::AlignLeft);

        auto label = new QLabel(u8"该插件免费提供，\r\n如您是付费取得，可向商家申请退款\r\n免费领红包或投喂支持插件作者。", cr);
        innerLayout->addWidget(label, 0, 0, 1, 2);
        innerLayout->setColumnStretch(0, 4);
        auto label2 = new QLabel(u8"作者：雷鸣", cr);
        innerLayout->addWidget(label2, 1, 0, 1, 1);
        auto btnFeed = new QPushButton(u8"支持", cr);
        innerLayout->addWidget(btnFeed, 1, 1, 1, 1);
        
        QObject::connect(btnFeed, &QPushButton::clicked, [this]() {
            const char redbagpng[] = 
                "iVBORw0KGgoAAAANSUhEUgAAAJgAAACXAQMAAADTWgC3AAAABlBMVEUAAAD///+l2Z/dAAAAAWJLR0Q"
                "AiAUdSAAAAAlwSFlzAAAuIwAALiMBeKU/dgAAAWtJREFUSMe1lk2OgzAMhY1YZJkj5CbkYkggcTG4SY"
                "6QZRaonmcHqs7PYtTaVVWSLxJu7JfnEP/+0H9ZIaKRA0aZz4QJJXuGQFsJO9HU104H1ihuTENl4IS12"
                "YmVcFSa4unJuE2xZV69mOav5XrX6nRgMi6Ii+3Nr9p4k8m7w5OtmOVbzw8ZrOmbxs0Y/ktENMlfnQnx"
                "NweG2vB1ZrZCPoyPfmbwXWSPiz1DvIrHwOHgFQsQtTkrmG6McRvqUu4aGbM28Cm1wRM62HtOP2DwkFF"
                "ypKVoU/dEa8Y9rtaFJLg5EzscoSfBKMWgZ8aY9bj4EQ1jo9GDIR68kKukMCF/6pPWTPW0R9XulVNzpj"
                "6Z5ZzMpOZrzvRElPC49Awx2LOi3k7aP+akhnL1AEMmPYphvtqeGD032TPt5zB2kQBq5Mgo9hrl7lceT"
                "MQsEkD80YH1O9xRw9Vzn/cSQ6Y6EK1JH3nVxvss/GCf3L3/YF97Nxv6vuoIAwAAAABJRU5ErkJggg=="
                ;
            const char alipaypng[] = 
                "iVBORw0KGgoAAAANSUhEUgAAALsAAAC4AQMAAACByg+HAAAABlBMVEUAAAD///+l2Z/dAAAAAWJLR0Q"
                "AiAUdSAAAAAlwSFlzAAAuIwAALiMBeKU/dgAAAVtJREFUWMPNmEGugzAMRM0qx+CmIbkpx8gK1zM2/U"
                "L66w4RogqvC5dhxqbm/6/TfgEuwzr8PNwnjhP7pgYNxR1r97XPhUvxjUsPztj0hkJ7O7c4m/V3gCg36"
                "r5Q68uA7SHtaC8BlBbP2T4awFNzEaANOtSt4+EPEciEiKJn2eCZJSIQ5XbU5yFt+HNUfIgBNqFo6Grh"
                "BDxttIQcoL5ICrsTbdAQWgBPIltxK7uVRaeLQToTFRMbb+JoYoAHjsG63UWXtFIQm2w/mV9x9vodUnC"
                "vCWfuiA+oqwaLH7Al8qL/aS4GA9I6zkz/bbFBSsHVKi++Dftg89YC53DDq8iLTJCSVgcq6DlMZGRkzm"
                "pBtW2jRRH3flU3UIIacWBLduv0gxxxltGTaUtOFS4HVSJHnBp35jvAsbJnV/RbewWgtLVyAlODkjZSd"
                "eMrkNfsIwX3awYfuLLEaGKg/OvlAz+wXVruSNSgAAAAAElFTkSuQmCC";
            const char wechatpng[] = 
                "iVBORw0KGgoAAAANSUhEUgAAAK8AAACtAQMAAAD8lL09AAAABlBMVEUAAAD///+l2Z/dAAAAAWJLR0Q"
                "AiAUdSAAAAAlwSFlzAAAuIwAALiMBeKU/dgAAAbpJREFUSMe9l0GugzAMRI26yDJHyE3Si1WiEhcrN8"
                "kRWLJA8Z9xaNUv/eUfUIXgqQtn4pkY87+ubv+Cd8O1T3g2y4unzveqxXf3BniUeLKa3XcxrnZr+32zg"
                "iXPDvy0S/C04WYo4jpsKCK97FH2K3DovfqzoJzAX9sgwtFVNS/tvH03mwYPs6zb7PjDrf22lAZT6ugq"
                "wwtaCzLYWLwO13yUUQl3N70yDSTFWOqCJbORsdloLYguxrSNgxyWhl3Z0l2LjWaZUE541qaRFUoM342"
                "5sDTYdY5EZE1KjHU/z25+0UV3yi/GE4LeubHe0V9QYFZjEOhN70QOmp0JocSdGT82Nh+D7nrcQoHkDO"
                "GOcmLnhXil3vAsy5nRWhS9SjGkhmMiddHIMO6580oMu5a0xpAC0aOSOHd0mC+jodjNnhyVqDH1Tj3Ts"
                "0hEm3CGv7dYhDkdQGr0cOJxxquMM02GI44g+tKiqwwZVd4HugjHfOKxeEYvQ9jVOKawzrRnQkQSf4Yz"
                "EeasiWHhwfYNF8WkIscc985pKCaGSzA9S6eGAmpMvRlFzonBLZ6qFp8nyhxSM/IP+3x2arDwc/kHxnM"
                "tm62qBBUAAAAASUVORK5CYII=";
            auto donateWnd = new QDialog();
            donateWnd->setWindowTitle(u8"赞助");
            QTabWidget* tab = new QTabWidget(donateWnd);
            auto redbagQr = new QLabel(donateWnd);
            auto redbagQrBmp = QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(QByteArray::fromRawData(redbagpng, sizeof(redbagpng) - 1)), "png"));
            redbagQr->setPixmap(redbagQrBmp);
            tab->addTab(redbagQr, u8"支付宝领红包");
            auto aliQr = new QLabel(donateWnd);
            auto aliQrBmp = QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(QByteArray::fromRawData(alipaypng, sizeof(alipaypng) - 1)), "png"));
            aliQr->setPixmap(aliQrBmp);
            tab->addTab(aliQr, u8"支付宝打赏");
            auto weQr = new QLabel(donateWnd);
            auto weQrBmp = QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(QByteArray::fromRawData(wechatpng, sizeof(wechatpng) - 1)), "png"));
            weQr->setPixmap(weQrBmp);
            tab->addTab(weQr, u8"微信打赏");

            auto layout = new QGridLayout();
            layout->setRowStretch(0, 1);
            layout->setColumnStretch(0, 1);
            layout->addWidget(new QLabel(u8"打赏并非购买，不提供退款。", donateWnd), 0, 0);
            layout->addWidget(tab, 1, 0);
            donateWnd->setLayout(layout);
            donateWnd->setMinimumWidth(360);
            donateWnd->exec();
        });

        layout_->addWidget(cr);
    }
    else
    {
        auto label = new QLabel(u8"<p>Confluence Multistream — by NOX TAIPAN</p>", container_);
        label->setTextFormat(Qt::RichText);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #7d8399; font-size: 11px;");
        layout_->addWidget(label);
    }

    scroll_.setWidgetResizable(true);
    scroll_.setWidget(container_);

    auto fullLayout = new QGridLayout(this);
    fullLayout->setContentsMargins(0, 0, 0, 0);
    fullLayout->setRowStretch(0, 1);
    fullLayout->setColumnStretch(0, 1);
    fullLayout->addWidget(&scroll_, 0, 0);
}

// ✅ FIX: RefreshUI method (now declared as a slot)
void MultiOutputWidget::RefreshUI() {
    // Clear existing widgets from itemLayout_
    QLayoutItem* item;
    while ((item = itemLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();  // Use deleteLater for safety
        }
        delete item;
    }
    
    // Reload the config
    LoadConfig();
    
    // Force UI update
    container_->adjustSize();
    update();
    
    blog(LOG_INFO, TAG "UI refreshed successfully");
}

std::vector<PushWidget*> MultiOutputWidget::GetAllPushWidgets()
{
    std::vector<PushWidget*> result;
    
    // Check all widgets in the item layout
    for (int i = 0; i < itemLayout_->count(); ++i) {
        QLayoutItem* item = itemLayout_->itemAt(i);
        if (item && item->widget()) {
            PushWidget* pushWidget = qobject_cast<PushWidget*>(item->widget());
            if (pushWidget) {
                result.push_back(pushWidget);
            }
        }
    }
    
    return result;
}

void MultiOutputWidget::SaveConfig()
{
    SaveMultiOutputConfig();
}

void MultiOutputWidget::LoadConfig()
{
    // Clear existing widgets safely
    QLayoutItem* item;
    while ((item = itemLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    GlobalMultiOutputConfig() = {};

    if (LoadMultiOutputConfig()) {
        for(auto x: GlobalMultiOutputConfig().targets)
        {
            auto pushwidget = createPushWidget(x->id, container_);
            itemLayout_->addWidget(pushwidget);
        }
    }
}

bool MultiOutputWidget::AddNewTarget(const QString& name, const QString& protocol)
{
    // ✅ ADD INPUT VALIDATION
    if (name.isEmpty()) {
        blog(LOG_WARNING, TAG "AddNewTarget failed: Empty name provided");
        return false;
    }
    
    // Validate protocol
    QString validProtocol = protocol.toLower();
    if (validProtocol.isEmpty()) {
        validProtocol = "rtmp"; // Default to RTMP
    }
    
    // Check if protocol is supported
    QSet<QString> supportedProtocols = {"rtmp", "srt"}; // Add more as needed
    if (!supportedProtocols.contains(validProtocol)) {
        blog(LOG_WARNING, TAG "AddNewTarget failed: Unsupported protocol '%s'", validProtocol.toUtf8().constData());
        return false;
    }
    
    auto& global = GlobalMultiOutputConfig();
    auto newid = GenerateId(global);
    auto target = std::make_shared<OutputTargetConfig>();
    target->id = newid;
    target->name = name.toStdString();
    target->protocol = validProtocol.toStdString();
    global.targets.emplace_back(target);
    
    SaveConfig();
    RefreshUI();
    
    blog(LOG_INFO, TAG "Added new target: %s (Protocol: %s)", name.toUtf8().constData(), validProtocol.toUtf8().constData());
    return true;
}

bool MultiOutputWidget::CloneTarget(const QString& sourceId, const QString& newName, const QString& newStreamKey)
{
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == sourceId) {
            auto& global = GlobalMultiOutputConfig();
            auto newid = GenerateId(global);
            
            auto sourceConfig = widget->GetConfig();
            if (!sourceConfig) {
                return false;
            }
            
            auto target = std::make_shared<OutputTargetConfig>(*sourceConfig);
            target->id = newid;
            target->name = newName.toStdString();
            
            if (!newStreamKey.isEmpty()) {
                try {
                    auto serviceParams = nlohmann::json::parse(target->serviceParam.dump());
                    
                    if (target->protocol == "RTMP" || target->protocol == "rtmp") {
                        if (serviceParams.contains("key")) {
                            serviceParams["key"] = newStreamKey.toStdString();
                        }
                    } else if (target->protocol == "SRT" || target->protocol == "srt") {
                        if (serviceParams.contains("streamid")) {
                            serviceParams["streamid"] = newStreamKey.toStdString();
                        }
                    }
                    
                    target->serviceParam = serviceParams;
                    
                } catch (const std::exception& e) {
                    blog(LOG_WARNING, TAG "Failed to parse service parameters for stream key update: %s", e.what());
                }
            }
            
            global.targets.emplace_back(target);
            SaveConfig();
            
            // ✅ FIX: Direct call to RefreshUI (assumes we're in UI thread)
            RefreshUI();
            
            return true;
        }
    }
    return false;
}

bool MultiOutputWidget::UpdateTargetName(const QString& targetId, const QString& newName)
{
    // ✅ ADD INPUT VALIDATION
    if (newName.isEmpty()) {
        blog(LOG_WARNING, TAG "UpdateTargetName failed: Empty name provided for target %s", targetId.toUtf8().constData());
        return false;
    }
    
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == targetId) {
            widget->GetConfig()->name = newName.toStdString();
            widget->UpdateUI();
            SaveConfig();
            RefreshUI();
            
            blog(LOG_INFO, TAG "Updated target name: %s -> %s", 
                 widget->GetConfig()->name.c_str(), newName.toUtf8().constData());
            return true;
        }
    }
    
    blog(LOG_WARNING, TAG "UpdateTargetName failed: Target not found %s", targetId.toUtf8().constData());
    return false;
}

bool MultiOutputWidget::UpdateTargetStreamKey(const QString& targetId, const QString& streamKey)
{
    if (streamKey.isEmpty()) {
        blog(LOG_WARNING, TAG "UpdateStreamKey failed: Empty stream key provided for target %s", targetId.toUtf8().constData());
        return false;
    }
    
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == targetId) {
            auto config = widget->GetConfig();
            if (!config) {
                blog(LOG_WARNING, TAG "UpdateStreamKey failed: Could not get config for target %s", targetId.toUtf8().constData());
                return false;
            }
            
            try {
                // Parse the current service parameters
                auto serviceParams = nlohmann::json::parse(config->serviceParam.dump());
                
                // Use protocol-specific parameter names
                std::string param_name = "key"; // Default for RTMP and most protocols
                
                if (config->protocol == "srt" || config->protocol == "SRT") {
                    param_name = "streamid"; // SRT uses streamid
                }
                // WHIP uses "key" same as RTMP, so no special handling needed
                
                serviceParams[param_name] = streamKey.toStdString();
                config->serviceParam = serviceParams;
                
                blog(LOG_INFO, TAG "Updated stream key for target %s (%s): %s = %s", 
                     targetId.toUtf8().constData(), 
                     config->protocol.c_str(),
                     param_name.c_str(),
                     streamKey.toUtf8().constData());
                
                SaveConfig();
                RefreshUI();
                return true;
                
            } catch (const std::exception& e) {
                blog(LOG_WARNING, TAG "Failed to update stream key for target %s: %s", 
                     targetId.toUtf8().constData(), e.what());
                return false;
            }
        }
    }
    
    blog(LOG_WARNING, TAG "UpdateStreamKey failed: Target not found %s", targetId.toUtf8().constData());
    return false;
}

bool MultiOutputWidget::UpdateTargetServiceParam(const QString& targetId, const QString& key, const QString& value)
{
    if (key.isEmpty() || value.isEmpty()) {
        blog(LOG_WARNING, TAG "UpdateServiceParam failed: Empty key or value for target %s", targetId.toUtf8().constData());
        return false;
    }
    
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == targetId) {
            auto config = widget->GetConfig();
            if (!config) {
                blog(LOG_WARNING, TAG "UpdateServiceParam failed: Could not get config for target %s", targetId.toUtf8().constData());
                return false;
            }
            
            try {
                auto serviceParams = nlohmann::json::parse(config->serviceParam.dump());
                
                std::string keyStr = key.toStdString();
                std::string valueStr = value.toStdString();
                
                // Special handling for boolean parameters
                if (valueStr == "true" || valueStr == "false") {
                    serviceParams[keyStr] = (valueStr == "true");
                } else {
                    serviceParams[keyStr] = valueStr;
                }
                
                config->serviceParam = serviceParams;
                
                blog(LOG_INFO, TAG "Updated service parameter for target %s: %s = %s", 
                     targetId.toUtf8().constData(), key.toUtf8().constData(), value.toUtf8().constData());
                
                SaveConfig();
                RefreshUI();
                return true;
                
            } catch (const std::exception& e) {
                blog(LOG_WARNING, TAG "Failed to update service parameter for target %s: %s", 
                     targetId.toUtf8().constData(), e.what());
                return false;
            }
        }
    }
    
    blog(LOG_WARNING, TAG "UpdateServiceParam failed: Target not found %s", targetId.toUtf8().constData());
    return false;
}

bool MultiOutputWidget::DeleteTarget(const QString& targetId)
{
    auto& global = GlobalMultiOutputConfig();
    auto it = std::find_if(global.targets.begin(), global.targets.end(), 
        [&](auto& target) { return target->id == targetId.toStdString(); });
    
    if (it != global.targets.end()) {
        global.targets.erase(it);
        SaveConfig();
        RefreshUI();  // ✅ FIX: Direct call to RefreshUI
        return true;
    }
    
    return false;
}

bool MultiOutputWidget::UpdateSyncStart(const QString& targetId, bool syncStart)
{
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == targetId) {
            auto config = widget->GetConfig();
            if (!config) {
                blog(LOG_WARNING, TAG "UpdateSyncStart failed: Could not get config for target %s", targetId.toUtf8().constData());
                return false;
            }
            
            config->syncStart = syncStart;
            
            blog(LOG_INFO, TAG "Updated sync start for target %s: %s", 
                 targetId.toUtf8().constData(), syncStart ? "true" : "false");
            
            SaveConfig();
            RefreshUI();
            return true;
        }
    }
    
    blog(LOG_WARNING, TAG "UpdateSyncStart failed: Target not found %s", targetId.toUtf8().constData());
    return false;
}

bool MultiOutputWidget::UpdateSyncStop(const QString& targetId, bool syncStop)
{
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == targetId) {
            auto config = widget->GetConfig();
            if (!config) {
                blog(LOG_WARNING, TAG "UpdateSyncStop failed: Could not get config for target %s", targetId.toUtf8().constData());
                return false;
            }
            
            config->syncStop = syncStop;
            
            blog(LOG_INFO, TAG "Updated sync stop for target %s: %s", 
                 targetId.toUtf8().constData(), syncStop ? "true" : "false");
            
            SaveConfig();
            RefreshUI();
            return true;
        }
    }
    
    blog(LOG_WARNING, TAG "UpdateSyncStop failed: Target not found %s", targetId.toUtf8().constData());
    return false;
}

// Vivo/muerto se decide leyendo el PID que el propio server.js escribe en
// scripts/confluence.pid y chequeando si ese proceso sigue corriendo -
// mismo archivo que ya usa scripts/stop.ps1, sin agregar una dependencia
// de red (Qt Network) solo para esto.
static bool IsConfluenceProcessAlive()
{
#ifdef _WIN32
    std::ifstream f(std::string(ConfluenceDir()) + "/scripts/confluence.pid");
    if (!f) return false;

    DWORD pid = 0;
    f >> pid;
    if (pid == 0) return false;

    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;

    DWORD exitCode = 0;
    bool alive = GetExitCodeProcess(h, &exitCode) && exitCode == STILL_ACTIVE;
    CloseHandle(h);
    return alive;
#else
    return false;
#endif
}

void MultiOutputWidget::CheckConfluenceStatus()
{
    if (!confluenceDot_) return;

    bool alive = IsConfluenceProcessAlive();
    confluenceDot_->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
        .arg(alive ? "#45d9a0" : "#3a4050"));
    confluenceStatusLabel_->setText(obs_module_text(alive ? "Confluence.Online" : "Confluence.Offline"));
}

void MultiOutputWidget::RestartConfluenceServer()
{
    confluenceRestartBtn_->setEnabled(false);
    confluenceStatusLabel_->setText(obs_module_text("Confluence.Restarting"));

    QString dir = QString::fromUtf8(ConfluenceDir());
    // mismos dos scripts que usa obs-autostart.lua al abrir/cerrar OBS -
    // stop.ps1 mata el PID guardado si sigue vivo, start-hidden.vbs lanza
    // uno nuevo oculto. node server.js ya sale solo sin romper nada si el
    // puerto termina ocupado por otra instancia (ver server.js).
    QProcess::execute("powershell", { "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", dir + "/scripts/stop.ps1" });
    QProcess::startDetached("wscript.exe", { dir + "/scripts/start-hidden.vbs" });

    QTimer::singleShot(2000, this, [this]() {
        CheckConfluenceStatus();
        confluenceRestartBtn_->setEnabled(true);
    });
}

void MultiOutputWidget::UpdateMainStreamButton()
{
    if (!mainStreamButton_) return;

    bool active = obs_frontend_streaming_active();
    mainStreamButton_->setText(active
        ? obs_module_text("Btn.StopMainStream")
        : obs_module_text("Btn.StartMainStream"));
    mainStreamButton_->setStyleSheet(active
        ? "background-color: #1c3b30; color: #6ee7b7; border-color: #45d9a0; font-weight: 700;"
        : "");
}

void MultiOutputWidget::OnOBSEvent(obs_frontend_event event)
{
    switch (event) {
    case OBS_FRONTEND_EVENT_STREAMING_STARTING:
    case OBS_FRONTEND_EVENT_STREAMING_STARTED:
    case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
    case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
        UpdateMainStreamButton();
        break;
    default:
        break;
    }
}

PushWidget* MultiOutputWidget::FindPushWidgetById(const QString& targetId)
{
    for (auto* widget : GetAllPushWidgets()) {
        if (widget->GetTargetId() == targetId) {
            return widget;
        }
    }
    return nullptr;
}