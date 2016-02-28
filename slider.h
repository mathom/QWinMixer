#ifndef SLIDER_H
#define SLIDER_H

#include <QWidget>

#include <Audiopolicy.h>

namespace Ui {
class Slider;
}

class Slider : public QWidget, public IAudioSessionEvents
{
    Q_OBJECT

public:
    explicit Slider(QWidget *parent,
                    const QString &name,
                    const QString &icon,
                    ISimpleAudioVolume *volume,
                    IAudioSessionControl *session);
    ~Slider();

    const QString &name() { return _name; }

    // Windows garbage
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP OnSimpleVolumeChanged(float volume, BOOL muted, LPCGUID context);
    STDMETHODIMP OnDisplayNameChanged(LPCWSTR name, LPCGUID context);
    STDMETHODIMP OnIconPathChanged(LPCWSTR icon, LPCGUID context);
    STDMETHODIMP OnChannelVolumeChanged(DWORD,float[],DWORD,LPCGUID);
    STDMETHODIMP OnGroupingParamChanged(LPCGUID,LPCGUID);
    STDMETHODIMP OnStateChanged(AudioSessionState);
    STDMETHODIMP OnSessionDisconnected(AudioSessionDisconnectReason);


public slots:
    void setMute(bool muted);
    void setVolume(int amt);

signals:
    void sessionDestroyed();

private:
    void loadIcon();
    void updateVolume();

    Ui::Slider *ui;

    QPixmap _enabledIcon;
    QPixmap _disabledIcon;

    QString _icon;
    QString _name;

    bool _muted;
    ISimpleAudioVolume *_volume;
    IAudioSessionControl *_sessionControl;
    LONG _cRef;
};

#endif // SLIDER_H
