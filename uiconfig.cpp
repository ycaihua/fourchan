#include "uiconfig.h"
#include "ui_uiconfig.h"

UIConfig::UIConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UIConfig)
{
    ui->setupUi(this);
    settings = new QSettings("settings.ini", QSettings::IniFormat);
    timeoutValueEditor = new UIListEditor(this);
    timeoutValueEditor->setModal(true);

    loadSettings();

    connect(timeoutValueEditor, SIGNAL(valuesChanged()), this, SLOT(loadSettings()));
}

void UIConfig::loadSettings(void) {
    QStringList sl;
    int index;
    bool b;

    settings->beginGroup("options");
    ui->leDefaultSavepath->setText(settings->value("default_directory","").toString());
    ui->cmbTabPosition->setCurrentIndex(settings->value("tab_position",3).toInt());
    b = settings->value("automatic_close",false).toBool();
        ui->cbAutoClose->setChecked(b);

    b = settings->value("resume_session",false).toBool();
        ui->cbReopenTabs->setChecked(b);

    b = settings->value("enlarge_thumbnails",false).toBool();
        ui->cbEnlargeThumbnails->setChecked(b);

    b = settings->value("hq_thumbnails",false).toBool();
        ui->cbHQThumbnail->setChecked(b);

    b = settings->value("default_original_filename",false).toBool();
        ui->cbDefaultOriginalFilename->setChecked(b);

    b = settings->value("remember_directory",false).toBool();
        ui->cbRememberDirectory->setChecked(b);

    ui->sbConcurrentDownloads->setValue(settings->value("concurrent_downloads",1).toInt());
    ui->sbRescheduleInterval->setValue(settings->value("reschedule_interval", 60).toInt());
    ui->sbThumbnailHeight->setValue(settings->value("thumbnail_height",200).toInt());
    ui->sbThumbnailWidth->setValue(settings->value("thumbnail_width",200).toInt());

    sl = settings->value("timeout_values", (QStringList()<<"30"<<"60"<<"120"<<"300"<<"600")).toStringList();
    ui->cbRescanInterval->clear();
    ui->cbRescanInterval->addItem("Never", 0);
    foreach (QString s, sl) {
        int value;
        int i;
        bool ok;
        QString text;

        i = s.toInt(&ok);
        if (ok) {
            if (i > 60 && ((i%60) == 0)) {              // Display as minutes
                if (i > 3600 && ((i%3600) == 0)) {      // Display as hours
                    if (i > 86400 && ((i%86400)==0)) {  // Display as days
                        value = i/86400;
                        text = "days";
                    }
                    else {
                        value = i/3600;
                        text = "hours";
                    }
                }
                else {
                    value = i/60;
                    text = "minutes";
                }
            }
            else {
                value = i;
                text = "seconds";
            }

            ui->cbRescanInterval->addItem(QString("every %1 %2").arg(value).arg(text), i);
        }
    }

    index = ui->cbRescanInterval->findData(settings->value("default_timeout", 0).toInt());
    if (index != -1) ui->cbRescanInterval->setCurrentIndex(index);
    else ui->cbRescanInterval->setCurrentIndex(0);
    settings->endGroup();

    settings->beginGroup("blacklist");
    b = settings->value("use_blacklist",true).toBool();
    ui->cbUseBlackList->setChecked(b);
    ui->sbBlackListCheckInterval->setValue(settings->value("blacklist_check_interval", 600).toInt());
    settings->endGroup();

    timeoutValueEditor->loadSettings();
}

void UIConfig::accept(void) {
    settings->beginGroup("options");
        settings->setValue("default_directory", ui->leDefaultSavepath->text());
        settings->setValue("tab_position", ui->cmbTabPosition->currentIndex());
        if (ui->cbAutoClose->isChecked())
            settings->setValue("automatic_close", true);
        else
            settings->setValue("automatic_close", false);

        if (ui->cbReopenTabs->isChecked())
            settings->setValue("resume_session", true);
        else
            settings->setValue("resume_session", false);

        if (ui->cbEnlargeThumbnails->isChecked())
            settings->setValue("enlarge_thumbnails", true);
        else
            settings->setValue("enlarge_thumbnails", false);

        if (ui->cbHQThumbnail->isChecked())
            settings->setValue("hq_thumbnails", true);
        else
            settings->setValue("hq_thumbnails", false);

        if (ui->cbDefaultOriginalFilename->isChecked())
            settings->setValue("default_original_filename", true);
        else
            settings->setValue("default_original_filename", false);

        if (ui->cbRememberDirectory->isChecked())
            settings->setValue("remember_directory", true);
        else
            settings->setValue("remember_directory", false);

        settings->setValue("concurrent_downloads", ui->sbConcurrentDownloads->value());
        settings->setValue("reschedule_interval", ui->sbRescheduleInterval->value());
        settings->setValue("thumbnail_width", ui->sbThumbnailWidth->value());
        settings->setValue("thumbnail_height", ui->sbThumbnailHeight->value());

        settings->setValue("default_timeout", ui->cbRescanInterval->itemData(ui->cbRescanInterval->currentIndex()));
    settings->endGroup();
    settings->beginGroup("blacklist");
        if (ui->cbUseBlackList->isChecked())
            settings->setValue("use_blacklist", true);
        else
            settings->setValue("use_blacklist", false);

        settings->setValue("blacklist_check_interval", ui->sbBlackListCheckInterval->value());
    settings->endGroup();
    settings->sync();

    emit configurationChanged();
    hide();
}

void UIConfig::reject(void) {
    hide();
}

void UIConfig::chooseLocation(void) {
    QString loc;

    loc = QFileDialog::getExistingDirectory(this, "Choose storage directory", ui->leDefaultSavepath->text());

    ui->leDefaultSavepath->setText(loc);
}

void UIConfig::editTimeoutValues(void) {
    timeoutValueEditor->show();
}

UIConfig::~UIConfig()
{
    delete ui;
}