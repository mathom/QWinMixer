#include "slider.h"
#include "ui_slider.h"

#include <QtWinExtras>
#include <QDebug>

#include <Audiopolicy.h>
#include <Shellapi.h>
#include <Shlwapi.h>

#include "macros.h"

Slider::Slider(QWidget *parent,
               const QString &name,
               const QString &icon,
               ISimpleAudioVolume *volume,
               IAudioSessionControl *session) :
    QWidget(parent),
    ui(new Ui::Slider),
    _muted(false),
    _volume(volume),
    _sessionControl(session),
    _name(name),
    _icon(icon),
    _cRef(1)
{
    ui->setupUi(this);

    _volume->AddRef();
    _sessionControl->AddRef();

    ui->label->setText(name);
    loadIcon();
    updateVolume();

    connect(ui->muteButton, SIGNAL(clicked(bool)), this, SLOT(setMute(bool)));
    connect(ui->slider, SIGNAL(sliderMoved(int)), this, SLOT(setVolume(int)));
    _sessionControl->RegisterAudioSessionNotification(this);
}

Slider::~Slider()
{
    delete ui;

    _sessionControl->UnregisterAudioSessionNotification(this);
    SAFE_RELEASE(_volume);
    SAFE_RELEASE(_sessionControl);
}

HRESULT Slider::QueryInterface(const IID &riid, void **ppv)
{
    static const QITAB qit[2] =
    {
        QITABENT(Slider, IAudioSessionEvents),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) Slider::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) Slider::Release()
{
    LONG cRef = InterlockedDecrement( &_cRef );
    if (cRef == 0)
    {
        // yeah, no...
        //delete this;
    }
    return cRef;
}

HRESULT Slider::OnSimpleVolumeChanged(float volume, BOOL muted, LPCGUID)
{
    ui->slider->setValue(volume*100);
    setMute(muted);
    return S_OK;
}

HRESULT Slider::OnDisplayNameChanged(LPCWSTR name, LPCGUID)
{
    ui->label->setText(QString::fromWCharArray(name));
    return S_OK;
}

HRESULT Slider::OnIconPathChanged(LPCWSTR icon, LPCGUID)
{
    _icon = QString::fromWCharArray(icon);
    loadIcon();
    return S_OK;
}

HRESULT Slider::OnChannelVolumeChanged(DWORD, float[], DWORD, LPCGUID)
{
    return S_OK;
}

HRESULT Slider::OnGroupingParamChanged(LPCGUID, LPCGUID)
{
    return S_OK;
}

HRESULT Slider::OnStateChanged(AudioSessionState)
{
    return S_OK;
}

HRESULT Slider::OnSessionDisconnected(AudioSessionDisconnectReason)
{
    qDebug() << _name << "disconnected";
    emit sessionDestroyed();
    return S_OK;
}

void Slider::setMute(bool muted)
{
    if (_muted != muted) {
        _volume->SetMute(muted, NULL);
    }
    _muted = muted;

    ui->label->setDisabled(muted);
    ui->slider->setDisabled(muted);

    if (muted) {
        ui->muteButton->setText(tr("Unmute"));
        ui->icon->setPixmap(_disabledIcon);
    }
    else {
        ui->muteButton->setText(tr("Mute"));
        ui->icon->setPixmap(_enabledIcon);
    }
}

void Slider::setVolume(int amt)
{
    float volume = amt*0.01f;
    _volume->SetMasterVolume(volume, NULL);
}

void Slider::loadIcon()
{
    HICON largeIcon = 0;

    int extracted = 0;
    WCHAR buff[2048];
    memset(buff, 0, 2048);

    _icon.toWCharArray(buff);

    extracted = ExtractIconEx(buff, 0, &largeIcon, NULL, 1);

    if (extracted && largeIcon) {
        QPixmap icon = QtWin::fromHICON(largeIcon);
        ui->icon->setPixmap(icon);

        _enabledIcon = icon;
        _disabledIcon = icon.copy();
        QPainter p(&_disabledIcon);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(_disabledIcon.rect(), QColor(255,255,255,150));

        DestroyIcon(largeIcon);
    }
}

void Slider::updateVolume()
{
    float volume;
    BOOL muted;
    _volume->GetMasterVolume(&volume);
    _volume->GetMute(&muted);

    setMute(muted);

    ui->muteButton->setChecked(muted);
    ui->slider->setValue(volume*100);
}
